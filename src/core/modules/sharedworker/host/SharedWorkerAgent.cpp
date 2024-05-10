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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/WorkerIPCAddress.h"
#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/host/WorkerHostManager.h"
#include "core/modules/worker/util/LocalStorageHelper.h"
#include "core/modules/sharedworker/SharedWorkerMessage.h"
#include "core/modules/sharedworker/SharedWorkerKey.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"
#include "core/modules/sharedworker/host/SharedWorkerThread.h"
#include "core/modules/sharedworker/host/SharedWorkerGlobalScope.h"
#include "core/modules/sharedworker/host/SharedWorkerAgentServer.h"
#include "core/modules/sharedworker/host/SharedWorkerAgent.h"

namespace Starfish {

WorkerAgent* WorkerAgent::create(Starfish* starfish)
{
    TRACE(SHAREDWORKER);

    if (!WorkerAgent::g_workerAgentInstance) {
        WorkerAgent::g_workerAgentInstance =
            new (NoGC) SharedWorkerAgent(starfish);
    }

    return WorkerAgent::g_workerAgentInstance;
}

SharedWorkerAgent* SharedWorkerAgent::instance()
{
    STARFISH_ASSERT(WorkerAgent::g_workerAgentInstance != nullptr);

    return reinterpret_cast<SharedWorkerAgent*>(
        WorkerAgent::g_workerAgentInstance);
}

SharedWorkerAgent::SharedWorkerAgent(Starfish* starfish)
    : WorkerAgent(starfish)
    , m_messageLoop(MessageLoop::create())
    , m_mutex(new Mutex())
{
    PerProcess* perProcess = m_workerHostManager->perProcess();
    perProcess->initialize();

    m_ipcAddress = new WorkerIPCAddress(perProcess->workerSettings(),
                                        PATH_SHARED_WORKER_IPC_DIR);
    m_ipcAddress->acquire();

    m_server = new SharedWorkerAgentServer(
        perProcess, m_ipcAddress->createIPCAddress(WORKER_IPC_PROCESS_NAME));
}

void SharedWorkerAgent::start()
{
    m_server->start();
}

void SharedWorkerAgent::destroy()
{
    TRACE(SHAREDWORKER);
    if (WorkerAgent::g_workerAgentInstance == nullptr) {
        return;
    }

    m_server->close();
    m_server->~SharedWorkerAgentServer();
    m_server = nullptr;

    m_ipcAddress->release();

    WorkerAgent::destroy();
}

SharedWorkerThread* SharedWorkerAgent::getWorkerThread(
    const SharedWorkerMessage::RequestGetSharedWorker& message)
{
    const auto& iter = m_workerThreads.find(message.sharedWorkerKey());
    if (iter != m_workerThreads.end()) {
        SharedWorkerThread* thread = iter->second;
        if (!thread->wasTerminated() && !thread->globalScope()->isClosing()) {
            return iter->second;
        } else {
            m_workerThreads.erase(iter);
        }
    }

    SharedWorkerThread* thread =
        new SharedWorkerThread(m_starfish, m_messageLoop, message.name(),
                               message.workerHostInitData());

    m_workerThreads.insert({ message.sharedWorkerKey(), thread });

    return thread;
}

uint32_t SharedWorkerAgent::createIdentifier()
{
    IdHash hash;

    while (true) {
        uint32_t identifier = hash(SharedWorkerIdentifier::generate());
        std::string path =
            m_ipcAddress->getIPCHandlePath() + "/" + std::to_string(identifier);
        if (!LocalStorageHelper::File::exists(path)) {
            return identifier;
        }
    }
}

MessagePortConnectionInfo* SharedWorkerAgent::createConnectionInfo(
    uint32_t clientID, SharedWorkerThread* thread)
{
    uint32_t identifier = createIdentifier();
    auto* info = new MessagePortConnectionInfo(identifier, clientID, thread);

    {
        Locker<Mutex> lock(*m_mutex);
        m_connectionInfos.push_back(info);
    }

    return info;
}

void SharedWorkerAgent::connectWorkerThread(
    const SharedWorkerMessage::RequestGetSharedWorker& message)
{
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());

    SharedWorkerThread* thread =
        SharedWorkerAgent::instance()->getWorkerThread(message);

    MessagePortConnectionInfo* info =
        createConnectionInfo(message.clientID(), thread);
    if (!thread->isRunning()) {
        thread->startWithIdentifier(info->identifier);
    } else {
        thread->requestSharedWorkerConnection(info);
    }
}

void SharedWorkerAgent::didGlobalScopeConnected(
    SharedWorkerGlobalScope* globalScope,
    SharedWorkerMessagePortConnection* connection)
{
    STARFISH_ASSERT(!m_messageLoop->calledOnValidThread());

    m_messageLoop->addIdlerWithNoGCRootingInOtherThread(
        nullptr,
        [](size_t handle, void* data, void* data1) {
            auto* connection =
                static_cast<SharedWorkerMessagePortConnection*>(data1);

            SharedWorkerAgent::instance()
                ->server()
                ->responseShareWorkerConnection(connection);
        },
        globalScope, connection);
}

Nullable<MessagePortConnectionInfo*> SharedWorkerAgent::getConnectionInfo(
    uint32_t identifier)
{
    Locker<Mutex> lock(*m_mutex);

    const auto& iter =
        std::find_if(m_connectionInfos.begin(), m_connectionInfos.end(),
                     [identifier](MessagePortConnectionInfo* info) {
                         return info->identifier == identifier;
                     });

    if (iter != m_connectionInfos.end()) {
        return *iter;
    }

    return Nullable<MessagePortConnectionInfo*>();
}

} // namespace Starfish

#endif
