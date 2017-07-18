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
    size_t addTimer(double delay, Window* window,
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
