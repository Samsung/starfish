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

#include "core/modules/serviceworker/WorkerConfig.h"

#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"
#include "core/dom/ExecutionContext.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/JobQueue.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostConnection.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"

#include "core/modules/serviceworker/PerProcess.h"

namespace Starfish {

void ServiceWorkerServer::destroy()
{
    if (m_client != nullptr) {
        m_client->onSWServerTerminated();
    }
}

ServiceWorkerServer::ServiceWorkerServer()
{
}

ServiceWorkerServer::~ServiceWorkerServer()
{
}

void ServiceWorkerServer::init(PerProcess* perProcess)
{
    STARFISH_ASSERT(perProcess != nullptr);

#ifdef STARFISH_WEBWORKER_HOST
    GlobalOptions::instance().set("app", "HOST");
#endif

    Message::init();

    m_perProcess = perProcess;
    m_jobHandler =
        new ServiceWorkerHostJobHandler(m_perProcess->messageLoop(), this);
}

// TODO: use ServiceWorkerProcessManager::createAddress once its process
// creation is seperated.
static std::string createAddress(const std::string& lastAddress = "")
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

void ServiceWorkerServer::start()
{
    // create a connection
    m_connection = new ServiceWorkerHostConnection(this);
    registerConnection(m_connection);

    std::string address = createAddress();

    SWHOST_LOG_IF_ALLOWED(1, "host: bind: %s", address.c_str());
    m_connection->socket()->bind(address.c_str());
    m_perProcess->ioRunnable()->addClient(m_connection);
}

void ServiceWorkerServer::start(std::shared_ptr<ProgramOptions> programOptions)
{
    STARFISH_ASSERT(programOptions != nullptr);

    std::string origin = programOptions->get("origin");
    std::string encodedOrigin = Base64Utils::encodeBase64(origin);

    // create a connection
    m_connection = new ServiceWorkerHostConnection(this);

    registerConnection(m_connection);

    std::string address = createAddress(encodedOrigin);

    SWHOST_LOG_IF_ALLOWED(1, "host: bind: %s", address.c_str());
    SWHOST_LOG_IF_ALLOWED(1, "host: origin: %s", origin.c_str());

    STARFISH_ASSERT(m_connection != nullptr);

    m_connection->socket()->bind(address.c_str());
    m_perProcess->ioRunnable()->addClient(m_connection);
}

void ServiceWorkerServer::registerConnection(
    ServiceWorkerHostConnection* connection)
{
    STARFISH_ASSERT(connection != nullptr);
    m_connections.push_back(connection);
}

void ServiceWorkerServer::getConnections(
    GCVector<IServiceWorkerClientConnection*>& connections)
{
    connections.assign(m_connections.begin(), m_connections.end());
}

ServiceWorkerHostJobHandler* ServiceWorkerServer::jobHandler()
{
    STARFISH_ASSERT(m_jobHandler != nullptr);
    return m_jobHandler;
}

bool ServiceWorkerServer::isTerminating()
{
    return m_isTerminating;
}

bool ServiceWorkerServer::tryTerminate()
{
    SWHOST_LOG_IF_ALLOWED(1, "0. called");

    if (m_isTerminating == true) {
        return true;
    }

    if (jobHandler()->isEmptyRegistrationMap() == true) {
        m_isTerminating = true;
        // TODO: notify termination
        m_perProcess->messageLoop()->addIdler(
            nullptr,
            [](size_t handle, void* data0) {
                SWHOST_LOG_IF_ALLOWED(1, "1. destroy SW server");
                castTo<ServiceWorkerServer*>(data0)->destroy();
            },
            this);
    } else {
        SWHOST_LOG_IF_ALLOWED(1, "1. registration map isn't empty");
        return false;
    }

    return true;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
