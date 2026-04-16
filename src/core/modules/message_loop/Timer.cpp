/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"
#include "platform/message_loop/TimerEFL.h"
#include "platform/message_loop/TimerLibUV.h"
#include "platform/message_loop/TimerWindows.h"
#include "platform/message_loop/TimerGLib.h"
#include "core/page/WebBase.h"

namespace Starfish {

Timer* Timer::create(WebBase* webBase)
{
#if defined(PORT_EVENTLOOP_BACKEND_EFL)
    return new TimerEFL(webBase);
#elif defined(PORT_EVENTLOOP_BACKEND_LIBUV)
    return new TimerLibUV(webBase);
#elif defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    return new TimerWindows(webBase);
#elif defined(PORT_EVENTLOOP_BACKEND_GLIB)
    return new TimerGLib(webBase);
#else
#error "Unknown EventLoop back-end"
#endif
}

#if defined(STARFISH_ENABLE_WORKER)
Timer* Timer::createForWorker(WebBase* webBase)
{
    return new TimerLibUV(webBase);
}
#endif

Timer::Timer(WebBase* webBase)
    : m_webBase(webBase)
    , m_timeoutCounter(0)
    , m_requestAnimationFrameCounter(0)
    , m_animationCounter(0)
{
}

uint32_t Timer::requestAnimationFrame(GlobalScope* globalScope,
                                      TimerHandler handler, void* data)
{
    Timer::RequestAnimationFrameData* ad = new Timer::RequestAnimationFrameData;
    ad->m_timer = this;
    uint32_t id = ++m_requestAnimationFrameCounter;
    ad->m_id = id;
    ad->m_data = data;
    ad->m_handler = handler;
    ad->m_globalScope = globalScope;
    m_requestAnimationFrameHandler.push_back(std::make_pair(id, ad));

    m_webBase->setNeedsRendering();

    return id;
}
void Timer::cancelAnimationFrame(size_t reqID)
{
    for (size_t i = 0; i < m_requestAnimationFrameHandler.size(); i++) {
        if (m_requestAnimationFrameHandler[i].first == reqID) {
            m_requestAnimationFrameHandler.erase(i);
            return;
        }
    }
}
} // namespace Starfish
