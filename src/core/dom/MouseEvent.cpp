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
#include "core/dom/MouseEvent.h"

namespace StarFish {

MouseEventInit::MouseEventInit()
    : MouseEventInit(false)
{
}

MouseEventInit::MouseEventInit(bool bubbles)
    : MouseEventInit(bubbles, false)
{
}

MouseEventInit::MouseEventInit(bool bubbles, bool cancelable)
    : EventModifierInit(bubbles, cancelable)
    , m_screenX(0)
    , m_screenY(0)
    , m_clientX(0)
    , m_clientY(0)
    , m_button(0)
    , m_buttons(0)
    , m_relatedTarget(nullptr)
{
}

int32_t MouseEventInit::screenX() const
{
    return m_screenX;
}

void MouseEventInit::setScreenX(int32_t screenX)
{
    m_screenX = screenX;
}

int32_t MouseEventInit::screenY() const
{
    return m_screenY;
}

void MouseEventInit::setScreenY(int32_t screenY)
{
    m_screenY = screenY;
}

int32_t MouseEventInit::clientX() const
{
    return m_clientX;
}

void MouseEventInit::setClientX(int32_t clientX)
{
    m_clientX = clientX;
}

int32_t MouseEventInit::clientY() const
{
    return m_clientY;
}

void MouseEventInit::setClientY(int32_t clientY)
{
    m_clientY = clientY;
}

short MouseEventInit::button() const
{
    return m_button;
}

void MouseEventInit::setButton(short button)
{
    m_button = button;
}

unsigned short MouseEventInit::buttons() const
{
    return m_buttons;
}

void MouseEventInit::setButtons(unsigned short buttons)
{
    m_buttons = buttons;
}

EventTarget* MouseEventInit::relatedTarget() const
{
    return m_relatedTarget;
}

void MouseEventInit::setRelatedTarget(EventTarget* relatedTarget)
{
    m_relatedTarget = relatedTarget;
}
}
