/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishThreadPool__
#define __StarfishThreadPool__

#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Semaphore.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadClient.h"

#include <condition_variable>
#include <mutex>

namespace Starfish {

class ExecutionContext;
class MessageLoop;

class ThreadPool : public ThreadClient, public gc {
public:
    ThreadPool(size_t maxThreadCount, MessageLoop* ml);
    ~ThreadPool()
    {
    }

    // if dataPointerComesFromNoGC is true, ThreadPool will GC_FREE(data) on
    // destroy if needs
    void addWork(ExecutionContext* ctx, ThreadWorker fn, void* data,
                 bool dataPointerComesFromNoGC = false);
    void clearWork(ExecutionContext* ctx); // give nullptr to clear every idlers
    // waitForActiveWork: block until already-dequeued jobs finish. Only pools
    // whose jobs never call back into the destroying thread may pass true.
    void destroy(bool waitForActiveWork = false);

    void onThreadStarted(Thread* thread) override;
    void onThreadFinished(Thread* thread) override;

    MessageLoop* messageLoop();

private:
    bool m_isClosed;
    MessageLoop* m_messageLoop;

    // In Starfish strategy, pooled threads are mainly used for a short-term
    // task. Long-lived tasks such as media streaming are individually created
    // using Thread class. Through ThreadClient interface, This class observes
    // each state of the threads individually created.
    // TODO: Consider renaming ThreadPool to ThreadManager
    GCVector<Thread*> m_activePooledThreads;
    GCVector<Thread*> m_activeUnPooledThreads;

    struct WorkerData {
        bool dataPointerComesFromNoGC;
        void* data;
        ExecutionContext* ctx;
    };

    void clearWorkLocked(
        ExecutionContext* ctx); // caller holds m_workerQueueMutex

    std::list<std::pair<ThreadWorker, WorkerData*>> m_workerQueue;
    std::mutex m_workerQueueMutex;
    std::condition_variable m_workerQueueCondition;
    size_t m_idleWaiterCount{ 0 }; // guarded by m_workerQueueMutex
    // Dequeued, currently-running jobs; guarded by m_workerQueueMutex.
    // destroy() waits for this to reach zero.
    size_t m_activeWorkCount{ 0 };
};
} // namespace Starfish

#endif
