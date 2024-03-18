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

#include "StarfishPlatform.h"
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV) || defined(STARFISH_ENABLE_WORKER)

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/page/GlobalScope.h"
#include "platform/message_loop/RunLoopLibUV.h"
#include "platform/message_loop/MessageLoopLibUV.h"

namespace Starfish {

static void on_close_handle(uv_handle_t* handle)
{
    free(handle);
}

static uv_async_t g_idlerThreadSyncHandle;
static pthread_mutex_t g_threadSyncExecuteGuard;
static pthread_mutex_t g_threadSyncFlowControler;
static RunLoopLibUV g_defaultRunLoop(uv_default_loop());

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    int m_pararmNum;
    uv_timer_t* m_idler_uv;
    MessageLoopLibUV* m_ml;
    GlobalScope* m_globalScope;
    volatile bool m_shouldExecute;
    bool m_isMainThreadData;
};

MessageLoopLibUV::MessageLoopLibUV()
    : MessageLoopLibUV(&g_defaultRunLoop)
{
}

MessageLoopLibUV::MessageLoopLibUV(RunLoopLibUV* runLoop)
    : m_runLoop(runLoop)
{
    m_idlerThreadAsyncHandle = new uv_async_t();

    uv_async_init(uvLoop(), m_idlerThreadAsyncHandle, [](uv_async_t* handle) {
        {
            MessageLoopLibUV* ml = static_cast<MessageLoopLibUV*>(handle->data);

            std::list<size_t> jobs;
            {
                Locker<Mutex> l(*ml->m_idlersFromOtherThreadMutex);
                jobs = std::move(ml->m_idlersFromOtherThreadForUV);
            }

            while (!jobs.empty()) {
                IdlerData* id = nullptr;
                {
                    id = (IdlerData*)*jobs.begin();
                    jobs.erase(jobs.begin());
                }

                if (id) {
                    if (id->m_shouldExecute) {
                        id->m_ml->invokeMicroTasksIfExist();
                        if (id->m_pararmNum == 1) {
                            id->m_fn((size_t)id, id->m_data);
                        } else if (id->m_pararmNum == 2) {
                            ((void (*)(size_t, void*, void*))id->m_fn)(
                                (size_t)id, id->m_data, id->m_data1);
                        }
                    }
                    delete id;
                }
            }
        }
    });

    m_idlerThreadAsyncHandle->data = this;
}

size_t MessageLoopLibUV::addIdler(GlobalScope* globalScope,
                                  void (*fn)(size_t, void*), void* data)
{
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_pararmNum = 1;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    id->m_idler_uv = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uvLoop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_timer_start(
        id->m_idler_uv,
        [](uv_timer_t* handle) {
            IdlerData* id = (IdlerData*)handle->data;
            id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
            id->m_ml->invokeMicroTasksIfExist();
            id->m_fn((size_t)id, id->m_data);
            uv_timer_stop(handle);
            GC_FREE(id);
            uv_close((uv_handle_t*)handle, on_close_handle);
        },
        0, 0);

    return (size_t)id;
}

size_t MessageLoopLibUV::addIdler(GlobalScope* globalScope,
                                  void (*fn)(size_t, void*, void*), void* data,
                                  void* data1)
{
    STARFISH_ASSERT(calledOnValidThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_pararmNum = 2;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    id->m_idler_uv = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uvLoop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_timer_start(
        id->m_idler_uv,
        [](uv_timer_t* handle) {
            IdlerData* id = (IdlerData*)handle->data;
            id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
            id->m_ml->invokeMicroTasksIfExist();
            ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data,
                                                       id->m_data1);
            uv_timer_stop(handle);
            GC_FREE(id);
            uv_close((uv_handle_t*)handle, on_close_handle);
        },
        0, 0);
    return (size_t)id;
}

size_t MessageLoopLibUV::addIdler(GlobalScope* globalScope,
                                  void (*fn)(size_t, void*, void*, void*),
                                  void* data, void* data1, void* data2)
{
    STARFISH_ASSERT(calledOnValidThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_data2 = data2;
    id->m_pararmNum = 3;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    id->m_idler_uv = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uvLoop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_timer_start(
        id->m_idler_uv,
        [](uv_timer_t* handle) {
            IdlerData* id = (IdlerData*)handle->data;
            id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
            id->m_ml->invokeMicroTasksIfExist();
            ((void (*)(size_t, void*, void*, void*))id->m_fn)(
                (size_t)id, id->m_data, id->m_data1, id->m_data2);
            uv_timer_stop(handle);
            GC_FREE(id);
            uv_close((uv_handle_t*)handle, on_close_handle);
        },
        0, 0);
    return (size_t)id;
}

void uv_close_cb(uv_handle_t* handle)
{
    delete (IdlerData*)handle->data;
}

size_t MessageLoopLibUV::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*), void* data)
{
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_shouldExecute = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_pararmNum = 1;
    id->m_ml = this;
    id->m_globalScope = globalScope;

    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThreadForUV.push_back((size_t)id);
    }

    uv_async_send(m_idlerThreadAsyncHandle);
    return (size_t)id;
}

size_t MessageLoopLibUV::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*, void*), void* data,
    void* data1)
{
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_shouldExecute = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_pararmNum = 2;
    id->m_ml = this;
    id->m_globalScope = globalScope;

    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThreadForUV.push_back((size_t)id);
    }

    uv_async_send(m_idlerThreadAsyncHandle);
    return (size_t)id;
}

void MessageLoopLibUV::removeIdler(size_t handle)
{
    IdlerData* id = (IdlerData*)handle;
    m_idlers.erase(m_idlers.find(handle));
    uv_timer_stop(id->m_idler_uv);
    uv_close((uv_handle_t*)id->m_idler_uv, on_close_handle);
    GC_FREE(id);
}

void MessageLoopLibUV::removeIdlerWithNoGCRooting(size_t handle)
{
    IdlerData* id = (IdlerData*)handle;
    id->m_shouldExecute = false;
}

void MessageLoopLibUV::clearPendingIdlers(GlobalScope* globalScope)
{
    clearMicroTasks(globalScope);

    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        if (id->m_globalScope == globalScope || globalScope == nullptr) {
            iter = m_idlers.erase(iter);
            uv_timer_stop(id->m_idler_uv);
            uv_close((uv_handle_t*)id->m_idler_uv, on_close_handle);
            GC_FREE(id);
        } else {
            iter++;
        }
    }

    Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
    auto iter2 = m_idlersFromOtherThreadForUV.begin();
    while (iter2 != m_idlersFromOtherThreadForUV.end()) {
        IdlerData* id = (IdlerData*)*iter2;
        if (id->m_globalScope == globalScope || globalScope == nullptr) {
            id->m_shouldExecute = false;
        }
        iter2++;
    }
}

void MessageLoopLibUV::destroy()
{
    m_inClosingState = true;

    while (true) {
        while (!m_idlersFromOtherThreadForUV.empty()) {
            IdlerData* id = nullptr;
            {
                Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
                id = (IdlerData*)*m_idlersFromOtherThreadForUV.begin();
                m_idlersFromOtherThreadForUV.erase(
                    m_idlersFromOtherThreadForUV.begin());
            }

            if (id) {
                if (id->m_shouldExecute) {
                    if (id->m_pararmNum == 1)
                        id->m_fn((size_t)id, id->m_data);
                    else if (id->m_pararmNum == 2)
                        ((void (*)(size_t, void*, void*))id->m_fn)(
                            (size_t)id, id->m_data, id->m_data1);
                }
                delete id;
            }
        }

        while (!m_idlers.empty()) {
            IdlerData* id = (IdlerData*)*m_idlers.begin();
            id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));

            ((void (*)(size_t, void*, void*, void*))id->m_fn)(
                (size_t)id, id->m_data, id->m_data1, id->m_data2);
            uv_timer_stop(id->m_idler_uv);
            uv_close((uv_handle_t*)id->m_idler_uv, on_close_handle);
            GC_FREE(id);
        }

        bool e;
        {
            Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
            e = m_idlersFromOtherThreadForUV.empty();
        }

        if (m_idlers.empty() && e) {
            STARFISH_LOG_INFO("[Starfish] message loop [m_idlers:%zu]",
                              m_idlers.size());
            break;
        }
    }

    Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
    auto iter2 = m_idlersFromOtherThreadForUV.begin();
    while (iter2 != m_idlersFromOtherThreadForUV.end()) {
        IdlerData* id = (IdlerData*)*iter2;
        id->m_shouldExecute = false;
        iter2++;
    }

    uv_close((uv_handle_t*)m_idlerThreadAsyncHandle, on_close_handle);
}

void MessageLoopLibUV::runOnMainThreadAsync(
    const std::function<void()>& functor)
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

RunLoop* MessageLoopLibUV::runLoop()
{
    return m_runLoop;
}

uv_loop_t* MessageLoopLibUV::uvLoop()
{
    return m_runLoop->uvLoop();
}

#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)

void MessageLoopLibUV::init()
{
    static bool needsInit = true;
    if (UNLIKELY(needsInit)) {
        needsInit = false;
        pthread_mutex_init(&g_threadSyncExecuteGuard, NULL);
        pthread_mutex_init(&g_threadSyncFlowControler, NULL);

        uv_async_init(uv_default_loop(), &g_idlerThreadSyncHandle,
                      [](uv_async_t* handle) {
                          {
                              const std::function<void()>* pFunctor =
                                  (const std::function<void()>*)handle->data;
                              (*pFunctor)();
                              pthread_mutex_unlock(&g_threadSyncFlowControler);
                          }
                      });
    }
}

void MessageLoopLibUV::run()
{
    g_defaultRunLoop.run();
}

void MessageLoopLibUV::stop()
{
    g_defaultRunLoop.stop();
}

void MessageLoopLibUV::runOnMainThreadSync(const std::function<void()>& functor)
{
    if (isMainThread()) {
        functor();
        return;
    }
    pthread_mutex_lock(&g_threadSyncExecuteGuard);
    g_idlerThreadSyncHandle.data = (void*)&functor;

    pthread_mutex_lock(&g_threadSyncFlowControler);
    uv_async_send(&g_idlerThreadSyncHandle);
    pthread_mutex_lock(&g_threadSyncFlowControler);
    pthread_mutex_unlock(&g_threadSyncFlowControler);
    pthread_mutex_unlock(&g_threadSyncExecuteGuard);
}

#endif

} // namespace Starfish
#endif
