/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#if defined(PORT_EVENTLOOP_BACKEND_EFL)

#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/GlobalScope.h"
#include "core/page/WebBase.h"

#include <Ecore.h>

namespace Starfish {

Timer::Timer(WebBase* webBase)
    : m_webBase(webBase)
{
    m_timeoutCounter = 0;
    m_requestAnimationFrameCounter = 1;
    m_animationCounter = 0;
}

struct AnimationTickData : public gc {
    Timer* m_timer;
    int32_t m_id;
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    Ecore_Animator* m_timerID;
#else
    Ecore_Timer* m_timerID;
#endif
    void* m_data;
    GenericAnimationHandler m_handler;
    GlobalScope* m_globalScope;
};

struct TimeoutData : public gc {
    Timer* m_timer;
    int32_t m_id;
    Ecore_Timer* m_timerID;
    GlobalScope* m_globalScope;
    void* m_data;
    TimerHandler m_handler;
};

size_t Timer::addTimer(unsigned delay, GlobalScope* globalScope,
                       TimerHandler handler, void* data, bool repetitive)
{
    STARFISH_ASSERT(isMainThread());

    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_timer = this;
    auto id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_globalScope = globalScope;
    td->m_data = data;
    td->m_handler = handler;

    if (repetitive) {
        td->m_timerID = ecore_timer_add(
            delay / 1000.0,
            [](void* data) -> Eina_Bool {
                TimeoutData* td = (TimeoutData*)data;
                auto a = td->m_timer->m_timeoutHandler.find(td->m_id);
                td->m_timer->m_webBase->messageLoop()
                    ->invokeMicroTasksIfExist();
                td->m_handler(td->m_data);
                return ECORE_CALLBACK_RENEW;
            },
            td);

    } else {
        td->m_timerID = ecore_timer_add(
            delay / 1000.0,
            [](void* data) -> Eina_Bool {
                TimeoutData* td = (TimeoutData*)data;
                Timer* timer = td->m_timer;
                int32_t id = td->m_id;
                td->m_timer->m_webBase->messageLoop()
                    ->invokeMicroTasksIfExist();
                td->m_handler(td->m_data);
                auto iter = timer->m_timeoutHandler.find(id);
                if (iter != timer->m_timeoutHandler.end()) {
                    timer->m_timeoutHandler.erase(iter);
                    GC_FREE(td);
                }
                return ECORE_CALLBACK_DONE;
            },
            td);
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
        ecore_timer_freeze(td->m_timerID);
        ecore_timer_del(td->m_timerID);
        GC_FREE(td);
        m_timeoutHandler.erase(handlerData);
    }
}

size_t Timer::addAnimator(GlobalScope* globalScope,
                          GenericAnimationHandler handler, void* data)
{
    STARFISH_ASSERT(isMainThread());
    AnimationTickData* ad = new (NoGC) AnimationTickData;
    ad->m_timer = this;
    auto id = ++m_animationCounter;
    ad->m_id = id;
    ad->m_data = data;
    ad->m_handler = handler;
    ad->m_globalScope = globalScope;
    ad->m_timerID =
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
        ecore_animator_add(
#else
        ecore_timer_add(
            0.0001,
#endif
            [](void* data) -> Eina_Bool {
                AnimationTickData* ad = (AnimationTickData*)data;
                auto a = ad->m_timer->m_animationHandler.find(ad->m_id);
                if (ad->m_handler(ad->m_data)) {
                    return ECORE_CALLBACK_RENEW;
                }
                a = ad->m_timer->m_animationHandler.find(ad->m_id);
                if (ad->m_timer->m_animationHandler.end() != a) {
                    ad->m_timer->m_animationHandler.erase(a);
                }
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
                ecore_animator_freeze(ad->m_timerID);
                ecore_animator_del(ad->m_timerID);
#else
                ecore_timer_freeze(ad->m_timerID);
                ecore_timer_del(ad->m_timerID);
#endif
                GC_FREE(ad);
                return ECORE_CALLBACK_CANCEL;
            },
            ad);
    m_animationHandler.insert(std::make_pair(id, ad));
    return id;
}

void Timer::removeGenericAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_animationHandler.find(reqID);
    if (handlerData != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)handlerData->second;
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
        ecore_animator_freeze(ad->m_timerID);
        ecore_animator_del(ad->m_timerID);
#else
        ecore_timer_freeze(ad->m_timerID);
        ecore_timer_del(ad->m_timerID);
#endif
        GC_FREE(ad);
        m_animationHandler.erase(handlerData);
    }
}

void Timer::clear(GlobalScope* globalScope)
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        if ((td->m_globalScope && td->m_globalScope == globalScope) ||
            globalScope == nullptr) {
            ecore_timer_freeze(td->m_timerID);
            ecore_timer_del(td->m_timerID);
            GC_FREE(td);
            timerIter = m_timeoutHandler.erase(timerIter);
        } else {
            timerIter++;
        }
    }

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        RequestAnimationFrameData* td =
            (RequestAnimationFrameData*)aniIter->second;
        if ((td->m_globalScope && td->m_globalScope == globalScope) ||
            globalScope == nullptr) {
            aniIter = m_requestAnimationFrameHandler.erase(aniIter);
        } else {
            aniIter++;
        }
    }

    auto aniIter2 = m_animationHandler.begin();
    while (aniIter2 != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)aniIter2->second;
        if ((ad->m_globalScope && ad->m_globalScope == globalScope) ||
            globalScope == nullptr) {
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
            ecore_animator_freeze(ad->m_timerID);
            ecore_animator_del(ad->m_timerID);
#else
            ecore_timer_freeze(ad->m_timerID);
            ecore_timer_del(ad->m_timerID);
#endif
            GC_FREE(ad);
            aniIter2 = m_animationHandler.erase(aniIter2);
        } else {
            aniIter2++;
        }
    }
}

void Timer::destroy()
{
    STARFISH_LOG_INFO("TimerEFL::destroy");
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        ecore_timer_freeze(td->m_timerID);
        ecore_timer_del(td->m_timerID);
        GC_FREE(td);
        timerIter++;
    }
    m_timeoutHandler.clear();

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        RequestAnimationFrameData* td =
            (RequestAnimationFrameData*)aniIter->second;
        aniIter++;
    }
    m_requestAnimationFrameHandler.clear();

    auto aniIter2 = m_animationHandler.begin();
    while (aniIter2 != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)aniIter2->second;
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
        ecore_animator_freeze(ad->m_timerID);
        ecore_animator_del(ad->m_timerID);
#else
        ecore_timer_freeze(ad->m_timerID);
        ecore_timer_del(ad->m_timerID);
#endif
        GC_FREE(ad);
        aniIter2++;
    }
    m_animationHandler.clear();
}
} // namespace Starfish
#endif
