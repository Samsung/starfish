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

#include "StarfishConfig.h"

#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/Task.h"
#include "core/modules/serviceworker/Message.h"
#include "platform/process/base/ProcessType.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/ProgramOptions.h"
#include "core/modules/serviceworker/WorkerConfig.h"

#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"
#include "core/dom/ExecutionContext.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/JobQueue.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostConnection.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerClient.h"
#include "core/modules/serviceworker/host/ServiceWorkerContextManager.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"

namespace Starfish {

ServiceWorkerHostProcess* ServiceWorkerHostProcess::m_instance = nullptr;

ServiceWorkerHostProcess* ServiceWorkerHostProcess::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new ServiceWorkerHostProcess();
    }

    STARFISH_ASSERT(m_instance != nullptr);
    return m_instance;
}

void ServiceWorkerHostProcess::destroy()
{
    if (m_instance != nullptr) {
        m_instance->~ServiceWorkerHostProcess();
        m_instance = nullptr;
    }
}

ServiceWorkerHostProcess::ServiceWorkerHostProcess()
{
}

ServiceWorkerHostProcess::~ServiceWorkerHostProcess()
{
    if (m_SWContextManager != nullptr) {
        m_SWContextManager->destroy();
        m_SWContextManager = nullptr;
    }
}

void ServiceWorkerHostProcess::init(ThreadPool* threadPool)
{
    STARFISH_ASSERT(threadPool != nullptr);

#ifdef SERVICE_WORKER_USE_MULTI_PROCESS
    WorkerConfig::instance().set("app", "HOST");
#endif

    Message::init();

    m_SWContextManager = ServiceWorkerContextManager::instance();
    m_SWContextManager->init(threadPool);

    m_threadPool = threadPool;
    m_messageLoop = m_threadPool->messageLoop();

    m_jobHandler = new ServiceWorkerHostJobHandler(m_messageLoop, this);
    m_ioRunnable = new IORunnable(m_messageLoop);
    m_ioThread = new AdaptedThread(m_threadPool);

    // start I/O runner
    m_ioThread->start(m_ioRunnable);
}

void ServiceWorkerHostProcess::start(
    std::shared_ptr<ProgramOptions> programOptions)
{
    STARFISH_ASSERT(programOptions != nullptr);

    std::string origin = programOptions->get("origin");
    std::string encodedOrigin = StringUtils::toBase64(origin);

    // create a connection
    m_connection = new ServiceWorkerHostConnection(this);

    registerConnection(m_connection);

    std::string address = IPC_PROTOCOL;
    address.append(IPC_ADDRESS_PREFIX);
#ifdef SERVICE_WORKER_USE_HOST_ON_EACH_PROCESS
    address.append(encodedOrigin);
#else
    address.append(IPC_ADDRESS);
#endif

    SWHOST_LOG_IF_ALLOWED(1, "host: bind: %s\n", address.c_str());
    SWHOST_LOG_IF_ALLOWED(1, "host: origin: %s\n", origin.c_str());

    STARFISH_ASSERT(m_connection != nullptr);
    STARFISH_ASSERT(m_ioRunnable != nullptr);

    m_connection->socket()->bind(address.c_str());
    m_ioRunnable->addClient(m_connection);
}

void ServiceWorkerHostProcess::registerConnection(
    ServiceWorkerHostConnection* connection)
{
    STARFISH_ASSERT(connection != nullptr);
    m_connections.push_back(connection);
}

void ServiceWorkerHostProcess::getConnections(
    GCVector<ServiceWorkerClientProcessInterface*>& connections)
{
    connections.assign(m_connections.begin(), m_connections.end());
}

ServiceWorkerHostJobHandler* ServiceWorkerHostProcess::jobHandler()
{
    STARFISH_ASSERT(m_jobHandler != nullptr);
    return m_jobHandler;
}

bool ServiceWorkerHostProcess::isTerminating()
{
    return m_isTerminating;
}

bool ServiceWorkerHostProcess::tryTerminate()
{
    SWHOST_LOG_IF_ALLOWED(1, "0. called\n");

    if (m_isTerminating == true) {
        return true;
    }

    if (jobHandler()->isEmptyRegistrationMap() == true) {
        m_isTerminating = true;
        // TODO: notify termination
        m_messageLoop->addIdler(
            nullptr,
            [](size_t handle, void* data0) {
                SWHOST_LOG_IF_ALLOWED(1, "1. destroy SW server\n");
                castTo<ServiceWorkerHostProcess*>(data0)->destroy();
            },
            this);
    } else {
        SWHOST_LOG_IF_ALLOWED(1, "1. registration map isn't empty\n");
        return false;
    }

    return true;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
