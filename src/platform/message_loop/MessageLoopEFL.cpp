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

#include "StarfishPlatform.h"

#if defined(PORT_EVENTLOOP_BACKEND_EFL)

#include "StarfishBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/page/GlobalScope.h"

#include <Ecore.h>

#if defined(PORT_WINDOW_BACKEND_EFL)
#include <Elementary.h>
#endif

namespace Starfish {

MessageLoop::MessageLoop()
    : m_inClosingState(false)
    , m_idlersFromOtherThreadMutex(new Mutex())
#ifdef STARFISH_MESSAGELOOP_DEBUG
    , m_countingMutex(new Mutex())
    , m_runningThreadCount(0)
    , m_unjoinedThreadCount(0)
    , m_runningPoolWorkerCount(0)
#endif
{
    ecore_animator_frametime_set(1 / 120.0);
}

void MessageLoop::destroy()
{
    m_inClosingState = true;

#ifndef STARFISH_WEBWORKER_HOST
    onDestroyed();
#endif

    while (true) {
        {
            Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
            if (m_idlers.size() == 0 && m_idlersFromOtherThread.size() == 0) {
                break;
            }
        }

        ecore_main_loop_iterate();
    }

    std::unordered_set<size_t>().swap(m_idlers);
    std::unordered_set<size_t>().swap(m_idlersFromOtherThread);
}

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    Ecore_Timer* m_idler;
    MessageLoop* m_ml;
    GlobalScope* m_globalScope;
    volatile bool m_valid;
    bool m_isMainThreadData;
};

static void removeIderFromList(std::unordered_set<size_t>& list, IdlerData* id)
{
    list.erase(list.find((size_t)id));
}

size_t MessageLoop::addIdler(GlobalScope* globalScope,
                             void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    id->m_idler =
        ecore_timer_add(0.0,
                        [](void* data) -> Eina_Bool {
                            IdlerData* id = (IdlerData*)data;
                            removeIderFromList(id->m_ml->m_idlers, id);

                            id->m_fn((size_t)id, id->m_data);

                            GC_FREE(id);
                            return ECORE_CALLBACK_CANCEL;
                        },
                        id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(GlobalScope* globalScope,
                             void (*fn)(size_t, void*, void*), void* data,
                             void* data1)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    id->m_idler =
        ecore_timer_add(0.0,
                        [](void* data) -> Eina_Bool {
                            IdlerData* id = (IdlerData*)data;
                            removeIderFromList(id->m_ml->m_idlers, id);
                            ((void (*)(size_t, void*, void*))id->m_fn)(
                                (size_t)id, id->m_data, id->m_data1);

                            GC_FREE(id);
                            return ECORE_CALLBACK_CANCEL;
                        },
                        id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(GlobalScope* globalScope,
                             void (*fn)(size_t, void*, void*, void*),
                             void* data, void* data1, void* data2)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_data2 = data2;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    id->m_idler = ecore_timer_add(
        0.0,
        [](void* data) -> Eina_Bool {
            IdlerData* id = (IdlerData*)data;
            removeIderFromList(id->m_ml->m_idlers, id);
            ((void (*)(size_t, void*, void*, void*))id->m_fn)(
                (size_t)id, id->m_data, id->m_data1, id->m_data2);

            GC_FREE(id);
            return ECORE_CALLBACK_CANCEL;
        },
        id);

    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(!isMainThread());
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_valid = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    ecore_main_loop_thread_safe_call_async(
        [](void* data) -> void {
            ecore_timer_add(
                0.0,
                [](void* data) -> Eina_Bool {
                    IdlerData* id = (IdlerData*)data;
                    {
                        Locker<Mutex> l(
                            *id->m_ml->m_idlersFromOtherThreadMutex);
                        removeIderFromList(id->m_ml->m_idlersFromOtherThread,
                                           id);
                    }
                    if (id->m_valid) {
                        id->m_fn((size_t)id, id->m_data);
                    }

                    delete id;
                    return ECORE_CALLBACK_CANCEL;
                },
                data);
        },
        id);
    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*, void*), void* data,
    void* data1)
{
    STARFISH_ASSERT(!isMainThread());
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_valid = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    ecore_main_loop_thread_safe_call_async(
        [](void* data) -> void {
            ecore_timer_add(
                0.0,
                [](void* data) -> Eina_Bool {
                    IdlerData* id = (IdlerData*)data;
                    {
                        Locker<Mutex> l(
                            *id->m_ml->m_idlersFromOtherThreadMutex);
                        removeIderFromList(id->m_ml->m_idlersFromOtherThread,
                                           id);
                    }
                    if (id->m_valid) {
                        ((void (*)(size_t, void*, void*))id->m_fn)(
                            (size_t)id, id->m_data, id->m_data1);
                    }

                    delete id;
                    return ECORE_CALLBACK_CANCEL;
                },
                data);
        },
        id);
    return (size_t)id;
}

void MessageLoop::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    if (handle == MessageLoopInvalidID) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    removeIderFromList(m_idlers, id);
    ecore_timer_freeze(id->m_idler);
    ecore_timer_del(id->m_idler);
    GC_FREE(id);
}

void MessageLoop::removeIdlerWithNoGCRooting(size_t handle)
{
    if (handle == MessageLoopInvalidID) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    id->m_valid = false;
}

void MessageLoop::clearPendingIdlers(GlobalScope* globalScope)
{
    STARFISH_ASSERT(isMainThread());
    // Remove idlers
    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        if (id->m_globalScope == globalScope || globalScope == nullptr) {
            ecore_timer_freeze(id->m_idler);
            ecore_timer_del(id->m_idler);
            iter = m_idlers.erase(iter);
            GC_FREE(id);
        } else {
            iter++;
        }
    }
    // Remove idlers from other thread
    m_idlersFromOtherThreadMutex->lock();
    auto iterOther = m_idlersFromOtherThread.begin();
    while (iterOther != m_idlersFromOtherThread.end()) {
        IdlerData* id = (IdlerData*)*iterOther;
        if ((id->m_globalScope == globalScope || globalScope == nullptr) &&
            id->m_valid) {
            id->m_valid = false;
        }
        iterOther++;
    }
    m_idlersFromOtherThreadMutex->unlock();
}

void MessageLoop::init()
{
}

void MessageLoop::run()
{
    ecore_main_loop_begin();
}

void MessageLoop::stop()
{
    ecore_main_loop_quit();
}
}
#endif
