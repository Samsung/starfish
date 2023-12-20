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

namespace Starfish {

class ExecutionContext;
class GlobalScope;
class MessageLoop;
class SerializeWithTransferResult;
class WorkerThread;

class WorkerProxy : public gc {
public:
    using PostTask = void (*)(void*);

    WorkerProxy(ExecutionContext* executionContext, WorkerThread* workerThread);

    void terminate();

    void postTask(PostTask task, void* data);

    DEFINE_GETTER(bool, wasTerminate);
    DEFINE_GETTER(WorkerThread*, workerThread);

protected:
    ExecutionContext* m_ownerExecutionContext;
    WorkerThread* m_workerThread;
    bool m_wasTerminate;

    static GlobalScope* workerProxyGlobalScope();

    virtual MessageLoop* targetMessageLoop() = 0;
    virtual ExecutionContext* targetExecutionContext() = 0;

    void clearPendingPostTask();
};

} // namespace Starfish

#endif
