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
#include "core/dom/UIEvent.h"

namespace StarFish {

EventModifierInit::EventModifierInit()
    : UIEventInit()
    , m_eventModifierData()
{
}

EventModifierInit::EventModifierInit(EventModifierData& emdata)
    : UIEventInit()
    , m_eventModifierData(emdata)
{
}

bool EventModifierInit::ctrlKey() const
{
    return m_eventModifierData.ctrlKey();
}

void EventModifierInit::setCtrlKey(bool ctrlKey)
{
    m_eventModifierData.setCtrlKey(ctrlKey);
}

bool EventModifierInit::shiftKey() const
{
    return m_eventModifierData.shiftKey();
}

void EventModifierInit::setShiftKey(bool shiftKey)
{
    m_eventModifierData.setShiftKey(shiftKey);
}

bool EventModifierInit::altKey() const
{
    return m_eventModifierData.altKey();
}

void EventModifierInit::setAltKey(bool altKey)
{
    m_eventModifierData.setAltKey(altKey);
}

bool EventModifierInit::metaKey() const
{
    return m_eventModifierData.metaKey();
}

void EventModifierInit::setMetaKey(bool metaKey)
{
    m_eventModifierData.setMetaKey(metaKey);
}
}
