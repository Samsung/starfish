/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/dom/EventTarget.h"
#include "core/dom/ErrorEvent.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/csp/ContentSecurityPolicy.h"

#if !defined(STARFISH_WEBWORKER_HOST)
#include "core/dom/Document.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Element.h"
#endif

namespace Starfish {

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

enum ErrorArgumentSequence {
    ERROR_ARG_MESSAGE,
    ERROR_ARG_SRC,
    ERROR_ARG_LINENO,
    ERROR_ARG_COLNO,
    ERROR_ARG_ERROR,
    ERROR_ARG_SIZE
};

enum DefaultArgumentSequence { DEFAULT_ARG_EVENT, DEFAULT_ARG_SIZE };

ScriptValue EventListener::call(Event* event)
{
    ScriptValue listenerFunc = scriptValue();
    if (isCallableScriptValue(listenerFunc) ||
        isObjectScriptValue(listenerFunc)) {
        // Prepare arguments
        ScriptValue* argv;
        size_t argc = 0;
        if (event->isErrorEvent() && isAttribute()) {
            argc = ERROR_ARG_SIZE;
            argv = ALLOCA(sizeof(ScriptValue) * argc, ScriptValue);
            ErrorEvent* errorEvent = event->asErrorEvent();
            argv[ERROR_ARG_MESSAGE] = createScriptValue(errorEvent->message());
            argv[ERROR_ARG_SRC] = createScriptValue(errorEvent->filename());
            argv[ERROR_ARG_LINENO] = createScriptValue(errorEvent->lineno());
            argv[ERROR_ARG_COLNO] = createScriptValue(errorEvent->colno());
            argv[ERROR_ARG_ERROR] = errorEvent->error();
        } else {
            argc = DEFAULT_ARG_SIZE;
            argv = ALLOCA(sizeof(ScriptValue) * argc, ScriptValue);
            argv[DEFAULT_ARG_EVENT] = event->scriptValue();
        }

        // Call
        ScriptValue value;
        if (isCallableScriptValue(listenerFunc)) {
            value = callScriptFunction(event->scriptBindingInstance(),
                                       listenerFunc, argv, argc,
                                       event->currentTarget()->scriptValue());
        } else {
            if (!isAttribute()) {
                value = callHandleEventFunction(event->scriptBindingInstance(),
                                                listenerFunc, argv, argc,
                                                listenerFunc);
            }
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

EventTarget::EventTarget()
    : ScriptWrappable(this)
{
}

ScriptBindingInstance* EventTarget::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
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

#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
class WindowCurrentDispatchingEventChanger {
public:
    WindowCurrentDispatchingEventChanger(EventTarget* e, Event* nextEventValue)
        : m_eventTarget(e)
        , m_prevEventValue(nullptr)
    {
        STARFISH_ASSERT(e != nullptr);
        STARFISH_ASSERT(nextEventValue != nullptr);

        if (m_eventTarget->isWindow()) {
            m_prevEventValue = m_eventTarget->asWindow()->event();
            m_eventTarget->asWindow()->setEvent(nextEventValue);
        } else if (m_eventTarget->isNode()) {
            m_prevEventValue = m_eventTarget->asNode()->window()->event();
            m_eventTarget->asNode()->window()->setEvent(nextEventValue);
        }
    }

    ~WindowCurrentDispatchingEventChanger()
    {
        if (m_eventTarget->isWindow()) {
            m_eventTarget->asWindow()->setEvent(m_prevEventValue);
        } else if (m_eventTarget->isNode()) {
            m_eventTarget->asNode()->window()->setEvent(m_prevEventValue);
        }
    }

private:
    EventTarget* m_eventTarget;
    Event* m_prevEventValue;
};
#endif

bool EventTarget::dispatchEventByUA(EventTarget* origin, Event* event,
                                    bool onlyTarget)
{
    event->setIsTrusted(true);

#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
    WindowCurrentDispatchingEventChanger changer(this, event);
#endif
    return onlyTarget ? dispatchEventForTarget(origin, event)
                      : dispatchEvent(origin, event);
}

void EventTarget::dispatchEventIdleTimeByUA(Event* event)
{
    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t handle, void* data0, void* data1) {
            ((EventTarget*)data0)->dispatchEventByUA((Event*)data1);
        },
        this, event);
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
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR, nullptr);
    }

    // https://www.w3.org/TR/dom/#dispatching-events
    // 1. Let event be the event that is dispatched.
    // 2. Set event's dispatch flag.
    event->setIsDispatched(true);

    // 3. Initialize event's target attribute to target override, if it is
    // given, and the object to which event is dispatched otherwise.
    event->setTarget(origin);

    EventTarget* activationTarget = nullptr;
    GCVector<EventTarget*> eventPath;

#if !defined(STARFISH_WEBWORKER_HOST)
    // https://dom.spec.whatwg.org/#dom-eventtarget-dispatchevent
    // Let isActivationEvent be true, if event is a MouseEvent object and
    // event’s type attribute is "click", and false otherwise.
    bool isActivationEvent =
        event->isUIEvent() && event->type()->equals("click");

    // 4. If event's target attribute value is participating in a tree, let
    // event path be a static ordered list of all its ancestors in tree order,
    // and let event path be the empty list otherwise.
    EventTarget* eventTarget = origin;
    while (eventTarget) {
        if (isActivationEvent && !activationTarget &&
            eventTarget->hasActivationBehavior()) {
            activationTarget = eventTarget;
        }
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
#else
    eventPath.push_back(origin);
#endif /* !defined(STARFISH_WEBWORKER_HOST) */

    // 5. Initialize event's eventPhase attribute to CAPTURING_PHASE.
    // 1) Path : highest ancestor -> origin
    // 6. For each object in event path, invoke its event listeners with event
    // event, as long as event's stop propagation flag is unset.
    event->setEventPhase(Event::CAPTURING_PHASE);

    // If activationTarget is non-null and activationTarget has
    // legacy-pre-activation behavior, then run activationTarget’s
    // legacy-pre-activation behavior.
    if (activationTarget) {
        activationTarget->legacyPreActivationBehavior();
    }

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
                if (event->stopImmediatePropagationValue()) {
                    break;
                }
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
#if !defined(STARFISH_WEBWORKER_HOST)
    if (event->defaultPrevented()) {
        if (event->type()->equals("keydown")) {
            executionContext()
                ->document()
                ->browsingContext()
                ->setKeydownEventDefaultPrevented(true);
        } else if (event->type() ==
                   staticStrings()->m_compositionstart.localName()) {
            executionContext()
                ->document()
                ->browsingContext()
                ->setCompositionStartEventDefeaultPrevented(true);
        }
    }
#endif /* !defined(STARFISH_WEBWORKER_HOST) */
    // dispatch default event
    if (!event->defaultPrevented()) {
        for (size_t i = 0; i < eventPath.size(); i++) {
            if (eventPath[i]->handleDefaultEvent(event)) {
                break;
            }
        }
    }

    // 10. Unset event's dispatch flag, stop propagation flag,
    //     and stop immediate propagation flag.
    event->setIsDispatched(false);
    event->unsetPropagation();

    // 11. Initialize event's eventPhase attribute to NONE.
    event->setEventPhase(Event::NONE);

    // If activationTarget is non-null, then:
    // 1) If event’s canceled flag is unset, then run activationTarget’s
    //    activation behavior with event.
    // 2) Otherwise, if activationTarget has legacy-canceled-activation
    //    behavior, then run activationTarget’s legacy-canceled-activation
    //    behavior.
    if (activationTarget) {
        // NOTE canceled flag -> defaultPrevented
        if (!event->defaultPrevented()) {
            activationTarget->activationBehavior();
        } else {
            activationTarget->legacyCanceledActivationBehavior();
        }
    }

    // 12. Initialize event's currentTarget attribute to null.
    event->setCurrentTarget(nullptr);

    // 13. Return false if event's canceled flag is set, and true otherwise.
    //     Returns true if either event's cancelable attribute value is false or
    //     its preventDefault() method was not invoked, and false otherwise.
    return (event->cancelable() && event->defaultPrevented()) ? false : true;
}

bool EventTarget::dispatchEventForTarget(EventTarget* origin, Event* event)
{
    event->setTarget(origin);
    event->setEventPhase(Event::AT_TARGET);
    // Invoke event listeners
    GCVector<EventListener*>* originals =
        origin->getEventListeners(event->type());
    if (originals) {
        if (!event->stopPropagationValue()) {
            // Iterate Copied Vector : listeners can be removed during iteration
            GCVector<EventListener*> copies =
                GCVector<EventListener*>(*originals);
            for (auto listener : copies) {
                STARFISH_ASSERT(listener);
                if (event->stopImmediatePropagationValue()) {
                    break;
                }
                if (std::find(originals->begin(), originals->end(), listener) !=
                    originals->end()) {
                    event->setCurrentTarget(origin);
                    listener->call(event);
                }
            }
        }
    }
    event->setEventPhase(Event::NONE);
    return (event->cancelable() && event->defaultPrevented()) ? false : true;
}

void EventTarget::setAttributeEventListener(const QualifiedName& eventTypeName,
                                            String* str, EventTarget* target)
{
    if (!executionContext()
             ->contentSecurityPolicy()
             ->allowInlineEventHandler()) {
        return;
    }

    auto eventType = eventTypeName.localName();
    EventListener* l = EventListener::toEventListener(str, target, true);
    setAttributeEventListener(eventType, l);
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

StaticStrings* EventTarget::staticStrings()
{
    return executionContext()->webBase()->starfish()->staticStrings();
}
}
