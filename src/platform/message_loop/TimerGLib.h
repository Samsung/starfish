/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#if defined(PORT_EVENTLOOP_BACKEND_GLIB)

#ifndef __StarfishTimerGLib__
#define __StarfishTimerGLib__

#include "core/modules/message_loop/Timer.h"

namespace Starfish {

class TimerGLib : public Timer {
public:
    TimerGLib(WebBase* webBase);

    size_t addTimer(unsigned delay, GlobalScope* globalScope,
                    TimerHandler handler, void* data, bool repetitive) override;
    void removeTimer(size_t reqID) override;

    size_t addAnimator(GlobalScope* globalScope,
                       GenericAnimationHandler handler, void* data) override;
    void removeGenericAnimator(size_t reqID) override;

    void clear(GlobalScope* globalScope) override;

    void destroy() override;

    static void setAnimationFrameInterval(unsigned intervalMs);
    static unsigned animationFrameInterval();

private:
    static unsigned s_animationFrameInterval;
};

} // namespace Starfish

#endif

#endif