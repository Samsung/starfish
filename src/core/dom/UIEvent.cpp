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
