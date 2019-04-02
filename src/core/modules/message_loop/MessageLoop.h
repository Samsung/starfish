/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishMessageLoop__
#define __StarfishMessageLoop__

#ifdef STARFISH_WEBWORKER_HOST
#define BASE_CLASS gc
#else
#include "core/modules/message_loop/MessageLoopMixin.h"
#define BASE_CLASS gc, public MessageLoopMixin
#endif

namespace Starfish {

class GlobalScope;
class Mutex;

constexpr size_t MessageLoopInvalidID{ SIZE_MAX };

class MessageLoop : public BASE_CLASS {
    friend class MessageLoopImpl;
    friend class MessageLoopMixin;

public:
    MessageLoop();
    size_t addIdler(GlobalScope* globalScope, void (*fn)(size_t handle, void*),
                    void* data);
    size_t addIdler(GlobalScope* globalScope,
                    void (*fn)(size_t handle, void*, void*), void* data,
                    void* data1);
    size_t addIdler(GlobalScope* globalScope,
                    void (*fn)(size_t handle, void*, void*, void*), void* data,
                    void* data1, void* data2);
    size_t addIdlerWithNoGCRootingInOtherThread(
        GlobalScope* globalScope, void (*fn)(size_t handle, void*), void* data);
    size_t addIdlerWithNoGCRootingInOtherThread(GlobalScope* globalScope,
                                                void (*fn)(size_t handle, void*,
                                                           void*),
                                                void* data, void* data1);

    void removeIdler(size_t handle);
    void removeIdlerWithNoGCRooting(size_t handle);

    void clearPendingIdlers(
        GlobalScope* globalScope); // give nullptr to clear every idlers

    void destroy();

    static void init();
    static void run();
    static void stop();
    static size_t runOnMainThreadSync(const std::function<size_t()>& functor);
    void runOnMainThreadAsync(const std::function<void()>& functor);

protected:
    bool m_inClosingState;
    std::unordered_set<size_t> m_idlers;
    Mutex* m_idlersFromOtherThreadMutex;
    std::unordered_set<size_t> m_idlersFromOtherThread;

#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    std::list<size_t> m_idlersFromOtherThreadForUV;
    void* m_idlerThreadAsyncHandle;
#endif
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
