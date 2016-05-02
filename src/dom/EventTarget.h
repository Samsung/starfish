#ifndef __StarFishEventTarget__
#define __StarFishEventTarget__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Event;
class Node;
class Window;

class EventListener : public gc {
public:
    EventListener(ScriptValue fn, bool isAttribute = false, bool useCapture = false)
        : m_listener(fn)
        , m_isAttribute(isAttribute)
        , m_capture(useCapture)
    {
    }
    bool isAttribute() const
    {
        return m_isAttribute;
    }
    bool compare(const EventListener* other) const
    {
        return (m_isAttribute == other->m_isAttribute) && (m_listener == other->m_listener) && (m_capture == other->m_capture);
    }
    bool capture() const
    {
        return m_capture;
    }
    void setCapture(bool capture)
    {
        m_capture = capture;
    }
    ScriptValue scriptValue()
    {
        return m_listener;
    }
    ScriptValue scriptFunction(EventTarget* target, const String* eventType);
    ScriptValue scriptFunction(EventTarget* target, const String* eventType, bool& error);
    ScriptValue call(Event* event);

protected:
    ScriptValue m_listener;
    bool m_isAttribute;
    bool m_capture;
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

    bool setAttributeEventListener(const String* eventType, EventListener* listener);
    EventListener* getAttributeEventListener(const String* eventType);
    bool clearAttributeEventListener(const String* eventType);

protected:
    std::unordered_map<String*, EventListenerVector*, std::hash<String*>, std::equal_to<String*>,
        gc_allocator<std::pair<String*, EventListenerVector*>>> m_eventListeners;
};

}

#endif
