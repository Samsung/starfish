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
    static Timer* create(WebBase* webBase);
#if defined(STARFISH_ENABLE_WORKER)
    static Timer* createForWorker(WebBase* webBase);
#endif

    virtual size_t addTimer(unsigned delay, GlobalScope* globalScope,
                            TimerHandler handler, void* data,
                            bool repetitive) = 0;
    virtual void removeTimer(size_t reqID) = 0;

    virtual size_t addAnimator(GlobalScope* globalScope,
                               GenericAnimationHandler handler, void* data) = 0;
    virtual void removeGenericAnimator(size_t reqID) = 0;

    virtual void clear(
        GlobalScope* globalScope) = 0; // give nullptr to clear every timer

    virtual void destroy() = 0;

    uint32_t requestAnimationFrame(GlobalScope* globalScope,
                                   TimerHandler handler, void* data);
    void cancelAnimationFrame(size_t reqID);

    // Timer initialization steps for setTimeout()/setInterval(): a timeout
    // scheduled from a timer task nested more than 5 deep waits at least
    // 4 ms, so a self-rescheduling setTimeout(0) chain still yields to
    // rendering and I/O. Clamps |timeout| and returns the nesting level of
    // the task it schedules.
    // https://html.spec.whatwg.org/multipage/timers-and-user-prompts.html#timer-initialisation-steps
    uint8_t initializeNestedTimeout(int32_t& timeout) const
    {
        if (m_timerNestingLevel > 5 && timeout < 4) {
            timeout = 4;
        }
        return nextTimerNestingLevel(m_timerNestingLevel);
    }
    // Only "more than 5" is observable, so the level saturates there.
    static uint8_t nextTimerNestingLevel(uint8_t level)
    {
        return level > 5 ? level : static_cast<uint8_t>(level + 1);
    }
    // The level of the script timer task now running; 0 outside one.
    uint8_t timerNestingLevel() const
    {
        return m_timerNestingLevel;
    }
    void setTimerNestingLevel(uint8_t level)
    {
        m_timerNestingLevel = level;
    }

protected:
    Timer(WebBase* webBase);

    struct RequestAnimationFrameData : public gc {
        Timer* m_timer;
        uint32_t m_id;
        void* m_data;
        TimerHandler m_handler;
        GlobalScope* m_globalScope;
    };

    WebBase* m_webBase;
    uint32_t m_timeoutCounter;
    GCAtomicUnorderedMap<uint32_t, void*> m_timeoutHandler;
    uint32_t m_requestAnimationFrameCounter;
    GCVector<std::pair<uint32_t, RequestAnimationFrameData*>>
        m_requestAnimationFrameHandler;
    size_t m_animationCounter;
    GCAtomicUnorderedMap<uint32_t, void*> m_animationHandler;
    uint8_t m_timerNestingLevel = 0;
};
} // namespace Starfish

#endif
