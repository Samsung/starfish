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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/util/Archivable.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/notification/NotificationService.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"

namespace Starfish {

#if !defined(SERVICE_WORKER_THREAD_POOL_SIZE)
#define SERVICE_WORKER_THREAD_POOL_SIZE 1
#endif

ServiceWorkerAgent* ServiceWorkerAgent::m_instance = nullptr;

ServiceWorkerAgent* ServiceWorkerAgent::create(Starfish* starfish)
{
    STARFISH_ASSERT(m_instance == nullptr);
    STARFISH_ASSERT(starfish != nullptr);

    m_instance = new (NoGC) ServiceWorkerAgent(starfish);

    return m_instance;
}

ServiceWorkerAgent* ServiceWorkerAgent::instance()
{
    STARFISH_ASSERT(m_instance != nullptr);

    return m_instance;
}

ServiceWorkerAgent::ServiceWorkerAgent(Starfish* starfish)
    : m_starfish(starfish)
    , m_messageLoop(new MessageLoop())
    , m_threadPool(
          new ThreadPool(SERVICE_WORKER_THREAD_POOL_SIZE, m_messageLoop))
    , m_SWServer(ServiceWorkerServer::instance())
    , m_notificationService(new NotificationService())
{
    STARFISH_ASSERT(starfish != nullptr);

    m_SWServer->init(m_threadPool);
    m_SWServer->start();
}

ServiceWorkerAgent::~ServiceWorkerAgent()
{
}

bool ServiceWorkerAgent::isCreated()
{
    return (m_instance != nullptr);
}

void ServiceWorkerAgent::destroy()
{
    STARFISH_ASSERT(m_instance != nullptr);

    if (m_SWServer != nullptr) {
        m_SWServer->destroy();
        m_SWServer = nullptr;
    }

#if defined(STARFISH_WEBWORKER_HOST)
    for (const auto& webWorker : m_webWorkerList) {
        webWorker->destroy();
    }
    m_webWorkerList.clear();
#endif

    if (m_threadPool != nullptr) {
        m_threadPool->destroy();
        m_threadPool = nullptr;
    }

    if (m_messageLoop != nullptr) {
        m_messageLoop->destroy();
        m_messageLoop = nullptr;
    }

    m_instance->ServiceWorkerAgent::~ServiceWorkerAgent();
    GC_FREE(m_instance);
    m_instance = nullptr;
}

void ServiceWorkerAgent::onWebWorkerTerminated(WebWorker* worker)
{
    STARFISH_ASSERT(worker != nullptr);
}

void ServiceWorkerAgent::registerOnStatusChangedHandler(
    ServiceWorkerAgentStateHandler func)
{
    STARFISH_ASSERT(func != nullptr);
    m_clientFunc = func;
}

void ServiceWorkerAgent::runServiceWorker(ServiceWorkerData* serviceWorker)
{
#if defined(STARFISH_WEBWORKER_HOST)
    STARFISH_ASSERT(serviceWorker != nullptr);
    // https://w3c.github.io/ServiceWorker/#run-service-worker

    // 4.1 Call the JavaScript InitializeHostDefinedRealm() abstract
    // operation with the following customizations:

    // - For the global object, create a new ServiceWorkerGlobalScope object.
    // Let workerGlobalScope be the created object.
    WebWorker* webWorker = WebWorker::create(m_starfish, "ko-KR", "Asia/Seoul",
                                             String::emptyString);
    auto workerGlobalScope =
        webWorker->createGlobalScope(serviceWorker->scriptURL);
    m_webWorkerList.push_back(webWorker);
    // - Let realmExecutionContext be the created JavaScript execution context.

    // 4.2 Set serviceWorker’s global object to workerGlobalScope.
    serviceWorker->setGlobalObject(workerGlobalScope);
    // 4.3 Let workerEventLoop be a newly created event loop.

    // 4.4 Let settingsObject be a new environment settings object whose
    // algorithms are defined as follows:

    // 4.11. If serviceWorker is an active worker, and there are any tasks
    // queued in serviceWorker’s containing service worker registration’s
    // task queues, queue them to serviceWorker’s event loop’s task queues
    // in the same order using their original task sources.

    // 4.12 Let evaluationStatus be the result of running the classic script
    // script if script is a classic script, otherwise, the result of running
    // the module script script if script is a module script.
    ScriptLoadResult evaluationStatus =
        workerGlobalScope->workerScriptController()->loadJavaScript(
            workerGlobalScope->executionContext()->documentURI());
    if (evaluationStatus != ScriptLoadResult::Success) {
        STARFISH_LOG_WARN("Fail to load script: %s\n",
                          CSTR(serviceWorker->scriptURL));
        return;
    }

// 4.15. Run the responsible event loop specified by settingsObject until
// it is destroyed.

// 4.16. Empty workerGlobalScope’s list of active timers.
#endif /* STARFISH_WEBWORKER_HOST */
}

void ServiceWorkerAgent::runServiceWorker(String* scriptURL)
{
    STARFISH_ASSERT(scriptURL != nullptr);
    ServiceWorkerData* data = new ServiceWorkerData();
    data->scriptURL = scriptURL;
    runServiceWorker(data);
}

void ServiceWorkerAgent::abortServiceWorkerScript(
    ServiceWorkerData* serviceWorker)
{
    STARFISH_ASSERT(serviceWorker != nullptr);
}
}
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
