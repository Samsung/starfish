/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {
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
