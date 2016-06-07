#include "StarFishConfig.h"
#include "ThreadPool.h"

namespace StarFish {

ThreadPool::ThreadPool(size_t maxThreadCount)
{
    m_workerQueueMutex = new Mutex();
    for (size_t i = 0; i < maxThreadCount; i ++) {
        m_threads.push_back(new Thread());
    }
}

void ThreadPool::addWork(ThreadWorker fn, void* data)
{
    m_workerQueueMutex->lock();
    m_workerQueue.push_back(std::make_pair(fn, data));
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
                // STARFISH_LOG_INFO("threadPool worker start\n");
                while (true) {
                    rooter->pool->m_workerQueueMutex->lock();
                    if (!rooter->pool->m_workerQueue.size()) {
                        rooter->pool->m_workerQueueMutex->unlock();
                        break;
                    }
                    auto first = rooter->pool->m_workerQueue.front();
                    rooter->pool->m_workerQueue.pop_front();
                    rooter->pool->m_workerQueueMutex->unlock();

                    first.first(first.second);
                }
                // STARFISH_LOG_INFO("threadPool worker end\n");
                GC_FREE(rooter);
                return NULL;
            };

            m_threads[i]->run(worker, rooter);
            break;
        }
    }
}


}
