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

#ifndef __StarFishTransitionEvent__
#define __StarFishTransitionEvent__

#include "core/dom/Event.h"

namespace StarFish {

class Window;

// https://drafts.csswg.org/css-transitions/#interface-transitionevent
struct TransitionEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    TransitionEventInit(String* propertyName = String::emptyString,
                        float elapsedTime = 0.0,
                        String* pseudoElement = String::emptyString)
        : EventInit()
        , m_propertyName(propertyName)
        , m_elapsedTime(elapsedTime)
        , m_pseudoElement(pseudoElement)
    {
    }

    String* propertyName() const
    {
        return m_propertyName;
    }

    void setPropertyName(String* propertyName)
    {
        m_propertyName = propertyName;
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

    String* m_propertyName;
    float m_elapsedTime;
    String* m_pseudoElement;
};

class TransitionEvent : public Event {
public:
    TransitionEvent(Document* document)
        : Event(document)
        , m_propertyName(String::emptyString)
        , m_elapsedTime(0)
        , m_pseudoElement(String::emptyString)
    {
    }

    TransitionEvent(Document* document, String* eventType)
        : Event(document, eventType)
        , m_propertyName(String::emptyString)
        , m_elapsedTime(0)
        , m_pseudoElement(String::emptyString)
    {
    }

    TransitionEvent(Document* document, String* eventType,
                    const TransitionEventInit& init)
        : Event(document, eventType, init)
        , m_propertyName(init.m_propertyName)
        , m_elapsedTime(init.m_elapsedTime)
        , m_pseudoElement(init.m_pseudoElement)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTransitionEvent() const override;

    String* propertyName() const
    {
        return m_propertyName;
    }

    void setPropertyName(String* propertyName)
    {
        m_propertyName = propertyName;
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
    String* m_propertyName;
    float m_elapsedTime;
    String* m_pseudoElement;
};
}

#endif
