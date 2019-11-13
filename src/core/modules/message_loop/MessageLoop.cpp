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
        m_microTaskIdler = addIdler(globalScope,
                                    [](size_t handle, void* data) {
                                        MessageLoop* self = (MessageLoop*)data;
                                        self->m_microTaskIdler =
                                            MessageLoopInvalidID;
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
    for (size_t i = 0; i < m_microTasks.size(); i++) {
        m_microTasks[i].m_callback(m_microTasks[i].m_id,
                                   m_microTasks[i].m_data);
    }

    m_microTasks.clear();
}

void MessageLoop::clearMicroTasks(GlobalScope* globalScope)
{
    if (globalScope == nullptr) {
        m_microTasks.clear();
        if (m_microTaskIdler != MessageLoopInvalidID) {
            removeIdler(m_microTaskIdler);
        }
    } else {
        for (size_t i = 0; i < m_microTasks.size(); i++) {
            if (m_microTasks[i].m_globalScope == globalScope) {
                m_microTasks.erase(i);
                i--;
            }
        }
    }
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
