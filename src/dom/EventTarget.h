#ifndef __StarFishEventTarget__
#define __StarFishEventTarget__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Event;
class Node;
class Element;
class Window;

struct AttributeStringEventFunctionData : public gc {
public:
    AttributeStringEventFunctionData(Element* target, String* scriptStringNeedToParse)
        : m_target(target)
        , m_scriptStringNeedToParse(scriptStringNeedToParse)
    {
    }
    Element* m_target;
    String* m_scriptStringNeedToParse;
};

class EventListener : public gc {
public:
    EventListener(ScriptValue fn, bool isAttribute = false, bool useCapture = false)
        : m_isAttribute(isAttribute)
        , m_capture(useCapture)
        , m_isNeedToParse(false)
        , m_listener(fn)
    {
    }

    EventListener(String* scriptString, Element* target, bool isAttribute = false, bool useCapture = false)
        : m_isAttribute(isAttribute)
        , m_capture(useCapture)
        , m_isNeedToParse(true)
        , m_scriptStringNeedToParse(new AttributeStringEventFunctionData(target, scriptString))
    {
    }

    bool isAttribute() const
    {
        return m_isAttribute;
    }

    bool compare(const EventListener* other) const
    {
        return (m_isAttribute == other->m_isAttribute) && (scriptValue() == other->scriptValue()) && (m_capture == other->m_capture);
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

typedef std::vector<EventListener*, gc_allocator<EventListener*>> EventListenerVector;

class EventTarget : public ScriptWrappable {
protected:
    EventTarget()
        : ScriptWrappable(this)
    {
    }

    virtual ~EventTarget()
    {

    }

public:

    virtual bool isWindow()
    {
        return false;
    }

    Window* asWindow()
    {
        STARFISH_ASSERT(isWindow());
        return (Window*)this;
    }

    virtual bool isNode() const
    {
        return false;
    }

    Node* asNode()
    {
        STARFISH_ASSERT(isNode());
        return (Node*)this;
    }

    EventListenerVector* getEventListeners(const String* eventType);

    bool addEventListener(const String* eventType, EventListener* listener, bool useCapture = false);
    bool removeEventListener(const String* eventType, EventListener* listener, bool useCapture = false);
    virtual bool dispatchEvent(Event* event);
    bool dispatchEvent(EventTarget* origin, Event* event);

    void setAttributeEventListener(const QualifiedName& eventTypeName, ScriptValue f)
    {
        auto eventType = eventTypeName.localName();
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
    }
    void setAttributeEventListener(const QualifiedName& eventTypeName, String* str, Element* target)
    {
        auto eventType = eventTypeName.localName();
        EventListener* l = new EventListener(str, target, true);
        setAttributeEventListener(eventType, l);
    }
    bool setAttributeEventListener(const String* eventType, EventListener* listener);
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
        if (!l)
            return ScriptValueNull;
        return l->scriptValue();
    }

protected:
    std::unordered_map<String*, EventListenerVector*, std::hash<String*>, std::equal_to<String*>,
        gc_allocator<std::pair<String*, EventListenerVector*>>> m_eventListeners;
};

}

#endif
