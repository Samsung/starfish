/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "StarfishPlatform.h"

#if defined(PORT_EVENTLOOP_BACKEND_GLIB)

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/page/GlobalScope.h"
#include "platform/message_loop/MessageLoopGLib.h"

#include <glib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <condition_variable>
#include <mutex>

namespace Starfish {

static bool isSystemMainThread()
{
    return getCurrentThreadID() == getpid();
}

MessageLoopGLib::MessageLoopGLib()
    : MessageLoop()
{
}

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    guint m_idler;
    MessageLoopGLib* m_ml;
    GlobalScope* m_globalScope;
    volatile bool m_needsRun;
    volatile bool m_isDestroyed;
    bool m_isMainThreadData;
};

void MessageLoopGLib::destroy()
{
    STARFISH_LOG_INFO("MessageLoopGLib::destroy()");
    m_inClosingState = true;
    while (true) {
        {
            Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
            if (m_idlers.size() == 0 && m_idlersFromOtherThread.size() == 0) {
                break;
            }
        }

        g_main_context_iteration(nullptr, FALSE);
    }
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        auto iterOther = m_idlersFromOtherThread.begin();
        while (iterOther != m_idlersFromOtherThread.end()) {
            IdlerData* id = (IdlerData*)*iterOther;
            id->m_isDestroyed = true;
            iterOther++;
        }
    }
    std::unordered_set<size_t>().swap(m_idlers);
    std::unordered_set<size_t>().swap(m_idlersFromOtherThread);
}

static void removeIderFromList(std::unordered_set<size_t>& list, IdlerData* id)
{
    list.erase(list.find((size_t)id));
}

size_t MessageLoopGLib::addIdler(GlobalScope* globalScope,
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
    id->m_idler = g_timeout_add(
        0,
        [](gpointer data) -> gboolean {
            IdlerData* id = (IdlerData*)data;
            removeIderFromList(id->m_ml->m_idlers, id);

            id->m_fn((size_t)id, id->m_data);

            GC_FREE(id);
            return G_SOURCE_REMOVE;
        },
        id);

    return (size_t)id;
}

size_t MessageLoopGLib::addIdler(GlobalScope* globalScope,
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
    id->m_idler = g_timeout_add(
        0,
        [](gpointer data) -> gboolean {
            IdlerData* id = (IdlerData*)data;
            removeIderFromList(id->m_ml->m_idlers, id);

            ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data,
                                                       id->m_data1);

            GC_FREE(id);
            return G_SOURCE_REMOVE;
        },
        id);

    return (size_t)id;
}

size_t MessageLoopGLib::addIdler(GlobalScope* globalScope,
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
    id->m_idler = g_timeout_add(
        0,
        [](gpointer data) -> gboolean {
            IdlerData* id = (IdlerData*)data;
            removeIderFromList(id->m_ml->m_idlers, id);

            ((void (*)(size_t, void*, void*, void*))id->m_fn)(
                (size_t)id, id->m_data, id->m_data1, id->m_data2);

            GC_FREE(id);
            return G_SOURCE_REMOVE;
        },
        id);

    return (size_t)id;
}

size_t MessageLoopGLib::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(!isMainThread());
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_needsRun = true;
    id->m_isDestroyed = false;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    g_idle_add_full(
        G_PRIORITY_DEFAULT,
        [](gpointer data) -> gboolean {
            g_timeout_add(
                0,
                [](gpointer data) -> gboolean {
                    IdlerData* id = (IdlerData*)data;
                    if (!id->m_isDestroyed) {
                        if (id->m_ml &&
                            id->m_ml->m_idlersFromOtherThreadMutex) {
                            Locker<Mutex> l(
                                *id->m_ml->m_idlersFromOtherThreadMutex);
                            removeIderFromList(
                                id->m_ml->m_idlersFromOtherThread, id);
                        }
                        if (id->m_needsRun) {
                            id->m_fn((size_t)id, id->m_data);
                        }
                    }

                    id->m_ml = nullptr;
                    delete id;
                    return G_SOURCE_REMOVE;
                },
                data);
            return G_SOURCE_REMOVE;
        },
        id, NULL);
    return (size_t)id;
}

size_t MessageLoopGLib::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*, void*), void* data,
    void* data1)
{
    STARFISH_ASSERT(!isMainThread());
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_needsRun = true;
    id->m_isDestroyed = false;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    g_idle_add_full(
        G_PRIORITY_DEFAULT,
        [](gpointer data) -> gboolean {
            g_timeout_add(
                0,
                [](gpointer data) -> gboolean {
                    IdlerData* id = (IdlerData*)data;
                    if (!id->m_isDestroyed) {
                        id->m_isDestroyed = true;
                        if (id->m_ml &&
                            id->m_ml->m_idlersFromOtherThreadMutex) {
                            Locker<Mutex> l(
                                *id->m_ml->m_idlersFromOtherThreadMutex);
                            removeIderFromList(
                                id->m_ml->m_idlersFromOtherThread, id);
                        }
                        if (id->m_needsRun) {
                            ((void (*)(size_t, void*, void*))id->m_fn)(
                                (size_t)id, id->m_data, id->m_data1);
                        }
                    }

                    id->m_ml = nullptr;
                    delete id;
                    return G_SOURCE_REMOVE;
                },
                data);
            return G_SOURCE_REMOVE;
        },
        id, NULL);
    return (size_t)id;
}

void MessageLoopGLib::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    if (handle == MessageLoopInvalidID) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    removeIderFromList(m_idlers, id);
    g_source_remove(id->m_idler);
    GC_FREE(id);
}

void MessageLoopGLib::removeIdlerWithNoGCRooting(size_t handle)
{
    if (handle == MessageLoopInvalidID) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    id->m_needsRun = false;
}

void MessageLoopGLib::clearPendingIdlers(GlobalScope* globalScope)
{
    STARFISH_LOG_INFO("clearPendingIdlers: globalScope[%p]", globalScope);
    STARFISH_ASSERT(isMainThread());

    // Remove idlers
    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        if (id->m_globalScope == globalScope || globalScope == nullptr) {
            g_source_remove(id->m_idler);
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
            id->m_needsRun) {
            id->m_needsRun = false;
        }
        iterOther++;
    }
    m_idlersFromOtherThreadMutex->unlock();
}

void MessageLoopGLib::runOnMainThreadAsync(const std::function<void()>& functor)
{
    struct Param {
        std::function<void()> functor;
    };

    Param* p = new Param();
    p->functor = functor;

    if (isMainThread()) {
        addIdler(
            nullptr,
            [](size_t, void* data) -> void {
                Param* p = (Param*)data;
                p->functor();
                delete p;
            },
            p);
        return;
    } else {
        addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                Param* p = (Param*)data;
                p->functor();
                delete p;
            },
            p);
    }
}

void MessageLoopGLib::init()
{
    STARFISH_RELEASE_ASSERT(isSystemMainThread());
}

void MessageLoopGLib::run()
{
}

void MessageLoopGLib::stop()
{
}

void MessageLoopGLib::runOnMainThreadSync(const std::function<void()>& functor)
{
    if (isMainThread()) {
        functor();
        return;
    }

    struct Param {
        std::function<void()> functor;
        std::mutex mutex;
        std::condition_variable cv;
        bool completed;
    };

    Param* p = new Param();
    p->functor = functor;
    p->completed = false;

    g_idle_add(
        [](gpointer data) -> gboolean {
            Param* p = (Param*)data;
            p->functor();
            {
                std::lock_guard<std::mutex> lock(p->mutex);
                p->completed = true;
            }
            p->cv.notify_one();
            return G_SOURCE_REMOVE;
        },
        p);

    // Wait for completion
    {
        std::unique_lock<std::mutex> lock(p->mutex);
        p->cv.wait(lock, [p] { return p->completed; });
    }
    delete p;
}

} // namespace Starfish
#endif
