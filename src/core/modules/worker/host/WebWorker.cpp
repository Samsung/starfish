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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/dom/ExecutionContext.h"

#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
#include "core/modules/serviceworker/host/ServiceWorkerContextManager.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/worker/host/WebWorker.h"

namespace Starfish {

WebWorker::WebWorker(Starfish* starfish, const char* locale,
                     const char* timezoneID, String* customUserAgentString)
    : WebBase(starfish, locale, timezoneID, customUserAgentString)
{
    STARFISH_ASSERT(starfish != nullptr);
    STARFISH_ASSERT(locale != nullptr);
    STARFISH_ASSERT(timezoneID != nullptr);
    STARFISH_ASSERT(customUserAgentString != nullptr);
    STARFISH_ASSERT(isMainThread() == true);

    m_SWContextManager = ServiceWorkerContextManager::instance();
    m_SWContextManager->init(threadPool());

    m_SWServer = ServiceWorkerServer::instance();
    m_SWServer->init(threadPool());
    m_SWServer->start();
}

WebWorker::~WebWorker()
{
    if (m_SWContextManager != nullptr) {
        m_SWContextManager->destroy();
        m_SWContextManager = nullptr;
    }

    if (m_SWServer != nullptr) {
        m_SWServer->destroy();
        m_SWServer = nullptr;
    }
}

WebWorker* WebWorker::create(Starfish* starfish, const char* locale,
                             const char* timezoneID,
                             String* customUserAgentString)
{
    STARFISH_ASSERT(starfish != nullptr);
    STARFISH_ASSERT(locale != nullptr);
    STARFISH_ASSERT(timezoneID != nullptr);
    STARFISH_ASSERT(customUserAgentString != nullptr);

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    return (WebWorker*)MessageLoop::runOnMainThreadSync([&]() -> size_t {
        WebWorker* webWorker =
            new WebWorker(starfish, locale, timezoneID, customUserAgentString);
        return (size_t)webWorker;
    });
#else
    return new WebWorker(starfish, locale, timezoneID, customUserAgentString);
#endif
}

void WebWorker::destory()
{
    this->~WebWorker();
}

void WebWorker::createScriptEngineInstance()
{
    STARFISH_ASSERT(isMainThread() == true);

    if (!m_scriptEngineInstance) {
        PromiseJobListener listener = [](Escargot::ExecutionStateRef* state,
                                         Escargot::JobRef* job) {
            STARFISH_ASSERT(state != nullptr);
            STARFISH_ASSERT(job != nullptr);

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
    STARFISH_ASSERT(isMainThread() == true);

    if (m_scriptEngineInstance) {
        m_scriptEngineInstance->dispose();

        delete m_scriptEngineInstance;
        m_scriptEngineInstance = nullptr;
    }
}

void WebWorker::loadJavaScript(const std::string& scriptURL,
                               const std::string& baseURL)
{
#ifdef STARFISH_WEBWORKER_HOST
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

        m_workerGlobalScope->workerScriptController()->loadJavaScript(
            resourceURL);
    });
#endif
}

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
