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

template<typename T>
class EventTarget : public T {
protected:
    EventTarget()
    {
    }

    virtual ~EventTarget()
    {

    }

public:
    inline void* operator new(size_t size)
    {
        return GC_MALLOC(size);
    }

    inline void operator delete(void* obj)
    {
        GC_free(obj);
    }

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

    bool addEventListener(QualifiedName& eventType, EventListener* listener, bool useCapture = false);
    bool removeEventListener(QualifiedName& eventType, EventListener* listener, bool useCapture = false);
    // bool dispatchEvent(PassRefPtrWillBeRawPtr<Event>);

    bool setAttributeEventListener(QualifiedName& eventType, EventListener* listener);
    EventListener* getAttributeEventListener(QualifiedName& eventType);
    bool clearAttributeEventListener(QualifiedName& eventType);

protected:
    std::unordered_map<QualifiedName, EventListenerVector*, std::hash<QualifiedName>, std::equal_to<QualifiedName>,
        gc_allocator<std::pair<QualifiedName, EventListenerVector*>>> m_eventListeners;
};

template<typename T>
bool EventTarget<T>::addEventListener(QualifiedName& eventType, EventListener* listener, bool useCapture)
{
    if (!listener) return false;
    auto pair = m_eventListeners.find(eventType);
    EventListenerVector* v;
    if (pair == m_eventListeners.end()) {
        v = new(GC) EventListenerVector();
        m_eventListeners.insert(std::make_pair(eventType, v));
    } else {
        v = pair->second;
    }
    for (auto i = v->begin(); i != v->end(); i++) {
        if (listener->compare(*i)) {
            STARFISH_LOG_INFO("EventTarget::addEventListener - Duplicated listener \"%s[%lu]\"\n", eventType.string()->utf8Data(), i - v->begin());
            return false;
        }
    }
    v->push_back(listener);
    STARFISH_LOG_INFO("EventTarget::addEventListener - Added \"%s[%lu]\"\n", eventType.string()->utf8Data(), v->size() - 1);
    return true;
}

template<typename T>
bool EventTarget<T>::removeEventListener(QualifiedName& eventType, EventListener* listener, bool useCapture)
{
    if (!listener) return false;
    auto pair = m_eventListeners.find(eventType);
    if (pair == m_eventListeners.end()) {
        STARFISH_LOG_INFO("EventTarget::removeEventListener - No such listener \"%s\"\n", eventType.string()->utf8Data());
        return false;
    }
    EventListenerVector* v = pair->second;
    for (auto i = v->begin(); i != v->end(); i++) {
        if (listener->compare(*i)) {
            STARFISH_LOG_INFO("EventTarget::removeEventListener - Removed \"%s[%lu]\"\n", eventType.string()->utf8Data(), i - v->begin());
            v->erase(i);
            return true;
        }
    }
    STARFISH_LOG_INFO("EventTarget::removeEventListener - No such listener \"%s\"\n", eventType.string()->utf8Data());
    return false;
}

template<typename T>
bool EventTarget<T>::setAttributeEventListener(QualifiedName& eventType, EventListener* listener)
{
    STARFISH_ASSERT(listener->isAttribute());
    clearAttributeEventListener(eventType);
    return addEventListener(eventType, listener, false);
}

template<typename T>
EventListener* EventTarget<T>::getAttributeEventListener(QualifiedName& eventType)
{
    auto pair = m_eventListeners.find(eventType);
    if (pair == m_eventListeners.end()) {
        return nullptr;
    }
    EventListenerVector* v = pair->second;
    for (auto i = v->begin(); i != v->end(); i++) {
        if ((*i)->isAttribute()) {
            return (*i);
        }
    }
    return nullptr;
}

template<typename T>
bool EventTarget<T>::clearAttributeEventListener(QualifiedName& eventType)
{
    auto listener = getAttributeEventListener(eventType);
    return removeEventListener(eventType, listener, false);
}

}

#endif
