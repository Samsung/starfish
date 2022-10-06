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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "WorkerConfig.h"

#include "platform/file/PlatformDirectory.h"

#include "core/util/Archivable.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/serviceworker/PerProcess.h"
#include "core/modules/serviceworker/ServiceWorkerOption.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/serviceworker/util/LocalStorageHelper.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/notification/NotificationService.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"

#if defined(STARFISH_ENABLE_CAST_SERVICE)
#include "core/modules/cast/CastServer.h"
#endif

namespace Starfish {

ServiceWorkerAgent* ServiceWorkerAgent::m_instance = nullptr;

ServiceWorkerAgent* ServiceWorkerAgent::create(Starfish* starfish,
                                               PerProcess* perProcess)
{
    TRACE_SCOPE(SVCWORKER);
    // TODO: remove this instantiation after checking service worker
    // host running on another process. (host/ServiceWorkerExecutor)

    STARFISH_ASSERT(m_instance == nullptr);

    m_instance = new (NoGC) ServiceWorkerAgent(starfish, perProcess);

    return m_instance;
}

ServiceWorkerAgent* ServiceWorkerAgent::instance()
{
    STARFISH_ASSERT(m_instance != nullptr);

    return m_instance;
}

ServiceWorkerAgent::ServiceWorkerAgent(Starfish* starfish,
                                       PerProcess* perProcess)
    : m_starfish(starfish)
    , m_perProcess(perProcess)
    , m_notificationService(new NotificationService())
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(perProcess);

    m_SWServer = new ServiceWorkerServer(m_starfish);
    m_SWServer->init(m_perProcess);
    m_SWServer->start();

#if defined(STARFISH_ENABLE_CAST_SERVICE)
    m_castServer = CastServer::instance();
    m_castServer->start();
#endif
    createLocalStorageRootDir();
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
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(m_instance != nullptr);
    if (m_clientFunc) {
        m_clientFunc(ServiceWorkerAgentState::Terminated);
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

#if defined(STARFISH_WEBWORKER_HOST)
    for (const auto& webWorker : m_webWorkerList) {
        webWorker->destroy();
    }
    m_webWorkerList.clear();
#endif

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
    TRACE_SCOPE(SVCWORKER);
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
    addGlobalScope(serviceWorker->clientContextId, workerGlobalScope);

    TRACE(SVCWORKER, "create a global scope",
          serviceWorker->scriptURL->toUTF8NonGCString());

    // Register this global ccope as the current one.
    WorkerGlobalScope::enter(workerGlobalScope);
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

    WorkerScriptController* workerScriptController =
        workerGlobalScope->workerScriptController();

    // 4.12 Let evaluationStatus be the result of running the classic script
    // script if script is a classic script, otherwise, the result of running
    // the module script script if script is a module script.
    ScriptLoadResult evaluationStatus = workerScriptController->loadJavaScript(
        workerGlobalScope->executionContext()->documentURI());
    if (evaluationStatus != ScriptLoadResult::Success) {
        STARFISH_LOG_WARN("Fail to load script: %s",
                          CSTR(serviceWorker->scriptURL));
        return;
    }

    // 4.15. Run the responsible event loop specified by settingsObject until
    // it is destroyed.

// 4.16. Empty workerGlobalScope’s list of active timers.
#endif /* STARFISH_WEBWORKER_HOST */
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

void ServiceWorkerAgent::runServiceWorker(String* scriptURL)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(scriptURL != nullptr);
    ServiceWorkerData* data = new ServiceWorkerData();
    data->scriptURL = scriptURL;
    runServiceWorker(data);
}

void ServiceWorkerAgent::abortServiceWorkerScript(
    ServiceWorkerData* serviceWorker)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(serviceWorker != nullptr);
}

void ServiceWorkerAgent::createLocalStorageRootDir()
{
    LocalStorageHelper::File::mkdirIfNotExists(
        m_starfish->serviceWorkerOption()->localStorageRootDir());
}

} // namespace Starfish
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
