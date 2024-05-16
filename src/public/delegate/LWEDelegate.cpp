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

#include <EscargotPublic.h>

#if defined(STARFISH_WINDOWS)
#include <fontconfig/fontconfig.h>
#endif

using namespace Escargot;

namespace LWEDelegate {

Starfish::Starfish* g_starfishInstance;

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
        auto executionContext = Starfish::fetchExecutionContext(relatedContext);
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

static void StarfishGCMemoryLogger(void* data)
{
    STARFISH_LOG_INFO("Done GC: HeapSize: [%f MB , %f MB]",
                      GC_get_memory_use() / 1024.f / 1024.f,
                      GC_get_heap_size() / 1024.f / 1024.f);
}

void LWE::Initialize(const char* localStorageDataFilePath,
                     const char* cookieStoreDataFilePath,
                     const char* httpCacheDataDirectorypath)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());

    // TODO: Provide API to determine whether to use threaded call or not.
    std::string backend = STARFISH_BACKEND_STR;
    bool isThreadMode = false;
    Starfish::StarfishRendererType rendererType =
        Starfish::StarfishRendererType::kOpenGL;
    if (backend == "uv_cairo_gl" || backend == "dali" || backend == "flutter") {
        isThreadMode = true;
    }
    if (backend == "dali") {
        rendererType = Starfish::StarfishRendererType::kSoftware;
    } else if (backend == "efl_headless") {
        rendererType = Starfish::StarfishRendererType::kHeadless;
    }

    Starfish::MessageLoop::init();

    ThreadedCallHelper::Instance()->Initialize(isThreadMode);

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
#if defined(STARFISH_WINDOWS)
        FcInitLoadConfigAndFonts();
#endif
        Starfish::StarfishConfiguration config;
        config.localStorageDataFilePath = localStorageDataFilePath;
        config.cookieStoreDataFilePath = cookieStoreDataFilePath;
        config.httpCacheDataDirectorypath = httpCacheDataDirectorypath;
        config.gcFrequency = BDWGC_FREE_SPACE_DIVISOR;
        config.isThreadMode = isThreadMode;
        config.backend = STARFISH_BACKEND_STR;
        config.rendererType = rendererType;

        Escargot::Globals::initialize(new EscargotStarfishPlatform());
        g_starfishInstance = new (NoGC) Starfish::Starfish(config);
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

        clearStack<ELABORATE_CLEAR_STACK_SIZE>();

        // do implicit calling GC funciton takes a lots time
        // we should remove this if possible
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();

        // Escargot::Globals::finalize should be invoked after full gc
        Escargot::Globals::finalize();
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
void LWEDelegate_LWE_Initialize(const char* localStorageDataFilePath,
                                const char* cookieStoreDataFilePath,
                                const char* httpCacheDataDirectorypath)

{
    LWEDelegate::LWE::Initialize(localStorageDataFilePath,
                                 cookieStoreDataFilePath,
                                 httpCacheDataDirectorypath);
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
