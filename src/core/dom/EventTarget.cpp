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
#include "core/dom/Document.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Element.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/HTMLSlotElement.h"

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
            isBooleanScriptValue(value) && !scriptValueAsBoolean(value)) {
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

Optional<GCVector<EventListener*>*> EventTarget::getEventListeners(
    const String* eventType)
{
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end();
         ++it) {
        // Fast path: event types are usually interned (StaticStrings) so a
        // pointer compare hits before the byte-wise equals().
        if (it->first == eventType || it->first->equals(eventType)) {
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
        if (it->first == eventType || it->first->equals(eventType)) {
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
            // listener \"%s[%lu]\"",
            // eventType.string()->toUTF8NonGCString().data(), i -
            // v->begin());
            return false;
        }
    }
    v->push_back(listener);
    // STARFISH_LOG_INFO("EventTarget::addEventListener - Added \"%s[%lu]\"",
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
        if (it->first == eventType || it->first->equals(eventType)) {
            listener->setCapture(useCapture);
            v = it->second;
            hasEvent = true;
            break;
        }
    }
    if (!hasEvent) {
        // STARFISH_LOG_INFO("EventTarget::removeEventListener - No such
        // listener \"%s\"", eventType.string()->toUTF8NonGCString().data());
        return false;
    }

    for (auto i = v->begin(); i != v->end(); i++) {
        if (listener->compare(*i)) {
            // STARFISH_LOG_INFO("EventTarget::removeEventListener - Removed
            // \"%s[%lu]\"", eventType.string()->toUTF8NonGCString().data(), i
            // - v->begin());
            (*i)->markRemoved();
            v->erase(i);
            return true;
        }
    }
    // STARFISH_LOG_INFO("EventTarget::removeEventListener - No such listener
    // \"%s\"", eventType.string()->toUTF8NonGCString().data());
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
    dispatchEventIdleTimeByUA(this, event);
}

void EventTarget::dispatchEventIdleTimeByUA(EventTarget* origin, Event* event,
                                            bool onlyTarget)
{
    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t handle, void* data0, void* data1, void* data2) {
            reinterpret_cast<EventTarget*>(data0)->dispatchEventByUA(
                reinterpret_cast<EventTarget*>(data0),
                reinterpret_cast<Event*>(data1), data2 ? true : false);
        },
        origin, event, onlyTarget ? reinterpret_cast<void*>(0x8) : nullptr);
}

// This method should only be called by JS binding
bool EventTarget::dispatchEvent(Event* event)
{
    // https://www.w3.org/TR/dom/#dom-eventtarget-dispatchevent
    event->setIsTrusted(false);
    return dispatchEvent(this, event);
}

bool EventTarget::hasListenerForTypeOnPath(const String* eventType)
{
    // Walk the ancestor chain dispatchEvent() builds for the event path
    // (self -> ancestors -> document -> window) and return true as soon as any
    // node on that path has a non-empty listener vector for eventType. No
    // vector copies, no listener invocations.
    //
    // This is purely an optimization that lets a caller skip a redundant
    // dispatch when nothing listens, so it MUST be conservative: only return
    // false when the whole path was resolved and genuinely has no listener.
    // In any ambiguous case (unexpected node type, a node detached from its
    // document, a shadow-tree boundary parentNode() does not cross, etc.) we
    // return true so the dispatch still happens. Returning false here when a
    // listener actually exists -- e.g. a drag handler bound on document/window
    // by a page that mounts its controls in shadow DOM, like the YouTube
    // embedded player's seek bar -- would silently drop the event.
    EventTarget* eventTarget = this;
    size_t guard = 0;
    while (eventTarget && guard++ < 8192) {
        auto listeners = eventTarget->getEventListeners(eventType);
        if (listeners && !listeners->empty()) {
            return true;
        }
        if (eventTarget->isWindow()) {
            // Reached the top of the path with nothing found.
            return false;
        }
        if (!eventTarget->isNode()) {
            // Unknown target type: cannot prove absence -> dispatch to be safe.
            return true;
        }
        Node* node = eventTarget->asNode();
        if (node->isDocument()) {
            EventTarget* window = node->asDocument()->window();
            auto windowListeners = window->getEventListeners(eventType);
            return windowListeners && !windowListeners->empty();
        }
        Node* parent = node->parentNode();
        if (!parent) {
            // Detached from the document, or a shadow-tree node whose host
            // chain we cannot follow here: be safe and allow the dispatch.
            return true;
        }
        eventTarget = parent;
    }
    // Ran out of nodes (or hit the guard) without conclusively reaching the
    // window: do not risk skipping the dispatch.
    return true;
}

#if defined(STARFISH_WEBWORKER_NOT_HOST)
// Whether `ancestor` is a shadow-including inclusive ancestor of `node`:
// an inclusive ancestor in the ordinary tree, or - recursing across shadow
// boundaries - an ancestor of the host of a shadow root that (ordinary-tree)
// contains `node`.
// https://dom.spec.whatwg.org/#concept-shadow-including-inclusive-ancestor
static bool isShadowIncludingInclusiveAncestor(Node* ancestor, Node* node)
{
    Node* current = node;
    while (current) {
        if (ancestor->contains(current)) {
            return true;
        }
        Node* root = current->getRootNode();
        if (!root->isShadowRoot()) {
            return false;
        }
        current = root->asShadowRoot()->host();
    }
    return false;
}

// https://dom.spec.whatwg.org/#retarget
static EventTarget* retarget(EventTarget* a, EventTarget* b)
{
    while (true) {
        if (!a || !a->isNode()) {
            return a;
        }
        Node* aRoot = a->asNode()->getRootNode();
        if (!aRoot->isShadowRoot()) {
            return a;
        }
        if (b && b->isNode() &&
            isShadowIncludingInclusiveAncestor(aRoot, b->asNode())) {
            return a;
        }
        a = aRoot->asShadowRoot()->host();
    }
}

static EventPathStruct makeEventPathStruct(
    EventTarget* target, EventTarget* invocationTarget,
    Optional<EventTarget*> originalRelatedTarget, bool rootOfClosedTree,
    bool slotInClosedTree)
{
    EventPathStruct s;
    s.invocationTarget = invocationTarget;
    s.shadowAdjustedTarget = retarget(target, invocationTarget);
    // Same per-struct independent retarget() pattern as shadowAdjustedTarget
    // above, applied to the event's original (dispatch-start) relatedTarget.
    s.relatedTarget = originalRelatedTarget
        ? Optional<EventTarget*>(
              retarget(originalRelatedTarget.value(), invocationTarget))
        : NullOption;
    s.rootOfClosedTree = rootOfClosedTree;
    s.slotInClosedTree = slotInClosedTree;
    return s;
}

// Flat-tree parent for event-path computation (WHATWG DOM "get the parent"):
// a slottable assigned to a slot has that slot as its parent so bubbling events
// traverse into the slot's tree; a shadow root's parent is its host only for
// composed events. Used only by the flat-tree event path in dispatchEvent.
static Node* eventFlatTreeParent(Node* node, Event* event)
{
    if (node->isSlotted()) {
        if (Optional<HTMLSlotElement*> slot = node->assignedSlotInternal()) {
            return slot.value();
        }
    }
    if (node->isShadowRoot()) {
        return event->composed() ? node->asShadowRoot()->host() : nullptr;
    }
    return node->parentNode();
}
#endif /* defined(STARFISH_WEBWORKER_NOT_HOST) */

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
    // Set below (host build only) when path shortening's empty-path case
    // applies -- relatedTarget fully absorbs origin AND origin is itself a
    // shadow host, so nothing at all should be invoked. This is NOT the same
    // as eventPath being empty for an unrelated reason (e.g. origin is a
    // plain EventTarget like XMLHttpRequest, neither Node nor Window, so the
    // build loop below never covers it) -- that case must still dispatch to
    // origin's own listeners exactly as before, so guard on this flag
    // specifically rather than on eventPath.empty().
    bool suppressDispatchEntirely = false;
    // A reference to event's own path member (not a local list): composedPath
    // reads this during and after dispatch, so the path must live on event,
    // not as a dispatchEvent-local. Cleared defensively in case this Event
    // object is being re-dispatched (the dispatch flag allows sequential
    // re-dispatch; the end of a prior dispatch already clears it via
    // clearEventPath(), see below).
    GCVector<EventPathStruct>& eventPath = event->eventPath();
    eventPath.clear();

#if defined(STARFISH_WEBWORKER_NOT_HOST)
    // https://dom.spec.whatwg.org/#dom-eventtarget-dispatchevent
    // Let isActivationEvent be true, if event is a MouseEvent object and
    // event’s type attribute is "click", and false otherwise.
    bool isActivationEvent =
        event->isUIEvent() && event->type()->equals("click");

    // Captured once, up front: every struct's relatedTarget is computed via
    // an independent retarget() call against THIS fixed original value (same
    // pattern as shadowAdjustedTarget's use of the fixed `origin` above) --
    // never against an already-retargeted intermediate value.
    Optional<EventTarget*> originalRelatedTarget =
        event->relatedTargetForDispatch();

    // Path shortening (WHATWG "if parent is relatedTarget, then set parent to
    // null", the "get the parent" main-loop counterpart of this build loop's
    // ancestor walk): retarget relatedTarget against origin once -- this is
    // "where relatedTarget appears from origin's own point of view" (itself,
    // if already visible there; otherwise the nearest shadow-tree host that
    // is). No reliable spec excerpt was available for the exact branch this
    // session, so this was reverse-engineered against every topology in
    // event-with-related-target.html + event-composed-path-with-related-
    // target.html:
    // - If that result IS origin itself: origin fully absorbs relatedTarget.
    //   If origin is itself a shadow host, nothing can be dispatched at all
    //   (target and relatedTarget are indistinguishable from every possible
    //   external point of view) -- empty path. Otherwise (plain node, no
    //   shadow tree involved anywhere) this is a no-op -- no shortening.
    // - Otherwise, if that result's own root is a shadow root: stop growing
    //   the path right after including a struct for that root (inclusive --
    //   e.g. relatedTarget buried inside an ancestor's shadow tree), whether
    //   or not retargeting actually had to hop through a host to reach it.
    // - Otherwise (a plain light-DOM ancestor, not itself shadow-rooted) --
    //   only when retargeting needed zero hops, i.e. relatedTarget was
    //   already this exact node with no shadow tree involved at all: stop
    //   right before reaching it (exclusive -- it would fully reveal
    //   relatedTarget with nothing left to retarget, so it and anything
    //   beyond are excluded). A plain light-DOM ancestor reached only via a
    //   hop (relatedTarget itself was hidden somewhere else entirely, e.g. a
    //   sibling shadow tree under a shared ancestor) does not shorten at all
    //   -- that ancestor is a normal, unrelated part of origin's own path.
    Optional<Node*> stopAfterNode = NullOption;
    Optional<Node*> stopBeforeNode = NullOption;
    if (originalRelatedTarget && originalRelatedTarget.value()->isNode()) {
        Node* relatedNode = originalRelatedTarget.value()->asNode();
        EventTarget* retargetedRelated = retarget(relatedNode, origin);
        if (retargetedRelated == origin) {
            if (origin->isNode()) {
                // origin fully absorbs relatedTarget. If origin itself lives
                // inside a shadow tree, that shadow root is still a real
                // boundary to stop after (inclusive) even though origin
                // wasn't hidden behind ANOTHER host to get here. Otherwise,
                // if origin merely hosts its own shadow tree (but itself
                // lives in ordinary light DOM), nothing can be dispatched at
                // all -- empty path (see the block comment above).
                Node* originRoot = origin->asNode()->getRootNode();
                if (originRoot->isShadowRoot()) {
                    stopAfterNode = originRoot;
                } else if (origin->asNode()->isElement() &&
                           origin->asNode()
                               ->asElement()
                               ->internalShadowRoot()) {
                    stopBeforeNode = origin->asNode();
                }
            }
        } else if (retargetedRelated->isNode()) {
            Node* relatedRoot = retargetedRelated->asNode()->getRootNode();
            if (relatedRoot->isShadowRoot()) {
                stopAfterNode = relatedRoot;
            } else if (retargetedRelated == relatedNode) {
                stopBeforeNode = retargetedRelated->asNode();
            }
        }
    }
    suppressDispatchEntirely = stopBeforeNode && origin->isNode() &&
        stopBeforeNode.value() == origin->asNode();

    // 4. If event's target attribute value is participating in a tree, let
    // event path be a static ordered list of all its ancestors in tree order,
    // and let event path be the empty list otherwise.
    EventTarget* eventTarget = origin;
    // Whether the struct about to be pushed is a slot assigned-to from the
    // previous (slotted) node, and that slot's root is a closed shadow root.
    // https://dom.spec.whatwg.org/#concept-event-dispatch step 4.9's
    // "append to an event path" slot-in-closed-tree bookkeeping.
    bool slotInClosedTree = false;
    while (eventTarget) {
        // Path-shortening exclusive stop point (see stopBeforeNode above):
        // this candidate itself (and everything beyond) is excluded.
        if (stopBeforeNode && eventTarget->isNode() &&
            eventTarget->asNode() == stopBeforeNode.value()) {
            break;
        }
        if (isActivationEvent && !activationTarget &&
            eventTarget->hasActivationBehavior()) {
            activationTarget = eventTarget;
        }
        bool thisSlotInClosedTree = slotInClosedTree;
        slotInClosedTree = false;
        if (eventTarget->isNode()) {
            Node* node = eventTarget->asNode();
            bool rootOfClosedTree =
                node->isShadowRoot() && node->asShadowRoot()->isClosed();
            if (node->isHTMLElement() || node->isSVGElement()) {
                eventPath.push_back(makeEventPathStruct(
                    origin, eventTarget, originalRelatedTarget,
                    rootOfClosedTree, thisSlotInClosedTree));
            } else if (node->isShadowRoot()) {
                // A shadow root is an ancestor in the (flat) tree, so it must
                // participate in the event path; otherwise bubbling events such
                // as slotchange never reach listeners (or the onslotchange
                // attribute handler) registered on the shadow root.
                eventPath.push_back(makeEventPathStruct(
                    origin, eventTarget, originalRelatedTarget,
                    rootOfClosedTree, thisSlotInClosedTree));
            } else if (node->isDocument()) {
                eventPath.push_back(makeEventPathStruct(
                    origin, eventTarget, originalRelatedTarget, false,
                    thisSlotInClosedTree));
                eventPath.push_back(makeEventPathStruct(
                    origin, eventTarget->asDocument()->window(),
                    originalRelatedTarget, false, false));
                break;
            }
            // Path-shortening stop point (see stopAfterNode above): the
            // struct for it was just pushed; don't walk any further.
            if (stopAfterNode && node == stopAfterNode.value()) {
                break;
            }
            // If we're about to cross a slot assignment boundary, remember
            // whether that slot lives in a closed shadow tree so the struct
            // pushed for the slot itself (next iteration) is marked.
            if (node->isSlotted()) {
                if (Optional<HTMLSlotElement*> slot =
                        node->assignedSlotInternal()) {
                    Node* slotRoot = slot.value()->getRootNode();
                    slotInClosedTree = slotRoot->isShadowRoot() &&
                        slotRoot->asShadowRoot()->isClosed();
                }
            }
            eventTarget = eventFlatTreeParent(node, event);
        } else if (eventTarget->isWindow()) {
            eventPath.push_back(makeEventPathStruct(
                origin, eventTarget, originalRelatedTarget, false, false));
            break;
        } else {
            break;
        }
    }
#else
    {
        EventPathStruct s;
        s.invocationTarget = origin;
        s.shadowAdjustedTarget = origin;
        s.relatedTarget = event->relatedTargetForDispatch();
        s.rootOfClosedTree = false;
        s.slotInClosedTree = false;
        eventPath.push_back(s);
    }
#endif /* defined(STARFISH_WEBWORKER_NOT_HOST) */

    // "Clear targets" (see the full explanation further down) must be
    // decided from the tree shape as it was AT THIS POINT -- right after
    // building the path, before any listener runs. A listener can mutate the
    // tree (move target in/out of a shadow tree) during dispatch; querying
    // getRootNode() live at the end would answer based on the POST-mutation
    // tree instead of the tree the path was actually built against. Snapshot
    // it now into plain booleans so later mutation can't change the answer.
    bool clearTargets = false;
    if (!eventPath.empty()) {
        const EventPathStruct& lastPathStruct = eventPath.back();
        auto rootIsShadowRootNow = [](Optional<EventTarget*> t) {
            return t && t.value()->isNode() &&
                t.value()->asNode()->getRootNode()->isShadowRoot();
        };
        clearTargets =
            rootIsShadowRootNow(lastPathStruct.shadowAdjustedTarget) ||
            rootIsShadowRootNow(lastPathStruct.relatedTarget);
    }

    // Path shortening's empty-path case (stopBeforeNode matching origin
    // itself, see above) means literally nothing is invoked -- not even
    // listeners on origin itself skip straight past capture/target/bubble.
    if (!suppressDispatchEntirely) {
        // 5. Initialize event's eventPhase attribute to CAPTURING_PHASE.
        // 1) Path : highest ancestor -> origin
        // 6. For each object in event path, invoke its event listeners with
        // event event, as long as event's stop propagation flag is unset.
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
            const EventPathStruct& pathStruct = eventPath[i - 1];
            EventTarget* eventTarget = pathStruct.invocationTarget;
            // https://dom.spec.whatwg.org/#concept-event-dispatch "invoke":
            // retarget event.target to this struct's shadow-adjusted target
            // before calling its listeners.
            event->setTarget(pathStruct.shadowAdjustedTarget.value());
            event->setRelatedTargetForDispatch(pathStruct.relatedTarget);
            auto originals = eventTarget->getEventListeners(event->type());
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
                    if (listener->capture() && !listener->isRemoved()) {
                        // STARFISH_LOG_INFO("[CAPTURING_PHASE] node: %s",
                        // node->localName()->toUTF8NonGCString().data());
                        event->setCurrentTarget(eventTarget);
                        listener->call(event);
                    }
                }
            }
        }

        // 7. Initialize event's eventPhase attribute to AT_TARGET.
        event->setEventPhase(Event::AT_TARGET);
        // retarget(origin, origin) is always origin (a target always sees
        // itself untargeted); restore it explicitly since the capture loop
        // above may have left event.target retargeted to an ancestor's
        // shadow-adjusted target. eventPath[0] is origin's own struct when
        // present (see the build loop above), so its relatedTarget is the
        // correspondingly-restored value; eventPath can still be empty here
        // for a reason unrelated to path shortening (origin is neither a
        // Node nor a Window, e.g. XMLHttpRequest, so the build loop never
        // covered it at all) -- in that case relatedTarget was never
        // retargeted to begin with, so leave it untouched.
        event->setTarget(origin);
        if (!eventPath.empty()) {
            event->setRelatedTargetForDispatch(eventPath[0].relatedTarget);
        }

        // 8. Invoke the event listeners of event's target attribute value with
        // event, if event's stop propagation flag is unset.
        auto originals = origin->getEventListeners(event->type());
        if (originals) {
            if (!event->stopPropagationValue()) {
                // Iterate Copied Vector : listeners can be removed during
                // iteration
                GCVector<EventListener*> copies =
                    GCVector<EventListener*>(*originals);
                for (auto listener : copies) {
                    STARFISH_ASSERT(listener);
                    if (event->stopImmediatePropagationValue()) {
                        break;
                    }
                    if (!listener->isRemoved()) {
                        // STARFISH_LOG_INFO("[AT_TARGET] node: %s",
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
        // 3) For each object in event path, invoke its event listeners, with
        // event event as long as event's stop propagation flag is unset.
        if (event->bubbles()) {
            event->setEventPhase(Event::BUBBLING_PHASE);
            for (size_t i = 1; i < eventPath.size(); i++) {
                if (event->stopPropagationValue()) {
                    break;
                }
                const EventPathStruct& pathStruct = eventPath[i];
                EventTarget* eventTarget = pathStruct.invocationTarget;
                // See the CAPTURING_PHASE loop above for the retargeting
                // rationale.
                event->setTarget(pathStruct.shadowAdjustedTarget.value());
                event->setRelatedTargetForDispatch(pathStruct.relatedTarget);
                auto originals = eventTarget->getEventListeners(event->type());
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
                        if (!listener->capture() && !listener->isRemoved()) {
                            // STARFISH_LOG_INFO("[BUBBLING_PHASE] node: %s",
                            // node->localName()->toUTF8NonGCString().data());
                            event->setCurrentTarget(eventTarget);
                            listener->call(event);
                        }
                    }
                }
            }
        }
    } // if (!eventPath.empty())
#if defined(STARFISH_WEBWORKER_NOT_HOST)
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
#endif /* defined(STARFISH_WEBWORKER_NOT_HOST) */
    // dispatch default event
    if (!event->defaultPrevented()) {
        for (size_t i = 0; i < eventPath.size(); i++) {
            if (eventPath[i].invocationTarget->handleDefaultEvent(event)) {
                break;
            }
        }
    }

    // https://dom.spec.whatwg.org/#dom-event-dispatchevent "clear targets":
    // BOTH event's target and relatedTarget are nulled together (not
    // independently per-field) when the pre-dispatch snapshot (clearTargets,
    // computed above before any listener could mutate the tree) says so, so
    // neither is observable post-dispatch.
    if (clearTargets) {
        event->setTarget(nullptr);
        event->setRelatedTargetForDispatch(NullOption);
    }
    // https://dom.spec.whatwg.org/#dom-event-dispatchevent "empty event's
    // path": composedPath() called after dispatch must return an empty list.
    event->clearEventPath();

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
    auto originals = getEventListeners(event->type());
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
                if (!listener->isRemoved()) {
                    event->setCurrentTarget(this);
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
} // namespace Starfish
