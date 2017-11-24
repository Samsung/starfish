/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "Thread.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "Mutex.h"
#include "Locker.h"
#include <unistd.h>
#include <sys/syscall.h>

namespace StarFish {

pid_t main_tid;
void registerMainThread()
{
#ifdef SYS_gettid
    main_tid = syscall(SYS_gettid);
#else
#error "SYS_gettid unavailable on this system"
#endif
}

bool isMainThread()
{
#ifdef SYS_gettid
    return syscall(SYS_gettid) == main_tid;
#else
    return gettid() == main_tid;
#endif
    return true;
}

Thread::Thread(StarFish* starFish)
    : StarFishHoldable(starFish)
    , m_alive(false)
    , m_mutex(new Mutex())
    , m_currentUnjoined(nullptr)
{
}

void Thread::finishUnjoined()
{
    STARFISH_ASSERT(isMainThread());
    if (!m_currentUnjoined) {
        return;
    }
    Locker<Mutex> l(*m_currentUnjoined->m_thread->m_mutex);
    m_alive = false;
    if (m_currentUnjoined->m_joinHandle != SIZE_MAX) {
        m_currentUnjoined->m_messageLoop->removeIdlerWithNoGCRooting(
            m_currentUnjoined->m_joinHandle);
    }
    void* ret;
    pthread_join(m_currentUnjoined->m_tid, &ret);
    m_starFish->removeActiveThread(this);
#ifdef STARFISH_MESSAGELOOP_DEBUG
    m_currentUnjoined->m_messageLoop->decreaseUnjoinedThreadCount();
#endif
    GC_FREE(m_currentUnjoined);
    m_currentUnjoined = nullptr;
}

void Thread::run(MessageLoop* msgLoop, ThreadWorker fn, void* data)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_RELEASE_ASSERT(!m_alive);

    finishUnjoined();
    Locker<Mutex> l(*m_mutex);
    m_starFish->addActiveThread(this);
    m_currentUnjoined = new (NoGC) ThreadData(this, msgLoop, fn, data);

#ifdef STARFISH_MESSAGELOOP_DEBUG
    msgLoop->increaseRunningThreadCount();
    msgLoop->increaseUnjoinedThreadCount();
    STARFISH_LOG_INFO(
        "[%p] Run thread: runningThread(%d), unjoinedThread(%d), "
        "runningWorkers(%d)\n",
        this, msgLoop->runningThreadCount(), msgLoop->unjoinedThreadCount(),
        msgLoop->runningPoolWorkerCount());
#endif

    int retValue = pthread_create(
        &m_currentUnjoined->m_tid, NULL,
        [](void* data) -> void* {
            ThreadData* d = (ThreadData*)data;
            Locker<Mutex> l(*d->m_thread->m_mutex);
            auto ret = d->m_fn(d->m_data);
#ifdef STARFISH_MESSAGELOOP_DEBUG
            d->m_messageLoop->decreaseRunningThreadCount();
#endif
            if (d->m_thread->m_alive) {
                d->m_thread->m_alive = false;
                d->m_joinHandle =
                    d->m_messageLoop->addIdlerWithNoGCRootingInOtherThread(
                        nullptr,
                        [](size_t handle, void* data) {
                            ThreadData* d = (ThreadData*)data;
                            d->m_thread->finishUnjoined();
                        },
                        d);
            } // else: joinIfNeeds() called while thread running
            pthread_exit(ret);
        },
        m_currentUnjoined);
    if (retValue == 0) {
        m_alive = true;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}
void Thread::joinIfNeeds()
{
    STARFISH_ASSERT(isMainThread());
    finishUnjoined();
}
}
