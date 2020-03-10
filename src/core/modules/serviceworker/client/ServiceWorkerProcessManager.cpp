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

#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/Message.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"

#if !defined(SERVICE_WORKER_USE_SEPERATED_PROCESS)
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
namespace LWE {
extern Starfish::Starfish* g_starfishInstance;
}
#endif
#include "core/modules/serviceworker/ServiceWorker.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"

#include "core/modules/serviceworker/push/PushServiceAgent.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

#define SERVICE_WORKER_THREAD_POOL_SIZE 1

ServiceWorkerProcessManager* ServiceWorkerProcessManager::m_instance = nullptr;

ServiceWorkerProcessManager* ServiceWorkerProcessManager::instance()
{
    if (m_instance == nullptr) {
        m_instance = new ServiceWorkerProcessManager();
    }

    STARFISH_ASSERT(m_instance);
    return m_instance;
}

void ServiceWorkerProcessManager::init()
{
    GlobalOptions::instance().set("app", "CLIT");

    Message::init();

    m_messageLoop = new MessageLoop();
    m_threadPool =
        new ThreadPool(SERVICE_WORKER_THREAD_POOL_SIZE, m_messageLoop);

    // start I/O runner
    m_ioRunnable = new IORunnable(m_threadPool->messageLoop());
    m_ioThread = new AdaptedThread(m_threadPool);

    m_ioThread->start(m_ioRunnable);
    m_pushServiceAgent = new PushServiceAgent();
}

void ServiceWorkerProcessManager::destroy()
{
    if (m_instance != nullptr) {
        m_instance->~ServiceWorkerProcessManager();
        m_instance = nullptr;
    }
}

ServiceWorkerProcessManager::ServiceWorkerProcessManager()
{
}

ServiceWorkerProcessManager::~ServiceWorkerProcessManager()
{
    if (m_ioThread != nullptr) {
        m_ioThread->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (m_threadPool != nullptr) {
        m_threadPool->destroy();
    }

    if (m_messageLoop != nullptr) {
        m_messageLoop->destroy();
    }

#if !defined(SERVICE_WORKER_USE_SEPERATED_PROCESS)
    // create mock instances
    if (m_agent != nullptr) {
        m_agent->destroy();
    }

#endif
}

std::string ServiceWorkerProcessManager::createAddress(
    const std::string& lastAddress)
{
    std::string address = IPC_PROTOCOL;
    address.append(IPC_ADDRESS_PREFIX);

#ifdef SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
    address.append(IPC_ADDRESS);
#else
    address.append(lastAddress);
#endif

    return address;
}

ServiceWorkerClientConnection* ServiceWorkerProcessManager::getConnection(
    String* originSerialized)
{
    STARFISH_ASSERT(originSerialized != nullptr);
    STARFISH_ASSERT(m_threadPool != nullptr);

    std::string origin = CSTR(originSerialized);

    std::shared_ptr<ProcessData> processData = nullptr;

    std::string encodedOrigin = Base64Utils::encodeBase64(origin);
    std::string address = createAddress(encodedOrigin);

    // check if a process for this origin exists
    auto it = m_mapOriginToProcessData.find(origin);
    if (it == m_mapOriginToProcessData.end()) {
        processData = std::make_shared<ProcessData>();
        m_mapOriginToProcessData.insert(std::make_pair(origin, processData));

#if !defined(SERVICE_WORKER_USE_SEPERATED_PROCESS)
        // create mock instances
        m_agent = ServiceWorkerAgent::create(LWE::g_starfishInstance);
#else
        // TODO: extract process creation
        // TODO: check if instance exists
        // create an arguments
        std::vector<std::string> args;

        // TODO: use a constant executable name
        args.push_back("./StarfishWebWorker");
        args.push_back("--debug-worker=" +
                       GlobalOptions::instance().get("DEBUG_WORKER"));

        if (ProcessUtil::launchProcess(args, &processData->pid) == true) {
            SWCLIENT_LOG_IF_ALLOWED(1, "launchProcess: success\n");
        } else {
            SWCLIENT_LOG_IF_ALLOWED(1, "launchProcess: fail\n");
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
            processData->connection->socket()->connect(address.c_str());
            m_ioRunnable->addClient(processData->connection);
        }
#ifdef SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
        if (m_connection == nullptr) {
            m_connection = processData->connection;
        }
#endif
    }

    STARFISH_ASSERT(processData->connection != nullptr);

    SWCLIENT_LOG_IF_ALLOWED(1, "client: connect: %s\n", address.c_str());
    SWCLIENT_LOG_IF_ALLOWED(1, "client: origin: %s\n", origin.c_str());

    return processData->connection;
}

void ServiceWorkerProcessManager::registerActiveGlobalScope(
    Id<GlobalScope> id, GlobalScope* globalScope)
{
    STARFISH_ASSERT(globalScope != nullptr);

    SWCLIENT_LOG_IF_ALLOWED(1, "1: %s\n",
                            CSTR(globalScope->executionContext()->urlString()));

    m_mapIdToActiveGlobalScope.insert(std::make_pair(id, globalScope));

    if (m_connection) {
        // TODO: check whether of not this context's serviceworker is valid.
        // m_connection->sendContextRequest(new ContextRequestData(
        //     id, ServiceWorkerClientRequestType::Register));
    }
}

void ServiceWorkerProcessManager::deregisterActiveGlobalScope(
    Id<GlobalScope> id)
{
    // find activeWorker
    auto globalScope = findGlobalScope(id);

    STARFISH_ASSERT(globalScope != nullptr);

    SWCLIENT_LOG_IF_ALLOWED(1, "1: %s\n",
                            CSTR(globalScope->executionContext()->urlString()));

    m_mapIdToActiveGlobalScope.erase(id);

    if (m_connection != nullptr) {
        auto executionContext = globalScope->executionContext();
        auto activeServiceWorker = executionContext->activeServiceWorker();
        auto registrationId = activeServiceWorker
                                  ? activeServiceWorker->data()->registrationId
                                  : ServiceWorkerRegistrationId();

        m_connection->updateServiceWorkerClient(new ContextRequestData(
            id, ServiceWorkerClientRequestType::Unregister, registrationId));
    }
}

NULLABLE GlobalScope* ServiceWorkerProcessManager::findGlobalScope(
    Id<GlobalScope> id)
{
    auto it = m_mapIdToActiveGlobalScope.find(id);
    if (it == m_mapIdToActiveGlobalScope.end()) {
        return nullptr;
    }
    return it->second;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
