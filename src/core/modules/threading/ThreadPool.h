/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishThreadPool__
#define __StarFishThreadPool__

#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Semaphore.h"
#include "core/modules/threading/Thread.h"

namespace StarFish {

class BrowsingContext;

class ThreadPool : public gc {
public:
    ThreadPool(size_t maxThreadCount, MessageLoop* ml);
    ~ThreadPool()
    {
    }
    void addWork(BrowsingContext* ctx, ThreadWorker fn, void* data);
    void clearWork(BrowsingContext* ctx); // give nullptr to clear every idlers
protected:
    MessageLoop* m_messageLoop;
    GCVector<Thread*> m_threads;
    std::list<std::pair<ThreadWorker, void*>> m_workerQueue;
    Mutex* m_workerQueueMutex;
};
}

#endif
