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

#include "StarfishConfig.h"
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)

#include "Starfish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include <uv.h>

namespace Starfish {

void on_close_handle(uv_handle_t* handle);

static uv_async_t g_idlerThreadSyncHandle;
static size_t g_uvRunCount;
static pthread_mutex_t g_threadSyncExecuteGuard;
static pthread_mutex_t g_threadSyncFlowControler;

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    int m_pararmNum;
    uv_timer_t* m_idler_uv;
    MessageLoop* m_ml;
    BrowsingContext* m_ctx;
    volatile bool m_shouldExecute;
    bool m_isMainThreadData;
};

MessageLoop::MessageLoop(WebView* webView)
    : WebViewHoldable(webView)
    , m_inClosingState(false)
    , m_idlersFromOtherThreadMutex(new Mutex())
    , m_navigateInvokeIdler(nullptr)
#ifdef STARFISH_MESSAGELOOP_DEBUG
    , m_countingMutex(new Mutex())
    , m_runningThreadCount(0)
    , m_unjoinedThreadCount(0)
    , m_runningPoolWorkerCount(0)
#endif
{
    m_idlerThreadAsyncHandle = malloc(sizeof(uv_async_t));

    uv_async_init(
        uv_default_loop(), (uv_async_t*)m_idlerThreadAsyncHandle,
        [](uv_async_t* handle) {
            {
                MessageLoop* ml = (MessageLoop*)handle->data;

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

    ((uv_async_t*)m_idlerThreadAsyncHandle)->data = this;
}

size_t MessageLoop::addIdler(BrowsingContext* ctx, void (*fn)(size_t, void*),
                             void* data)
{
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_pararmNum = 1;
    id->m_ml = this;
    id->m_ctx = ctx;
    id->m_idler_uv = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uv_default_loop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_timer_start(id->m_idler_uv,
                   [](uv_timer_t* handle) {
                       IdlerData* id = (IdlerData*)handle->data;
                       id->m_ml->m_idlers.erase(
                           id->m_ml->m_idlers.find((size_t)id));
                       id->m_fn((size_t)id, id->m_data);
                       uv_timer_stop(handle);
                       GC_FREE(id);
                       uv_close((uv_handle_t*)handle, on_close_handle);
                   },
                   0, 0);

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
    id->m_pararmNum = 2;
    id->m_ml = this;
    id->m_ctx = ctx;
    id->m_idler_uv = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uv_default_loop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_timer_start(
        id->m_idler_uv,
        [](uv_timer_t* handle) {
            IdlerData* id = (IdlerData*)handle->data;
            id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
            ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data,
                                                       id->m_data1);
            uv_timer_stop(handle);
            GC_FREE(id);
            uv_close((uv_handle_t*)handle, on_close_handle);

        },
        0, 0);
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
    id->m_pararmNum = 3;
    id->m_ml = this;
    id->m_ctx = ctx;
    id->m_idler_uv = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uv_default_loop(), id->m_idler_uv);
    id->m_idler_uv->data = id;
    uv_timer_start(id->m_idler_uv,
                   [](uv_timer_t* handle) {
                       IdlerData* id = (IdlerData*)handle->data;
                       id->m_ml->m_idlers.erase(
                           id->m_ml->m_idlers.find((size_t)id));

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

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*), void* data)
{
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_shouldExecute = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_pararmNum = 1;
    id->m_ml = this;
    id->m_ctx = ctx;

    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThreadForUV.push_back((size_t)id);
    }

    uv_async_send((uv_async_t*)m_idlerThreadAsyncHandle);
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
    id->m_pararmNum = 2;
    id->m_ml = this;
    id->m_ctx = ctx;

    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThreadForUV.push_back((size_t)id);
    }

    uv_async_send((uv_async_t*)m_idlerThreadAsyncHandle);
    return (size_t)id;
}

void MessageLoop::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = (IdlerData*)handle;
    m_idlers.erase(m_idlers.find(handle));
    uv_timer_stop(id->m_idler_uv);
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
    uv_timer_t* idler;
    HistoryManagerAction action;
    void** extra;

    static void* operator new(size_t s)
    {
        return GC_MALLOC_UNCOLLECTABLE(s);
    }
};

void MessageLoop::destroy()
{
    m_inClosingState = true;
    if (m_navigateInvokeIdler) {
        uv_timer_stop(((InvokeNavigateData*)m_navigateInvokeIdler)->idler);
        uv_close(
            (uv_handle_t*)((InvokeNavigateData*)m_navigateInvokeIdler)->idler,
            on_close_handle);
        delete ((InvokeNavigateData*)m_navigateInvokeIdler);
        m_navigateInvokeIdler = nullptr;
    }

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
            STARFISH_LOG_INFO("[Starfish] message loop [m_idlers:%zu]\n",
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

void MessageLoop::invokeNavigate(WebView* wv, ResourceURL* url,
                                 ResourceURL* referrerURL,
                                 HistoryManagerAction action, bool force)
{
    if (m_navigateInvokeIdler != nullptr) {
        auto data = ((InvokeNavigateData*)m_navigateInvokeIdler);
        uv_timer_stop(data->idler);
        uv_close((uv_handle_t*)data->idler, on_close_handle);
        delete data;
    }

    InvokeNavigateData* data = new InvokeNavigateData();
    m_navigateInvokeIdler = data;
    data->extra = &m_navigateInvokeIdler;
    data->wv = wv;
    data->url = url;
    data->referrerURL = referrerURL;
    data->action = action;
    data->idler = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uv_default_loop(), data->idler);
    data->idler->data = data;
    uv_timer_start(
        data->idler,
        [](uv_timer_t* handle) {
            InvokeNavigateData* data = (InvokeNavigateData*)handle->data;
            data->wv->navigate(data->url, data->action, data->referrerURL);
            *(data->extra) = nullptr;
            uv_timer_stop(handle);
            delete data;
            uv_close((uv_handle_t*)handle, on_close_handle);
        },
        0, 0);
}

void MessageLoop::init()
{
    static bool needsInit = true;
    if (UNLIKELY(needsInit)) {
        needsInit = false;
        pthread_mutex_init(&g_threadSyncExecuteGuard, NULL);
        pthread_mutex_init(&g_threadSyncFlowControler, NULL);

        uv_async_init(uv_default_loop(), &g_idlerThreadSyncHandle,
                      [](uv_async_t* handle) {
                          {
                              const std::function<size_t()>* pFunctor =
                                  (const std::function<size_t()>*)handle->data;
                              size_t ret = (*pFunctor)();
                              handle->data = (void*)ret;
                              pthread_mutex_unlock(&g_threadSyncFlowControler);
                          }
                      });
    }
}

void MessageLoop::run()
{
    size_t theCountBefore = g_uvRunCount;
    g_uvRunCount++;
    while (true) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
        if (UNLIKELY(theCountBefore >= g_uvRunCount)) {
            break;
        }
    }
}

void MessageLoop::stop()
{
    if (g_uvRunCount) {
        g_uvRunCount--;
    }
    uv_stop(uv_default_loop());
}

size_t MessageLoop::runOnMainThreadSync(const std::function<size_t()>& functor)
{
    if (isMainThread()) {
        return functor();
    }
    pthread_mutex_lock(&g_threadSyncExecuteGuard);
    g_idlerThreadSyncHandle.data = (void*)&functor;

    pthread_mutex_lock(&g_threadSyncFlowControler);
    uv_async_send(&g_idlerThreadSyncHandle);
    pthread_mutex_lock(&g_threadSyncFlowControler);
    pthread_mutex_unlock(&g_threadSyncFlowControler);
    size_t ret = (size_t)g_idlerThreadSyncHandle.data;

    pthread_mutex_unlock(&g_threadSyncExecuteGuard);

    return ret;
}

void MessageLoop::runOnMainThreadAsync(const std::function<void()>& functor)
{
    struct Param {
        std::function<void()> functor;
    };

    Param* p = new Param();
    p->functor = functor;

    addIdlerWithNoGCRootingInOtherThread(nullptr,
                                         [](size_t, void* data) {
                                             Param* p = (Param*)data;
                                             p->functor();
                                             delete p;
                                         },
                                         p);

    return;
}
}
#endif
