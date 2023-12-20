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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/util/Id.h"
#include "core/util/Archivable.h"
#include "core/page/GlobalScope.h"
#include "platform/process/base/ProcessType.h"
#include "platform/process/base/Process.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/WebOrigin.h"

#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/Message.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/FetchEventHandler.h"

#include "core/modules/serviceworker/PerProcess.h"

#if !defined(STARFISH_USE_WORKER_PROCESS)
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
namespace LWEDelegate {
extern Starfish::Starfish* g_starfishInstance;
}
#endif
#include "core/modules/serviceworker/ServiceWorker.h"
#include "core/modules/serviceworker/client/RegistrationManager.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"

#include "core/modules/serviceworker/push/PushServiceAgent.h"
#include "core/modules/serviceworker/WorkerConfig.h"
#include <EscargotPublic.h>

#include "core/modules/serviceworker/util/MessageQueue/MessageQueue.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/ServiceWorkerOption.h"

#include <sys/stat.h>

#ifdef STARFISH_ENABLE_SERVICE_WORKER

using Escargot::Globals;

namespace Starfish {

#define SERVICE_WORKER_THREAD_POOL_SIZE 1

ServiceWorkerProcessManager* ServiceWorkerProcessManager::m_instance = nullptr;

ServiceWorkerProcessManager* ServiceWorkerProcessManager::instance()
{
    if (m_instance == nullptr) {
        m_instance = new ServiceWorkerProcessManager();
    }
    return m_instance;
}

void ServiceWorkerProcessManager::init(PerProcess* perProcess,
                                       ServiceWorkerOption* option)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(perProcess);

    Message::init();

    m_perProcess = perProcess;
    m_option = option;
    m_pushServiceAgent = new PushServiceAgent();
    m_registrationManager = new RegistrationManager(option);
}

void ServiceWorkerProcessManager::destroy()
{
    TRACE_SCOPE(SVCWORKER);
#if !defined(STARFISH_USE_WORKER_PROCESS)
    m_promiseStopThreadSignal.set_value();
#endif
}

static const int kMessageQueueTimeout = 500;

bool ServiceWorkerProcessManager::startWorkerOnThread(std::string scriptURL)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(Globals::supportsThreading());

#if !defined(STARFISH_USE_WORKER_PROCESS)
    // TODO: create a Runnable for this thread once verified.
    std::thread(
        [](PerProcess* perProcess, std::future<void>&& stopTask) {
            TRACE0(SVCWORKER, "Worker thread starts");

            Globals::initializeThread();

            auto messageQueue = std::make_unique<MessageQueue>();

            // TODO: create global variables that a starfish instance has.
            // We can not use the starfish instance on the main thread.
            Starfish* starfish = nullptr;
            ServiceWorkerAgent* agent =
                ServiceWorkerAgent::create(starfish, perProcess);

            // start a message loop
            messageQueue->run(kMessageQueueTimeout, [&]() -> bool {
                // TODO: create a StopTask and push it into this queue.
                if (stopTask.wait_for(std::chrono::milliseconds(1)) !=
                    std::future_status::timeout) {
                    messageQueue->stop();
                    return false;
                }
                return true;
            });

            agent->destroy();

            TRACE0(SVCWORKER, "Worker thread ends");
        },
        m_perProcess, std::move(m_promiseStopThreadSignal.get_future()))
        .detach();
    return true;
#else
    return false;
#endif
}

// TODO: There should be a better place for this.
// Seemingly, PlatformFile can't check accessibility well in case
// a type of a given file is for a pipe or a socket.
static bool isFile(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool ServiceWorkerProcessManager::processExist(const std::string identifier)
{
#if !defined(STARFISH_USE_WORKER_PROCESS)
    STARFISH_ASSERT_NOT_REACHED();
#endif
    // Here we use the socket handle promised exists.
    auto handlePath = Connection::Config::getHandlePath(identifier);
    bool exist = isFile(handlePath);
    TRACE(SVCWORKER, "result: ", exist);
    return exist;
}

ServiceWorkerClientConnection* ServiceWorkerProcessManager::getConnection(
    String* originSerialized)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(originSerialized != nullptr);

    std::string origin = CSTR(originSerialized);

    std::shared_ptr<ProcessData> processData = nullptr;

    std::string encodedOrigin = Base64Utils::encodeBase64(origin);
    std::string address = Connection::Config::createAddress(encodedOrigin);

    TRACE(SVCWORKER, "origin", origin);
    TRACE(SVCWORKER, "encodedOrigin", encodedOrigin);

    // check if a process for this origin exists
    auto it = m_mapOriginToProcessData.find(origin);
    if (it == m_mapOriginToProcessData.end()) {
        processData = std::make_shared<ProcessData>();
        m_mapOriginToProcessData.insert(std::make_pair(origin, processData));

#if !defined(STARFISH_USE_WORKER_PROCESS)
        startWorkerOnThread("");
#else
        auto swProcessExecutor = m_option->serviceWorkerProcessExecutor();
        if (swProcessExecutor) {
            if (!swProcessExecutor()) {
                STARFISH_LOG_ERROR("Fail to launch Service Worker process");
            }
        } else {
            // TODO: extract process creation
            // TODO: check if instance exists
            // create an arguments
            std::vector<std::string> args;

            // TODO: use a constant executable name
            args.push_back("./Starfish-serviceworker");
            args.push_back("--debug-worker=" +
                           GlobalOptions::instance().get("DEBUG_WORKER"));

            if (!processExist(encodedOrigin)) {
                if (ProcessUtil::launchProcess(args, &processData->pid) ==
                    true) {
                    TRACE(SVCWORKER, "launchProcess: success");
                } else {
                    TRACE(SVCWORKER, "launchProcess: fail");
                }
            }
        }

#endif

    } else {
        processData = it->second;
    }

    STARFISH_ASSERT(processData != nullptr);

    if (m_connection != nullptr) {
        processData->connection = m_connection;
    } else {
        if (processData->connection == nullptr) {
            processData->connection = new ServiceWorkerClientConnection();
            m_perProcess->ioRunnable()->addClient(processData->connection);
            processData->connection->socket()->connect(address.c_str());
        }
#ifdef SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
        if (m_connection == nullptr) {
            m_connection = processData->connection;
        }
#endif
    }

    STARFISH_ASSERT(processData->connection != nullptr);

    TRACE(SVCWORKER, "client: connect: ", address.c_str());
    TRACE(SVCWORKER, "client: origin: ", origin.c_str());

    return processData->connection;
}

const GCVector<ServiceWorkerEnvironment*>&
ServiceWorkerProcessManager::getSettingsObjects(String* scriptURL)
{
    TRACE_SCOPE(SVCWORKER);

    if (m_settingsObjectsNeedUpdated) {
        m_settingsObjects.clear();

        for (auto it = m_mapIdToActiveGlobalScope.begin();
             it != m_mapIdToActiveGlobalScope.end(); it++) {
            GlobalScope* globalScope = it->second;
            auto executionContext = globalScope->executionContext();

            // 3. Let settingsObjects be all environment settings objects whose
            // origin is worker’s script url's origin.
            auto url = new ResourceURL(scriptURL);
            auto origin = url->baseURI();

            TRACE(SVCWORKER, "worker’s script url's origin", CSTR(origin));
            TRACE(SVCWORKER, "baseURL()->baseURI()",
                  CSTR(executionContext->baseURL()->baseURI()));

            if ((executionContext != nullptr) &&
                origin->compare(executionContext->baseURL()->baseURI()) == 0) {
                TRACE(SVCWORKER, "a settingsObject is found");
                m_settingsObjects.push_back(it->second->executionContext());
            }
        }
        m_settingsObjectsNeedUpdated = false;
    }
    return m_settingsObjects;
}

void ServiceWorkerProcessManager::registerActiveGlobalScope(
    Id<GlobalScope> id, GlobalScope* globalScope)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(globalScope != nullptr);

    TRACE(CLIENT, "1: ", CSTR(globalScope->executionContext()->urlString()));

    m_mapIdToActiveGlobalScope.insert(std::make_pair(id, globalScope));

    auto fetchEventHandler = new FetchEventHandler();
    m_fetchEventHandlers.insert(std::make_pair(id, fetchEventHandler));

    auto scope = globalScope->executionContext()->baseURL()->baseURI();
    if (m_registrationManager->isActivatedRegistration(scope)) {
        TRACEF(CLIENT, "'%s' is a registered service worker.", CSTR(scope));
        TRACE(CLIENT, "ServiceWorker network mode");

        auto connection = getConnection(scope);
        m_registrationManager->startRegisteredServiceWorkerContext(connection,
                                                                   id, scope);
        fetchEventHandler->start(connection, scope);
        fetchEventHandler->setFetchFromServiceWorker(true);
    } else {
        TRACE(CLIENT, "Starfish network mode");
    }

    if (m_connection) {
        // TODO: check whether of not this context's serviceworker is valid.
        // m_connection->sendContextRequest(new ContextRequestData(
        //     id, ServiceWorkerClientRequestType::Register));
    }
    m_settingsObjectsNeedUpdated = true;
}

void ServiceWorkerProcessManager::deregisterActiveGlobalScope(
    Id<GlobalScope> id)
{
    TRACE_SCOPE(SVCWORKER);
    // find activeWorker
    auto globalScope = findGlobalScope(id);

    STARFISH_ASSERT(globalScope != nullptr);

    TRACE(CLIENT, "1: ", CSTR(globalScope->executionContext()->urlString()));

    m_mapIdToActiveGlobalScope.erase(id);

    if (GlobalOptions::instance().has("--leave-ipc-handle") == false) {
        if (m_connection != nullptr) {
            auto executionContext = globalScope->executionContext();
            auto activeServiceWorker = executionContext->activeServiceWorker();
            auto registrationId =
                activeServiceWorker
                    ? activeServiceWorker->data()->registrationId
                    : ServiceWorkerRegistrationId();

            m_connection->updateServiceWorkerClient(new ContextRequestData(
                id, ServiceWorkerClientRequestType::Unregister,
                registrationId));
        }
    } else {
        TRACE(IPC, "Not send unregistering service worker");
    }
    m_settingsObjectsNeedUpdated = true;
}

NULLABLE GlobalScope* ServiceWorkerProcessManager::findGlobalScope(
    Id<GlobalScope> id)
{
    TRACE_SCOPE(SVCWORKER);
    auto it = m_mapIdToActiveGlobalScope.find(id);
    if (it == m_mapIdToActiveGlobalScope.end()) {
        return nullptr;
    }
    return it->second;
}

Nullable<FetchEventHandler*> ServiceWorkerProcessManager::findFetchEventHandler(
    Id<GlobalScope> id)
{
    auto it = m_fetchEventHandlers.find(id);

    if (it == m_fetchEventHandlers.end()) {
        TRACE_SCOPE(SVCWORKER);
        return nullptr;
    }
    return it->second;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
