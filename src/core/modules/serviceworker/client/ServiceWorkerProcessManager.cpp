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

#include "core/modules/serviceworker/ProgramOptions.h"
#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/Message.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"

#ifndef SERVICE_WORKER_USE_MULTI_PROCESS
#include "core/modules/serviceworker/host/ServiceWorkerServerClient.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"
#endif

#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"

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
#ifndef SERVICE_WORKER_USE_MULTI_PROCESS
    WorkerConfig::instance().set("app", "BOTH");
#else
    WorkerConfig::instance().set("app", "CLIT");
#endif

    Message::init();

    m_messageLoop = new MessageLoop();
    m_threadPool =
        new ThreadPool(SERVICE_WORKER_THREAD_POOL_SIZE, m_messageLoop);

    // start I/O runner
    m_ioRunnable = new IORunnable(m_threadPool->messageLoop());
    m_ioThread = new AdaptedThread(m_threadPool);

    m_ioThread->start(m_ioRunnable);

#ifndef SERVICE_WORKER_USE_MULTI_PROCESS
    // create mock instances
    m_serviceWorkerHostProcess = ServiceWorkerHostProcess::getInstance();
    m_serviceWorkerHostProcess->init(m_threadPool);
#endif
}

void ServiceWorkerProcessManager::destroy()
{
    if (m_instance != nullptr) {
        m_instance->~ServiceWorkerProcessManager();
        m_instance = nullptr;
    }
}

ServiceWorkerProcessManager::ServiceWorkerProcessManager()
    : m_ioThread(nullptr)
    , m_threadPool(nullptr)
    , m_ioRunnable(nullptr)
    , m_serviceWorkerHostProcess(nullptr)
    , m_serviceWorkerClientProcess(nullptr)
    , m_connection(nullptr)
{
}

ServiceWorkerProcessManager::~ServiceWorkerProcessManager()
{
    if (m_ioThread != nullptr) {
        m_ioThread->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (m_threadPool) {
        m_threadPool->destroy();
    }

    if (m_messageLoop) {
        m_messageLoop->destroy();
    }
}

ServiceWorkerClientConnection* ServiceWorkerProcessManager::getConnection(
    String* originSerialized)
{
    STARFISH_ASSERT(originSerialized != nullptr);
    STARFISH_ASSERT(m_threadPool != nullptr);

    std::string origin = CSTR(originSerialized);

    std::shared_ptr<ProcessData> processData = nullptr;

    std::string encodedOrigin = StringUtils::toBase64(origin);

    // check if a process for this origin exists
    auto it = m_mapOriginToProcessData.find(origin);
    if (it == m_mapOriginToProcessData.end()) {
        processData = std::make_shared<ProcessData>();
        m_mapOriginToProcessData.insert(std::make_pair(origin, processData));

        // TODO: launch a service worker process
        auto po = std::make_shared<ProgramOptions>();

        STARFISH_ASSERT(po != nullptr);

        po->set("origin", origin.c_str());
        m_serviceWorkerHostProcess->start(po);

    } else {
        processData = it->second;
    }

    STARFISH_ASSERT(processData != nullptr);

#ifdef SERVICE_WORKER_USE_HOST_ON_EACH_PROCESS
    if (processData->connection == nullptr) {
        processData->connection = new ServiceWorkerClientConnection();

        std::string address = IPC_PROTOCOL;
        address.append(IPC_ADDRESS_PREFIX);
        address.append(encodedOrigin);

        SWCLIENT_LOG_IF_ALLOWED(1, "client: connect: %s\n", address.c_str());
        SWCLIENT_LOG_IF_ALLOWED(1, "client: origin: %s\n", origin.c_str());

        processData->connection->socket()->connect(address.c_str());
        m_ioRunnable->addClient(processData->connection);
    }
#else
    if (m_connection == nullptr) {
        m_connection = new ServiceWorkerClientConnection();
        std::string address = IPC_PROTOCOL;
        address.append(IPC_ADDRESS_PREFIX);
        address.append(IPC_ADDRESS);

        m_connection->socket()->connect(address.c_str());
        m_ioRunnable->addClient(m_connection);

        SWCLIENT_LOG_IF_ALLOWED(1, "client: connect: %s\n", address.c_str());
        SWCLIENT_LOG_IF_ALLOWED(1, "client: origin: %s\n", origin.c_str());
    }

    processData->connection = m_connection;
#endif

    return processData->connection;
}

void ServiceWorkerProcessManager::registerActiveGlobalScope(
    Id<GlobalScope> id, GlobalScope* globalScope)
{
    STARFISH_ASSERT(globalScope != nullptr);
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
    m_mapIdToActiveGlobalScope.erase(id);

    if (m_connection) {
        m_connection->updateServiceWorkerClient(new ContextRequestData(
            id, ServiceWorkerClientRequestType::Unregister));
    }
}

NULLABLE GlobalScope* ServiceWorkerProcessManager::find(Id<GlobalScope> id)
{
    auto it = m_mapIdToActiveGlobalScope.find(id);
    if (it == m_mapIdToActiveGlobalScope.end()) {
        return nullptr;
    }
    return it->second;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
