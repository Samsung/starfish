/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "StoragePathProvider.h"

#include "platform/file/PlatformDirectory.h"

#include "core/util/Archivable.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerHostManager.h"
#include "core/util/debug/Trace.h"
#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/WorkerManager.h"
#include "core/modules/worker/WorkerIPCAddress.h"
#include "core/modules/worker/PerProcess.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/notification/NotificationService.h"
#include "core/modules/serviceworker/host/ServiceWorkerScriptController.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"

#if defined(STARFISH_ENABLE_CAST_SERVICE)
#include "core/modules/cast/CastServer.h"
#endif

namespace Starfish {

WorkerAgent* WorkerAgent::create(Starfish* starfish)
{
    TRACE_SCOPE(SVCWORKER);

    if (!WorkerAgent::g_workerAgentInstance) {
        WorkerAgent::g_workerAgentInstance =
            new (NoGC) ServiceWorkerAgent(starfish);
    }

    return WorkerAgent::g_workerAgentInstance;
}

ServiceWorkerAgent* ServiceWorkerAgent::instance()
{
    STARFISH_ASSERT(WorkerAgent::g_workerAgentInstance != nullptr);

    return reinterpret_cast<ServiceWorkerAgent*>(
        WorkerAgent::g_workerAgentInstance);
}

ServiceWorkerAgent::ServiceWorkerAgent(Starfish* starfish)
    : WorkerAgent(starfish)
#if defined(STARFISH_ENABLE_SERVICE_WORKER_NOTIFICATION)
    , m_notificationService(new NotificationService())
#endif
{
    TRACE_SCOPE(SVCWORKER);

    m_workerHostManager->perProcess()->initialize();

    m_ipcAddress = new WorkerIPCAddress(
        starfish->storagePathProvider().getServiceWorkerDataDirectoryPath());
    m_ipcAddress->acquire();

    m_SWServer = new ServiceWorkerServer(perProcess(), m_ipcAddress);

#if defined(STARFISH_ENABLE_CAST_SERVICE)
    m_castServer = CastServer::instance();
#endif
}

void ServiceWorkerAgent::start()
{
    m_SWServer->start();

#if defined(STARFISH_ENABLE_CAST_SERVICE)
    m_castServer->start();
#endif
}

void ServiceWorkerAgent::destroy()
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(WorkerAgent::g_workerAgentInstance != nullptr);
    if (m_clientFunc) {
        m_clientFunc(WorkerAgentState::Terminated);
    }

    if (m_SWServer != nullptr) {
        m_SWServer->destroy();
        m_SWServer = nullptr;
    }

#if defined(STARFISH_ENABLE_CAST_SERVICE)
    if (m_castServer != nullptr) {
        m_castServer->destroy();
        m_castServer = nullptr;
    }
#endif

    for (const auto& webWorker : m_webWorkerList) {
        webWorker->destroy();
    }
    m_webWorkerList.clear();

    m_ipcAddress->release();

    WorkerAgent::destroy();
}

void ServiceWorkerAgent::onWebWorkerTerminated(WebWorker* worker)
{
    STARFISH_ASSERT(worker != nullptr);
}

void ServiceWorkerAgent::runServiceWorker(ServiceWorkerData* serviceWorker,
                                          bool forceBypassCache)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(serviceWorker != nullptr);
    // https://w3c.github.io/ServiceWorker/#run-service-worker

    // 4.1 Call the JavaScript InitializeHostDefinedRealm() abstract
    // operation with the following customizations:

    // - For the global object, create a new ServiceWorkerGlobalScope object.
    // Let workerGlobalScope be the created object.
    WebWorker* webWorker =
        new WebWorker(m_starfish, "ko-KR", "Asia/Seoul", String::emptyString);
    auto workerGlobalScope =
        webWorker->createGlobalScope<ServiceWorkerGlobalScope>(
            new ResourceURL(serviceWorker->scriptURL, serviceWorker->scopeURL));
    m_webWorkerList.push_back(webWorker);
    addGlobalScope(serviceWorker->clientContextId, workerGlobalScope);

    TRACE(SVCWORKER, "create a global scope",
          serviceWorker->scriptURL->toUTF8NonGCString());

    workerGlobalScope->setServiceWorkerData(serviceWorker);

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

    ServiceWorkerScriptController* serviceWorkerScriptController =
        workerGlobalScope->serviceWorkerScriptController();

    // 4.12 Let evaluationStatus be the result of running the classic script
    // script if script is a classic script, otherwise, the result of running
    // the module script script if script is a module script.
    ScriptLoadResult evaluationStatus = ScriptLoadResult::NotHandled;

    if (forceBypassCache) {
        TRACE(HOST, "Load main script from cache");
        evaluationStatus =
            serviceWorkerScriptController->loadJavaScriptFromCache(
                workerGlobalScope->executionContext()->documentURI());
    } else {
        TRACE(HOST, "Load main script from network");
        evaluationStatus = serviceWorkerScriptController->loadJavaScript(
            workerGlobalScope->executionContext()->documentURI());
    }

    if (evaluationStatus != ScriptLoadResult::Success) {
        STARFISH_LOG_WARN("Fail to load script: %s",
                          CSTR(serviceWorker->scriptURL));
        return;
    }

    // 4.15. Run the responsible event loop specified by settingsObject until
    // it is destroyed.

    // 4.16. Empty workerGlobalScope’s list of active timers.
}

void ServiceWorkerAgent::addGlobalScope(ServiceWorkerContextId id,
                                        ServiceWorkerGlobalScope* globalScope)
{
    STARFISH_ASSERT(m_globalScopeMap.find(id) == m_globalScopeMap.end());

    m_globalScopeMap.insert(std::make_pair(id, globalScope));
}

void ServiceWorkerAgent::removeGlobalScope(ServiceWorkerContextId id)
{
    m_globalScopeMap.erase(id);
}

Nullable<ServiceWorkerGlobalScope*>
ServiceWorkerAgent::findGlobalScopeByContextId(ServiceWorkerContextId id)
{
    auto it = m_globalScopeMap.find(id);
    if (it == m_globalScopeMap.end()) {
        return nullptr;
    }
    return it->second;
}

void ServiceWorkerAgent::runServiceWorker(ResourceURL* scriptURL)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(scriptURL != nullptr);
    ServiceWorkerData* data = new ServiceWorkerData();
    data->scriptURL = scriptURL->urlString();
    data->scopeURL = scriptURL->baseURI();
    runServiceWorker(data);
}

void ServiceWorkerAgent::abortServiceWorkerScript(
    ServiceWorkerData* serviceWorker)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(serviceWorker != nullptr);
}

} // namespace Starfish

#endif /* STARFISH_ENABLE_SERVICE_WORKER */
