/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishThread__
#define __StarfishThread__

namespace Starfish {

class MessageLoop;
class Mutex;
class Thread;

typedef void* (*ThreadWorker)(void*);
typedef void* (*StoppableThreadWorker)(void*, std::future<void>&&);

void registerMainThread();
bool isMainThread();
size_t mainThreadID();
size_t numberOfCores();

struct ThreadData {
    ThreadData(Thread* t, MessageLoop* m, ThreadWorker w,
               StoppableThreadWorker sw, void* d)
        : m_thread(t)
        , m_messageLoop(m)
        , m_fn(w)
        , m_stoppableFn(sw)
        , m_data(d)
#if !defined(OS_WINDOWS)
        , m_tid(0)
#endif
        , m_joinHandle(SIZE_MAX)
    {
    }
    Thread* m_thread;
    MessageLoop* m_messageLoop;
    ThreadWorker m_fn;
    StoppableThreadWorker m_stoppableFn;
    void* m_data;
    pthread_t m_tid;
    size_t m_joinHandle;
    std::promise<void> m_stopSignal;
};

class ThreadClient;

class Thread : public gc {
public:
    Thread(ThreadClient* client, const char* name = "");
    ~Thread()
    {
    }

    void run(MessageLoop* msgLoop, ThreadWorker fn, void* data);
    void run(MessageLoop* msgLoop, StoppableThreadWorker fn, void* data);

    void joinIfNeeds();
    bool isAlive()
    {
        return m_alive;
    }
    void finishUnjoined();

private:
    void run(MessageLoop* msgLoop, ThreadWorker fn,
             StoppableThreadWorker stoppableFn, void* data);
    static void cleanupHandler(void* data);

    ThreadClient* m_threadClient;

protected:
    volatile bool m_alive;
    Mutex* m_mutex;
    ThreadData* m_threadData;
};
} // namespace Starfish

#endif
