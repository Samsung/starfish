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

namespace Starfish {

ThreadPool::ThreadPool(size_t maxThreadCount, MessageLoop* ml)
    : m_isClosed(false)
    , m_messageLoop(ml)
{
    m_workerQueueMutex = new Mutex();
    for (size_t i = 0; i < maxThreadCount; i++) {
        m_activePooledThreads.push_back(new Thread(this));
    }
}

void ThreadPool::destroy()
{
    STARFISH_ASSERT(isMainThread());

    m_isClosed = true;

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
    STARFISH_ASSERT(isMainThread());
    m_activeUnPooledThreads.push_back(thread);
}

void ThreadPool::onThreadFinished(Thread* thread)
{
    STARFISH_ASSERT(isMainThread());
    auto it = std::find(m_activeUnPooledThreads.begin(),
                        m_activeUnPooledThreads.end(), thread);
    if (it != m_activeUnPooledThreads.end()) {
        m_activeUnPooledThreads.erase(it);
    }
}

struct DataRooter {
    void* data;
    ExecutionContext* ctx;
};

void ThreadPool::addWork(ExecutionContext* ctx, ThreadWorker fn, void* data)
{
    if (m_isClosed) {
        return;
    }
    STARFISH_ASSERT(isMainThread());
    m_workerQueueMutex->lock();
    DataRooter* r = new (NoGC) DataRooter;
    r->data = data;
    r->ctx = ctx;
    m_workerQueue.push_back(std::make_pair(fn, r));
    m_workerQueueMutex->unlock();

    for (size_t i = 0; i < m_activePooledThreads.size(); i++) {
        if (!m_activePooledThreads[i]->isAlive()) {
            struct Rooter {
                ThreadPool* pool;
            };
            Rooter* rooter = new (NoGC) Rooter;
            rooter->pool = this;

            ThreadWorker worker = [](void* data) -> void* {
                Rooter* rooter = (Rooter*)data;
                // STARFISH_LOG_INFO("threadPool worker start");
                while (true) {
                    rooter->pool->m_workerQueueMutex->lock();
                    if (!rooter->pool->m_workerQueue.size()) {
                        rooter->pool->m_workerQueueMutex->unlock();
                        break;
                    }
                    std::pair<ThreadWorker, void*> first =
                        rooter->pool->m_workerQueue.front();
                    rooter->pool->m_workerQueue.erase(
                        rooter->pool->m_workerQueue.begin());
                    rooter->pool->m_workerQueueMutex->unlock();

                    DataRooter* r = (DataRooter*)first.second;
                    first.first(r->data);
                    rooter->pool->m_messageLoop
                        ->addIdlerWithNoGCRootingInOtherThread(
                            nullptr,
                            [](size_t handle, void* data) { GC_FREE(data); },
                            r);
                }
#ifdef STARFISH_MESSAGELOOP_DEBUG
                rooter->pool->m_messageLoop->decreaseRunningPoolWorkerCount();
#endif
                // STARFISH_LOG_INFO("threadPool worker end");
                rooter->pool->m_messageLoop
                    ->addIdlerWithNoGCRootingInOtherThread(
                        nullptr,
                        [](size_t handle, void* data) { GC_FREE(data); },
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
    m_workerQueueMutex->lock();

    auto iter = m_workerQueue.begin();
    while (iter != m_workerQueue.end()) {
        if (((DataRooter*)iter->second)->ctx == ctx || ctx == nullptr) {
            iter = m_workerQueue.erase(iter);
        } else {
            iter++;
        }
    }

    m_workerQueueMutex->unlock();
}

} // namespace Starfish
