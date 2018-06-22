/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishTimerWrapper__
#define __StarFishTimerWrapper__

namespace StarFish {

class BrowsingContext;

typedef bool (*GenericAnimationHandler)(void* data);
typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Timer : public gc {
    friend class StarFish;
    friend class Window;

public:
    Timer(StarFish* sf);
    size_t addTimer(unsigned delay, Window* window,
                    WindowSetTimeoutHandler handler, void* data,
                    bool repetitive);
    void removeTimer(size_t reqID);

    size_t addAnimator(Window* window, WindowSetTimeoutHandler handler,
                       void* data);
    size_t addAnimator(Window* window, GenericAnimationHandler handler,
                       void* data);
    void removeWindowAnimator(size_t reqID);
    void removeGenericAnimator(size_t reqID);

    void clear(BrowsingContext* ctx); // give nullptr to clear every tiemr

    void close();

    StarFish* m_starFish;

    int32_t m_timeoutCounter;
    GCUnorderedMap<int32_t, void*> m_timeoutHandler;

    int32_t m_requestAnimationFrameCounter;
    GCUnorderedMap<int32_t, void*> m_requestAnimationFrameHandler;

    int32_t m_AnimationCounter;
    GCUnorderedMap<int32_t, void*> m_animationHandler;
};
}

#endif
