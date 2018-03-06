/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "core/dom/Event.h"
#include "core/dom/Document.h"
#include "core/modules/profiling/Profiling.h"

namespace StarFish {

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

Event::Event(Document* document)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
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

Event::Event(Document* document, String* eventType)
    : Event(document)
{
    m_type = eventType;
}

Event::Event(Document* document, String* eventType, const EventInit& init)
    : Event(document, eventType)
{
    m_bubbles = init.bubbles();
    m_cancelable = init.cancelable();
    m_composed = init.composed();
}

ScriptBindingInstance* Event::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}
}
