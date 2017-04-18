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
#include "Event.h"
#include "platform/profiling/Profiling.h"

namespace StarFish {

EventInit::EventInit()
    : EventInit(false, false, false)
{
}

EventInit::EventInit(bool bubbles)
    : EventInit(bubbles, false, false)
{
}

EventInit::EventInit(bool bubbles, bool cancelable)
    : EventInit(bubbles, cancelable, false)
{
}

EventInit::EventInit(bool bubbles, bool cancelable, bool composed)
    : bubbles(bubbles)
    , cancelable(cancelable)
    , composed(composed)
{
}

Event::Event()
    : Event(String::emptyString)
{
}

Event::Event(String* eventType, const EventInit& init)
    : ScriptWrappable(this)
    , m_isInitialized(true)
    , m_type(eventType)
    , m_target(nullptr)
    , m_currentTarget(nullptr)
    , m_eventPhase(0)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_bubbles(init.bubbles)
    , m_cancelable(init.cancelable)
    , m_defaultPrevented(false)
    , m_isDispatched(false)
{
    m_timeStamp = timestamp();
}
}
