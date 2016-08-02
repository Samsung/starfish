/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "ThreadPool.h"
#include "platform/message_loop/MessageLoop.h"

namespace StarFish {

ThreadPool::ThreadPool(size_t maxThreadCount, MessageLoop* ml)
    : m_messageLoop(ml)
{
    m_workerQueueMutex = new Mutex();
    for (size_t i = 0; i < maxThreadCount; i ++) {
        m_threads.push_back(new Thread());
    }
}

void ThreadPool::addWork(ThreadWorker fn, void* data)
{
    STARFISH_ASSERT(isMainThread());
    m_workerQueueMutex->lock();
    struct DataRooter {
        void* data;
    };
    DataRooter* r = new(NoGC) DataRooter;
    r->data = data;
    m_workerQueue.push_back(std::make_pair(fn, r));
    m_workerQueueMutex->unlock();

    for (size_t i = 0; i < m_threads.size(); i ++) {
        if (!m_threads[i]->isAlive()) {
            struct Rooter {
                ThreadPool* pool;
            };
            Rooter* rooter = new(NoGC) Rooter;
            rooter->pool = this;
            ThreadWorker worker = [](void* data) -> void* {
                Rooter* rooter = (Rooter*)data;
                STARFISH_LOG_INFO("threadPool worker start\n");
                while (true) {
                    rooter->pool->m_workerQueueMutex->lock();
                    if (!rooter->pool->m_workerQueue.size()) {
                        rooter->pool->m_workerQueueMutex->unlock();
                        break;
                    }
                    std::pair<ThreadWorker, void*> first = rooter->pool->m_workerQueue.front();
                    rooter->pool->m_workerQueue.erase(rooter->pool->m_workerQueue.begin());
                    rooter->pool->m_workerQueueMutex->unlock();

                    DataRooter* r = (DataRooter*)first.second;
                    first.first(r->data);
                    rooter->pool->m_messageLoop->addIdlerWithNoGCRootingInOtherThread([](size_t handle, void* data) {
                        GC_FREE(data);
                    }, r);
                }
                STARFISH_LOG_INFO("threadPool worker end\n");
                rooter->pool->m_messageLoop->addIdlerWithNoGCRootingInOtherThread([](size_t handle, void* data) {
                    GC_FREE(data);
                }, rooter);
                return NULL;
            };

            m_threads[i]->run(m_messageLoop, worker, rooter);
            break;
        }
    }
}


}
