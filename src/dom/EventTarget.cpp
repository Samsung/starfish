#include "StarFishConfig.h"
#include "EventTarget.h"
#include "Event.h"
#include "Document.h"
#include "Element.h"

namespace StarFish {

ScriptValue EventListener::scriptFunction(EventTarget* target, const String* eventType)
{
    bool error = false;
    return scriptFunction(target, eventType, error);
}

ScriptValue EventListener::scriptFunction(EventTarget* target, const String* eventType, bool& error)
{
    if (m_listener == ScriptValueNull) {
        STARFISH_ASSERT(isAttribute());
        STARFISH_ASSERT(target->isNode());
        STARFISH_ASSERT(target->asNode()->isElement());
        Element* element = target->asNode()->asElement();
        String* eventName = String::createASCIIString("on")->concat(const_cast<String*>(eventType));
        size_t idx = element->hasAttribute(QualifiedName::fromString(element->document()->window()->starFish(), eventName));
        STARFISH_ASSERT(idx != SIZE_MAX);

        String* bodyStr = element->getAttribute(idx);
        String* name[] = {String::createASCIIString("event")};
        m_listener = createScriptFunction(name, 1, bodyStr, error);
    }
    return m_listener;
}

ScriptValue EventListener::call(Event* event)
{
    bool hasError = false;
    ScriptValue listenerFunc = scriptFunction(event->target(), event->type(), hasError);
    if (!hasError) {
        ScriptValue argv[1] = { ScriptValue(event->scriptObject()) };
        callScriptFunction(listenerFunc, argv, 1, event->target()->scriptValue());
    }
    return listenerFunc;
}

EventListenerVector* EventTarget::getEventListeners(const String* eventType)
{
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end(); ++it) {
        if (it->first->equals(eventType)) {
            return it->second;
        }
    }
    return nullptr;
}

bool EventTarget::addEventListener(const String* eventType, EventListener* listener, bool useCapture)
{
    if (!listener)
        return false;

    listener->setCapture(useCapture);

    String* type = const_cast<String*>(eventType);
    EventListenerVector* v;
    bool hasEvent = false;
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end(); ++it) {
        if (it->first->equals(eventType)) {
            v = it->second;
            hasEvent = true;
            break;
        }
    }
    if (!hasEvent) {
        v = new (GC) EventListenerVector();
        m_eventListeners.insert(std::make_pair(type, v));
    }

    for (auto i = v->begin(); i != v->end(); i++) {
        if (listener->compare(*i)) {
            // STARFISH_LOG_INFO("EventTarget::addEventListener - Duplicated listener \"%s[%lu]\"\n", eventType.string()->utf8Data(), i - v->begin());
            return false;
        }
    }
    v->push_back(listener);
    // STARFISH_LOG_INFO("EventTarget::addEventListener - Added \"%s[%lu]\"\n", eventType.string()->utf8Data(), v->size() - 1);
    return true;
}

bool EventTarget::removeEventListener(const String* eventType, EventListener* listener, bool useCapture)
{
    if (!listener)
        return false;

    EventListenerVector* v;
    bool hasEvent = false;
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end(); ++it) {
        if (it->first->equals(eventType)) {
            listener->setCapture(useCapture);
            v = it->second;
            hasEvent = true;
            break;
        }
    }
    if (!hasEvent) {
        // STARFISH_LOG_INFO("EventTarget::removeEventListener - No such listener \"%s\"\n", eventType.string()->utf8Data());
        return false;
    }

    for (auto i = v->begin(); i != v->end(); i++) {
        if (listener->compare(*i)) {
            // STARFISH_LOG_INFO("EventTarget::removeEventListener - Removed \"%s[%lu]\"\n", eventType.string()->utf8Data(), i - v->begin());
            v->erase(i);
            return true;
        }
    }
    // STARFISH_LOG_INFO("EventTarget::removeEventListener - No such listener \"%s\"\n", eventType.string()->utf8Data());
    return false;
}

bool EventTarget::dispatchEvent(Event* event)
{
    return dispatchEvent(this, event);
}

bool EventTarget::dispatchEvent(EventTarget* origin, Event* event)
{
    ASSERT(origin);

    // https://www.w3.org/TR/dom/#dispatching-events
    // 1. Let event be the event that is dispatched.
    // 2. Set event's dispatch flag.
    event->setIsDispatched(true);

    // 3. Initialize event's target attribute to target override, if it is given, and the object to which event is dispatched otherwise.
    event->setTarget(origin);

    // 4. If event's target attribute value is participating in a tree, let event path be a static ordered list of all its ancestors in tree order, and let event path be the empty list otherwise.
    std::vector<EventTarget*, gc_allocator<EventTarget*> > eventPath;
    EventTarget* eventTarget = origin;
    while (eventTarget) {
        if (eventTarget->isNode()) {
            Node* node = eventTarget->asNode();
            if (node->isElement() && node->asElement()->isHTMLElement()) {
                eventPath.push_back(eventTarget);
            } else if (node->isDocument()) {
                eventPath.push_back(eventTarget);
                eventPath.push_back(eventTarget->asNode()->asDocument()->window());
                break;
            }
            eventTarget = node->parentNode();
        } else if (eventTarget->isWindow()) {
            eventPath.push_back(eventTarget);
            break;
        } else {
            break;
        }
    }

    // 5. Initialize event's eventPhase attribute to CAPTURING_PHASE.
    // 1) Path : highest ancestor -> origin
    // 6. For each object in event path, invoke its event listeners with event event, as long as event's stop propagation flag is unset.
    event->setEventPhase(Event::CAPTURING_PHASE);
    for (size_t i = eventPath.size(); i > 1; i--) {
        if (event->stopPropagation())
            break;
        EventTarget* eventTarget = eventPath[i - 1];
        EventListenerVector* originals = eventTarget->getEventListeners(event->type());
        if (originals) {
            // Iterate Copied Vector : listeners can be removed during iteration
            EventListenerVector copies = EventListenerVector(*originals);
            for (auto listener : copies) {
                STARFISH_ASSERT(listener);
                if (event->stopImmediatePropagation())
                    break;
                if (std::find(originals->begin(), originals->end(), listener) != originals->end() && listener->capture()) {
                    // STARFISH_LOG_INFO("[CAPTURING_PHASE] node: %s\n", node->localName()->utf8Data());
                    event->setCurrentTarget(eventTarget);
                    listener->call(event);
                }

            }
        }
    }

    // 7. Initialize event's eventPhase attribute to AT_TARGET.
    event->setEventPhase(Event::AT_TARGET);

    // 8. Invoke the event listeners of event's target attribute value with event, if event's stop propagation flag is unset.
    EventListenerVector* originals = origin->getEventListeners(event->type());
    if (originals) {
        if (!event->stopPropagation()) {
            // Iterate Copied Vector : listeners can be removed during iteration
            EventListenerVector copies = EventListenerVector(*originals);
            for (auto listener : copies) {
                STARFISH_ASSERT(listener);
                if (std::find(originals->begin(), originals->end(), listener) != originals->end()) {
                    // STARFISH_LOG_INFO("[AT_TARGET] node: %s\n", origin->localName()->utf8Data());
                    event->setCurrentTarget(origin);
                    listener->call(event);
                }
            }
        }
    }

    // 9. If event's bubbles attribute value is true, run these substeps:
    // 1) Path : origin -> highest ancestor
    // 2) Initialize event's eventPhase attribute to BUBBLING_PHASE.
    // 3) For each object in event path, invoke its event listeners, with event event as long as event's stop propagation flag is unset.
    if (event->bubbles()) {
        event->setEventPhase(Event::BUBBLING_PHASE);
        for (size_t i = 1; i < eventPath.size(); i++) {
            if (event->stopPropagation())
                break;
            EventTarget* eventTarget = eventPath[i];
            EventListenerVector* originals = eventTarget->getEventListeners(event->type());
            if (originals) {
                // Iterate Copied Vector : listeners can be removed during iteration
                EventListenerVector copies = EventListenerVector(*originals);
                for (auto listener : copies) {
                    STARFISH_ASSERT(listener);
                    if (event->stopImmediatePropagation())
                        break;
                    if (std::find(originals->begin(), originals->end(), listener) != originals->end() && !listener->capture()) {
                        // STARFISH_LOG_INFO("[BUBBLING_PHASE] node: %s\n", node->localName()->utf8Data());
                        event->setCurrentTarget(eventTarget);
                        listener->call(event);
                    }
                }
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
    //     Returns true if either event's cancelable attribute value is false or its preventDefault() method was not invoked, and false otherwise.
    return (event->cancelable() && event->defaultPrevented()) ? false : true;
}

bool EventTarget::setAttributeEventListener(const String* eventType, EventListener* listener)
{
    STARFISH_ASSERT(listener->isAttribute());
    clearAttributeEventListener(eventType);
    return addEventListener(eventType, listener, false);
}

EventListener* EventTarget::getAttributeEventListener(const String* eventType)
{
    EventListenerVector* v;
    bool hasEvent = false;
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end(); ++it) {
        if (it->first->equals(eventType)) {
            v = it->second;
            hasEvent = true;
            break;
        }
    }
    if (!hasEvent) {
        return nullptr;
    }

    for (auto i = v->begin(); i != v->end(); i++) {
        if ((*i)->isAttribute()) {
            return (*i);
        }
    }
    return nullptr;
}

bool EventTarget::clearAttributeEventListener(const String* eventType)
{
    auto listener = getAttributeEventListener(eventType);
    return removeEventListener(eventType, listener, false);
}

}
