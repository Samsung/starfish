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

#include "platform/process/base/ProcessType.h"

#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"

#include "platform/process/base/ProcessType.h"
#include "platform/process/base/Process.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/threading/ThreadPool.h"

#include "core/modules/threading/IRunnable.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/serviceworker/IORunnable.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientProcess.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#define IPC_ADDRESS_PREFIX "ipc://.ipc/"

namespace Starfish {

ServiceWorkerProcessManager* ServiceWorkerProcessManager::m_instance = nullptr;

ServiceWorkerProcessManager* ServiceWorkerProcessManager::getInstance()
{
    if (!m_instance) {
        m_instance = new ServiceWorkerProcessManager();
    }
    return m_instance;
}

void ServiceWorkerProcessManager::init(ThreadPool* threadPool)
{
    m_threadPool = threadPool;

    m_ioRunnable = new IORunnable(m_threadPool->messageLoop());
    m_ioThread = new AdaptedThread(m_threadPool);
    m_ioThread->start(m_ioRunnable);

    // create mock instances
    m_serviceWorkerHostProcess = ServiceWorkerHostProcess::getInstance();
    m_serviceWorkerHostProcess->init(m_threadPool->messageLoop());
    m_serviceWorkerClientProcess = ServiceWorkerClientProcess::getInstance();
}

void ServiceWorkerProcessManager::destroy()
{
    if (m_instance) {
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
{
}

ServiceWorkerProcessManager::~ServiceWorkerProcessManager()
{
    if (m_ioThread) {
        m_ioThread->stop();
    }
}

ServiceWorkerClientConnection* ServiceWorkerProcessManager::getConnection(
    std::string origin)
{
    STARFISH_ASSERT(m_threadPool);

    std::shared_ptr<ProcessData> processData = nullptr;

    // check if a process for this origin exists
    auto it = m_mapOriginToProcessData.find(origin);
    if (it == m_mapOriginToProcessData.end()) {
        processData = std::make_shared<ProcessData>();
        m_mapOriginToProcessData.insert(std::make_pair(origin, processData));
        // TODO: launch a service worker process
    } else {
        processData = it->second;
    }

    STARFISH_ASSERT(processData);

    if (processData->connection == nullptr) {
        processData->connection = new ServiceWorkerClientConnection();

        std::string address = IPC_ADDRESS_PREFIX;
        // TODO: change the origin string to fd string
        address.append(origin);

        processData->connection->socket()->connect(address.c_str());
        m_ioRunnable->addClient(processData->connection);
    }

    return processData->connection;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
