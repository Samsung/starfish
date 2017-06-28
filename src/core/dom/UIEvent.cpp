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
    , m_ctrlKey(false)
    , m_shiftKey(false)
    , m_altKey(false)
    , m_metaKey(false)
{
}

bool EventModifierInit::ctrlKey() const
{
    return m_ctrlKey;
}

void EventModifierInit::setCtrlKey(bool ctrlKey)
{
    m_ctrlKey = ctrlKey;
}

bool EventModifierInit::shiftKey() const
{
    return m_shiftKey;
}

void EventModifierInit::setShiftKey(bool shiftKey)
{
    m_shiftKey = shiftKey;
}

bool EventModifierInit::altKey() const
{
    return m_altKey;
}

void EventModifierInit::setAltKey(bool altKey)
{
    m_altKey = altKey;
}

bool EventModifierInit::metaKey() const
{
    return m_metaKey;
}

void EventModifierInit::setMetaKey(bool metaKey)
{
    m_metaKey = metaKey;
}
}
