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

#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/ExecutionContext.h"

#if defined(STARFISH_WEBWORKER_HOST)
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#endif

#include "core/page/Window.h"

#include "LWEWebView.h"
#include <EscargotPublic.h>

#if defined(STARFISH_WINDOWS)
#include <fontconfig/fontconfig.h>
#endif

#define THREAD_MINIMUM_STACK_SIZE \
    4 * 1024 * 1024 // we need at least 4MB for stack

using namespace Escargot;

namespace Starfish {
class EscargotStarfishPlatform : public Escargot::PlatformRef {
public:
    EscargotStarfishPlatform()
    {
    }

    virtual void customInfoLogger(const char* format, va_list arg)
    {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), format, arg);
        STARFISH_LOG_INFO("%s", buf);
    }

    virtual void customErrorLogger(const char* format, va_list arg)
    {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), format, arg);
        STARFISH_LOG_ERROR("%s", buf);
    }

    virtual void markJSJobEnqueued(
        Escargot::ContextRef* relatedContext) override
    {
        auto executionContext = fetchExecutionContext(relatedContext);
        executionContext->webBase()->messageLoop()->addMicroTask(
            executionContext->globalScope(),
            [](size_t handle, void* data) {
                VMInstanceRef* vm = (VMInstanceRef*)data;
                if (vm->hasPendingJob()) {
                    auto jobResult = vm->executePendingJob();
                    if (jobResult.error) {
                        STARFISH_LOG_ERROR("Uncaught Error in JS job");
                    }
                }
            },
            relatedContext->vmInstance());
    }

    virtual LoadModuleResult onLoadModule(Escargot::ContextRef* relatedContext,
                                          Escargot::ScriptRef* whereRequestFrom,
                                          Escargot::StringRef* moduleSrc,
                                          ModuleType type) override
    {
        return LoadModuleResult(Escargot::ErrorObjectRef::Code::None,
                                Escargot::StringRef::emptyString());
    }

    virtual void didLoadModule(
        Escargot::ContextRef* relatedContext,
        Escargot::OptionalRef<Escargot::ScriptRef> referrer,
        Escargot::ScriptRef* loadedModule) override
    {
    }

    virtual void hostImportModuleDynamically(ContextRef* relatedContext,
                                             ScriptRef* referrer,
                                             StringRef* src, ModuleType type,
                                             PromiseObjectRef* promise) override
    {
        LoadModuleResult loadedModuleResult =
            onLoadModule(relatedContext, referrer, src, type);

        Evaluator::EvaluatorResult executionResult = Evaluator::execute(
            relatedContext,
            [](ExecutionStateRef* state, LoadModuleResult loadedModuleResult,
               PromiseObjectRef* promise) -> ValueRef* {
                if (loadedModuleResult.script) {
                    if (loadedModuleResult.script.value()->isExecuted()) {
                        if (loadedModuleResult.script.value()
                                ->wasThereErrorOnModuleEvaluation()) {
                            state->throwException(
                                loadedModuleResult.script.value()
                                    ->moduleEvaluationError());
                        }
                    } else {
                        loadedModuleResult.script.value()->execute(state);
                    }
                } else {
                    state->throwException(ErrorObjectRef::create(
                        state, loadedModuleResult.errorCode,
                        loadedModuleResult.errorMessage));
                }
                return loadedModuleResult.script.value()->moduleNamespace(
                    state);
            },
            loadedModuleResult, promise);

        Evaluator::execute(
            relatedContext,
            [](ExecutionStateRef* state, bool isSuccessful, ValueRef* value,
               PromiseObjectRef* promise) -> ValueRef* {
                if (isSuccessful) {
                    promise->fulfill(state, value);
                } else {
                    promise->reject(state, value);
                }
                return ValueRef::createUndefined();
            },
            executionResult.isSuccessful(),
            executionResult.isSuccessful() ? executionResult.result
                                           : executionResult.error.value(),
            promise);
    }

    virtual void markJSJobFromAnotherThreadExists(
        ContextRef* relatedContext) override
    {
    }
};
} // namespace Starfish

namespace LWE {

Starfish::Starfish* g_starfishInstance;

static void StarfishGCMemoryLogger(void* data)
{
    STARFISH_LOG_INFO("Done GC: HeapSize: [%f MB , %f MB]",
                      GC_get_memory_use() / 1024.f / 1024.f,
                      GC_get_heap_size() / 1024.f / 1024.f);
}

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)

static bool g_isLWEThreadStarted;
static pthread_mutex_t g_mainThreadInitLocker;

void* LWEMainThread(void*)
{
    Starfish::MessageLoop::init();
    pthread_mutex_unlock(&g_mainThreadInitLocker);
    STARFISH_LOG_INFO("Worker thread started!");
    Starfish::MessageLoop::run();
    return nullptr;
}

void LWE::Initialize(const char* localStorageDataFilePath,
                     const char* cookieStoreDataFilePath,
                     const char* httpCacheDataDirectorypath)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());
    if (!g_isLWEThreadStarted) {
        pthread_mutex_init(&g_mainThreadInitLocker, NULL);
        pthread_mutex_lock(&g_mainThreadInitLocker);

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, THREAD_MINIMUM_STACK_SIZE);
        pthread_t tid;
        pthread_create(&tid, &attr, LWEMainThread, NULL);

        pthread_mutex_lock(&g_mainThreadInitLocker);
        pthread_mutex_unlock(&g_mainThreadInitLocker);
        pthread_mutex_destroy(&g_mainThreadInitLocker);
        g_isLWEThreadStarted = true;
    }

    Starfish::MessageLoop::runOnMainThreadSync([&]() -> size_t {
#if defined(STARFISH_WINDOWS)
        FcInitLoadConfigAndFonts();
#endif
        Escargot::Globals::initialize(new Starfish::EscargotStarfishPlatform());
        g_starfishInstance = new (NoGC) Starfish::Starfish(
            localStorageDataFilePath, cookieStoreDataFilePath,
            httpCacheDataDirectorypath);
        // add gc event listener
        Escargot::Memory::removeGCEventListener(
            Escargot::Memory::RECLAIM_END, StarfishGCMemoryLogger, nullptr);
        Escargot::Memory::addGCEventListener(Escargot::Memory::RECLAIM_END,
                                             StarfishGCMemoryLogger, nullptr);
        return 0;
    });
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());

    Starfish::MessageLoop::runOnMainThreadSync([]() -> size_t {
        STARFISH_RELEASE_ASSERT(g_starfishInstance->webViewInstanceCount() ==
                                0);

        g_starfishInstance->destroy();
        g_starfishInstance = nullptr;

        clearStack<ELABORATE_CLEAR_STACK_SIZE>();

        // do implicit calling GC funciton takes a lots time
        // we should remove this if possible
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();

        // Escargot::Globals::finalize should be invoked after full gc
        Escargot::Globals::finalize();
        return 0;
    });
}

#else
void LWE::Initialize(const char* localStorageDataFilePath,
                     const char* cookieStoreDataFilePath,
                     const char* httpCacheDataDirectorypath)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());

#if defined(STARFISH_WINDOWS)
    FcInitLoadConfigAndFonts();
#endif

    Escargot::Globals::initialize(new Starfish::EscargotStarfishPlatform());
    g_starfishInstance = new (NoGC)
        Starfish::Starfish(localStorageDataFilePath, cookieStoreDataFilePath,
                           httpCacheDataDirectorypath);
    // add gc event listener
    Escargot::Memory::removeGCEventListener(Escargot::Memory::RECLAIM_END,
                                            StarfishGCMemoryLogger, nullptr);
    Escargot::Memory::addGCEventListener(Escargot::Memory::RECLAIM_END,
                                         StarfishGCMemoryLogger, nullptr);
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    STARFISH_RELEASE_ASSERT(g_starfishInstance->webViewInstanceCount() == 0);

    g_starfishInstance->destroy();
    g_starfishInstance = nullptr;

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();

    // do implicit calling GC funciton takes a lots time
    // we should remove this if possible
    Starfish::Starfish::doFullGCWithoutSeeingStack();
    Starfish::Starfish::doFullGCWithoutSeeingStack();
    Starfish::Starfish::doFullGCWithoutSeeingStack();

    // Escargot::Globals::finalize should be invoked after full gc
    Escargot::Globals::finalize();
}
#endif

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
} // namespace LWE
