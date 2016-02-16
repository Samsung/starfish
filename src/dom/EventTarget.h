#ifndef __StarFishEventTarget__
#define __StarFishEventTarget__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Window;
class Node;

class EventListener : public gc {
public:
    EventListener(ScriptValue fn, bool isAttribute = false)
        : m_listener(fn)
        , m_isAttribute(isAttribute)
    {
    }
    bool isAttribute() const
    {
        return m_isAttribute;
    }
    bool compare(const EventListener* other) const
    {
        return (m_isAttribute == other->m_isAttribute) && (m_listener == other->m_listener);
    }
    ScriptValue scriptValue()
    {
        return m_listener;
    }

protected:
    ScriptValue m_listener;
    bool m_isAttribute;
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

    EventListenerVector* getEventListeners(const QualifiedName& eventType);

    bool addEventListener(const QualifiedName& eventType, EventListener* listener, bool useCapture = false);
    bool removeEventListener(const QualifiedName& eventType, EventListener* listener, bool useCapture = false);
    // bool dispatchEvent(PassRefPtrWillBeRawPtr<Event>);

    bool setAttributeEventListener(const QualifiedName& eventType, EventListener* listener);
    EventListener* getAttributeEventListener(const QualifiedName& eventType);
    bool clearAttributeEventListener(const QualifiedName& eventType);

protected:
    std::unordered_map<QualifiedName, EventListenerVector*, std::hash<QualifiedName>, std::equal_to<QualifiedName>,
        gc_allocator<std::pair<QualifiedName, EventListenerVector*>>> m_eventListeners;
};

}

#endif
