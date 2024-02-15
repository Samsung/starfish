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

    WorkerProxy(ExecutionContext* executionContext, WorkerThread* workerThread);

    virtual void terminate();

    void postTask(PostTask task, void* data);

    void postMessage(ScriptValue message,
                     const GCAtomicVector<ScriptObject>& transfer);

    DEFINE_GETTER(bool, wasTerminated);
    DEFINE_GETTER(ExecutionContext*, ownerExecutionContext);
    DEFINE_GETTER(WorkerThread*, workerThread);

protected:
    ExecutionContext* m_ownerExecutionContext;
    WorkerThread* m_workerThread;
    EventTarget* m_entangledEventTarget;
    bool m_wasTerminated;
    std::vector<SerializeWithTransferResult*> m_serializedMessages;

    static GlobalScope* workerProxyGlobalScope();

    virtual MessageLoop* targetMessageLoop() = 0;
    virtual ExecutionContext* targetExecutionContext() = 0;
    virtual bool isTargetClosed() = 0;

    virtual void postSerializedMessage(
        SerializeWithTransferResult* serializedMessage) = 0;

    void postMessageToEntangledEventTarget(
        SerializeWithTransferResult* serializedMessage);

    SerializeWithTransferResult* createSerializedMessage();
    void removeSerializedMessage(
        SerializeWithTransferResult* serializedMessage);
    void clearSerializedMessages();

    void clearPendingPostTask();

    DEFINE_GETTER_SETTER(EventTarget*, entangledEventTarget,
                         EntangledEventTarget);
};

} // namespace Starfish

#endif
