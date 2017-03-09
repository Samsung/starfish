/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishAnimation__
#define __StarFishAnimation__

#include "StarFishConfig.h"
#include <sys/time.h>
#include <Elementary.h>

namespace StarFish {
class Frame;
class CSSStyleValuePair;

class AnimationTask : public gc {
public:
    AnimationTask(Frame* target, CSSStyleValuePair* animatedCss)
    {
        m_target_element = target;
        m_target_css = animatedCss;
    }
    virtual int progress() = 0;
    virtual void execute() = 0;

private:
    struct timeval m_startTime;
    struct timeval m_lastModifiedTime;
    size_t m_duration;
    size_t m_delay;
    Frame* m_target_element;
    CSSStyleValuePair* m_target_css;
};

class AnimationExecutor : public gc {
public:
    AnimationExecutor(Window* window)
        : m_isAlive(false), m_platformAnimator(nullptr)
    {
        m_window = window;
    }

    bool isAlive()
    {
        return m_isAlive;
    }

    Window* window()
    {
        return m_window;
    }

    void registerAnimation(AnimationTask* newtask);
    void cancelAnimation();
    void startIfNeeds();
    void stopIfNeeds();
    void step();

private:
    Window* m_window;
    bool m_isAlive;
    Ecore_Animator* m_platformAnimator;
    GCVector<AnimationTask*> m_animationList;
};
}
#endif
