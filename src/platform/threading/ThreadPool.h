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
