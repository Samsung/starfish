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
