/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishEvent__
#define __StarFishEvent__

#include "binding/ScriptWrappable.h"

namespace StarFish {

struct EventInit {
public:
    STARFISH_MAKE_STACK_ALLOCATED()
    EventInit();

    // Constructor for internal use
    EventInit(bool bubbles);
    EventInit(bool bubbles, bool cancelable);

    bool bubbles() const;
    void setBubbles(bool bubbles);

    bool cancelable() const;
    void setCancelable(bool cancelable);

    bool composed() const;
    void setComposed(bool composed);

private:
    bool m_bubbles;
    bool m_cancelable;
    bool m_composed;
};

class Event : public ScriptWrappable {
protected:
public:
    enum PhaseType {
        NONE = 0,
        CAPTURING_PHASE = 1,
        AT_TARGET = 2,
        BUBBLING_PHASE = 3
    };

    Event(Document* document);
    Event(Document* document, String* eventType);
    Event(Document* document, String* eventType, const EventInit& init);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;
    virtual bool isEvent() const override;

    bool isTypeInitialized() const
    {
        return m_type.hasValue();
    }
    void unintializeType()
    {
        m_type = Nullable<String*>();
    }
    String* type() const
    {
        if (isTypeInitialized()) {
            return m_type.getValue();
        }
        return String::emptyString;
    }
    EventTarget* target() const
    {
        return m_target;
    }
    void setTarget(EventTarget* target)
    {
        m_target = target;
    }

    EventTarget* currentTarget() const
    {
        return m_currentTarget;
    }
    void setCurrentTarget(EventTarget* currentTarget)
    {
        m_currentTarget = currentTarget;
    }

    unsigned short eventPhase() const
    {
        return m_eventPhase;
    }
    void setEventPhase(unsigned short phase)
    {
        m_eventPhase = phase;
    }

    bool stopPropagationValue()
    {
        return m_propagationStopped || m_immediatePropagationStopped;
    }

    void stopPropagation()
    {
        m_propagationStopped = true;
    }

    bool stopImmediatePropagationValue()
    {
        return m_immediatePropagationStopped;
    }

    void stopImmediatePropagation()
    {
        m_propagationStopped = true;
        m_immediatePropagationStopped = true;
    }

    bool bubbles() const
    {
        return m_bubbles;
    }

    void setBubbles(bool bubbles)
    {
        m_bubbles = bubbles;
    }

    bool cancelable() const
    {
        return m_cancelable;
    }

    void setCancelable(bool cancelable)
    {
        m_cancelable = cancelable;
    }

    bool composed() const
    {
        return m_composed;
    }

    void setComposed(bool composed)
    {
        m_composed = composed;
    }

    void preventDefault()
    {
        if (m_cancelable) {
            m_defaultPrevented = true; // canceled flag
        }
    }
    bool defaultPrevented() const
    {
        return m_defaultPrevented;
    }
    void setDefaultPrevented(bool defaultPrevented)
    {
        m_defaultPrevented = defaultPrevented;
    }

    DOMTimeStamp timeStamp() const
    {
        return m_timeStamp;
    }

    bool isInitialized() const
    {
        return m_isInitialized;
    }
    void setIsInitialized(bool isInitialized)
    {
        m_isInitialized = isInitialized;
    }
    bool isDispatched() const
    {
        return m_isDispatched;
    }
    void setIsDispatched(bool isDispatched)
    {
        m_isDispatched = isDispatched;
    }

    void initEvent(String* type, bool bubbles, bool cancelable)
    {
        m_type = type;
        m_bubbles = bubbles;
        m_cancelable = cancelable;
    }

private:
    bool m_isInitialized; // initialized flag

    ScriptBindingInstance* m_scriptBindingInstance;
    Nullable<String*> m_type;
    EventTarget* m_target;
    EventTarget* m_currentTarget;

    unsigned short m_eventPhase;

    bool m_propagationStopped;          // stop propagation flag
    bool m_immediatePropagationStopped; // stop immediate propagation flag

    bool m_bubbles;
    bool m_cancelable;
    bool m_defaultPrevented; // canceled flag
    bool m_composed;

    DOMTimeStamp m_timeStamp;

    bool m_isDispatched; // dispatch flag
};
}

#endif
