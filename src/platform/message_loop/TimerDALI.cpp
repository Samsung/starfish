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
#if defined(PORT_GRAPHIC_BACKEND_DALI)

#include "StarFish.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/window/Window.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/Timer.h"

#include <dali-toolkit/dali-toolkit.h>

namespace StarFish {

TimerWrapper::TimerWrapper(StarFish* sf)
    : m_starFish(sf)
{
    m_timeoutCounter = 0;
    m_requestAnimationFrameCounter = 1;
    m_AnimationCounter = 0;
}

class AnimationTickData : public Dali::ConnectionTracker, public gc {
public:
    AnimationTickData()
    {
    }

    TimerWrapper* m_timer;
    int32_t m_id;
    Dali::Timer m_native_timer;
    void* m_data;
    GenericAnimationHandler m_handler;

    bool AnimationTick()
    {
        StarFishEnterer enter(m_timer->m_starFish);
        auto a = m_timer->m_animationHandler.find(m_id);
        if (m_handler(m_data)) {
            return true;
        }
        a = m_timer->m_animationHandler.find(m_id);
        if (m_timer->m_animationHandler.end() != a) {
            m_timer->m_animationHandler.erase(a);
        }
        GC_FREE(this);
        return false;
    }
};

class TimeoutData : public Dali::ConnectionTracker, public gc {
public:
    TimeoutData()
    {
    }

    TimerWrapper* m_timer;
    int32_t m_id;
    Dali::Timer m_native_timer;
    void* m_data;
    WindowSetTimeoutHandler m_handler;

    bool OnceTick()
    {
        StarFishEnterer enter(m_timer->m_starFish);
        TimerWrapper* timer = m_timer;
        int32_t id = m_id;
        m_handler(m_timer->m_starFish->window(), m_data);
        auto iter = m_timer->m_timeoutHandler.find(id);
        if (iter != m_timer->m_timeoutHandler.end()) {
            m_timer->m_timeoutHandler.erase(iter);
            GC_FREE(this);
        }
        return false;
    }

    bool OnTick()
    {
        StarFishEnterer enter(m_timer->m_starFish);
        auto a = m_timer->m_timeoutHandler.find(m_id);
        m_handler(m_timer->m_starFish->window(), m_data);
        return true;
    }

    bool AnimationTick()
    {
        StarFishEnterer enter(m_timer->m_starFish);
        auto a = m_timer->m_requestAnimationFrameHandler.find(m_id);
        m_handler(m_timer->m_starFish->window(), m_data);
        a = m_timer->m_requestAnimationFrameHandler.find(m_id);
        if (m_timer->m_requestAnimationFrameHandler.end() != a) {
            m_timer->m_requestAnimationFrameHandler.erase(a);
        }
        GC_FREE(this);
        return false;
    }
};

size_t TimerWrapper::addTimer(double delay, WindowSetTimeoutHandler handler,
                              void* data, bool repetitive)
{
    STARFISH_ASSERT(isMainThread());

    TimeoutData* td = new (NoGC) TimeoutData();
    td->m_timer = this;
    int32_t id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_native_timer = Dali::Timer::New(delay / 1000.0);
    if (repetitive) {
        td->m_native_timer.TickSignal().Connect(td, &TimeoutData::OnTick);
    } else {
        td->m_native_timer.TickSignal().Connect(td, &TimeoutData::OnceTick);
    }
    m_timeoutHandler.insert(std::make_pair(id, td));
    td->m_native_timer.Start();
    return id;
}

void TimerWrapper::removeTimer(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());
    auto handlerData = m_timeoutHandler.find(reqID);
    if (handlerData != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        td->m_native_timer.Stop();
        GC_FREE(td);
        m_timeoutHandler.erase(handlerData);
    }
}

size_t TimerWrapper::addAnimator(WindowSetTimeoutHandler handler, void* data)
{
    STARFISH_ASSERT(isMainThread());
    TimeoutData* td = new (NoGC) TimeoutData();
    td->m_timer = this;
    int32_t id = ++m_requestAnimationFrameCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_native_timer = Dali::Timer::New(0);
    td->m_native_timer.TickSignal().Connect(td, &TimeoutData::AnimationTick);
    m_requestAnimationFrameHandler.insert(std::make_pair(id, td));
    td->m_native_timer.Start();
    return id;
}

size_t TimerWrapper::addAnimator(GenericAnimationHandler handler, void* data)
{
    STARFISH_ASSERT(isMainThread());
    AnimationTickData* ad = new (NoGC) AnimationTickData();
    ad->m_timer = this;
    int32_t id = ++m_AnimationCounter;
    ad->m_data = data;
    ad->m_handler = handler;
    ad->m_native_timer = Dali::Timer::New(0);
    ad->m_native_timer.TickSignal().Connect(ad,
                                            &AnimationTickData::AnimationTick);
    m_animationHandler.insert(std::make_pair(id, ad));
    ad->m_native_timer.Start();
    return id;
}

void TimerWrapper::removeWindowAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_requestAnimationFrameHandler.find(reqID);

    if (handlerData != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        td->m_native_timer.Stop();
        GC_FREE(td);
        m_requestAnimationFrameHandler.erase(handlerData);
    }
}

void TimerWrapper::removeGenericAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_animationHandler.find(reqID);
    if (handlerData != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)handlerData->second;
        ad->m_native_timer.Stop();
        GC_FREE(ad);
        m_animationHandler.erase(handlerData);
    }
}

void TimerWrapper::clear()
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        td->m_native_timer.Stop();
        GC_FREE(td);
        timerIter++;
    }
    m_timeoutHandler.clear();

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)aniIter->second;
        td->m_native_timer.Stop();
        GC_FREE(td);
        aniIter++;
    }
    m_requestAnimationFrameHandler.clear();

    auto aniIter2 = m_animationHandler.begin();
    while (aniIter2 != m_animationHandler.end()) {
        AnimationTickData* td = (AnimationTickData*)aniIter2->second;
        td->m_native_timer.Stop();
        GC_FREE(td);
        aniIter2++;
    }
    m_animationHandler.clear();
}
}
#endif
