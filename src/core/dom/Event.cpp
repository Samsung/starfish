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
#include "core/dom/Event.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

EventInit::EventInit()
    : EventInit(false)
{
}

EventInit::EventInit(bool bubbles)
    : EventInit(bubbles, false)
{
}

EventInit::EventInit(bool bubbles, bool cancelable)
    : m_bubbles(bubbles)
    , m_cancelable(cancelable)
    , m_composed(false)
{
}

bool EventInit::bubbles() const
{
    return m_bubbles;
}

void EventInit::setBubbles(bool bubbles)
{
    m_bubbles = bubbles;
}

bool EventInit::cancelable() const
{
    return m_cancelable;
}

void EventInit::setCancelable(bool cancelable)
{
    m_cancelable = cancelable;
}

bool EventInit::composed() const
{
    return m_composed;
}

void EventInit::setComposed(bool composed)
{
    m_composed = composed;
}

Event::Event(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_target(nullptr)
    , m_currentTarget(nullptr)
    , m_eventPhase(0)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_bubbles(false)
    , m_cancelable(false)
    , m_defaultPrevented(false)
    , m_composed(false)
    , m_isTrusted(false)
    , m_timeStamp(timestamp())
    , m_isDispatched(false)
{
    uninitializeType();
}

Event::Event(ExecutionContext* executionContext, String* eventType)
    : Event(executionContext)
{
    m_type = eventType;
}

Event::Event(ExecutionContext* executionContext, String* eventType,
             const EventInit& init)
    : Event(executionContext, eventType)
{
    m_bubbles = init.bubbles();
    m_cancelable = init.cancelable();
    m_composed = init.composed();
}

// https://dom.spec.whatwg.org/#dom-event-composedpath
GCVector<EventTarget*> Event::composedPath()
{
    GCVector<EventTarget*> composedPath;
    if (m_eventPath.empty()) {
        return composedPath;
    }

    EventTarget* currentTarget = m_currentTarget;
    composedPath.push_back(currentTarget);

    size_t currentTargetIndex = 0;
    int currentTargetHiddenSubtreeLevel = 0;

    // Walk backward from the top of the path to locate currentTarget's index,
    // tallying closed-tree nesting depth (rootOfClosedTree deepens it,
    // slotInClosedTree shallows it) along the way.
    for (size_t i = m_eventPath.size(); i-- > 0;) {
        if (m_eventPath[i].rootOfClosedTree) {
            currentTargetHiddenSubtreeLevel++;
        }
        if (m_eventPath[i].invocationTarget == currentTarget) {
            currentTargetIndex = i;
            break;
        }
        if (m_eventPath[i].slotInClosedTree) {
            currentTargetHiddenSubtreeLevel--;
        }
    }

    int currentHiddenLevel = currentTargetHiddenSubtreeLevel;
    int maxHiddenLevel = currentTargetHiddenSubtreeLevel;

    // Walk backward from just below currentTarget toward the path's start,
    // prepending invocation targets that are visible (not hidden inside a
    // closed shadow tree deeper than currentTarget can see).
    for (size_t i = currentTargetIndex; i-- > 0;) {
        if (m_eventPath[i].rootOfClosedTree) {
            currentHiddenLevel++;
        }
        if (currentHiddenLevel <= maxHiddenLevel) {
            composedPath.insert((size_t)0, m_eventPath[i].invocationTarget);
        }
        if (m_eventPath[i].slotInClosedTree) {
            currentHiddenLevel--;
            if (currentHiddenLevel < maxHiddenLevel) {
                maxHiddenLevel = currentHiddenLevel;
            }
        }
    }

    currentHiddenLevel = currentTargetHiddenSubtreeLevel;
    maxHiddenLevel = currentTargetHiddenSubtreeLevel;

    // Walk forward from just above currentTarget toward the path's end,
    // appending invocation targets that are visible.
    for (size_t i = currentTargetIndex + 1; i < m_eventPath.size(); i++) {
        if (m_eventPath[i].slotInClosedTree) {
            currentHiddenLevel++;
        }
        if (currentHiddenLevel <= maxHiddenLevel) {
            composedPath.push_back(m_eventPath[i].invocationTarget);
        }
        if (m_eventPath[i].rootOfClosedTree) {
            currentHiddenLevel--;
            if (currentHiddenLevel < maxHiddenLevel) {
                maxHiddenLevel = currentHiddenLevel;
            }
        }
    }

    return composedPath;
}

ScriptBindingInstance* Event::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}
} // namespace Starfish
