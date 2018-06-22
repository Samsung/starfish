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
#if defined(PORT_EVENTLOOP_BACKEND_WINDOWS)

#include "StarFish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include <Windows.h>

namespace StarFish {

#define IDLE_MESSAGE (WM_USER + 20)
#define IDLE_MESSAGE_FROM_OTHER_THREAD (WM_USER + 21)
#define IDLE_MESSAGE_INVOKE_NAVIGATE (WM_USER + 22)

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    MessageLoop* m_ml;
    BrowsingContext* m_ctx;
    volatile bool m_shouldExecute;
    bool m_isMainThreadData;
    UINT_PTR m_timerID;
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
}

static_assert(sizeof(size_t) == sizeof(WPARAM), "");

struct InvokeNavigateData {
    WebView* wv;
    ResourceURL* url;
    ResourceURL* referrerURL;
    UINT_PTR timerID;
};

__declspec(thread) InvokeNavigateData* g_invokeNavigateData;

class MessageLoopImpl {
public:
    static void processMessage(MessageLoop* self, const MSG& message)
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
        switch (message.message) {
        case IDLE_MESSAGE: {
            STARFISH_ASSERT(message.message == IDLE_MESSAGE);
            IdlerData* id = (IdlerData*)message.wParam;
            if (id->m_shouldExecute) {
                StarFishEnterer enter(id->m_ml->m_starFish);
                if (message.lParam == 1) {
                    id->m_fn((size_t)id, id->m_data);
                } else if (message.lParam == 2) {
                    ((void (*)(size_t, void*, void*))id->m_fn)(
                        (size_t)id, id->m_data, id->m_data1);
                } else if (message.lParam == 3) {
                    ((void (*)(size_t, void*, void*, void*))id->m_fn)(
                        (size_t)id, id->m_data, id->m_data1, id->m_data2);
                }
            }
            self->m_idlers.erase((size_t)id);

            if (self->m_inClosingState && self->m_idlers.size() == 0 &&
                self->m_idlersFromOtherThread.size() == 0) {
                PostMessage(NULL, WM_QUIT, 0, 0);
            }

            GC_FREE(id);
        } break;
        case IDLE_MESSAGE_FROM_OTHER_THREAD: {
            STARFISH_ASSERT(message.message == IDLE_MESSAGE_FROM_OTHER_THREAD);
            IdlerData* id = (IdlerData*)message.wParam;
            if (id->m_shouldExecute) {
                StarFishEnterer enter(self->starFish());
                if (message.lParam == 1) {
                    id->m_fn((size_t)id, id->m_data);
                } else if (message.lParam == 2) {
                    ((void (*)(size_t, void*, void*))id->m_fn)(
                        (size_t)id, id->m_data, id->m_data1);
                }
            }
            STARFISH_ASSERT(_CrtCheckMemory());
            {
                Locker<Mutex> l(*self->m_idlersFromOtherThreadMutex);
                self->m_idlersFromOtherThread.erase((size_t)id);
            }
            if (self->m_inClosingState && self->m_idlers.size() == 0 &&
                self->m_idlersFromOtherThread.size() == 0) {
                PostMessage(NULL, WM_QUIT, 0, 0);
            }
            STARFISH_ASSERT(_CrtCheckMemory());
            delete id;
        } break;
        case IDLE_MESSAGE_INVOKE_NAVIGATE: {
            STARFISH_ASSERT(message.message == IDLE_MESSAGE_INVOKE_NAVIGATE);
            if ((size_t)g_invokeNavigateData == (size_t)message.wParam) {
                g_invokeNavigateData->wv->navigate(
                    g_invokeNavigateData->url, HistoryManager::Action::Add,
                    g_invokeNavigateData->referrerURL);
                g_invokeNavigateData = nullptr;
            }
            GC_FREE((void*)message.wParam);

            if (self->m_inClosingState && self->m_idlers.size() == 0 &&
                self->m_idlersFromOtherThread.size() == 0) {
                PostMessage(NULL, WM_QUIT, 0, 0);
            }
        } break;
        default:
            break;
        }
    }
};

void processMessage(MessageLoop* self, const MSG& message)
{
    MessageLoopImpl::processMessage(self, message);
}

void MessageLoop::close()
{
    m_inClosingState = true;
    g_invokeNavigateData = nullptr;

    if (m_idlers.size() != 0 || m_idlersFromOtherThread.size() != 0) {
        run();
    }
}

void MessageLoop::run()
{
    MSG message;
    BOOL ret;
    while ((ret = GetMessage(&message, NULL, 0, 0)) != 0) {
        if (ret == -1) {
            // handle the error and possibly exit
        } else {
            processMessage(this, message);
        }
    }
}

size_t MessageLoop::addIdler(BrowsingContext* ctx, void (*fn)(size_t, void*),
                             void* data)
{
    IdlerData* id = new (GC_MALLOC_UNCOLLECTABLE(sizeof(IdlerData))) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_shouldExecute = id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_ctx = ctx;
    PostMessage(NULL, IDLE_MESSAGE, (size_t)id, 1);
    return (size_t)id;
}

size_t MessageLoop::addIdler(BrowsingContext* ctx,
                             void (*fn)(size_t, void*, void*), void* data,
                             void* data1)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (GC_MALLOC_UNCOLLECTABLE(sizeof(IdlerData))) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_shouldExecute = id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_ctx = ctx;
    PostMessage(NULL, IDLE_MESSAGE, (size_t)id, 2);
    return (size_t)id;
}

size_t MessageLoop::addIdler(BrowsingContext* ctx,
                             void (*fn)(size_t, void*, void*, void*),
                             void* data, void* data1, void* data2)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (GC_MALLOC_UNCOLLECTABLE(sizeof(IdlerData))) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_shouldExecute = id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_data2 = data2;
    id->m_ml = this;
    id->m_ctx = ctx;
    PostMessage(NULL, IDLE_MESSAGE, (size_t)id, 3);
    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(_CrtCheckMemory());
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

    PostThreadMessage(mainThreadID(), IDLE_MESSAGE_FROM_OTHER_THREAD,
                      (size_t)id, 1);
    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(
    BrowsingContext* ctx, void (*fn)(size_t, void*, void*), void* data,
    void* data1)
{
    STARFISH_ASSERT(_CrtCheckMemory());
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

    PostThreadMessage(mainThreadID(), IDLE_MESSAGE_FROM_OTHER_THREAD,
                      (size_t)id, 2);
    return (size_t)id;
}

void MessageLoop::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = (IdlerData*)handle;
    id->m_shouldExecute = false;
}

void MessageLoop::removeIdlerWithNoGCRooting(size_t handle)
{
    IdlerData* id = (IdlerData*)handle;
    id->m_shouldExecute = false;
}

void MessageLoop::clearPendingIdlers(BrowsingContext* ctx)
{
    STARFISH_ASSERT(_CrtCheckMemory());
    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        if (id->m_ctx == ctx || ctx == nullptr) {
            id->m_shouldExecute = false;
        }
        iter++;
    }

    STARFISH_ASSERT(_CrtCheckMemory());
    Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
    auto iter2 = m_idlersFromOtherThread.begin();
    while (iter2 != m_idlersFromOtherThread.end()) {
        IdlerData* id = (IdlerData*)*iter2;
        if (id->m_ctx == ctx || ctx == nullptr) {
            id->m_shouldExecute = false;
        }
        iter2++;
    }
    STARFISH_ASSERT(_CrtCheckMemory());
}

void MessageLoop::invokeNavigate(WebView* wv, ResourceURL* url,
                                 ResourceURL* referrerURL)
{
    InvokeNavigateData* data =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(InvokeNavigateData)))
            InvokeNavigateData();
    g_invokeNavigateData = data;
    data->wv = wv;
    data->url = url;
    data->referrerURL = referrerURL;
    PostMessage(NULL, IDLE_MESSAGE_INVOKE_NAVIGATE, (size_t)data, 0);
}
} // namespace StarFish
#endif
