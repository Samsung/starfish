/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV) && \
    defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)

#include "StarFish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/Window.h"

#include <uv.h>

namespace StarFish {

uv_signal_t g_sigterm;
uv_signal_t g_sigint;

void uv_term_cb(uv_signal_t* handle, int signum)
{
    exit(0);
}

MessageLoop::MessageLoop(StarFish* sf)
    : m_starFish(sf)
    , m_idlersFromOtherThreadMutex(new Mutex())
{
    uv_signal_init(uv_default_loop(), &g_sigterm);
    uv_signal_start(&g_sigterm, &uv_term_cb, SIGTERM);

    uv_signal_init(uv_default_loop(), &g_sigint);
    uv_signal_start(&g_sigint, &uv_term_cb, SIGINT);
}

void MessageLoop::run()
{
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    uv_idle_t m_idler_uv;
    MessageLoop* m_ml;
    BrowsingContext* m_ctx;
    volatile bool m_shouldExecute;
    bool m_isMainThreadData;
};

struct IdlerDataAsync {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    uv_async_t m_idler_uv;
    MessageLoop* m_ml;
    BrowsingContext* m_ctx;
    volatile bool m_shouldExecute;
    bool m_isMainThreadData;
};

size_t MessageLoop::addIdler(BrowsingContext* ctx, void (*fn)(size_t, void*),
                             void* data)
{
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_ctx = ctx;
    uv_idle_init(uv_default_loop(), &id->m_idler_uv);
    id->m_idler_uv.data = id;
    uv_idle_start(&id->m_idler_uv, [](uv_idle_t* handle) {
        IdlerData* id = (IdlerData*)handle->data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        StarFishEnterer enter(id->m_ml->m_starFish);
        id->m_fn((size_t)id, id->m_data);
        uv_idle_stop(handle);
        GC_FREE(id);
    });

    return (size_t)id;
}

size_t MessageLoop::addIdler(BrowsingContext* ctx,
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
    id->m_ctx = ctx;
    uv_idle_init(uv_default_loop(), &id->m_idler_uv);
    id->m_idler_uv.data = id;
    uv_idle_start(&id->m_idler_uv, [](uv_idle_t* handle) {
        IdlerData* id = (IdlerData*)handle->data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        StarFishEnterer enter(id->m_ml->m_starFish);
        ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data,
                                                   id->m_data1);
        uv_idle_stop(handle);
        GC_FREE(id);
    });
    return (size_t)id;
}

size_t MessageLoop::addIdler(BrowsingContext* ctx,
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
    id->m_ctx = ctx;
    uv_idle_init(uv_default_loop(), &id->m_idler_uv);
    id->m_idler_uv.data = id;
    uv_idle_start(&id->m_idler_uv, [](uv_idle_t* handle) {
        IdlerData* id = (IdlerData*)handle->data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));

        StarFishEnterer enter(id->m_ml->m_starFish);
        ((void (*)(size_t, void*, void*, void*))id->m_fn)(
            (size_t)id, id->m_data, id->m_data1, id->m_data2);
        uv_idle_stop(handle);
        GC_FREE(id);
    });
    return (size_t)id;
}

void uv_close_cb(uv_handle_t* handle)
{
    delete (IdlerDataAsync*)handle->data;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*), void* data)
{
    IdlerDataAsync* id = new IdlerDataAsync;
    id->m_isMainThreadData = false;
    id->m_shouldExecute = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_ctx = ctx;

    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    uv_async_init(uv_default_loop(), &id->m_idler_uv, [](uv_async_t* handle) {
        IdlerDataAsync* id = (IdlerDataAsync*)handle->data;
        {
            Locker<Mutex> l(*id->m_ml->m_idlersFromOtherThreadMutex);
            id->m_ml->m_idlersFromOtherThread.erase(
                id->m_ml->m_idlersFromOtherThread.find((size_t)id));
        }
        if (id->m_shouldExecute) {
            StarFishEnterer enter(id->m_ml->m_starFish);
            id->m_fn((size_t)id, id->m_data);
        }
        uv_close((uv_handle_t*)handle, &uv_close_cb);
    });
    id->m_idler_uv.data = id;
    uv_async_send(&id->m_idler_uv);
    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*, void*), void* data,
    void* data1)
{
    IdlerDataAsync* id = new IdlerDataAsync;
    id->m_isMainThreadData = false;
    id->m_shouldExecute = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_ctx = ctx;

    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }
    uv_async_init(uv_default_loop(), &id->m_idler_uv, [](uv_async_t* handle) {
        IdlerDataAsync* id = (IdlerDataAsync*)handle->data;
        {
            Locker<Mutex> l(*id->m_ml->m_idlersFromOtherThreadMutex);
            id->m_ml->m_idlersFromOtherThread.erase(
                id->m_ml->m_idlersFromOtherThread.find((size_t)id));
        }
        if (id->m_shouldExecute) {
            StarFishEnterer enter(id->m_ml->m_starFish);
            ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data,
                                                       id->m_data1);
        }
        uv_close((uv_handle_t*)handle, &uv_close_cb);
    });
    id->m_idler_uv.data = id;
    uv_async_send(&id->m_idler_uv);
    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoScriptInstanceEntering(
    BrowsingContext* ctx, void (*fn)(size_t handle, void*, void*), void* data,
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
    id->m_ctx = ctx;
    uv_idle_init(uv_default_loop(), &id->m_idler_uv);
    id->m_idler_uv.data = id;
    uv_idle_start(&id->m_idler_uv, [](uv_idle_t* handle) {
        IdlerData* id = (IdlerData*)handle->data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data,
                                                   id->m_data1);
        uv_idle_stop(handle);
        GC_FREE(id);
    });
    return (size_t)id;
}

void MessageLoop::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = (IdlerData*)handle;
    m_idlers.erase(m_idlers.find(handle));
    uv_idle_stop(&id->m_idler_uv);
    GC_FREE(id);
}

void MessageLoop::removeIdlerWithNoGCRooting(size_t handle)
{
    IdlerData* id = (IdlerData*)handle;
    id->m_shouldExecute = false;
}

void MessageLoop::clearPendingIdlers(BrowsingContext* ctx)
{
    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        if (id->m_ctx == ctx || ctx == nullptr) {
            uv_idle_stop(&id->m_idler_uv);
            GC_FREE(id);
            m_idlers.erase(iter++);
        } else {
            iter++;
        }
    }

    Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
    auto iter2 = m_idlersFromOtherThread.begin();
    while (iter2 != m_idlersFromOtherThread.end()) {
        IdlerData* id = (IdlerData*)*iter2;
        if (id->m_ctx == ctx || ctx == nullptr) {
            id->m_shouldExecute = false;
        }
        iter2++;
    }
}
}
#endif
