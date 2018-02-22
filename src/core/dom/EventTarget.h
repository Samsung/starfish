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

#ifndef __StarFishEventTarget__
#define __StarFishEventTarget__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace StarFish {

class Event;
class Node;
class Element;
class Window;

struct AttributeStringEventFunctionData : public gc {
public:
    AttributeStringEventFunctionData(Element* target,
                                     String* scriptStringNeedToParse)
        : m_target(target)
        , m_scriptStringNeedToParse(scriptStringNeedToParse)
    {
    }
    Element* m_target;
    String* m_scriptStringNeedToParse;
};

class EventListener : public gc {
public:
    static EventListener* toEventListener(ScriptValue fn,
                                          bool isAttribute = false,
                                          bool useCapture = false)
    {
        if (isAttribute && !isObjectScriptValue(fn)) {
            return nullptr;
        }
        return new EventListener(fn, isAttribute, useCapture);
    }

    static EventListener* toEventListener(String* scriptString, Element* target,
                                          bool isAttribute = false,
                                          bool useCapture = false)
    {
        return new EventListener(scriptString, target, isAttribute, useCapture);
    }

    bool isAttribute() const
    {
        return m_isAttribute;
    }

    bool compare(const EventListener* other) const
    {
        return (m_isAttribute == other->m_isAttribute) &&
               (scriptValue() == other->scriptValue()) &&
               (m_capture == other->m_capture);
    }

    bool capture() const
    {
        return m_capture;
    }

    void setCapture(bool capture)
    {
        m_capture = capture;
    }

    bool needParse()
    {
        return m_isNeedToParse;
    }

    ScriptValue scriptValue() const;
    ScriptValue call(Event* event);

protected:
    bool m_isAttribute : 1;
    bool m_capture : 1;
    mutable bool m_isNeedToParse : 1;

    union {
        mutable AttributeStringEventFunctionData* m_scriptStringNeedToParse;
        mutable ScriptValue m_listener;
    };

private:
    EventListener(ScriptValue fn, bool isAttribute = false,
                  bool useCapture = false)
        : m_isAttribute(isAttribute)
        , m_capture(useCapture)
        , m_isNeedToParse(false)
        , m_listener(fn)
    {
    }

    EventListener(String* scriptString, Element* target,
                  bool isAttribute = false, bool useCapture = false)
        : m_isAttribute(isAttribute)
        , m_capture(useCapture)
        , m_isNeedToParse(true)
        , m_scriptStringNeedToParse(
              new AttributeStringEventFunctionData(target, scriptString))
    {
    }
};

class EventTarget : public ScriptWrappable, public DocumentHoldable {
protected:
    EventTarget(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
    {
    }

public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;
    virtual bool isEventTarget() const override;

    GCVector<EventListener*>* getEventListeners(const String* eventType);

    bool addEventListener(const String* eventType, EventListener* listener,
                          bool useCapture = false);
    bool removeEventListener(const String* eventType, EventListener* listener,
                             bool useCapture = false);

    bool dispatchEvent(Event* event);
    bool dispatchEventByUA(Event* event);
    bool dispatchEventByUA(EventTarget* origin, Event* event,
                           bool onlyTarget = false);
    void dispatchEventIdleTimeByUA(Event* event);

    virtual bool handleDefaultEvent(Event* event)
    {
        return false;
    }

    // https://dom.spec.whatwg.org/#eventtarget-activation-behavior
    virtual void activationBehavior()
    {
    }

    virtual bool hasActivationBehavior()
    {
        return false;
    }

    virtual void legacyPreActivationBehavior()
    {
    }

    virtual void legacyCanceledActivationBehavior()
    {
    }

    void setAttributeEventListener(const QualifiedName& eventTypeName,
                                   EventListener* l)
    {
        String* eventType = eventTypeName.localName();
        setAttributeEventListener(eventType, l);
    }
    void setAttributeEventListener(const QualifiedName& eventTypeName,
                                   String* str, Element* target)
    {
        auto eventType = eventTypeName.localName();
        EventListener* l = EventListener::toEventListener(str, target, true);
        setAttributeEventListener(eventType, l);
    }
    bool setAttributeEventListener(const String* eventType,
                                   EventListener* listener);
    EventListener* getAttributeEventListener(const QualifiedName& eventType)
    {
        return getAttributeEventListener(eventType.localName());
    }
    EventListener* getAttributeEventListener(const String* eventType);

    void clearAttributeEventListener(const QualifiedName& name)
    {
        clearAttributeEventListener(name.localName());
    }
    bool clearAttributeEventListener(const String* eventType);

    EventListener* attributeEventListener(const QualifiedName& name)
    {
        String* eventType = name.localName();
        return getAttributeEventListener(eventType);
    }

    void clearEventListeners()
    {
        m_eventListeners.clear();
    }

    enum GlobalPointingEventKind {
        GlobalPointingEventKindDown,
        GlobalPointingEventKindUp,
        GlobalPointingEventKindMove
    };
    virtual void onGlobalPointingEvent(float x, float y,
                                       GlobalPointingEventKind kind)
    {
    }

protected:
    GCVector<std::pair<String*, GCVector<EventListener*>*>> m_eventListeners;

private:
    bool dispatchEvent(EventTarget* origin, Event* event);
    bool dispatchEventForTarget(EventTarget* origin, Event* event);
};

#define DECLARE_EVENT_LISTENER(EVENT)            \
    VIRTUAL EventListener* on##EVENT() OVERRIDE; \
    VIRTUAL void setOn##EVENT(EventListener* on##EVENT) OVERRIDE;

#define GENERATE_ATTR(EVENT)                \
    Window* window = EventTarget::window(); \
    QualifiedName attr = window->starFish()->staticStrings()->m_##EVENT;

#define DEFINE_GLOBAL_EVENT_LISTENER(EVENT_TARGET, EVENT)       \
    EventListener* EVENT_TARGET::on##EVENT()                    \
    {                                                           \
        GENERATE_ATTR(EVENT);                                   \
                                                                \
        return window->attributeEventListener(attr);            \
    }                                                           \
                                                                \
    void EVENT_TARGET::setOn##EVENT(EventListener* on##EVENT)   \
    {                                                           \
        GENERATE_ATTR(EVENT);                                   \
                                                                \
        if (on##EVENT) {                                        \
            window->setAttributeEventListener(attr, on##EVENT); \
        } else {                                                \
            window->clearAttributeEventListener(attr);          \
        }                                                       \
    }

#define DEFINE_EVENT_LISTENER(EVENT_TARGET, EVENT)            \
    EventListener* EVENT_TARGET::on##EVENT()                  \
    {                                                         \
        GENERATE_ATTR(EVENT);                                 \
                                                              \
        return attributeEventListener(attr);                  \
    }                                                         \
                                                              \
    void EVENT_TARGET::setOn##EVENT(EventListener* on##EVENT) \
    {                                                         \
        GENERATE_ATTR(EVENT);                                 \
                                                              \
        if (on##EVENT) {                                      \
            setAttributeEventListener(attr, on##EVENT);       \
        } else {                                              \
            clearAttributeEventListener(attr);                \
        }                                                     \
    }
}

#endif
