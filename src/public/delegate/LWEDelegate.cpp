/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "Starfish.h"

#include "LWEDelegate.h"
#include "ThreadedCallHelper.h"

#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/ExecutionContext.h"

#include "core/page/Window.h"

#include "binding/ScriptEngineInstance.h"

#include <EscargotPublic.h>

#if defined(STARFISH_WINDOWS)
#include <fontconfig/fontconfig.h>
#endif

#if defined(STARFISH_EFL_CAIRO_GL) || defined(PORT_WEBVIEW_BRIDGE_EFL) || \
    defined(STARFISH_EFL_HEADLESS)
#include <Ecore.h>
#endif

using namespace Escargot;

namespace LWEDelegate {

Starfish::Starfish* g_starfishInstance;
Starfish::Starfish** g_starfishInstanceHolder;

static void StarfishGCMemoryLogger(void* data)
{
    STARFISH_LOG_INFO("Done GC: HeapSize: [%f MB , %f MB]",
                      GC_get_memory_use() / 1024.f / 1024.f,
                      GC_get_heap_size() / 1024.f / 1024.f);
}

void LWE::Initialize(const char* storageDirectoryPath)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());

    // TODO: Provide API to determine whether to use threaded call or not.
    std::string backend = STARFISH_BACKEND_STR;
    bool isThreadMode = false;
    Starfish::StarfishRendererType rendererType =
        Starfish::StarfishRendererType::kOpenGL;
    if (backend == "uv_cairo_gl" || backend == "flutter" ||
        backend == "uv_worker") {
        isThreadMode = true;
    }
    if (backend == "efl_headless" || backend == "glib_headless") {
        rendererType = Starfish::StarfishRendererType::kHeadless;
    }
#ifdef STARFISH_ENABLE_TEST
    setenv("BACKEND", backend.data(), TRUE);
#endif

    ThreadedCallHelper::Instance()->Initialize(isThreadMode);

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
#if defined(STARFISH_WINDOWS)
        FcInitLoadConfigAndFonts();
#endif
#if defined(STARFISH_EFL_CAIRO_GL) || defined(PORT_WEBVIEW_BRIDGE_EFL) || \
    defined(STARFISH_EFL_HEADLESS)
        ecore_main_loop_glib_integrate();
#endif
        Starfish::StarfishConfiguration config;
        config.storageDirectoryPath = storageDirectoryPath;
        config.gcFrequency = BDWGC_FREE_SPACE_DIVISOR;
        config.isThreadMode = isThreadMode;
        config.backend = STARFISH_BACKEND_STR;
        config.rendererType = rendererType;

        Starfish::staticallyInitScriptEngine();

        g_starfishInstanceHolder = reinterpret_cast<Starfish::Starfish**>(
            GC_MALLOC_UNCOLLECTABLE(sizeof(Starfish::Starfish**)));
        g_starfishInstance = *g_starfishInstanceHolder =
            new Starfish::Starfish(config);
        // add gc event listener
        Escargot::Memory::removeGCEventListener(
            Escargot::Memory::RECLAIM_END, StarfishGCMemoryLogger, nullptr);
        Escargot::Memory::addGCEventListener(Escargot::Memory::RECLAIM_END,
                                             StarfishGCMemoryLogger, nullptr);
    });
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([]() -> void {
        STARFISH_RELEASE_ASSERT(g_starfishInstance->webViewInstanceCount() ==
                                0);

        g_starfishInstance->destroy();
        g_starfishInstance = nullptr;
        GC_FREE(g_starfishInstanceHolder);
        g_starfishInstanceHolder = nullptr;

        clearStack<ELABORATE_CLEAR_STACK_SIZE>();

        // do implicit calling GC funciton takes a lots time
        // we should remove this if possible
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();

        Starfish::staticallyDestroyScriptEngine();
    });
}

bool LWE::IsInitialized()
{
    return g_starfishInstance;
}

void LWE::SetGCFrequency(unsigned char freq)
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    g_starfishInstance->setGCFrequency(freq);
}

unsigned char LWE::GetGCFrequency()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    return g_starfishInstance->gcFrequency();
}

void LWE::GetVersion(int* major, int* minor, int* patch)
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    g_starfishInstance->version(major, minor, patch);
}

} // namespace LWEDelegate

extern "C" {
void LWEDelegate_LWE_Initialize(const char* storageDirectoryPath)

{
    LWEDelegate::LWE::Initialize(storageDirectoryPath);
}

bool LWEDelegate_LWE_IsInitialized()
{
    return LWEDelegate::LWE::IsInitialized();
}

void LWEDelegate_LWE_Finalize()
{
    LWEDelegate::LWE::Finalize();
}

unsigned char LWEDelegate_LWE_GetGCFrequency()
{
    return LWEDelegate::LWE::GetGCFrequency();
}

void LWEDelegate_LWE_SetGCFrequency(unsigned char freq)
{
    LWEDelegate::LWE::SetGCFrequency(freq);
}

void LWEDelegate_LWE_GetVersion(int* major, int* minor, int* patch)
{
    LWEDelegate::LWE::GetVersion(major, minor, patch);
}
}
