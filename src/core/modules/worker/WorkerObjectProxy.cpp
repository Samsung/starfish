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

#include "core/page/WebBase.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/Worker.h"
#include "core/modules/worker/WorkerThread.h"
#include "core/modules/worker/WorkerHostProxy.h"
#include "core/modules/worker/DedicatedWorkerGlobalScope.h"
#include "core/modules/worker/WorkerObjectProxy.h"

namespace Starfish {

WorkerObjectProxy::WorkerObjectProxy(ExecutionContext* executionContext,
                                     Worker* worker)
    : WorkerProxy(executionContext, worker->workerThread())
    , m_workerObject(worker)
{
    entangleTarget(m_workerObject, m_workerObject->workerHostProxy());

    addChildWorker();
}

MessageLoop* WorkerObjectProxy::targetMessageLoop()
{
    return m_workerObject->executionContext()->webBase()->messageLoop();
}

ExecutionContext* WorkerObjectProxy::targetExecutionContext()
{
    return m_workerObject->executionContext();
}

String* WorkerObjectProxy::workerName() const
{
    return m_workerObject->workerOptions().name();
}

void WorkerObjectProxy::postSerializedMessage(
    SerializeWithTransferResult* serializedMessage)
{
    postMessageToEntangledEventTarget(serializedMessage);
}

void WorkerObjectProxy::terminateWorker()
{
    close();

    postTask(
        [](void* data) {
            auto* workerObjectProxy = static_cast<WorkerObjectProxy*>(data);
            workerObjectProxy->workerObject()->terminate();
        },
        this);
}

Optional<WorkerGlobalScope*> WorkerObjectProxy::parentWorkerGlobalScope()
{
    ExecutionContext* parentExecutionContext = targetExecutionContext();
    if (parentExecutionContext->hasWorkerGlobalScope()) {
        return parentExecutionContext->workerGlobalScope();
    }

    return Optional<WorkerGlobalScope*>();
}

void WorkerObjectProxy::addChildWorker()
{
    Optional<WorkerGlobalScope*> parent = parentWorkerGlobalScope();
    if (parent.hasValue()) {
        parent->asDedicatedWorkerGlobalScope()
            ->workerObjectProxy()
            ->workerThread()
            ->addChildThread(workerThread());
    }
}

void WorkerObjectProxy::removeChildWorker()
{
    Optional<WorkerGlobalScope*> parent = parentWorkerGlobalScope();
    if (parent.hasValue()) {
        if (!parent->asDedicatedWorkerGlobalScope()->isClosing()) {
            parent->asDedicatedWorkerGlobalScope()
                ->workerObjectProxy()
                ->workerThread()
                ->removeChildThread(workerThread());
        }
    }
}

void WorkerObjectProxy::terminate()
{
    removeChildWorker();

    WorkerProxy::terminate();
}

} // namespace Starfish

#endif
