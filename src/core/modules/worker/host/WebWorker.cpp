/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifdef STARFISH_WEBWORKER_HOST

#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/threading/Thread.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

WebWorker::WebWorker(Starfish* starfish, const char* locale,
                     const char* timezoneID, String* customUserAgentString)
    : WebBase(starfish, locale, timezoneID, customUserAgentString)
    , m_workerGlobalScope(nullptr)
    , m_scriptEngineInstance(nullptr)
{
    STARFISH_ASSERT(starfish != nullptr && locale != nullptr &&
                    timezoneID != nullptr && customUserAgentString != nullptr);
    STARFISH_ASSERT(isMainThread());
}

WebWorker* WebWorker::create(Starfish* starfish, const char* locale,
                             const char* timezoneID,
                             String* customUserAgentString)
{
    return (WebWorker*)MessageLoop::runOnMainThreadSync([&]() -> size_t {
        STARFISH_ASSERT(starfish != nullptr && locale != nullptr &&
                        timezoneID != nullptr &&
                        customUserAgentString != nullptr);
        WebWorker* webWorker =
            new WebWorker(starfish, locale, timezoneID, customUserAgentString);
        return (size_t)webWorker;
    });
}

void WebWorker::createScriptEngineInstance()
{
    STARFISH_ASSERT(isMainThread());
    if (!m_scriptEngineInstance) {
        PromiseJobListener listener = [](Escargot::ExecutionStateRef* state,
                                         Escargot::JobRef* job) {
            STARFISH_ASSERT(state != nullptr && job != nullptr);
            ExecutionContext* executionContext =
                fetchExecutionContext(state->context());

            executionContext->webBase()->messageLoop()->addIdler(
                executionContext->globalScope(),
                [](size_t, void* data, void* data2) {
                    ExecutionContext* executionContext =
                        static_cast<ExecutionContext*>(data);

                    Escargot::JobRef* job = (Escargot::JobRef*)data2;
                    auto sbresult = job->run();

                    if (sbresult.error.hasValue()) {
                        STARFISH_LOG_ERROR(
                            "Uncaught %s\n",
                            toBrowserString(
                                executionContext->scriptBindingInstance(),
                                Escargot::ValueRef::create(
                                    sbresult.error.getValue()))
                                ->toUTF8NonGCString()
                                .data());
                    }
                },
                executionContext, job);
        };

        m_scriptEngineInstance = new ScriptEngineInstance(
            locale().getName(), timezoneID()->toUTF8NonGCString().data(),
            listener);
    }
}

void WebWorker::removeScriptEngineInstance()
{
    STARFISH_ASSERT(isMainThread());

    if (m_scriptEngineInstance) {
        m_scriptEngineInstance->dispose();

        delete m_scriptEngineInstance;
        m_scriptEngineInstance = nullptr;
    }
}

void WebWorker::loadJavaScript(const std::string& scriptURL,
                               const std::string& baseURL)
{
    m_messageLoop->runOnMainThreadAsync([=]() -> void {
        clearBlobURLStore();

        clearStack<ELABORATE_CLEAR_STACK_SIZE>();

        if (m_workerGlobalScope) {
            m_workerGlobalScope->dispose();
        }

        removeScriptEngineInstance();
        createScriptEngineInstance();
        ResourceURL* resourceURL =
            new ResourceURL(String::fromUTF8(scriptURL.data()),
                            String::fromUTF8(baseURL.data()));
        m_workerGlobalScope = new ServiceWorkerGlobalScope(
            this, resourceURL, String::createASCIIString("UTF-8"));

        m_workerGlobalScope->workerScriptController()->evaluate(resourceURL);
    });
}

WebWorker::~WebWorker()
{
}

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
