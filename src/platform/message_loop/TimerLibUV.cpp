/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

Timer::Timer(StarFish* sf)
    : m_starFish(sf)
{
    m_timeoutCounter = 0;
    m_requestAnimationFrameCounter = 1;
    m_AnimationCounter = 0;
}

struct AnimationTickData {
    Timer* m_timer;
    int32_t m_id;
    uv_timer_t m_timerID;
    void* m_data;
    GenericAnimationHandler m_handler;
    Window* m_window;
};

struct TimeoutData {
    Timer* m_timer;
    int32_t m_id;
    uv_timer_t m_timerID;
    Window* m_window;
    void* m_data;
    WindowSetTimeoutHandler m_handler;
};

size_t Timer::addTimer(double delay, Window* window,
                       WindowSetTimeoutHandler handler, void* data,
                       bool repetitive)
{
    STARFISH_ASSERT(isMainThread());
    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_timer = this;
    int32_t id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_window = window;
    uv_timer_init(uv_default_loop(), &td->m_timerID);
    td->m_timerID.data = td;
    if (repetitive) {
        uv_timer_start(&td->m_timerID,
                       [](uv_timer_t* handle) -> void {
                           TimeoutData* td = (TimeoutData*)handle->data;
                           StarFishEnterer enter(td->m_timer->m_starFish);
                           auto a =
                               td->m_timer->m_timeoutHandler.find(td->m_id);
                           td->m_handler(td->m_window, td->m_data);
                       },
                       0, delay);
    } else {
        uv_timer_start(&td->m_timerID,
                       [](uv_timer_t* handle) -> void {
                           TimeoutData* td = (TimeoutData*)handle->data;
                           StarFishEnterer enter(td->m_timer->m_starFish);
                           Timer* timer = td->m_timer;
                           int32_t id = td->m_id;
                           td->m_handler(td->m_window, td->m_data);
                           auto iter = timer->m_timeoutHandler.find(id);
                           if (iter != timer->m_timeoutHandler.end()) {
                               timer->m_timeoutHandler.erase(iter);
                               GC_FREE(td);
                           }
                           uv_timer_stop(handle);
                       },
                       delay, 0);
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
        uv_timer_stop(&td->m_timerID);
        GC_FREE(td);
        m_timeoutHandler.erase(handlerData);
    }
}

size_t Timer::addAnimator(Window* window, WindowSetTimeoutHandler handler,
                          void* data)
{
    STARFISH_ASSERT(isMainThread());
    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_timer = this;
    int32_t id = ++m_requestAnimationFrameCounter;
    td->m_id = id;
    td->m_window = window;
    td->m_data = data;
    td->m_handler = handler;
    uv_timer_init(uv_default_loop(), &td->m_timerID);
    td->m_timerID.data = td;
    uv_timer_start(
        &td->m_timerID,
        [](uv_timer_t* handle) -> void {
            TimeoutData* td = (TimeoutData*)handle->data;
            StarFishEnterer enter(td->m_timer->m_starFish);
            auto a = td->m_timer->m_requestAnimationFrameHandler.find(td->m_id);
            td->m_handler(td->m_window, td->m_data);
            a = td->m_timer->m_requestAnimationFrameHandler.find(td->m_id);
            if (td->m_timer->m_requestAnimationFrameHandler.end() != a) {
                td->m_timer->m_requestAnimationFrameHandler.erase(a);
            }
            GC_FREE(td);
        },
        0, 0);
    m_requestAnimationFrameHandler.insert(std::make_pair(id, td));
    return id;
}

size_t Timer::addAnimator(Window* window, GenericAnimationHandler handler,
                          void* data)
{
    STARFISH_ASSERT(isMainThread());
    AnimationTickData* ad = new (NoGC) AnimationTickData;
    ad->m_timer = this;
    int32_t id = ++m_AnimationCounter;
    ad->m_data = data;
    ad->m_window = window;
    ad->m_handler = handler;
    uv_timer_init(uv_default_loop(), &ad->m_timerID);
    ad->m_timerID.data = ad;
    uv_timer_start(&ad->m_timerID,
                   [](uv_timer_t* handle) -> void {
                       AnimationTickData* ad = (AnimationTickData*)handle->data;
                       StarFishEnterer enter(ad->m_timer->m_starFish);
                       auto a = ad->m_timer->m_animationHandler.find(ad->m_id);
                       if (ad->m_handler(ad->m_data)) {
                           return;
                       }
                       a = ad->m_timer->m_animationHandler.find(ad->m_id);
                       if (ad->m_timer->m_animationHandler.end() != a) {
                           ad->m_timer->m_animationHandler.erase(a);
                       }
                       GC_FREE(ad);
                       uv_timer_stop(handle);
                   },
                   0, 1);
    m_animationHandler.insert(std::make_pair(id, ad));
    return id;
}

void Timer::removeWindowAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_requestAnimationFrameHandler.find(reqID);

    if (handlerData != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        uv_timer_stop(&td->m_timerID);
        GC_FREE(td);
        m_requestAnimationFrameHandler.erase(handlerData);
    }
}

void Timer::removeGenericAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_animationHandler.find(reqID);
    if (handlerData != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)handlerData->second;
        uv_timer_stop(&ad->m_timerID);
        GC_FREE(ad);
        m_animationHandler.erase(handlerData);
    }
}

void Timer::clear(BrowsingContext* ctx)
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        if (td->m_window->browsingContext() == ctx || ctx == nullptr) {
            uv_timer_stop(&td->m_timerID);
            GC_FREE(td);
            m_timeoutHandler.erase(timerIter++);
        } else {
            timerIter++;
        }
    }

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)aniIter->second;
        if (td->m_window->browsingContext() == ctx || ctx == nullptr) {
            uv_timer_stop(&td->m_timerID);
            GC_FREE(td);
            m_requestAnimationFrameHandler.erase(aniIter++);
        } else {
            aniIter++;
        }
    }

    auto aniIter2 = m_animationHandler.begin();
    while (aniIter2 != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)aniIter2->second;
        if (ad->m_window->browsingContext() == ctx || ctx == nullptr) {
            uv_timer_stop(&ad->m_timerID);
            GC_FREE(ad);
            m_animationHandler.erase(aniIter2++);
        } else {
            aniIter2++;
        }
    }
}
}
#endif
