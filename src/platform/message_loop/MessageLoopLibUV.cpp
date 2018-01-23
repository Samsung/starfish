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

// TODO Need update code

#include "StarFishConfig.h"
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV) &&         \
    (defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER) || \
     defined(PORT_GRAPHIC_BACKEND_ANDROID))

#include "StarFish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include <uv.h>

namespace StarFish {

void on_close_handle(uv_handle_t* handle);

uv_async_t m_idler_thread_async_handle1;
uv_async_t m_idler_thread_async_handle2;

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    uv_idle_t* m_idler_uv;
    MessageLoop* m_ml;
    BrowsingContext* m_ctx;
    volatile bool m_shouldExecute;
    bool m_isMainThreadData;
};

MessageLoop::MessageLoop(StarFish* sf)
    : StarFishHoldable(sf)
    , m_idlersFromOtherThreadMutex(new Mutex())
    , m_navigateInvokeIdler(nullptr)
#ifdef STARFISH_MESSAGELOOP_DEBUG
    , m_countingMutex(new Mutex())
    , m_runningThreadCount(0)
    , m_unjoinedThreadCount(0)
    , m_runningPoolWorkerCount(0)
#endif
{
    uv_async_init(
        uv_default_loop(), &m_idler_thread_async_handle1,
        [](uv_async_t* handle) {
            {
                MessageLoop* ml = (MessageLoop*)handle->data;
                // TODO: need to lock following whole section
                while (!ml->m_idlersFromOtherThread.empty()) {
                    IdlerData* id = nullptr;
                    {
                        Locker<Mutex> l(*ml->m_idlersFromOtherThreadMutex);
                        id = (IdlerData*)*ml->m_idlersFromOtherThread.begin();
                        ml->m_idlersFromOtherThread.erase(
                            ml->m_idlersFromOtherThread.begin());
                    }

                    if (id) {
                        if (id->m_shouldExecute) {
                            StarFishEnterer enter(ml->m_starFish);
                            id->m_fn((size_t)id, id->m_data);
                        }
                        delete id;
                    }
                }
            }
        });

    uv_async_init(
        uv_default_loop(), &m_idler_thread_async_handle2,
        [](uv_async_t* handle) {
            {
                MessageLoop* ml = (MessageLoop*)handle->data;
                while (!ml->m_idlersFromOtherThread.empty()) {
                    IdlerData* id = nullptr;
                    {
                        Locker<Mutex> l(*ml->m_idlersFromOtherThreadMutex);
                        id = (IdlerData*)*ml->m_idlersFromOtherThread.begin();
                        ml->m_idlersFromOtherThread.erase(
                            ml->m_idlersFromOtherThread.begin());
                    }

                    if (id) {
                        if (id->m_shouldExecute) {
                            StarFishEnterer enter(ml->m_starFish);
                            ((void (*)(size_t, void*, void*))id->m_fn)(
                                (size_t)id, id->m_data, id->m_data1);
                        }
                        delete id;
                    }
                }
            }
        });
}

void MessageLoop::run()
{
}

size_t MessageLoop::addIdler(BrowsingContext* ctx, void (*fn)(size_t, void*),
                             void* data)
{
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_ctx = ctx;
    id->m_idler_uv = (uv_idle_t*)malloc(sizeof(uv_idle_t));
    uv_idle_init(uv_default_loop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_idle_start(id->m_idler_uv, [](uv_idle_t* handle) {
        IdlerData* id = (IdlerData*)handle->data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        StarFishEnterer enter(id->m_ml->m_starFish);
        id->m_fn((size_t)id, id->m_data);
        uv_idle_stop(handle);
        GC_FREE(id);
        uv_close((uv_handle_t*)handle, on_close_handle);
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
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_ctx = ctx;
    id->m_idler_uv = (uv_idle_t*)malloc(sizeof(uv_idle_t));
    uv_idle_init(uv_default_loop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_idle_start(id->m_idler_uv, [](uv_idle_t* handle) {
        IdlerData* id = (IdlerData*)handle->data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        StarFishEnterer enter(id->m_ml->m_starFish);
        ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data,
                                                   id->m_data1);
        uv_idle_stop(handle);
        GC_FREE(id);
        uv_close((uv_handle_t*)handle, on_close_handle);

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
    id->m_idler_uv = (uv_idle_t*)malloc(sizeof(uv_idle_t));
    uv_idle_init(uv_default_loop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_idle_start(id->m_idler_uv, [](uv_idle_t* handle) {
        IdlerData* id = (IdlerData*)handle->data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));

        StarFishEnterer enter(id->m_ml->m_starFish);
        ((void (*)(size_t, void*, void*, void*))id->m_fn)(
            (size_t)id, id->m_data, id->m_data1, id->m_data2);
        uv_idle_stop(handle);
        GC_FREE(id);
        uv_close((uv_handle_t*)handle, on_close_handle);

    });
    return (size_t)id;
}

void uv_close_cb(uv_handle_t* handle)
{
    delete (IdlerData*)handle->data;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*), void* data)
{
    IdlerData* id = new IdlerData;
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

    m_idler_thread_async_handle1.data = this;
    uv_async_send(&m_idler_thread_async_handle1);
    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*, void*), void* data,
    void* data1)
{
    IdlerData* id = new IdlerData;
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

    m_idler_thread_async_handle2.data = this;
    uv_async_send(&m_idler_thread_async_handle2);
    return (size_t)id;
}

void MessageLoop::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = (IdlerData*)handle;
    m_idlers.erase(m_idlers.find(handle));
    uv_idle_stop(id->m_idler_uv);
    uv_close((uv_handle_t*)id->m_idler_uv, on_close_handle);
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
            iter = m_idlers.erase(iter);
            uv_idle_stop(id->m_idler_uv);
            uv_close((uv_handle_t*)id->m_idler_uv, on_close_handle);
            GC_FREE(id);
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

struct InvokeNavigateData : public gc {
    WebView* wv;
    ResourceURL* url;
    ResourceURL* referrerURL;
    uv_idle_t* idler;
    void** extra;

    static void* operator new(size_t s)
    {
        return GC_MALLOC_UNCOLLECTABLE(s);
    }
};

void MessageLoop::invokeNavigate(WebView* wv, ResourceURL* url,
                                 ResourceURL* referrerURL)
{
    if (m_navigateInvokeIdler != nullptr) {
        auto data = ((InvokeNavigateData*)m_navigateInvokeIdler);
        uv_idle_stop(data->idler);
        uv_close((uv_handle_t*)data->idler, on_close_handle);
        delete data;
    }

    InvokeNavigateData* data = new InvokeNavigateData();
    m_navigateInvokeIdler = data;
    data->extra = &m_navigateInvokeIdler;
    data->wv = wv;
    data->url = url;
    data->referrerURL = referrerURL;
    data->idler = (uv_idle_t*)malloc(sizeof(uv_idle_t));
    uv_idle_init(uv_default_loop(), data->idler);
    data->idler->data = data;
    uv_idle_start(data->idler, [](uv_idle_t* handle) {
        InvokeNavigateData* data = (InvokeNavigateData*)handle->data;
        data->wv->navigate(data->url, HistoryManager::Action::Add,
                           data->referrerURL);
        *(data->extra) = nullptr;
        uv_idle_stop(handle);
        delete data;
        uv_close((uv_handle_t*)handle, on_close_handle);
    });
}
}
#endif
