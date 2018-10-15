/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)

#include "StarFish.h"
#include "binding/ScriptBindingInstance.h"
#include "core/page/Window.h"
#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/Timer.h"

#include <uv.h>

namespace StarFish {

Timer::Timer(WebView* sf)
    : m_webView(sf)
{
    m_timeoutCounter = 0;
    m_requestAnimationFrameCounter = 1;
    m_animationCounter = 0;
}

struct AnimationTickData {
    Timer* m_timer;
    size_t m_id;
    uv_timer_t* m_timerID;
    void* m_data;
    GenericAnimationHandler m_handler;
    Window* m_window;
};

struct TimeoutData {
    Timer* m_timer;
    size_t m_id;
    uv_timer_t* m_timerID;
    Window* m_window;
    void* m_data;
    WindowSetTimeoutHandler m_handler;
};

void on_close_handle(uv_handle_t* handle)
{
    free(handle);
}

size_t Timer::addTimer(unsigned delay, Window* window,
                       WindowSetTimeoutHandler handler, void* data,
                       bool repetitive)
{
    STARFISH_ASSERT(isMainThread());

    auto id = ++m_timeoutCounter;
    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_timer = this;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_window = window;
    td->m_timerID = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    td->m_timerID->data = td;
    td->m_timerID->type = UV_UNKNOWN_HANDLE;

    uv_timer_init(uv_default_loop(), td->m_timerID);
    if (repetitive) {
        if (delay == 0) {
            delay = 1;
        }
        uv_timer_start(
            td->m_timerID,
            [](uv_timer_t* handle) -> void {
                TimeoutData* td = (TimeoutData*)handle->data;
                auto a = td->m_timer->m_timeoutHandler.find(td->m_id);
                td->m_handler(td->m_window, td->m_data);
            },
            static_cast<uint64_t>(delay), static_cast<uint64_t>(delay));
    } else {
        uv_timer_start(td->m_timerID,
                       [](uv_timer_t* handle) -> void {
                           TimeoutData* td = (TimeoutData*)handle->data;
                           auto iter =
                               td->m_timer->m_timeoutHandler.find(td->m_id);
                           if (iter != td->m_timer->m_timeoutHandler.end()) {
                               td->m_timer->m_timeoutHandler.erase(iter);
                               td->m_handler(td->m_window, td->m_data);
                               GC_FREE(td);
                           }
                           uv_timer_stop(handle);
                           uv_close((uv_handle_t*)handle, on_close_handle);
                       },
                       static_cast<uint64_t>(delay), 0);
    }
    m_timeoutHandler.insert(std::make_pair(id, td));
    return id;
}

void Timer::removeTimer(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());
    auto handlerData = m_timeoutHandler.find(reqID);
    if (handlerData != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        uv_handle_t* handle = (uv_handle_t*)td->m_timerID;
        m_timeoutHandler.erase(handlerData);
        if ((handle->type) == UV_UNKNOWN_HANDLE) {
            return;
        }
        uv_timer_stop(td->m_timerID);
        uv_close((uv_handle_t*)td->m_timerID, on_close_handle);
        GC_FREE(td);
    }
}

size_t Timer::addAnimator(Window* window, GenericAnimationHandler handler,
                          void* data)
{
    STARFISH_ASSERT(isMainThread());
    auto id = ++m_animationCounter;
    AnimationTickData* ad = new (NoGC) AnimationTickData;
    ad->m_timer = this;
    ad->m_data = data;
    ad->m_window = window;
    ad->m_handler = handler;
    ad->m_timerID = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    ad->m_timerID->data = ad;
    ad->m_timerID->type = UV_UNKNOWN_HANDLE;
    uv_timer_init(uv_default_loop(), ad->m_timerID);
    uv_timer_start(ad->m_timerID,
                   [](uv_timer_t* handle) -> void {
                       AnimationTickData* ad = (AnimationTickData*)handle->data;
                       auto a = ad->m_timer->m_animationHandler.find(ad->m_id);
                       if (ad->m_handler(ad->m_data)) {
                           return;
                       }
                       a = ad->m_timer->m_animationHandler.find(ad->m_id);
                       if (ad->m_timer->m_animationHandler.end() != a) {
                           ad->m_timer->m_animationHandler.erase(a);
                           GC_FREE(ad);
                           uv_timer_stop(handle);
                           uv_close((uv_handle_t*)handle, on_close_handle);
                       }

                   },
                   0, 10);
    m_animationHandler.insert(std::make_pair(id, ad));
    return id;
}

void Timer::removeGenericAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_animationHandler.find(reqID);
    if (handlerData != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)handlerData->second;
        m_animationHandler.erase(handlerData);
        uv_handle_t* handle = (uv_handle_t*)ad->m_timerID;
        uv_timer_stop(ad->m_timerID);
        uv_close((uv_handle_t*)ad->m_timerID, on_close_handle);
        GC_FREE(ad);
    }
}

void Timer::clear(BrowsingContext* ctx)
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        if ((td->m_window && td->m_window->browsingContext() == ctx) ||
            ctx == nullptr) {
            timerIter = m_timeoutHandler.erase(timerIter);
            uv_handle_t* handle = (uv_handle_t*)td->m_timerID;
            uv_timer_stop(td->m_timerID);
            uv_close((uv_handle_t*)td->m_timerID, on_close_handle);
            GC_FREE(td);
        } else {
            timerIter++;
        }
    }

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        RequestAnimationFrameData* td =
            (RequestAnimationFrameData*)aniIter->second;
        if ((td->m_window && td->m_window->browsingContext() == ctx) ||
            ctx == nullptr) {
            aniIter = m_requestAnimationFrameHandler.erase(aniIter);
        } else {
            aniIter++;
        }
    }

    auto aniIter2 = m_animationHandler.begin();
    while (aniIter2 != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)aniIter2->second;
        if ((ad->m_window && ad->m_window->browsingContext() == ctx) ||
            ctx == nullptr) {
            aniIter2 = m_animationHandler.erase(aniIter2);
            uv_handle_t* handle = (uv_handle_t*)ad->m_timerID;
            uv_timer_stop(ad->m_timerID);
            uv_close((uv_handle_t*)ad->m_timerID, on_close_handle);
            GC_FREE(ad);
        } else {
            aniIter2++;
        }
    }
}

void Timer::destroy()
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        timerIter = m_timeoutHandler.erase(timerIter);
        uv_timer_stop(td->m_timerID);
        uv_close((uv_handle_t*)td->m_timerID, on_close_handle);
        GC_FREE(td);
    }

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        RequestAnimationFrameData* td =
            (RequestAnimationFrameData*)aniIter->second;
        aniIter = m_requestAnimationFrameHandler.erase(aniIter);
    }

    auto aniIter2 = m_animationHandler.begin();
    while (aniIter2 != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)aniIter2->second;
        aniIter2 = m_animationHandler.erase(aniIter2);
        uv_handle_t* handle = (uv_handle_t*)ad->m_timerID;
        uv_timer_stop(ad->m_timerID);
        uv_close((uv_handle_t*)ad->m_timerID, on_close_handle);
        GC_FREE(ad);
    }
}
}
#endif
