#include "StarFishConfig.h"
#include "EventTarget.h"
#include "Event.h"
#include "Document.h"
#include "Element.h"

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
    listener->setCapture(useCapture);
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
    listener->setCapture(useCapture);
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

bool EventTarget::dispatchEvent(Event* event)
{
    // TODO
    return false;
}

bool EventTarget::dispatchEvent(Node* origin, Event* event)
{
    ASSERT(origin);

    // 1. Let event be the event that is dispatched.
    // 2. Set event's dispatch flag.
    event->setIsDispatched(true);

    // 3. Initialize event's target attribute to target override, if it is given, and the object to which event is dispatched otherwise.
    event->setTarget(origin);

    // 4. If event's target attribute value is participating in a tree, let event path be a static ordered list of all its ancestors in tree order, and let event path be the empty list otherwise.
    std::vector<Node*, gc_allocator<Node*>> eventPath;
    Node* node = origin->parentNode();
    while (node) {
        if (node->isElement() && node->asElement()->isHTMLElement()) {
            eventPath.push_back(node);
        }
        node = node->parentNode();
    }

    // 5. Initialize event's eventPhase attribute to CAPTURING_PHASE.
    // 6. For each object in event path, invoke its event listeners with event event, as long as event's stop propagation flag is unset.
    if (!event->stopPropagation()) {
        event->setEventPhase(Event::CAPTURING_PHASE);
        for (size_t i = 0; i < eventPath.size(); i++) {
            Node* node = eventPath[i];
            EventListenerVector* listener = node->getEventListeners(event->type());
            if (listener && listener->at(0)->capture()) {
                STARFISH_ASSERT(listener->at(0)->scriptValue() != ScriptValueNull);
                // STARFISH_LOG_INFO("[CAPTURING_PHASE] node: %s\n", origin->localName()->utf8Data());
                callScriptFunction(listener->at(0)->scriptValue(), NULL, 0, node->scriptValue());
            }
        }
    }

    // 7. Initialize event's eventPhase attribute to AT_TARGET.
    event->setEventPhase(Event::AT_TARGET);

    // 8. Invoke the event listeners of event's target attribute value with event, if event's stop propagation flag is unset.
    EventListenerVector* listener = origin->getEventListeners(event->type());
    if (listener && !event->stopPropagation()) {
        STARFISH_ASSERT(listener->at(0)->scriptValue() != ScriptValueNull);
        // STARFISH_LOG_INFO("[AT_TARGET] node: %s\n", origin->localName()->utf8Data());
        callScriptFunction(listener->at(0)->scriptValue(), NULL, 0, origin->scriptValue());
    }

    // 9. If event's bubbles attribute value is true, run these substeps:
    // 1) Reverse the order of event path.
    // 2) Initialize event's eventPhase attribute to BUBBLING_PHASE.
    // 3) For each object in event path, invoke its event listeners, with event event as long as event's stop propagation flag is unset.
    if (event->bubbles() && !event->stopPropagation()) {
        event->setEventPhase(Event::BUBBLING_PHASE);
        for (size_t i = eventPath.size(); i > 0; i--) {
            Node* node = eventPath[i - 1];
            EventListenerVector* listener = node->getEventListeners(event->type());
            if (listener && !listener->at(0)->capture()) {
                STARFISH_ASSERT(listener->at(0)->scriptValue() != ScriptValueNull);
                // STARFISH_LOG_INFO("[BUBBLING_PHASE] node: %s\n", node->localName()->utf8Data());
                callScriptFunction(listener->at(0)->scriptValue(), NULL, 0, node->scriptValue());
            }
        }
    }

    // 10. Unset event's dispatch flag.
    event->setIsDispatched(false);

    // 11. Initialize event's eventPhase attribute to NONE.
    event->setEventPhase(Event::NONE);

    // 12. Initialize event's currentTarget attribute to null.
    event->setCurrentTarget(nullptr);

    // 13. Return false if event's canceled flag is set, and true otherwise.
    return event->cancelable() ? false : true;
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
