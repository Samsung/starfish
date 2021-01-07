/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAnimationEvent__
#define __StarfishAnimationEvent__

#include "core/dom/Event.h"

namespace Starfish {

class Window;

// https://drafts.csswg.org/css-animations/#interface-animationevent
struct AnimationEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    AnimationEventInit(String* animationName = String::emptyString,
                       float elapsedTime = 0.0,
                       String* pseudoElement = String::emptyString)
        : EventInit()
        , m_animationName(animationName)
        , m_elapsedTime(elapsedTime)
        , m_pseudoElement(pseudoElement)
    {
    }

    String* animationName() const
    {
        return m_animationName;
    }

    void setAnimationName(String* animationName)
    {
        m_animationName = animationName;
    }

    float elapsedTime() const
    {
        return m_elapsedTime;
    }

    void setElapsedTime(float elapsedTime)
    {
        m_elapsedTime = elapsedTime;
    }

    String* pseudoElement() const
    {
        return m_pseudoElement;
    }

    void setPseudoElement(String* pseudoElement)
    {
        m_pseudoElement = pseudoElement;
    }

    String* m_animationName;
    float m_elapsedTime;
    String* m_pseudoElement;
};

class AnimationEvent : public Event {
public:
    AnimationEvent(ExecutionContext* executionContext)
        : Event(executionContext)
        , m_animationName(String::emptyString)
        , m_elapsedTime(0)
        , m_pseudoElement(String::emptyString)
    {
    }

    AnimationEvent(ExecutionContext* executionContext, String* eventType)
        : Event(executionContext, eventType)
        , m_animationName(String::emptyString)
        , m_elapsedTime(0)
        , m_pseudoElement(String::emptyString)
    {
    }

    AnimationEvent(ExecutionContext* executionContext, String* eventType,
                   const AnimationEventInit& init)
        : Event(executionContext, eventType, init)
        , m_animationName(init.m_animationName)
        , m_elapsedTime(init.m_elapsedTime)
        , m_pseudoElement(init.m_pseudoElement)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isAnimationEvent() const override;

    String* animationName() const
    {
        return m_animationName;
    }

    void setAnimationName(String* animationName)
    {
        m_animationName = animationName;
    }

    float elapsedTime() const
    {
        return m_elapsedTime;
    }

    void setElapsedTime(float elapsedTime)
    {
        m_elapsedTime = elapsedTime;
    }

    String* pseudoElement() const
    {
        return m_pseudoElement;
    }

    void setPseudoElement(String* pseudoElement)
    {
        m_pseudoElement = pseudoElement;
    }

private:
    String* m_animationName;
    float m_elapsedTime;
    String* m_pseudoElement;
};
} // namespace Starfish

#endif
