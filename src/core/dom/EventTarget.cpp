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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/dom/EventTarget.h"
#include "core/dom/Event.h"
#include "core/page/Window.h"

namespace StarFish {

ScriptValue EventListener::scriptValue() const
{
    if (m_isNeedToParse) {
        bool error;
        m_listener = createAttributeStringEventFunction(
            m_scriptStringNeedToParse->m_target,
            m_scriptStringNeedToParse->m_scriptStringNeedToParse, error);
        if (error) {
            m_listener = scriptNull();
        }
        m_isNeedToParse = false;
    }
    return m_listener;
}

ScriptValue EventListener::call(Event* event)
{
    ScriptValue listenerFunc = scriptValue();
    if (isCallableScriptValue(listenerFunc) ||
        isObjectScriptValue(listenerFunc)) {
        ScriptValue argv[1] = { ScriptValue(event->scriptObject()) };
        ScriptValue value;
        if (isCallableScriptValue(listenerFunc)) {
            value = callScriptFunction(event->scriptBindingInstance(),
                                       listenerFunc, argv, 1,
                                       event->currentTarget()->scriptValue());
        } else {
            value = callHandleEventFunction(
                event->scriptBindingInstance(), listenerFunc, argv, 1,
                event->currentTarget()->scriptValue());
        }

        // NOTE: non-standard, but many browsers do this.
        // https://www.w3.org/TR/DOM-Level-3-Events/#event-flow
        if (isAttribute() && event->cancelable() &&
            scriptValueIsBoolean(value) && !scriptValueAsBoolean(value)) {
            event->preventDefault();
        }
    }
    return listenerFunc;
}

ScriptBindingInstance* EventTarget::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

GCVector<EventListener*>* EventTarget::getEventListeners(
    const String* eventType)
{
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end();
         ++it) {
        if (it->first->equals(eventType)) {
            return it->second;
        }
    }
    return nullptr;
}

bool EventTarget::addEventListener(const String* eventType,
                                   EventListener* listener, bool useCapture)
{
    if (!listener) {
        return false;
    }

    listener->setCapture(useCapture);

    String* type = const_cast<String*>(eventType);
    GCVector<EventListener*>* v = nullptr;
    bool hasEvent = false;
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end();
         ++it) {
        if (it->first->equals(eventType)) {
            v = it->second;
            hasEvent = true;
            break;
        }
    }
    if (!hasEvent) {
        v = new (GC) GCVector<EventListener*>();
        m_eventListeners.insert(m_eventListeners.end(),
                                std::make_pair(type, v));
    }

    for (auto i = v->begin(); i != v->end(); i++) {
        if (listener->compare(*i)) {
            // STARFISH_LOG_INFO("EventTarget::addEventListener - Duplicated
            // listener \"%s[%lu]\"\n",
            // eventType.string()->toUTF8NonGCString().data(), i -
            // v->begin());
            return false;
        }
    }
    v->push_back(listener);
    // STARFISH_LOG_INFO("EventTarget::addEventListener - Added \"%s[%lu]\"\n",
    // eventType.string()->toUTF8NonGCString().data(), v->size() - 1);
    return true;
}

bool EventTarget::removeEventListener(const String* eventType,
                                      EventListener* listener, bool useCapture)
{
    if (!listener) {
        return false;
    }

    GCVector<EventListener*>* v = nullptr;
    bool hasEvent = false;
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end();
         ++it) {
        if (it->first->equals(eventType)) {
            listener->setCapture(useCapture);
            v = it->second;
            hasEvent = true;
            break;
        }
    }
    if (!hasEvent) {
        // STARFISH_LOG_INFO("EventTarget::removeEventListener - No such
        // listener \"%s\"\n", eventType.string()->toUTF8NonGCString().data());
        return false;
    }

    for (auto i = v->begin(); i != v->end(); i++) {
        if (listener->compare(*i)) {
            // STARFISH_LOG_INFO("EventTarget::removeEventListener - Removed
            // \"%s[%lu]\"\n", eventType.string()->toUTF8NonGCString().data(), i
            // - v->begin());
            v->erase(i);
            return true;
        }
    }
    // STARFISH_LOG_INFO("EventTarget::removeEventListener - No such listener
    // \"%s\"\n", eventType.string()->toUTF8NonGCString().data());
    return false;
}

// Use this method if user agent dispatches an event
bool EventTarget::dispatchEventByUA(Event* event)
{
    return dispatchEventByUA(this, event);
}

bool EventTarget::dispatchEventByUA(EventTarget* origin, Event* event)
{
    event->setIsTrusted(true);
    return dispatchEvent(origin, event);
}

// This method should only be called by JS binding
bool EventTarget::dispatchEvent(Event* event)
{
    // https://www.w3.org/TR/dom/#dom-eventtarget-dispatchevent
    event->setIsTrusted(false);
    return dispatchEvent(this, event);
}

bool EventTarget::dispatchEvent(EventTarget* origin, Event* event)
{
    STARFISH_ASSERT(origin);

    // The dispatchEvent method throws DOM exception
    // if the event's type was not specified by initializing the event
    // before the method was called, or if the event's type is null or
    // an empty string.
    if (!event->isTypeInitialized()) {
        throw new DOMException(document(),
                               DOMException::Code::INVALID_STATE_ERR, nullptr);
    }

    // https://www.w3.org/TR/dom/#dispatching-events
    // 1. Let event be the event that is dispatched.
    // 2. Set event's dispatch flag.
    event->setIsDispatched(true);

    // 3. Initialize event's target attribute to target override, if it is
    // given, and the object to which event is dispatched otherwise.
    event->setTarget(origin);

    // 4. If event's target attribute value is participating in a tree, let
    // event path be a static ordered list of all its ancestors in tree order,
    // and let event path be the empty list otherwise.
    GCVector<EventTarget*> eventPath;
    EventTarget* eventTarget = origin;
    while (eventTarget) {
        if (eventTarget->isNode()) {
            Node* node = eventTarget->asNode();
            if (node->isHTMLElement()) {
                eventPath.push_back(eventTarget);
            } else if (node->isDocument()) {
                eventPath.push_back(eventTarget);
                eventPath.push_back(eventTarget->asDocument()->window());
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
    // 6. For each object in event path, invoke its event listeners with event
    // event, as long as event's stop propagation flag is unset.
    event->setEventPhase(Event::CAPTURING_PHASE);
    for (size_t i = eventPath.size(); i > 1; i--) {
        if (event->stopPropagationValue()) {
            break;
        }
        EventTarget* eventTarget = eventPath[i - 1];
        GCVector<EventListener*>* originals =
            eventTarget->getEventListeners(event->type());
        if (originals) {
            // Iterate Copied Vector : listeners can be removed during iteration
            GCVector<EventListener*> copies =
                GCVector<EventListener*>(*originals);
            for (auto listener : copies) {
                STARFISH_ASSERT(listener);
                if (event->stopImmediatePropagationValue()) {
                    break;
                }
                if (listener->capture() &&
                    std::find(originals->begin(), originals->end(), listener) !=
                        originals->end()) {
                    // STARFISH_LOG_INFO("[CAPTURING_PHASE] node: %s\n",
                    // node->localName()->toUTF8NonGCString().data());
                    event->setCurrentTarget(eventTarget);
                    listener->call(event);
                }
            }
        }
    }

    // 7. Initialize event's eventPhase attribute to AT_TARGET.
    event->setEventPhase(Event::AT_TARGET);

    // 8. Invoke the event listeners of event's target attribute value with
    // event, if event's stop propagation flag is unset.
    GCVector<EventListener*>* originals =
        origin->getEventListeners(event->type());
    if (originals) {
        if (!event->stopPropagationValue()) {
            // Iterate Copied Vector : listeners can be removed during iteration
            GCVector<EventListener*> copies =
                GCVector<EventListener*>(*originals);
            for (auto listener : copies) {
                STARFISH_ASSERT(listener);
                if (std::find(originals->begin(), originals->end(), listener) !=
                    originals->end()) {
                    // STARFISH_LOG_INFO("[AT_TARGET] node: %s\n",
                    // origin->localName()->toUTF8NonGCString().data());
                    event->setCurrentTarget(origin);
                    listener->call(event);
                }
            }
        }
    }

    // 9. If event's bubbles attribute value is true, run these substeps:
    // 1) Path : origin -> highest ancestor
    // 2) Initialize event's eventPhase attribute to BUBBLING_PHASE.
    // 3) For each object in event path, invoke its event listeners, with event
    // event as long as event's stop propagation flag is unset.
    if (event->bubbles()) {
        event->setEventPhase(Event::BUBBLING_PHASE);
        for (size_t i = 1; i < eventPath.size(); i++) {
            if (event->stopPropagationValue()) {
                break;
            }
            EventTarget* eventTarget = eventPath[i];
            GCVector<EventListener*>* originals =
                eventTarget->getEventListeners(event->type());
            if (originals) {
                // Iterate Copied Vector : listeners can be removed during
                // iteration
                GCVector<EventListener*> copies =
                    GCVector<EventListener*>(*originals);
                for (auto listener : copies) {
                    STARFISH_ASSERT(listener);
                    if (event->stopImmediatePropagationValue()) {
                        break;
                    }
                    if (!listener->capture() &&
                        std::find(originals->begin(), originals->end(),
                                  listener) != originals->end()) {
                        // STARFISH_LOG_INFO("[BUBBLING_PHASE] node: %s\n",
                        // node->localName()->toUTF8NonGCString().data());
                        event->setCurrentTarget(eventTarget);
                        listener->call(event);
                    }
                }
            }
        }
    }

    // dispatch default event
    if (!event->defaultPrevented()) {
        for (size_t i = 0; i < eventPath.size(); i++) {
            if (eventPath[i]->handleDefaultEvent(event)) {
                event->preventDefault();
                break;
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
    //     Returns true if either event's cancelable attribute value is false or
    //     its preventDefault() method was not invoked, and false otherwise.
    return (event->cancelable() && event->defaultPrevented()) ? false : true;
}

bool EventTarget::setAttributeEventListener(const String* eventType,
                                            EventListener* listener)
{
    STARFISH_ASSERT(listener->isAttribute());
    clearAttributeEventListener(eventType);
    return addEventListener(eventType, listener, false);
}

EventListener* EventTarget::getAttributeEventListener(const String* eventType)
{
    GCVector<EventListener*>* v;
    bool hasEvent = false;
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end();
         ++it) {
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
