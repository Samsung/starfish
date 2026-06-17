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

#include "StarfishConfig.h"
#include "ThreadPool.h"
#include "core/modules/message_loop/MessageLoop.h"

#include <chrono>

namespace {
// MSE appendBuffer demux work arrives every ~100-500ms during YouTube
// playback; 2s covers >= 4x the worst-case cadence plus ABR/network jitter,
// while still releasing threads ~2s after playback stops or page goes idle.
const std::chrono::milliseconds kWorkerLingerDuration(2000);
} // namespace

namespace Starfish {

ThreadPool::ThreadPool(size_t maxThreadCount, MessageLoop* ml)
    : m_isClosed(false)
    , m_messageLoop(ml)
{
    for (size_t i = 0; i < maxThreadCount; i++) {
        m_activePooledThreads.push_back(new Thread(this));
    }
}

void ThreadPool::destroy()
{
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());
    {
        std::lock_guard<std::mutex> lock(m_workerQueueMutex);
        m_isClosed = true;
        clearWorkLocked(nullptr);
    }
    m_workerQueueCondition.notify_all();

    // Finish unpooled thread
    GCVector<Thread*> copies = m_activeUnPooledThreads;
    for (auto const& thread : copies) {
        thread->finishUnjoined();
    }
}

MessageLoop* ThreadPool::messageLoop()
{
    return m_messageLoop;
}

void ThreadPool::onThreadStarted(Thread* thread)
{
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());
    m_activeUnPooledThreads.push_back(thread);
}

void ThreadPool::onThreadFinished(Thread* thread)
{
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());
    auto it = std::find(m_activeUnPooledThreads.begin(),
                        m_activeUnPooledThreads.end(), thread);
    if (it != m_activeUnPooledThreads.end()) {
        m_activeUnPooledThreads.erase(it);
    }
}

void ThreadPool::addWork(ExecutionContext* ctx, ThreadWorker fn, void* data,
                         bool dataPointerComesFromNoGC)
{
    if (m_isClosed) {
        if (dataPointerComesFromNoGC) {
            GC_FREE(data);
        }
        return;
    }
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());
    {
        std::lock_guard<std::mutex> lock(m_workerQueueMutex);
        WorkerData* r = new (NoGC) WorkerData;
        r->dataPointerComesFromNoGC = dataPointerComesFromNoGC;
        r->data = data;
        r->ctx = ctx;
        m_workerQueue.push_back(std::make_pair(fn, r));
        if (m_idleWaiterCount > 0) {
            // A lingering worker is parked on the condition variable; the
            // enqueue above happened under the same mutex its predicate
            // runs under, so the wakeup cannot be lost.
            m_workerQueueCondition.notify_one();
            return;
        }
    }
    // No idle waiter: spawn a worker on a free Thread slot, as before.
    for (size_t i = 0; i < m_activePooledThreads.size(); i++) {
        if (!m_activePooledThreads[i]->isAlive()) {
            struct Rooter {
                ThreadPool* pool;
            };
            Rooter* rooter = new (NoGC) Rooter;
            rooter->pool = this;

            ThreadWorker worker = [](void* data) -> void* {
                Rooter* rooter = (Rooter*)data;
                ThreadPool* pool = rooter->pool;
                // STARFISH_LOG_INFO("threadPool worker start");
                {
                    std::unique_lock<std::mutex> lock(pool->m_workerQueueMutex);
                    while (true) {
                        if (!pool->m_workerQueue.empty()) {
                            std::pair<ThreadWorker, WorkerData*> first =
                                pool->m_workerQueue.front();
                            pool->m_workerQueue.pop_front();
                            lock.unlock();

                            WorkerData* r = first.second;
                            first.first(r->data);
                            pool->m_messageLoop
                                ->addIdlerWithNoGCRootingInOtherThread(
                                    nullptr,
                                    [](size_t handle, void* data) {
                                        GC_FREE(data);
                                    },
                                    r);

                            lock.lock();
                            continue;
                        }
                        if (pool->m_isClosed) {
                            break;
                        }
                        // Queue empty and pool open: linger instead of exiting,
                        // so the next demux/decode job reuses this OS thread
                        // rather than paying pthread creation again.
                        pool->m_idleWaiterCount++;
                        bool hasWorkOrClosed =
                            pool->m_workerQueueCondition.wait_for(
                                lock, kWorkerLingerDuration, [pool] {
                                    return !pool->m_workerQueue.empty() ||
                                           pool->m_isClosed;
                                });
                        pool->m_idleWaiterCount--;
                        if (!hasWorkOrClosed) {
                            break;
                        }
                    }
                }
#ifdef STARFISH_MESSAGELOOP_DEBUG
                pool->m_messageLoop->decreaseRunningPoolWorkerCount();
#endif
                // STARFISH_LOG_INFO("threadPool worker end");
                pool->m_messageLoop->addIdlerWithNoGCRootingInOtherThread(
                    nullptr, [](size_t handle, void* data) { GC_FREE(data); },
                    rooter);
                return NULL;
            };
#ifdef STARFISH_MESSAGELOOP_DEBUG
            m_messageLoop->increaseRunningPoolWorkerCount();
#endif
            m_activePooledThreads[i]->run(m_messageLoop, worker, rooter);
            break;
        }
    }
}

void ThreadPool::clearWork(ExecutionContext* ctx)
{
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());
    std::lock_guard<std::mutex> lock(m_workerQueueMutex);
    clearWorkLocked(ctx);
}

void ThreadPool::clearWorkLocked(ExecutionContext* ctx)
{
    auto iter = m_workerQueue.begin();
    while (iter != m_workerQueue.end()) {
        if ((iter->second)->ctx == ctx || ctx == nullptr) {
            if (iter->second->dataPointerComesFromNoGC) {
                GC_FREE(iter->second->data);
                iter->second->data = nullptr;
            }
            iter = m_workerQueue.erase(iter);
        } else {
            iter++;
        }
    }
}

} // namespace Starfish
