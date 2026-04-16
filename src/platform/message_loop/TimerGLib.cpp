/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#if defined(PORT_EVENTLOOP_BACKEND_GLIB)

#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/GlobalScope.h"
#include "core/page/WebBase.h"

#include "platform/message_loop/TimerGLib.h"

#include <glib.h>

namespace Starfish {

unsigned TimerGLib::s_animationFrameInterval = 16; // Default 60fps

TimerGLib::TimerGLib(WebBase* webBase)
    : Timer(webBase)
{
}

struct AnimationTickData : public gc {
    TimerGLib* m_timer;
    int32_t m_id;
    guint m_timerID;
    void* m_data;
    GenericAnimationHandler m_handler;
    GlobalScope* m_globalScope;
};

struct TimeoutData : public gc {
    TimerGLib* m_timer;
    int32_t m_id;
    guint m_timerID;
    GlobalScope* m_globalScope;
    void* m_data;
    TimerHandler m_handler;
};

size_t TimerGLib::addTimer(unsigned delay, GlobalScope* globalScope,
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
        td->m_timerID = g_timeout_add(
            delay,
            [](gpointer data) -> gboolean {
                TimeoutData* td = (TimeoutData*)data;
                if (td->m_handler && td->m_timer && td->m_id && td->m_timerID) {
                    td->m_handler(td->m_data);
                }
                return G_SOURCE_CONTINUE;
            },
            td);

    } else {
        td->m_timerID = g_timeout_add(
            delay,
            [](gpointer data) -> gboolean {
                TimeoutData* td = (TimeoutData*)data;
                TimerGLib* timer = td->m_timer;
                int32_t id = td->m_id;
                if (td->m_handler && td->m_timer && td->m_id && td->m_timerID) {
                    td->m_handler(td->m_data);
                    auto iter = timer->m_timeoutHandler.find(id);
                    if (iter != timer->m_timeoutHandler.end()) {
                        timer->m_timeoutHandler.erase(iter);
                        GC_FREE(td);
                    }
                }
                return G_SOURCE_REMOVE;
            },
            td);
    }

    m_timeoutHandler.insert(std::make_pair(id, td));
    return id;
}

void TimerGLib::removeTimer(size_t reqID)
{
    STARFISH_RELEASE_ASSERT(isMainThread());

    auto handlerData = m_timeoutHandler.find(reqID);
    if (handlerData != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        g_source_remove(td->m_timerID);
        td->m_timer = nullptr;
        GC_FREE(td);
        m_timeoutHandler.erase(handlerData);
    }
}

size_t TimerGLib::addAnimator(GlobalScope* globalScope,
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
    ad->m_timerID = g_timeout_add(
        s_animationFrameInterval,
        [](gpointer data) -> gboolean {
            AnimationTickData* ad = (AnimationTickData*)data;
            auto a = ad->m_timer->m_animationHandler.find(ad->m_id);
            if (ad->m_handler(ad->m_data)) {
                return G_SOURCE_CONTINUE;
            }
            a = ad->m_timer->m_animationHandler.find(ad->m_id);
            if (ad->m_timer->m_animationHandler.end() != a) {
                ad->m_timer->m_animationHandler.erase(a);
            }
            g_source_remove(ad->m_timerID);
            GC_FREE(ad);
            return G_SOURCE_REMOVE;
        },
        ad);
    m_animationHandler.insert(std::make_pair(id, ad));
    return id;
}

void TimerGLib::removeGenericAnimator(size_t reqID)
{
    STARFISH_RELEASE_ASSERT(isMainThread());

    auto handlerData = m_animationHandler.find(reqID);
    if (handlerData != m_animationHandler.end()) {
        AnimationTickData* ad = (AnimationTickData*)handlerData->second;
        g_source_remove(ad->m_timerID);
        GC_FREE(ad);
        m_animationHandler.erase(handlerData);
    }
}

void TimerGLib::clear(GlobalScope* globalScope)
{
    STARFISH_RELEASE_ASSERT(isMainThread());

    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        if ((td->m_globalScope && td->m_globalScope == globalScope) ||
            globalScope == nullptr) {
            g_source_remove(td->m_timerID);
            td->m_timer = nullptr;
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
            g_source_remove(ad->m_timerID);
            GC_FREE(ad);
            aniIter2 = m_animationHandler.erase(aniIter2);
        } else {
            aniIter2++;
        }
    }
}

void TimerGLib::destroy()
{
    STARFISH_RELEASE_ASSERT(isMainThread());

    STARFISH_LOG_INFO("TimerGLib::destroy");
    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        g_source_remove(td->m_timerID);
        td->m_timer = nullptr;
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
        g_source_remove(ad->m_timerID);
        GC_FREE(ad);
        aniIter2++;
    }
    m_animationHandler.clear();
}

void TimerGLib::setAnimationFrameInterval(unsigned intervalMs)
{
    s_animationFrameInterval = intervalMs;
}

unsigned TimerGLib::animationFrameInterval()
{
    return s_animationFrameInterval;
}

} // namespace Starfish
#endif
