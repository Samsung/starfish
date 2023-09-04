/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishMessageLoop__
#define __StarfishMessageLoop__

#include "core/modules/message_loop/MessageLoopInterface.h"

#if defined(SERVICE_WORKER_USE_SEPARATE_PROCESS) && \
    defined(STARFISH_WEBWORKER_HOST)
#define BASE_CLASS gc, public IMessageLoop
#else
#include "core/modules/message_loop/MessageLoopMixin.h"
#define BASE_CLASS gc, public MessageLoopMixin, public IMessageLoop
#endif

namespace Starfish {
class GlobalScope;
class Mutex;

constexpr size_t MessageLoopInvalidID{ SIZE_MAX };

class MessageLoop : public BASE_CLASS {
    friend class MessageLoopImpl;
    friend class MessageLoopMixin;
    friend class Timer;

public:
    static MessageLoop* create();

#if !defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    static void init();
    static void run();
    static void stop();
    static size_t runOnMainThreadSync(const std::function<size_t()>& functor);
#endif

    virtual size_t addIdler(GlobalScope* globalScope,
                            void (*fn)(size_t handle, void*), void* data) = 0;
    virtual size_t addIdler(GlobalScope* globalScope,
                            void (*fn)(size_t handle, void*, void*), void* data,
                            void* data1) = 0;
    virtual size_t addIdler(GlobalScope* globalScope,
                            void (*fn)(size_t handle, void*, void*, void*),
                            void* data, void* data1, void* data2) = 0;

    virtual void removeIdler(size_t handle) = 0;
    virtual void removeIdlerWithNoGCRooting(size_t handle) = 0;

    virtual void clearPendingIdlers(
        GlobalScope* globalScope) = 0; // give nullptr to clear every idlers

    virtual void destroy() = 0;

    virtual void runOnMainThreadAsync(const std::function<void()>& functor) = 0;

    // microtask is similar with idler, but it is executed before
    // idler(microtask has higher priority)
    size_t addMicroTask(GlobalScope* globalScope,
                        void (*fn)(size_t handle, void*), void* data);
    void removeMicroTask(size_t handle);

protected:
    MessageLoop();

    bool m_inClosingState;
    std::unordered_set<size_t> m_idlers;
    Mutex* m_idlersFromOtherThreadMutex;
    std::unordered_set<size_t> m_idlersFromOtherThread;

    struct MicroTask {
        size_t m_id;
        GlobalScope* m_globalScope;
        void (*m_callback)(size_t handle, void*);
        void* m_data;
    };

    void invokeMicroTasksIfExist();
    void clearMicroTasks(GlobalScope* globalScope);

    size_t m_microTaskCounter;
    size_t m_microTaskIdler;
    GCVector<MicroTask> m_microTasks;

#ifdef STARFISH_MESSAGELOOP_DEBUG
public:
    Mutex* m_countingMutex;
    volatile int m_runningThreadCount;
    volatile int m_unjoinedThreadCount;
    volatile int m_runningPoolWorkerCount;
    int runningThreadCount();
    void increaseRunningThreadCount();
    void decreaseRunningThreadCount();
    int unjoinedThreadCount();
    void increaseUnjoinedThreadCount();
    void decreaseUnjoinedThreadCount();
    int runningPoolWorkerCount();
    void increaseRunningPoolWorkerCount();
    void decreaseRunningPoolWorkerCount();
#endif
};
} // namespace Starfish

#undef BASE_CLASS

#endif
