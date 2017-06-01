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
    : Event(document, String::emptyString)
{
}

Event::Event(Document* document, String* eventType, const EventInit& init)
    : ScriptWrappable(this)
    , m_isInitialized(true)
    , m_scriptBindingInstance(document->scriptBindingInstance())
    , m_type(eventType)
    , m_target(nullptr)
    , m_currentTarget(nullptr)
    , m_eventPhase(0)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_bubbles(init.bubbles())
    , m_cancelable(init.cancelable())
    , m_defaultPrevented(false)
    , m_isDispatched(false)
{
    m_timeStamp = timestamp();
}

ScriptBindingInstance* Event::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}
}
