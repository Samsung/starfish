/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER)

#include "StarfishConfig.h"

#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/WorkerIPCAddress.h"
#include "core/modules/sharedworker/SharedWorker.h"
#include "core/modules/sharedworker/SharedWorkerMessage.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"
#include "core/modules/sharedworker/client/SharedWorkerClient.h"
#include "core/modules/sharedworker/client/SharedWorkerProcessManager.h"

namespace Starfish {

SharedWorkerProcessManager* SharedWorkerProcessManager::m_instance = nullptr;

SharedWorkerProcessManager* SharedWorkerProcessManager::instance()
{
    if (m_instance == nullptr) {
        m_instance = new SharedWorkerProcessManager();
    }
    return m_instance;
}

SharedWorkerProcessManager::SharedWorkerProcessManager()
    : m_perProcess(nullptr)
    , m_ipcAddress(nullptr)
    , m_client(nullptr)
    , m_isStarted(false)
{
}

void SharedWorkerProcessManager::init(PerProcess* perProcess)
{
    STARFISH_ASSERT(!m_perProcess);
    // For shared worker, the PerProcess is initialized when the shared worker
    // is used.
    m_perProcess = perProcess;
}

void SharedWorkerProcessManager::start()
{
    STARFISH_ASSERT(m_perProcess);

    if (m_isStarted) {
        return;
    }
    m_isStarted = true;

    m_perProcess->initialize();

    m_ipcAddress = new WorkerIPCAddress(m_perProcess->workerSettings(),
                                        PATH_SHARED_WORKER_IPC_DIR);

    m_client = new SharedWorkerClient(
        m_perProcess, m_ipcAddress->createIPCAddress(WORKER_IPC_PROCESS_NAME));

    m_client->start();
}

void SharedWorkerProcessManager::requestConnection(SharedWorker* sharedWorker)
{
    start();

    m_client->requestConnection(sharedWorker);

    addSharedWorkerObject(sharedWorker);
}

void SharedWorkerProcessManager::addSharedWorkerObject(
    SharedWorker* sharedWorker)
{
    m_sharedWorkers.insert({ sharedWorker->clientID(), sharedWorker });
}

Nullable<SharedWorker*> SharedWorkerProcessManager::getSharedWorkerObject(
    int32_t clientID)
{
    const auto& iter = m_sharedWorkers.find(clientID);
    if (iter != m_sharedWorkers.end()) {
        return iter->second;
    }

    return Nullable<SharedWorker*>();
}

void SharedWorkerProcessManager::destroy()
{
    if (!m_isStarted) {
        return;
    }

    for (const auto& connection : m_connections) {
        connection->close();
    }

    for (const auto& iter : m_sharedWorkers) {
        m_client->requestClose(iter.second);
    }

    m_client->~SharedWorkerClient();
    m_client = nullptr;

    m_sharedWorkers.clear();

    m_instance = nullptr;
}

SharedWorkerMessagePortConnection*
SharedWorkerProcessManager::createMessagePortConnection(
    SharedWorker* sharedWorker,
    const SharedWorkerMessage::ResponseGetSharedWorker& message)
{
    SharedWorkerMessagePortConnection* connection =
        new SharedWorkerMessagePortConnection(
            SharedWorkerProcessManager::instance()->perProcess(),
            sharedWorker->port(), message.identifier(), message.clientID(),
            message.ipcAddress());

    m_connections.push_back(connection);

    return connection;
}

void SharedWorkerProcessManager::startMessagePortConnection(
    const SharedWorkerMessage::ResponseGetSharedWorker& message)
{
    TRACE(SHAREDWORKER, message.ipcAddress());

    Nullable<SharedWorker*> sharedWorker =
        SharedWorkerProcessManager::instance()->getSharedWorkerObject(
            message.clientID());

    if (!sharedWorker.hasValue()) {
        TRACE(SHAREDWORKER, "shared worker closed.");
        return;
    }

    SharedWorkerMessagePortConnection* connection =
        createMessagePortConnection(sharedWorker.value(), message);

    connection->connect();

    // TODO: start message port
}

} // namespace Starfish

#endif
