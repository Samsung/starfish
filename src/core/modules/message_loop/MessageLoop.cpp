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
#include "platform/message_loop/MessageLoopLibUV.h"
#include "platform/message_loop/MessageLoopWindows.h"
#include "platform/message_loop/MessageLoopGLib.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

MessageLoop* MessageLoop::create()
{
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    return new MessageLoopLibUV();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    return new MessageLoopWindows();
#elif defined(PORT_EVENTLOOP_BACKEND_GLIB)
    return new MessageLoopGLib();
#else
#error "Unknown EventLoop back-end"
#endif
}

#if defined(STARFISH_ENABLE_WORKER)
MessageLoop* MessageLoop::createForWorker(RunLoop* runLoop)
{
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    return new MessageLoopLibUV(reinterpret_cast<RunLoopLibUV*>(runLoop));
#elif defined(PORT_EVENTLOOP_BACKEND_GLIB)
    return new MessageLoopGLib(reinterpret_cast<RunLoopGLib*>(runLoop));
#endif
}
#endif

void MessageLoop::init()
{
    registerMainThread();
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::init();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::init();
#elif defined(PORT_EVENTLOOP_BACKEND_GLIB)
    MessageLoopGLib::init();
#else
#error "Unknown EventLoop back-end"
#endif
}

void MessageLoop::run()
{
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::run();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::run();
#elif defined(PORT_EVENTLOOP_BACKEND_GLIB)
    MessageLoopGLib::run();
#else
#error "Unknown EventLoop back-end"
#endif
}

void MessageLoop::stop()
{
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::stop();
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::stop();
#elif defined(PORT_EVENTLOOP_BACKEND_GLIB)
    MessageLoopGLib::stop();
#else
#error "Unknown EventLoop back-end"
#endif
}

void MessageLoop::runOnMainThreadSync(const std::function<void()>& functor)
{
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    MessageLoopLibUV::runOnMainThreadSync(functor);
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    MessageLoopWindows::runOnMainThreadSync(functor);
#elif defined(PORT_EVENTLOOP_BACKEND_GLIB)
    MessageLoopGLib::runOnMainThreadSync(functor);
#else
#error "Unknown EventLoop back-end"
#endif
}

MessageLoop::MessageLoop()
    : m_inClosingState(false)
    , m_idlersFromOtherThreadMutex(new Mutex())
    , m_currentThreadID(getCurrentThreadID())
#ifdef STARFISH_MESSAGELOOP_DEBUG
    , m_countingMutex(new Mutex())
    , m_runningThreadCount(0)
    , m_unjoinedThreadCount(0)
    , m_runningPoolWorkerCount(0)
#endif
{
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
