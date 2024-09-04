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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorkerProxy__)
#define __StarfishWorkerProxy__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class ExecutionContext;
class EventTarget;
class GlobalScope;
class MessageLoop;
class SerializeWithTransferResult;
class WorkerThread;

class WorkerProxy : public gc {
public:
    using PostTask = void (*)(void*);

    static void onPostMessageDone(
        WorkerProxy* WorkerProxy,
        SerializeWithTransferResult* serializedMessage);

    virtual void terminate();

    void entangleTarget(EventTarget* eventTarget, WorkerProxy* proxy);

    void postTask(PostTask task, void* data);

    void postMessage(ScriptValue message,
                     const GCVector<ScriptObject>& transfer);

    void close();

    DEFINE_GETTER(bool, wasTerminated);
    DEFINE_GETTER(ExecutionContext*, ownerExecutionContext);
    DEFINE_GETTER(WorkerThread*, workerThread);
    DEFINE_GETTER(bool, isClosed);

    EventTarget* entangledEventTarget() const;
    WorkerProxy* entangledWorkerProxy() const;

protected:
    WorkerProxy(ExecutionContext* executionContext, WorkerThread* workerThread);

    virtual MessageLoop* targetMessageLoop() = 0;
    virtual ExecutionContext* targetExecutionContext() = 0;

    virtual void postSerializedMessage(
        SerializeWithTransferResult* serializedMessage) = 0;

    void postMessageToEntangledEventTarget(
        SerializeWithTransferResult* serializedMessage);

    SerializeWithTransferResult* createSerializedMessage();
    void removeSerializedMessage(
        SerializeWithTransferResult* serializedMessage);
    void clearSerializedMessages();

    void clearPendingPostTask();

    ExecutionContext* m_ownerExecutionContext;
    WorkerThread* m_workerThread;
    EventTarget* m_entangledEventTarget;
    WorkerProxy* m_entangledWorkerProxy;
    bool m_wasTerminated;
    std::atomic_bool m_isClosed;
    GCAtomicVector<SerializeWithTransferResult*> m_serializedMessages;
};

} // namespace Starfish

#endif
