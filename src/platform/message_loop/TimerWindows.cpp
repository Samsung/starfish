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
#if defined(PORT_EVENTLOOP_BACKEND_WINDOWS)

#include "Starfish.h"

#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/GlobalScope.h"
#include "core/page/WebBase.h"

#include "platform/message_loop/TimerWindows.h"

#include <Windows.h>

namespace Starfish {

TimerWindows::TimerWindows(WebBase* webBase)
    : Timer(webBase)
{
}

struct AnimationTickData {
    TimerWindows* m_timer;
    int32_t m_id;
    UINT_PTR m_timerID;
    void* m_data;
    GenericAnimationHandler m_handler;
    GlobalScope* m_globalScope;
};

struct TimeoutData {
    TimerWindows* m_timer;
    int32_t m_id;
    UINT_PTR m_timerID;
    GlobalScope* m_globalScope;
    bool m_repetitive;
    void* m_data;
    TimerHandler m_handler;
};

__declspec(thread) std::unordered_map<size_t, size_t> g_windowsTimerData;

size_t TimerWindows::addTimer(unsigned delay, GlobalScope* globalScope,
                              TimerHandler handler, void* data, bool repetitive)
{
    STARFISH_ASSERT(isMainThread());

    int32_t id = ++m_timeoutCounter;
    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_timer = this;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_globalScope = globalScope;
    td->m_repetitive = repetitive;
    td->m_timerID = SetTimer(
        NULL, (size_t)td, delay, [](HWND, UINT, UINT_PTR timerId, DWORD) {
            TimeoutData* td = (TimeoutData*)g_windowsTimerData[timerId];
            if (!td->m_repetitive) {
                KillTimer(NULL, timerId);
                auto iter = td->m_timer->m_timeoutHandler.find(td->m_id);
                if (iter != td->m_timer->m_timeoutHandler.end()) {
                    td->m_timer->m_timeoutHandler.erase(iter);
                    td->m_handler(td->m_data);
                    GC_FREE(td);
                    g_windowsTimerData.erase((size_t)timerId);
                }
            } else {
                auto a = td->m_timer->m_timeoutHandler.find(td->m_id);
                td->m_handler(td->m_data);
            }
        });
    g_windowsTimerData[(size_t)td->m_timerID] = (size_t)td;
    m_timeoutHandler.insert(std::make_pair(id, td));
    return id;
}

void TimerWindows::removeTimer(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());
    auto handlerData = m_timeoutHandler.find(reqID);
    if (handlerData != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        m_timeoutHandler.erase(handlerData);
        KillTimer(NULL, td->m_timerID);
        GC_FREE(td);
        g_windowsTimerData.erase((size_t)td->m_timerID);
    }
}

size_t TimerWindows::addAnimator(GlobalScope* globalScope,
                                 GenericAnimationHandler handler, void* data)
{
    STARFISH_ASSERT(isMainThread());
    int32_t id = ++m_animationCounter;
    AnimationTickData* ad = new (NoGC) AnimationTickData;
    ad->m_timer = this;
    ad->m_data = data;
    ad->m_globalScope = globalScope;
    ad->m_handler = handler;
    ad->m_timerID =
        SetTimer(NULL, (size_t)ad, USER_TIMER_MINIMUM + 1,
                 [](HWND, UINT, UINT_PTR timerId, DWORD) {
                     AnimationTickData* ad =
                         (AnimationTickData*)g_windowsTimerData[timerId];
                     auto a = ad->m_timer->m_animationHandler.find(ad->m_id);
                     if (ad->m_handler(ad->m_data)) {
                         return;
                     }
                     a = ad->m_timer->m_animationHandler.find(ad->m_id);
                     if (ad->m_timer->m_animationHandler.end() != a) {
                         ad->m_timer->m_animationHandler.erase(a);
                         KillTimer(NULL, timerId);
                         GC_FREE(ad);
                         g_windowsTimerData.erase((size_t)timerId);
                     }
                 });

    g_windowsTimerData[(size_t)ad->m_timerID] = (size_t)ad;
    m_animationHandler.insert(std::make_pair(id, ad));
    return id;
}

void TimerWindows::removeGenericAnimator(size_t reqID)
{
    STARFISH_ASSERT(isMainThread());

    auto handlerData = m_animationHandler.find(reqID);
    if (handlerData != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)handlerData->second;
        m_animationHandler.erase(handlerData);
        KillTimer(NULL, (size_t)ad->m_timerID);
        GC_FREE(ad);
    }
}

void TimerWindows::clear(GlobalScope* globalScope)
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        if (td->m_globalScope == nullptr || td->m_globalScope == globalScope ||
            globalScope == nullptr) {
            timerIter = m_timeoutHandler.erase(timerIter);
            KillTimer(NULL, (size_t)td->m_timerID);
            GC_FREE(td);
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
            aniIter2 = m_animationHandler.erase(aniIter2);
            KillTimer(NULL, (size_t)ad->m_timerID);
            GC_FREE(ad);
        } else {
            aniIter2++;
        }
    }
}

void TimerWindows::destroy()
{
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        KillTimer(NULL, (size_t)td->m_timerID);
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
        KillTimer(NULL, (size_t)ad->m_timerID);
        GC_FREE(ad);
        aniIter2++;
    }
    m_animationHandler.clear();
}

} // namespace Starfish
#endif
