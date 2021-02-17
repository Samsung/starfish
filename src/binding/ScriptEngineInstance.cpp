/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptBindingInstance.h"

#include "core/page/WebView.h"
#include "core/page/Window.h"

#include "core/modules/message_loop/MessageLoop.h"

#include <EscargotPublic.h>

namespace Starfish {

ScriptEngineInstance::ScriptEngineInstance(const char* locale,
                                           const char* timezone, WebView* wv)
{
    class EscargotStarfishPlatform : public Escargot::PlatformRef {
    public:
        EscargotStarfishPlatform(WebView* wv)
            : m_webView(wv)
        {
        }

        virtual void markJSJobEnqueued(
            Escargot::ContextRef* relatedContext) override
        {
            Window* window =
                (Window*)relatedContext->globalObject()->extraData();

            window->webView()->messageLoop()->addMicroTask(
                window,
                [](size_t handle, void* data) {
                    VMInstanceRef* vm = (VMInstanceRef*)data;
                    if (vm->hasPendingJob()) {
                        auto jobResult = vm->executePendingJob();
                        if (jobResult.error) {
                            STARFISH_LOG_ERROR(
                                "Uncaught Error in JS job\n");
                        }
                    }
                },
                relatedContext->vmInstance());
        }

        virtual LoadModuleResult onLoadModule(
            Escargot::ContextRef* relatedContext,
            Escargot::ScriptRef* whereRequestFrom,
            Escargot::StringRef* moduleSrc) override
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

        virtual void hostImportModuleDynamically(
            ContextRef* relatedContext, ScriptRef* referrer, StringRef* src,
            PromiseObjectRef* promise) override
        {
            LoadModuleResult loadedModuleResult =
                onLoadModule(relatedContext, referrer, src);

            Evaluator::EvaluatorResult executionResult = Evaluator::execute(
                relatedContext,
                [](ExecutionStateRef* state,
                   LoadModuleResult loadedModuleResult,
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

        WebView* m_webView;
    };

    m_engineInstance = Escargot::VMInstanceRef::create(
        new EscargotStarfishPlatform(wv), locale, timezone);
}

void ScriptEngineInstance::dispose()
{
    m_engineInstance = nullptr;
}
} // namespace Starfish
