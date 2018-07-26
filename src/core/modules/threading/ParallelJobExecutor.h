/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "binding/StarFishHoldable.h"
#include "Thread.h"

#ifndef __StarFishParallelJobExecutor__
#define __StarFishParallelJobExecutor__

namespace StarFish {
class Thread;

static int numberOfCores()
{
    int ret = 1;
    long sysconfResult = sysconf(_SC_NPROCESSORS_ONLN);
    ret = sysconfResult < 0 ? 1 : static_cast<int>(sysconfResult);
    return ret;
}

template <typename ParameterType>
class ParallelJobExecutor : public gc, public StarFishHoldable {
public:
    using ParallelJobWorker = void* (*)(void*); // Same as ThreadWorker

    ParallelJobExecutor(StarFish* starfish, ParallelJobWorker worker,
                        int requestWorkerSize)
        : StarFishHoldable(starfish)
        , m_jobWorker(worker)
    {
        STARFISH_ASSERT(isMainThread());
        STARFISH_ASSERT(m_jobWorker);

        int max = numberOfCores();
        if (!requestWorkerSize || requestWorkerSize > max) {
            requestWorkerSize = static_cast<unsigned>(max);
        }
#ifdef STARFISH_ENABLE_TEST
        STARFISH_LOG_INFO("ParallelJobExecutor worker size is : %d\n",
                          requestWorkerSize);
#endif
        auto& threadPool = m_starFish->parallelJobExecutorThreadPool();
        for (int i = 0; i < requestWorkerSize; ++i) {
            if (threadPool.size() < static_cast<size_t>(i) + 1U) {
                threadPool.push_back(new Thread(m_starFish));
            }
            m_threadVector.push_back(threadPool[i]);
            m_paramVector.push_back(ParameterType());
        }
    }

    ParameterType& parameters(size_t index)
    {
        return m_paramVector[index];
    }

    int numberOfThread()
    {
        return m_threadVector.size();
    }

    void execute()
    {
        STARFISH_ASSERT(isMainThread());
        // excute
        for (size_t i = 0; i < m_threadVector.size(); ++i) {
            m_threadVector[i]->run(m_starFish->messageLoop(), m_jobWorker,
                                   &m_paramVector[i]);
        }
        // join
        for (size_t i = 0; i < m_threadVector.size(); ++i) {
            m_threadVector[i]->joinIfNeeds();
        }
    }

private:
    GCVector<ParameterType> m_paramVector;
    GCVector<Thread*> m_threadVector;
    ParallelJobWorker m_jobWorker;
};
}
#endif
