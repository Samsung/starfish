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

#include "core/modules/threading/Mutex.h"
#ifdef STARFISH_MESSAGELOOP_DEBUG
#include "core/modules/threading/Locker.h"
#endif

#include "core/modules/message_loop/MessageLoopInterface.h"

namespace Starfish {

class WebView;
class ScriptContext;
enum class HistoryManagerAction;

class MessageLoop : public MessageLoopInterface, public gc {
    friend class MessageLoopImpl;

public:
    MessageLoop();
    size_t addIdler(ScriptContext* ctx, void (*fn)(size_t handle, void*),
                    void* data);
    size_t addIdler(ScriptContext* ctx, void (*fn)(size_t handle, void*, void*),
                    void* data, void* data1);
    size_t addIdler(ScriptContext* ctx,
                    void (*fn)(size_t handle, void*, void*, void*), void* data,
                    void* data1, void* data2);
    size_t addIdlerWithNoGCRootingInOtherThread(
        ScriptContext* ctx, void (*fn)(size_t handle, void*), void* data);
    size_t addIdlerWithNoGCRootingInOtherThread(ScriptContext* ctx,
                                                void (*fn)(size_t handle, void*,
                                                           void*),
                                                void* data, void* data1);

    void removeIdler(size_t handle);
    void removeIdlerWithNoGCRooting(size_t handle);

    void clearPendingIdlers(
        ScriptContext* ctx); // give nullptr to clear every idlers

    void destroy();
    void invokeNavigate(WebView* wv, ResourceURL* url, ReferrerURL* referrerURL,
                        HistoryManagerAction action, bool force = false);

    // methods not related with WebView Context
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
    void* m_navigateInvokeIdler;

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
} // namespace Starfish

#endif
