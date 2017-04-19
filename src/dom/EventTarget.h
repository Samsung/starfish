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
};

class EventTarget : public ScriptWrappable {
protected:
    EventTarget(Document* document)
        : ScriptWrappable(this)
        , m_document(document)
    {
    }

public:
    virtual bool isEventTarget() const override
    {
        return true;
    }

    GCVector<EventListener*>* getEventListeners(const String* eventType);

    bool addEventListener(const String* eventType, EventListener* listener,
                          bool useCapture = false);
    bool removeEventListener(const String* eventType, EventListener* listener,
                             bool useCapture = false);
    bool dispatchEvent(Event* event);
    virtual void handleDefaultEvent(Event* event)
    {
    }
    bool dispatchEvent(EventTarget* origin, Event* event);

    void setAttributeEventListener(const QualifiedName& eventTypeName,
                                   ScriptValue f)
    {
        auto eventType = eventTypeName.localName();
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
    }
    void setAttributeEventListener(const QualifiedName& eventTypeName,
                                   String* str, Element* target)
    {
        auto eventType = eventTypeName.localName();
        EventListener* l = new EventListener(str, target, true);
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

    ScriptValue attributeEventListener(const QualifiedName& name)
    {
        auto eventType = name.localName();
        EventListener* l = getAttributeEventListener(eventType);
        if (!l) {
            return ESValue(ESValue::ESNull);
        }
        return l->scriptValue();
    }

    void clearEventListeners()
    {
        m_eventListeners.clear();
    }

    Document* document()
    {
        return m_document;
    }

protected:
    GCUnorderedMap<String*, GCVector<EventListener*>*> m_eventListeners;
    Document* m_document;
};

#define DECLARE_EVENT_LISTENER(EVENT)       \
    ScriptValue on##EVENT##EventListener(); \
    void setOn##EVENT##EventListener(ScriptValue on##EVENT);

#define DEFINE_EVENT_LISTENER(EVENT_TARGET, EVENT)                           \
    ScriptValue EVENT_TARGET::on##EVENT##EventListener()                     \
    {                                                                        \
        Window* window = document()->window();                               \
        QualifiedName attr = window->starFish()->staticStrings()->m_##EVENT; \
                                                                             \
        return window->attributeEventListener(attr);                         \
    }                                                                        \
                                                                             \
    void EVENT_TARGET::setOnprogressEventListener(ScriptValue on##EVENT)     \
    {                                                                        \
        Window* window = document()->window();                               \
        QualifiedName attr = window->starFish()->staticStrings()->m_##EVENT; \
                                                                             \
        if (on##EVENT.isObject()) {                                          \
            setAttributeEventListener(attr, on##EVENT);                      \
        } else {                                                             \
            clearAttributeEventListener(attr);                               \
        }                                                                    \
    }
}

#endif
