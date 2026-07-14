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
#include "core/modules/threading/Thread.h"

#include <unordered_set>
#include <functional>

namespace Starfish {
class GlobalScope;
class Mutex;
class RunLoop;

constexpr size_t MessageLoopInvalidID{ SIZE_MAX };

class MessageLoop : public gc, public IMessageLoop {
    friend class MessageLoopImpl;
    friend class MessageLoopMixin;
    friend class TimerEFL;
    friend class TimerLibUV;
    friend class TimerWindows;

public:
    static MessageLoop* create();
#if defined(STARFISH_ENABLE_WORKER)
    static MessageLoop* createForWorker(RunLoop* runLoop = nullptr);
#endif

    static void init();
    static void run();
    static void stop();
    static void runOnMainThreadSync(const std::function<void()>& functor);

    // Runs `functor` on the calling thread while excluding the process main
    // thread from running any of its own code, for backends/modes where the
    // LWE main thread is a dedicated thread distinct from the process main
    // thread (e.g. GLib isolated thread mode). Backends/modes that have no
    // such distinction just run `functor` directly.
    static void runWithProcessMainThreadPausedSync(
        const std::function<void()>& functor);

    // Diagnostic only: reports whether the calling thread is, right now,
    // confirmed to be executing from within this backend's own native event
    // loop dispatch (e.g. GLib: getCurrentThreadID() == getpid() and
    // g_main_depth() > 0). Backends/modes with no way to confirm this (or no
    // such concept at all) return false -- false means "unconfirmed", not
    // "definitely not running one", so this must never be used to gate
    // behavior (see runWithProcessMainThreadPausedSync's own bounded runtime
    // handshake for that); it exists purely so callers can log/understand
    // the calling context, e.g. once at LWE::Initialize() time.
    static bool isCallerInsideBackendEventLoop();

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

    virtual RunLoop* runLoop();

    bool calledOnValidThread();

protected:
    MessageLoop();

    bool m_inClosingState;
    std::unordered_set<size_t> m_idlers;
    Mutex* m_idlersFromOtherThreadMutex;
    std::unordered_set<size_t> m_idlersFromOtherThread;

    ThreadID m_currentThreadID;

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
