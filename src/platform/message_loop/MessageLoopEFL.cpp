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

#include "StarFishConfig.h"
#if defined(PORT_EVENTLOOP_BACKEND_EFL)

#include "StarFish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include <Ecore.h>

#if defined(PORT_WINDOW_BACKEND_EFL)
#include <Elementary.h>
#endif

#if defined(STARFISH_TIZEN_TV)
#define USE_IDLER_AS_ANIMATOR // There timer order issue in tv. we can remove
                              // this define after platform issue solved.
#endif

namespace StarFish {

MessageLoop::MessageLoop(StarFish* sf)
    : StarFishHoldable(sf)
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
    ecore_animator_frametime_set(1 / 120.0);
}

void MessageLoop::run()
{
    ecore_main_loop_begin();
}

void MessageLoop::stop()
{
    ecore_main_loop_quit();
}

struct InvokeNavigateData : public gc {
    WebView* wv;
    ResourceURL* url;
    ResourceURL* referrerURL;
    Ecore_Animator* idler;
    void** extra;

    static void* operator new(size_t s)
    {
        return GC_MALLOC_UNCOLLECTABLE(s);
    }
};

void MessageLoop::close()
{
    m_inClosingState = true;
    if (m_navigateInvokeIdler) {
        ecore_animator_freeze(
            ((InvokeNavigateData*)m_navigateInvokeIdler)->idler);
        ecore_animator_del(((InvokeNavigateData*)m_navigateInvokeIdler)->idler);
        delete ((InvokeNavigateData*)m_navigateInvokeIdler);
        m_navigateInvokeIdler = nullptr;
    }

    if (m_idlers.size() != 0 || m_idlersFromOtherThread.size() != 0) {
        ecore_main_loop_begin();
    }
}

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
#if defined(USE_IDLER_AS_ANIMATOR)
    Ecore_Animator* m_idler;
#else
    Ecore_Timer* m_idler;
#endif
    MessageLoop* m_ml;
    BrowsingContext* m_ctx;
    volatile bool m_valid;
    bool m_isMainThreadData;
};

static void removeIderFromList(std::unordered_set<size_t>& list, IdlerData* id)
{
    list.erase(list.find((size_t)id));
}

static bool validateContext(BrowsingContext* context)
{
    // NOTE null value of context means the idler does not related with browsing
    // context
    return !context || context->isActive();
}

size_t MessageLoop::addIdler(BrowsingContext* ctx, void (*fn)(size_t, void*),
                             void* data)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(validateContext(ctx));
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_ctx = ctx;
#if defined(USE_IDLER_AS_ANIMATOR)
    id->m_idler = ecore_animator_add(
#else
    id->m_idler = ecore_timer_add(
        0.0,
#endif
        [](void* data) -> Eina_Bool {
            IdlerData* id = (IdlerData*)data;
            removeIderFromList(id->m_ml->m_idlers, id);
            if (validateContext(id->m_ctx)) {
                id->m_fn((size_t)id, id->m_data);
            }
            if (id->m_ml->m_inClosingState && id->m_ml->m_idlers.size() == 0 &&
                id->m_ml->m_idlersFromOtherThread.size() == 0) {
                ecore_main_loop_quit();
            }

            GC_FREE(id);
            return ECORE_CALLBACK_CANCEL;
        },
        id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(BrowsingContext* ctx,
                             void (*fn)(size_t, void*, void*), void* data,
                             void* data1)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(validateContext(ctx));
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_ctx = ctx;
#if defined(USE_IDLER_AS_ANIMATOR)
    id->m_idler = ecore_animator_add(
#else
    id->m_idler = ecore_timer_add(
        0.0,
#endif
        [](void* data) -> Eina_Bool {
            IdlerData* id = (IdlerData*)data;
            removeIderFromList(id->m_ml->m_idlers, id);
            if (validateContext(id->m_ctx)) {
                ((void (*)(size_t, void*, void*))id->m_fn)(
                    (size_t)id, id->m_data, id->m_data1);
            }

            if (id->m_ml->m_inClosingState && id->m_ml->m_idlers.size() == 0 &&
                id->m_ml->m_idlersFromOtherThread.size() == 0) {
                ecore_main_loop_quit();
            }
            GC_FREE(id);
            return ECORE_CALLBACK_CANCEL;
        },
        id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(BrowsingContext* ctx,
                             void (*fn)(size_t, void*, void*, void*),
                             void* data, void* data1, void* data2)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(validateContext(ctx));
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_data2 = data2;
    id->m_ml = this;
    id->m_ctx = ctx;
#if defined(USE_IDLER_AS_ANIMATOR)
    id->m_idler = ecore_animator_add(
#else
    id->m_idler = ecore_timer_add(
        0.0,
#endif
        [](void* data) -> Eina_Bool {
            IdlerData* id = (IdlerData*)data;
            removeIderFromList(id->m_ml->m_idlers, id);
            if (validateContext(id->m_ctx)) {
                ((void (*)(size_t, void*, void*, void*))id->m_fn)(
                    (size_t)id, id->m_data, id->m_data1, id->m_data2);
            }

            if (id->m_ml->m_inClosingState && id->m_ml->m_idlers.size() == 0 &&
                id->m_ml->m_idlersFromOtherThread.size() == 0) {
                ecore_main_loop_quit();
            }
            GC_FREE(id);
            return ECORE_CALLBACK_CANCEL;
        },
        id);

    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(!isMainThread());
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_valid = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_ctx = ctx;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    ecore_main_loop_thread_safe_call_async(
        [](void* data) -> void {
#if defined(USE_IDLER_AS_ANIMATOR)
            ecore_animator_add(
#else
            ecore_timer_add(
                0.0,
#endif
                [](void* data) -> Eina_Bool {
                    IdlerData* id = (IdlerData*)data;
                    {
                        Locker<Mutex> l(
                            *id->m_ml->m_idlersFromOtherThreadMutex);
                        removeIderFromList(id->m_ml->m_idlersFromOtherThread,
                                           id);
                    }
                    if (id->m_valid && validateContext(id->m_ctx)) {
                        id->m_fn((size_t)id, id->m_data);
                    }

                    if (id->m_ml->m_inClosingState &&
                        id->m_ml->m_idlers.size() == 0 &&
                        id->m_ml->m_idlersFromOtherThread.size() == 0) {
                        ecore_main_loop_quit();
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
    BrowsingContext* ctx, void (*fn)(size_t, void*, void*), void* data,
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
    id->m_ctx = ctx;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    ecore_main_loop_thread_safe_call_async(
        [](void* data) -> void {
#if defined(USE_IDLER_AS_ANIMATOR)
            ecore_animator_add(
#else
            ecore_timer_add(
                0.0,
#endif
                [](void* data) -> Eina_Bool {
                    IdlerData* id = (IdlerData*)data;
                    {
                        Locker<Mutex> l(
                            *id->m_ml->m_idlersFromOtherThreadMutex);
                        removeIderFromList(id->m_ml->m_idlersFromOtherThread,
                                           id);
                    }
                    if (id->m_valid && validateContext(id->m_ctx)) {
                        ((void (*)(size_t, void*, void*))id->m_fn)(
                            (size_t)id, id->m_data, id->m_data1);
                    }

                    if (id->m_ml->m_inClosingState &&
                        id->m_ml->m_idlers.size() == 0 &&
                        id->m_ml->m_idlersFromOtherThread.size() == 0) {
                        ecore_main_loop_quit();
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
    if (handle == SIZE_MAX) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    removeIderFromList(m_idlers, id);
#if defined(USE_IDLER_AS_ANIMATOR)
    ecore_animator_freeze(id->m_idler);
    ecore_animator_del(id->m_idler);
#else
    ecore_timer_freeze(id->m_idler);
    ecore_timer_del(id->m_idler);
#endif
    GC_FREE(id);
}

void MessageLoop::removeIdlerWithNoGCRooting(size_t handle)
{
    if (handle == SIZE_MAX) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    id->m_valid = false;
}

void MessageLoop::clearPendingIdlers(BrowsingContext* ctx)
{
    STARFISH_ASSERT(isMainThread());
    // Remove idlers
    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        if (id->m_ctx == ctx || ctx == nullptr) {
#if defined(USE_IDLER_AS_ANIMATOR)
            ecore_animator_freeze(id->m_idler);
            ecore_animator_del(id->m_idler);
#else
            ecore_timer_freeze(id->m_idler);
            ecore_timer_del(id->m_idler);
#endif
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
        if ((id->m_ctx == ctx || ctx == nullptr) && id->m_valid) {
            id->m_valid = false;
        }
        iterOther++;
    }
    m_idlersFromOtherThreadMutex->unlock();
}

void MessageLoop::invokeNavigate(WebView* wv, ResourceURL* url,
                                 ResourceURL* referrerURL, bool force)
{
    if (m_navigateInvokeIdler != nullptr) {
        auto data = ((InvokeNavigateData*)m_navigateInvokeIdler);
        ecore_animator_freeze(data->idler);
        ecore_animator_del(data->idler);
        delete data;
    }

    InvokeNavigateData* data = new InvokeNavigateData();
    m_navigateInvokeIdler = data;
    data->extra = &m_navigateInvokeIdler;
    data->wv = wv;
    data->url = url;
    data->referrerURL = referrerURL;
    data->idler = ecore_animator_add(
        [](void* d) -> Eina_Bool {
            InvokeNavigateData* data = (InvokeNavigateData*)d;
            data->wv->navigate(data->url, HistoryManager::Action::Add,
                               data->referrerURL);
            *(data->extra) = nullptr;
            delete data;
            return ECORE_CALLBACK_CANCEL;
        },
        data);
}
}
#endif
