/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WORKER)

#include "StarfishConfig.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerHost.h"
#include "core/modules/worker/host/DedicatedWorkerGlobalScope.h"
#include "core/modules/worker/WorkerHostProxy.h"

namespace Starfish {

WorkerHostProxy::WorkerHostProxy(ExecutionContext* executionContext,
                                 WorkerThread* workerThread)
    : WorkerProxy(executionContext, workerThread)
    , m_workerHost(nullptr)
    , m_wasWorkerScriptLoaded(false)
{
}

void WorkerHostProxy::workerHostCreated(WorkerHost* workerHost)
{
    STARFISH_ASSERT(!m_workerHost);
    m_workerHost = workerHost;

    setEntangledEventTarget(m_workerHost->globalScope());
}

MessageLoop* WorkerHostProxy::targetMessageLoop()
{
    STARFISH_ASSERT(m_workerHost);
    return m_workerHost->webWorker()->messageLoop();
}

ExecutionContext* WorkerHostProxy::targetExecutionContext()
{
    STARFISH_ASSERT(m_workerHost);
    return m_workerHost->globalScope()->executionContext();
}

void WorkerHostProxy::onScriptLoadFinished()
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    if (m_wasWorkerScriptLoaded) {
        return;
    }

    m_wasWorkerScriptLoaded = true;

    handleQueuedEarlyMessages();
}

void WorkerHostProxy::handleQueuedEarlyMessages()
{
    STARFISH_ASSERT(m_wasWorkerScriptLoaded);
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    GCVector<SerializeWithTransferResult*> messages;
    m_queuedEarlyMessages.swap(messages);

    for (SerializeWithTransferResult* message : messages) {
        postMessageToEntangledEventTarget(message);
    }

    m_queuedEarlyMessages.clear();
    m_queuedEarlyMessages.shrink_to_fit();
}

void WorkerHostProxy::postSerializedMessage(
    SerializeWithTransferResult* serializedMessage)
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    // After the worker's script is loaded, a message is sent to the worker.
    if (!m_wasWorkerScriptLoaded) {
        m_queuedEarlyMessages.push_back(serializedMessage);
        return;
    }

    postMessageToEntangledEventTarget(serializedMessage);
}

} // namespace Starfish
#endif
