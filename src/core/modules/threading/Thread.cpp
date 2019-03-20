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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "Thread.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/ThreadClient.h"
#include "Mutex.h"
#include "Locker.h"
#if !defined(OS_WINDOWS)
#include <unistd.h>
#include <sys/syscall.h>
#else
#include <Windows.h>
#endif

namespace Starfish {

size_t numberOfCores()
{
    size_t ret = 1;
#ifndef STARFISH_WINDOWS
    long sysconfResult = sysconf(_SC_NPROCESSORS_ONLN);
#else
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    long sysconfResult = sysinfo.dwNumberOfProcessors;
#endif

    if (sysconfResult > 0) {
        ret = static_cast<int>(sysconfResult);
    }
    return ret;
}

#if !defined(OS_WINDOWS)
pid_t mainTid;
void registerMainThread()
{
#ifdef SYS_gettid
    mainTid = syscall(SYS_gettid);
#else
#error "SYS_gettid unavailable on this system"
#endif
}

bool isMainThread()
{
#ifdef SYS_gettid
    return syscall(SYS_gettid) == mainTid;
#else
    return gettid() == mainTid;
#endif
    return true;
}
size_t mainThreadID()
{
    return mainTid;
}
#else
DWORD mainTid;
void registerMainThread()
{
    mainTid = GetCurrentThreadId();
}

bool isMainThread()
{
    return GetCurrentThreadId() == mainTid;
}
size_t mainThreadID()
{
    return mainTid;
}
#endif

Thread::Thread(ThreadClient* client, const char* name)
    : m_threadClient(client)
    , m_alive(false)
    , m_mutex(new Mutex(name))
    , m_threadData(nullptr)
{
}

void Thread::finishUnjoined()
{
    STARFISH_ASSERT(isMainThread());
    if (!m_threadData) {
        return;
    }

    // NOTE: if this thread is still running, we send it a stop signal. A
    // worker, which possibly lives till here, should use StoppableThreadWorker.
    if (m_alive) {
        m_threadData->m_stopSignal.set_value();
    }

    {
        Locker<Mutex> l(*m_threadData->m_thread->m_mutex);
        m_alive = false;
        if (m_threadData->m_joinHandle != SIZE_MAX) {
            m_threadData->m_messageLoop->removeIdlerWithNoGCRooting(
                m_threadData->m_joinHandle);
        }
    }

    void* ret;
    pthread_join(m_threadData->m_tid, &ret);

    if (m_threadClient) {
        m_threadClient->onThreadFinished(this);
    }

    m_threadData->~ThreadData();
    GC_FREE(m_threadData);

#ifdef STARFISH_MESSAGELOOP_DEBUG
    m_threadData->m_messageLoop->decreaseUnjoinedThreadCount();
#endif
    m_threadData = nullptr;
}

void Thread::run(MessageLoop* msgLoop, StoppableThreadWorker fn, void* data)
{
    run(msgLoop, nullptr, fn, data);
}

void Thread::run(MessageLoop* msgLoop, ThreadWorker fn, void* data)
{
    run(msgLoop, fn, nullptr, data);
}

void Thread::run(MessageLoop* msgLoop, ThreadWorker fn,
                 StoppableThreadWorker stoppableFn, void* data)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_RELEASE_ASSERT(!m_alive);
    STARFISH_ASSERT(!(fn && stoppableFn));

    finishUnjoined();
    Locker<Mutex> l(*m_mutex);

    if (m_threadClient) {
        m_threadClient->onThreadStarted(this);
    }

    m_threadData = new (GC_MALLOC_UNCOLLECTABLE(sizeof(ThreadData)))
        ThreadData(this, msgLoop, fn, stoppableFn, data);
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
        &m_threadData->m_tid, NULL,
        [](void* data) -> void* {
            ThreadData* d = (ThreadData*)data;
            pthread_cleanup_push(Thread::cleanupHandler, data);
            {
                Locker<Mutex> l(*d->m_thread->m_mutex);

                if (d->m_fn) {
                    // normal worker
                    d->m_fn(d->m_data);
                } else {
                    // stoppable worker
                    auto future = d->m_stopSignal.get_future();
                    d->m_stoppableFn(d->m_data, std::move(future));
                }

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
            }
            pthread_cleanup_pop(0);
#if !defined(__SANITIZE_ADDRESS__) // GCC 4.8.5 & -fsanitize=address makes wrong
                                   // error with `pthread_exit(((void*)0));`
            pthread_exit(((void*)0));
#else
            return nullptr;
#endif
#if defined(COMPILER_MSVC)
            return nullptr;
#endif
        },
        m_threadData);
    if (retValue == 0) {
        m_alive = true;
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

void Thread::joinIfNeeds()
{
    STARFISH_ASSERT(isMainThread());
    finishUnjoined();
}

void Thread::cleanupHandler(void* data)
{
    STARFISH_LOG_INFO("Thread::cleanupHandler\n");
    ThreadData* td = (ThreadData*)data;
    td->m_thread->joinIfNeeds();
}
} // namespace Starfish
