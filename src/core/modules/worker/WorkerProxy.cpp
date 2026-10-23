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

    // Each thread collects its own heap (GC_THREAD_ISOLATE), which goes away
    // with the thread. The task therefore carries the receiver's proxy, which
    // lives on the receiver's heap, and reaches the sender's objects through
    // it only while the receiver's proxy is open: a side closes its proxy
    // before the other side's thread can end, and a worker thread ends only
    // after its page terminated it.
    targetMessageLoop()->addIdlerWithNoGCRootingInOtherThread(
        m_workerThread->workerMessageLoopGlobalScope(),
        [](size_t handle, void* data, void* data1) {
            auto* receiver = static_cast<WorkerProxy*>(data);

            // Only the receiver's state decides delivery: a worker that
            // called close() still gets the messages it posted before
            // delivered to its page (HTML: close() discards the worker's
            // own tasks, not the page's).
            if (receiver->isClosed()) {
                return;
            }
            WorkerProxy* sender = receiver->entangledWorkerProxy();

            auto* serializedMessage =
                static_cast<SerializeWithTransferResult*>(data1);

            STARFISH_ASSERT(
                sender->targetExecutionContext()->isContextThread());
            MessageEvent* event = new MessageEvent(
                sender->targetExecutionContext(), serializedMessage);
            sender->entangledEventTarget()->dispatchEventByUA(event);

            // The handler may have ended either side: a page that calls
            // worker.terminate() from its message handler closes the proxy
            // on this side and tears down the sender's loop concurrently.
            // The message then belongs to the sender's own teardown.
            if (receiver->isClosed() || sender->isClosed()) {
                return;
            }

            // Free serializedMessage in thread where this variable was created.
            sender->ownerExecutionContext()
                ->webBase()
                ->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    sender->ownerExecutionContext()->globalScope(),
                    [](size_t handle, void* data, void* data1) {
                        WorkerProxy::onPostMessageDone(
                            static_cast<WorkerProxy*>(data),
                            static_cast<SerializeWithTransferResult*>(data1));
                    },
                    sender, serializedMessage);
        },
        m_entangledWorkerProxy, serializedMessage);
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
        delete serializedMessage;
        m_serializedMessages.erase(iter);
    }
}

void WorkerProxy::dispose()
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    // Tasks the other side posted after close() cleared the loop are still
    // queued; its thread is gone by now, so they must not run.
    clearPendingPostTask();

    // A message the receiver never acknowledged is freed here: one it has
    // not dispatched yet, or one it dispatched after this proxy closed.
    for (SerializeWithTransferResult* serializedMessage :
         m_serializedMessages) {
        delete serializedMessage;
    }

    m_serializedMessages.clear();
    m_serializedMessages.shrink_to_fit();
}

void WorkerProxy::clearPendingPostTask()
{
    m_ownerExecutionContext->webBase()->messageLoop()->clearPendingIdlers(
        m_workerThread->workerMessageLoopGlobalScope());
}

void WorkerProxy::close()
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    m_isClosed = true;
    clearPendingPostTask();
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
