/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishMessageLoop__
#define __StarFishMessageLoop__

#include "core/modules/threading/Mutex.h"
#ifdef STARFISH_MESSAGELOOP_DEBUG
#include "core/modules/threading/Locker.h"
#endif
#include "binding/StarFishHoldable.h"

namespace StarFish {

class BrowsingContext;

class MessageLoop : public gc, public StarFishHoldable {
    friend class StarFish;
    friend class Window;

public:
    MessageLoop(StarFish* sf);
    size_t addIdler(BrowsingContext* ctx, void (*fn)(size_t handle, void*),
                    void* data, bool clearable = false);
    size_t addIdler(BrowsingContext* ctx,
                    void (*fn)(size_t handle, void*, void*), void* data,
                    void* data1, bool clearable = false);
    size_t addIdler(BrowsingContext* ctx,
                    void (*fn)(size_t handle, void*, void*, void*), void* data,
                    void* data1, void* data2, bool clearable = false);
    size_t addIdlerWithNoGCRootingInOtherThread(
        BrowsingContext* ctx, void (*fn)(size_t handle, void*), void* data,
        bool clearable = false);
    size_t addIdlerWithNoGCRootingInOtherThread(
        BrowsingContext* ctx, void (*fn)(size_t handle, void*, void*),
        void* data, void* data1, bool clearable = false);

    void removeIdler(size_t handle);
    void removeIdlerWithNoGCRooting(size_t handle);

    void clearOrInvokePendingIdlers(
        BrowsingContext* ctx); // give nullptr to clear every idlers
    void run();
    void invokeNavigate(WebView* wv, ResourceURL* url,
                        ResourceURL* referrerURL);

protected:
    std::unordered_set<size_t> m_idlers;
    Mutex* m_idlersFromOtherThreadMutex;
    std::unordered_set<size_t> m_idlersFromOtherThread;
    void* m_navigateInvokeIdler;
#ifdef STARFISH_MESSAGELOOP_DEBUG
public:
    Mutex* m_countingMutex;
    volatile int m_runningThreadCount;
    volatile int m_unjoinedThreadCount;
    volatile int m_runningPoolWorkerCount;
    int runningThreadCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        return (int)m_runningThreadCount;
    }
    void increaseRunningThreadCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        m_runningThreadCount++;
    }
    void decreaseRunningThreadCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        m_runningThreadCount--;
    }
    int unjoinedThreadCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        return (int)m_unjoinedThreadCount;
    }
    void increaseUnjoinedThreadCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        m_unjoinedThreadCount++;
    }
    void decreaseUnjoinedThreadCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        m_unjoinedThreadCount--;
    }
    int runningPoolWorkerCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        return (int)m_runningPoolWorkerCount;
    }
    void increaseRunningPoolWorkerCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        m_runningPoolWorkerCount++;
    }
    void decreaseRunningPoolWorkerCount()
    {
        Locker<Mutex> lock(*m_countingMutex);
        m_runningPoolWorkerCount--;
    }
#endif
};
}

#endif
