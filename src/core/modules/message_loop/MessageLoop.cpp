/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/message_loop/MessageLoopEFL.h"
#include "platform/message_loop/MessageLoopLibUV.h"
#include "platform/message_loop/MessageLoopWindows.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

MessageLoop* MessageLoop::create()
{
#if defined(PORT_EVENTLOOP_BACKEND_EFL)
    return new MessageLoopEFL();
#elif defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    return new MessageLoopLibUV();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    return new MessageLoopWindows();
#else
#error "Unknown EventLoop back-end"
#endif
}

#if defined(STARFISH_ENABLE_WORKER)
MessageLoop* MessageLoop::createForWorker(RunLoop* runLoop)
{
    return new MessageLoopLibUV(reinterpret_cast<RunLoopLibUV*>(runLoop));
}
#endif

void MessageLoop::init()
{
    registerMainThread();
#if defined(PORT_EVENTLOOP_BACKEND_EFL)
    MessageLoopEFL::init();
#elif defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::init();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::init();
#else
#error "Unknown EventLoop back-end"
#endif
}

void MessageLoop::run()
{
#if defined(PORT_EVENTLOOP_BACKEND_EFL)
    MessageLoopEFL::run();
#elif defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::run();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::run();
#else
#error "Unknown EventLoop back-end"
#endif
}

void MessageLoop::stop()
{
#if defined(PORT_EVENTLOOP_BACKEND_EFL)
    MessageLoopEFL::stop();
#elif defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::stop();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::stop();
#else
#error "Unknown EventLoop back-end"
#endif
}

void MessageLoop::runOnMainThreadSync(const std::function<void()>& functor)
{
#if defined(PORT_EVENTLOOP_BACKEND_EFL)
    MessageLoopEFL::runOnMainThreadSync(functor);
#elif defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::runOnMainThreadSync(functor);
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::runOnMainThreadSync(functor);
#else
#error "Unknown EventLoop back-end"
#endif
}

MessageLoop::MessageLoop()
    : m_inClosingState(false)
    , m_idlersFromOtherThreadMutex(new Mutex())
    , m_microTaskCounter(0)
    , m_microTaskIdler(MessageLoopInvalidID)
    , m_currentThreadID(getCurrentThreadID())
#ifdef STARFISH_MESSAGELOOP_DEBUG
    , m_countingMutex(new Mutex())
    , m_runningThreadCount(0)
    , m_unjoinedThreadCount(0)
    , m_runningPoolWorkerCount(0)
#endif
{
}

size_t MessageLoop::addMicroTask(GlobalScope* globalScope,
                                 void (*fn)(size_t handle, void*), void* data)
{
    MicroTask m;
    m.m_id = m_microTaskCounter++;
    m.m_globalScope = globalScope;
    m.m_data = data;
    m.m_callback = fn;
    m_microTasks.push_back(m);

    if (m_microTaskIdler == MessageLoopInvalidID) {
        m_microTaskIdler = addIdler(
            nullptr,
            [](size_t handle, void* data) {
                MessageLoop* self = (MessageLoop*)data;
                self->m_microTaskIdler = MessageLoopInvalidID;
            },
            this);
    }

    return m.m_id;
}

void MessageLoop::removeMicroTask(size_t handle)
{
    for (size_t i = 0; i < m_microTasks.size(); i++) {
        if (m_microTasks[i].m_id == handle) {
            m_microTasks.erase(i);
            break;
        }
    }
}

void MessageLoop::invokeMicroTasksIfExist()
{
    while (m_microTasks.size()) {
        auto tasks(std::move(m_microTasks));
        for (size_t i = 0; i < tasks.size(); i++) {
            tasks[i].m_callback(tasks[i].m_id, tasks[i].m_data);
        }
    }
}

void MessageLoop::clearMicroTasks(GlobalScope* globalScope)
{
    if (globalScope == nullptr) {
        m_microTasks.clear();
        if (m_microTaskIdler != MessageLoopInvalidID) {
            removeIdler(m_microTaskIdler);
        }
    } else {
        m_microTasks.erase(
            std::remove_if(m_microTasks.begin(), m_microTasks.end(),
                           [globalScope](const MicroTask& item) {
                               return item.m_globalScope == globalScope;
                           }),
            m_microTasks.end());
    }
}

RunLoop* MessageLoop::runLoop()
{
    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

bool MessageLoop::calledOnValidThread()
{
    return m_currentThreadID == getCurrentThreadID();
}

#ifdef STARFISH_MESSAGELOOP_DEBUG
int MessageLoop::runningThreadCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    return (int)m_runningThreadCount;
}
void MessageLoop::increaseRunningThreadCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    m_runningThreadCount++;
}
void MessageLoop::decreaseRunningThreadCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    m_runningThreadCount--;
}
int MessageLoop::unjoinedThreadCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    return (int)m_unjoinedThreadCount;
}
void MessageLoop::increaseUnjoinedThreadCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    m_unjoinedThreadCount++;
}
void MessageLoop::decreaseUnjoinedThreadCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    m_unjoinedThreadCount--;
}
int MessageLoop::runningPoolWorkerCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    return (int)m_runningPoolWorkerCount;
}
void MessageLoop::increaseRunningPoolWorkerCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    m_runningPoolWorkerCount++;
}
void MessageLoop::decreaseRunningPoolWorkerCount()
{
    Locker<Mutex> lock(*m_countingMutex);
    m_runningPoolWorkerCount--;
}
#endif
} // namespace Starfish
