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

#ifndef __StarfishTimerWrapper__
#define __StarfishTimerWrapper__

namespace Starfish {

class WebBase;
class GlobalScope;

typedef bool (*GenericAnimationHandler)(void* data);
typedef void (*TimerHandler)(void* data);

constexpr size_t TimerInvalidID{ SIZE_MAX };

class Timer : public gc {
    friend class Window;
    friend class WebView;

public:
    Timer(WebBase* webBase);

    size_t addTimer(unsigned delay, GlobalScope* globalScope,
                    TimerHandler handler, void* data, bool repetitive);
    void removeTimer(size_t reqID);

    size_t addAnimator(GlobalScope* globalScope,
                       GenericAnimationHandler handler, void* data);
    void removeGenericAnimator(size_t reqID);

    uint32_t requestAnimationFrame(GlobalScope* globalScope,
                                   TimerHandler handler, void* data);
    void cancelAnimationFrame(size_t reqID);

    void clear(GlobalScope* globalScope); // give nullptr to clear every timer
    void destroy();

protected:
    struct RequestAnimationFrameData : public gc {
        Timer* m_timer;
        uint32_t m_id;
        void* m_data;
        TimerHandler m_handler;
        GlobalScope* m_globalScope;
    };

    WebBase* m_webBase;
    uint32_t m_timeoutCounter;
    GCUnorderedMap<uint32_t, void*> m_timeoutHandler;
    uint32_t m_requestAnimationFrameCounter;
    GCVector<std::pair<uint32_t, RequestAnimationFrameData*>>
        m_requestAnimationFrameHandler;
    size_t m_animationCounter;
    GCUnorderedMap<uint32_t, void*> m_animationHandler;
};
} // namespace Starfish

#endif
