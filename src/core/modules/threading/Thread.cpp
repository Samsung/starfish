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
    , m_isJoined(false)
    , m_tid(0)
    , m_mutex(new Mutex())
{
}

void Thread::run(MessageLoop* msgLoop, ThreadWorker fn, void* data)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(!m_alive);

    Locker<Mutex> l(*m_mutex);
    m_starFish->addActiveThread(this);
    struct ThreadData {
        Thread* thread;
        MessageLoop* messageLoop;
        ThreadWorker fn;
        void* data;
        pthread_t tid;
    };

    ThreadData* d = new (NoGC) ThreadData;
    d->thread = this;
    d->messageLoop = msgLoop;
    d->fn = fn;
    d->data = data;

    int retValue = pthread_create(
        &d->tid, NULL,
        [](void* data) -> void* {
            ThreadData* d = (ThreadData*)data;
            Locker<Mutex> l(*d->thread->m_mutex);
            auto ret = d->fn(d->data);
            d->thread->m_alive = false;
            d->messageLoop->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t handle, void* data) {
                    ThreadData* d = (ThreadData*)data;
                    if (!d->thread->m_isJoined) {
                        d->thread->m_isJoined = true;
                        void* ret;
                        pthread_join(d->tid, &ret);
                        d->thread->m_starFish->removeActiveThread(d->thread);
                    }
                    GC_FREE(data);
                },
                d);
            pthread_exit(ret);
        },
        d);
    if (retValue == 0) {
        m_tid = d->tid;
        m_alive = true;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}
void Thread::joinIfNeeds()
{
    STARFISH_ASSERT(isMainThread());
    m_mutex->lock();
    if (m_tid && !m_isJoined) {
        m_isJoined = true;
        m_mutex->unlock();
        pthread_join(m_tid, nullptr);
        m_starFish->removeActiveThread(this);
    } else {
        m_mutex->unlock();
    }
    m_alive = false;
    m_tid = 0;
    m_isJoined = false;
}
}
