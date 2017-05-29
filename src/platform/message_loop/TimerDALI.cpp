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
#if defined(PORT_GRAPHIC_BACKEND_DALI)

#include "StarFish.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/window/Window.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/Timer.h"

namespace StarFish {

PlatformTimer::PlatformTimer(StarFish* sf)
    : m_starFish(sf)
{
    m_timeoutCounter = 0;
    m_requestAnimationFrameCounter = 1;
}

struct TimeoutData {
    PlatformTimer* m_timer;
    int32_t m_id;
    // Ecore_Timer* m_timerID;
    void* m_data;
    WindowSetTimeoutHandler m_handler;
};

size_t PlatformTimer::addTimer(double delay, WindowSetTimeoutHandler handler,
                               void* data, bool repetitive)
{
    STARFISH_ASSERT(isMainThread());

    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_timer = this;
    int32_t id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;

    if (repetitive) {
        // td->m_timerID = ecore_timer_add(
        //     delay / 1000.0,
        //     [](void* data) -> Eina_Bool {
        //         TimeoutData* td = (TimeoutData*)data;
        //         StarFishEnterer enter(td->m_timer->m_starFish);
        //         auto a = td->m_timer->m_timeoutHandler.find(td->m_id);
        //         td->m_handler(td->m_timer->m_starFish->window(), td->m_data);
        //         return ECORE_CALLBACK_RENEW;
        //     },
        //     td);

    } else {
        // td->m_timerID = ecore_timer_add(
        //     delay / 1000.0,
        //     [](void* data) -> Eina_Bool {
        //         TimeoutData* td = (TimeoutData*)data;
        //         StarFishEnterer enter(td->m_timer->m_starFish);
        //         PlatformTimer* timer = td->m_timer;
        //         int32_t id = td->m_id;
        //         td->m_handler(td->m_timer->m_starFish->window(), td->m_data);
        //         auto iter = timer->m_timeoutHandler.find(id);
        //         if (iter != timer->m_timeoutHandler.end()) {
        //             timer->m_timeoutHandler.erase(iter);
        //             GC_FREE(td);
        //         }
        //         return ECORE_CALLBACK_DONE;
        //     },
        //     td);
    }

    m_timeoutHandler.insert(std::make_pair(id, td));
    return id;
}

void PlatformTimer::removeTimer(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());
    auto handlerData = m_timeoutHandler.find(reqID);
    if (handlerData != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        // ecore_timer_del(td->m_timerID);
        GC_FREE(td);
        m_timeoutHandler.erase(handlerData);
    }
}

size_t PlatformTimer::addAnimator(WindowSetTimeoutHandler handler, void* data)
{
    STARFISH_ASSERT(isMainThread());
    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_timer = this;
    int32_t id = ++m_requestAnimationFrameCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    // td->m_timerID = (Ecore_Timer*)ecore_animator_add(
    //     [](void* data) -> Eina_Bool {
    //         TimeoutData* td = (TimeoutData*)data;
    //         StarFishEnterer enter(td->m_timer->m_starFish);
    //         auto a =
    //         td->m_timer->m_requestAnimationFrameHandler.find(td->m_id);
    //         td->m_handler(td->m_timer->m_starFish->window(), td->m_data);
    //         a = td->m_timer->m_requestAnimationFrameHandler.find(td->m_id);
    //         if (td->m_timer->m_requestAnimationFrameHandler.end() != a) {
    //             td->m_timer->m_requestAnimationFrameHandler.erase(a);
    //         }
    //         GC_FREE(td);
    //         return ECORE_CALLBACK_DONE;
    //     },
    //     td);

    m_requestAnimationFrameHandler.insert(std::make_pair(id, td));

    return id;
}
void PlatformTimer::removeAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_requestAnimationFrameHandler.find(reqID);

    if (handlerData != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        // ecore_animator_del((Ecore_Animator*)td->m_timerID);
        GC_FREE(td);
        m_requestAnimationFrameHandler.erase(handlerData);
    }
}

void PlatformTimer::clear()
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        // ecore_timer_del(td->m_timerID);
        GC_FREE(td);
        timerIter++;
    }
    m_timeoutHandler.clear();

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)aniIter->second;
        // ecore_animator_del((Ecore_Animator*)td->m_timerID);
        GC_FREE(td);
        aniIter++;
    }
    m_requestAnimationFrameHandler.clear();
}
}
#endif
