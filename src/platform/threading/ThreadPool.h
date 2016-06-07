#ifndef __StarFishThreadPool__
#define __StarFishThreadPool__

#include "platform/threading/Mutex.h"
#include "platform/threading/Semaphore.h"
#include "platform/threading/Thread.h"

namespace StarFish {

class ThreadPool : public gc {
public:
    ThreadPool(size_t maxThreadCount);
    ~ThreadPool() { }
    void addWork(ThreadWorker fn, void* data);
protected:
    std::vector<Thread*, gc_allocator<Thread*>> m_threads;
    std::list<std::pair<ThreadWorker, void*>, gc_allocator<std::pair<ThreadWorker, void*>>> m_workerQueue;
    Mutex* m_workerQueueMutex;

};

}


#endif
