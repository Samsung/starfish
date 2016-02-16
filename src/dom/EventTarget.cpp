#include "StarFishConfig.h"
#include "EventTarget.h"

namespace StarFish {

EventListenerVector* EventTarget::getEventListeners(const QualifiedName& eventType)
{
    auto pair = m_eventListeners.find(eventType);
    if (pair == m_eventListeners.end()) return nullptr;
    return pair->second;
}

bool EventTarget::addEventListener(const QualifiedName& eventType, EventListener* listener, bool useCapture)
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

bool EventTarget::removeEventListener(const QualifiedName& eventType, EventListener* listener, bool useCapture)
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

bool EventTarget::setAttributeEventListener(const QualifiedName& eventType, EventListener* listener)
{
    STARFISH_ASSERT(listener->isAttribute());
    clearAttributeEventListener(eventType);
    return addEventListener(eventType, listener, false);
}

EventListener* EventTarget::getAttributeEventListener(const QualifiedName& eventType)
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

bool EventTarget::clearAttributeEventListener(const QualifiedName& eventType)
{
    auto listener = getAttributeEventListener(eventType);
    return removeEventListener(eventType, listener, false);
}

}
