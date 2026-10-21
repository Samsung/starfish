/* Copyright (c) 2026-present Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "StarfishConfig.h"
#include "Starfish.h"
#include "public/contract/LWEDelegate.h"
#include "public/contract/LWEWebContainerDelegate.h"
#include "public/contract/CookieManagerDelegate.h"
#include "public/delegate/ThreadedCallHelper.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include <chrono>
#include <future>
#include <vector>
#if defined(PORT_EVENTLOOP_BACKEND_GLIB)
#include <glib.h>
#endif

// This TU is added only to test builds. Both kinds of static storage must
// reset after a real dlclose/dlopen, even when the same addresses are reused.
static unsigned g_unloadTestState;

namespace LWEDelegate {
extern Starfish::Starfish* g_starfishInstance;
}

extern "C" {
unsigned EXPORT_UNMANAGED_API LWEImplTest_SwapGlobal(unsigned value)
{
    unsigned previous = g_unloadTestState;
    g_unloadTestState = value;
    return previous;
}

unsigned EXPORT_UNMANAGED_API LWEImplTest_SwapStatic(unsigned value)
{
    static unsigned state;
    unsigned previous = state;
    state = value;
    return previous;
}

unsigned EXPORT_UNMANAGED_API LWEImplTest_SwapTLS(unsigned value)
{
    unsigned previous;
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&] {
            static thread_local unsigned state;
            previous = state;
            state = value;
        });
    return previous;
}

size_t EXPORT_UNMANAGED_API LWEImplTest_ThreadID()
{
    size_t tid = 0;
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&] { tid = Starfish::getCurrentThreadID(); });
    return tid;
}

unsigned EXPORT_UNMANAGED_API LWEImplTest_VDB()
{
    unsigned vdb = 0;
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&] { vdb = GC_get_actual_vdb(); });
    return vdb;
}

bool EXPORT_UNMANAGED_API LWEImplTest_Exercise(const char* url)
{
    // Model a shipped API that retains its cookie wrapper across Finalize.
    static auto* cookie = LWEDelegate::CookieManager::GetInstance();
    cookie->HasCookies();
    LWEDelegate::WebContainer* container;
    std::vector<uint32_t> pixels(320 * 240);
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&] {
#if defined(STARFISH_HEADLESS)
            container = LWEDelegate::WebContainer::CreateHeadless(
                320, 240, 1, "serif", "en-US", "UTC");
#else
            LWEDelegate::g_starfishInstance->setRendererType(
                Starfish::StarfishRendererType::kSoftware);
            container = LWEDelegate::WebContainer::CreateWithBuffer(
                pixels.data(), 320, 240, 320 * 4, 1, "serif", "en-US", "UTC");
            container->RegisterPreRenderingHandler([&] {
                return LWEDelegate::WebContainer::RenderInfo{ pixels.data(),
                                                              320 * 4 };
            });
            container->RegisterOnRenderedHandler(
                [](LWEDelegate::WebContainer*,
                   const LWEDelegate::WebContainer::RenderResult&) {});
#endif
        });
    std::promise<void> loaded;
    auto ready = loaded.get_future();
    container->RegisterOnPageLoadedHandler(
        [&](LWEDelegate::WebContainer*, const std::string&) {
            loaded.set_value();
        });
    container->LoadURL(url);
#if defined(PORT_EVENTLOOP_BACKEND_GLIB)
    if (!LWEDelegate::ThreadedCallHelper::Instance()->isThreadMode()) {
        auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (ready.wait_for(std::chrono::seconds(0)) !=
                   std::future_status::ready &&
               std::chrono::steady_clock::now() < deadline) {
            g_main_context_iteration(nullptr, FALSE);
        }
    }
#endif
    if (ready.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
        container->Destroy();
        return false;
    }
    std::string result = container->EvaluateJavaScript(
        "(function() {"
        "var e = document.createElement('div');"
        "e.textContent = String(6 * 7);"
        "return e.textContent;"
        "})()");
#if defined(STARFISH_ENABLE_WORKER)
    // Leave a live worker and its nested worker to document disposal. Also
    // exercise close() and termination before the worker finishes starting.
    container->EvaluateJavaScript(R"JS(
        var unloadWorkerReady = false;
        var unloadWorkerDeadline = 0;
        var closingWorker = new Worker('unload-close.js');
        var nestedWorker = new Worker('unload-parent.js');
        nestedWorker.onmessage = function(e) {
            unloadWorkerDeadline = e.data;
            unloadWorkerReady = true;
        };
    )JS");
    bool workerReady = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
#if defined(PORT_EVENTLOOP_BACKEND_GLIB)
        if (!LWEDelegate::ThreadedCallHelper::Instance()->isThreadMode()) {
            g_main_context_iteration(nullptr, FALSE);
        }
#endif
        if (container->EvaluateJavaScript("String(unloadWorkerReady)") ==
            "true") {
            workerReady = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    container->EvaluateJavaScript(R"JS(
        var startingWorker = new Worker('unload-close.js');
        startingWorker.terminate();
        startingWorker.terminate();
    )JS");
    if (!workerReady) {
        result = "worker startup failed";
    }
    auto remaining = std::stoll(container->EvaluateJavaScript(
        "String(Math.max(0, unloadWorkerDeadline - Date.now()))"));
    auto destroyStarted = std::chrono::steady_clock::now();
#endif
    container->Destroy();
#if defined(STARFISH_ENABLE_WORKER)
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - destroyStarted)
                       .count();
    if (remaining > 0) {
        std::printf("Worker shutdown waited %lld ms (JS remaining %lld ms)\n",
                    static_cast<long long>(elapsed), remaining);
    }
    if (elapsed + 100 < remaining) {
        return false;
    }
#endif
    return result == "42";
}

bool EXPORT_UNMANAGED_API LWEImplTest_ShutdownFromLWEThread()
{
    bool result;
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&] { result = LWEDelegate::LWE::ShutdownForUnload(); });
    return result;
}

void EXPORT_UNMANAGED_API LWEImplTest_PauseHost()
{
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [] {
            Starfish::MessageLoop::runWithProcessMainThreadPausedSync([] {});
        });
}
}
