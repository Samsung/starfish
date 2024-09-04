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
#include "binding/ScriptWrappable.h"
#include "core/page/WebBase.h"
#include "core/serialize/Serializer.h"
#include "core/dom/EventTarget.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessageEvent.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/WorkerThread.h"
#include "core/modules/worker/WorkerProxy.h"

namespace Starfish {

WorkerProxy::WorkerProxy(ExecutionContext* executionContext,
                         WorkerThread* workerThread)
    : m_ownerExecutionContext(executionContext)
    , m_workerThread(workerThread)
    , m_entangledEventTarget(nullptr)
    , m_entangledWorkerProxy(nullptr)
    , m_wasTerminated(false)
    , m_isClosed(false)
{
}

void WorkerProxy::onPostMessageDone(
    WorkerProxy* WorkerProxy, SerializeWithTransferResult* serializedMessage)
{
    WorkerProxy->removeSerializedMessage(serializedMessage);
}

void WorkerProxy::entangleTarget(EventTarget* eventTarget, WorkerProxy* proxy)
{
    m_entangledEventTarget = eventTarget;
    m_entangledWorkerProxy = proxy;
}

void WorkerProxy::postTask(PostTask task, void* data)
{
    if (m_isClosed) {
        return;
    }

    MessageLoop* messageLoop = targetMessageLoop();

    STARFISH_ASSERT(!messageLoop->calledOnValidThread());
    messageLoop->addIdlerWithNoGCRootingInOtherThread(
        m_workerThread->workerMessageLoopGlobalScope(),
        [](size_t handle, void* data, void* data1) {
            reinterpret_cast<PostTask>(data)(data1);
        },
        reinterpret_cast<void*>(task), data);
}

void WorkerProxy::postMessage(ScriptValue message,
                              const GCVector<ScriptObject>& transfer)
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    SerializeWithTransferResult* serializedMessage = createSerializedMessage();

    Serializer::serializeWithTransfer(m_ownerExecutionContext, message,
                                      transfer, *serializedMessage);

    postSerializedMessage(serializedMessage);
}

void WorkerProxy::postMessageToEntangledEventTarget(
    SerializeWithTransferResult* serializedMessage)
{
    STARFISH_ASSERT(m_entangledEventTarget);

    if (m_isClosed) {
        return;
    }

    targetMessageLoop()->addIdlerWithNoGCRootingInOtherThread(
        m_workerThread->workerMessageLoopGlobalScope(),
        [](size_t handle, void* data, void* data1) {
            auto* proxy = static_cast<WorkerProxy*>(data);

            if (proxy->entangledWorkerProxy()->isClosed()) {
                return;
            }

            auto* serializedMessage =
                static_cast<SerializeWithTransferResult*>(data1);

            STARFISH_ASSERT(proxy->targetExecutionContext()->isContextThread());
            MessageEvent* event = new MessageEvent(
                proxy->targetExecutionContext(), serializedMessage);
            proxy->entangledEventTarget()->dispatchEventByUA(event);

            if (proxy->isClosed()) {
                return;
            }

            // Free serializedMessage in thread where this variable was created.
            proxy->ownerExecutionContext()
                ->webBase()
                ->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    proxy->ownerExecutionContext()->globalScope(),
                    [](size_t handle, void* data, void* data1) {
                        WorkerProxy::onPostMessageDone(
                            static_cast<WorkerProxy*>(data),
                            static_cast<SerializeWithTransferResult*>(data1));
                    },
                    proxy, serializedMessage);
        },
        this, serializedMessage);
}

SerializeWithTransferResult* WorkerProxy::createSerializedMessage()
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    SerializeWithTransferResult* serializedMessage =
        new (NoGC) SerializeWithTransferResult();
    m_serializedMessages.push_back(serializedMessage);

    return serializedMessage;
}

void WorkerProxy::removeSerializedMessage(
    SerializeWithTransferResult* serializedMessage)
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    auto iter = std::find(m_serializedMessages.begin(),
                          m_serializedMessages.end(), serializedMessage);
    if (iter != m_serializedMessages.end()) {
        GC_FREE(serializedMessage);
        m_serializedMessages.erase(iter);
    }
}

void WorkerProxy::clearSerializedMessages()
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    // If the worker terminates unexpectedly, serializedMessage is not
    // freed and remains in the vector.
    for (SerializeWithTransferResult* serializedMessage :
         m_serializedMessages) {
        GC_FREE(serializedMessage);
    }

    m_serializedMessages.clear();
    m_serializedMessages.shrink_to_fit();
}

void WorkerProxy::terminate()
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    if (m_wasTerminated) {
        return;
    }

    m_wasTerminated = true;

    clearPendingPostTask();

    clearSerializedMessages();
}

void WorkerProxy::clearPendingPostTask()
{
    m_ownerExecutionContext->webBase()->messageLoop()->clearPendingIdlers(
        m_workerThread->workerMessageLoopGlobalScope());
}

void WorkerProxy::close()
{
    m_isClosed = true;
}

EventTarget* WorkerProxy::entangledEventTarget() const
{
    STARFISH_ASSERT(m_entangledEventTarget);
    return m_entangledEventTarget;
}

WorkerProxy* WorkerProxy::entangledWorkerProxy() const
{
    STARFISH_ASSERT(m_entangledWorkerProxy);
    return m_entangledWorkerProxy;
}

} // namespace Starfish
#endif
