/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
#include "KeyboardEvent.h"

namespace StarFish {

KeyboardEventInit::KeyboardEventInit()
    : KeyboardEventInit(false)
{
}

KeyboardEventInit::KeyboardEventInit(bool bubbles)
    : KeyboardEventInit(bubbles, false)
{
}

KeyboardEventInit::KeyboardEventInit(bool bubbles, bool cancelable)
    : EventModifierInit(bubbles, cancelable)
    , m_key(String::emptyString)
    , m_code(String::emptyString)
    , m_location(0)
    , m_repeat(false)
    , m_isComposing(false)
{
}

String* KeyboardEventInit::key() const
{
    return m_key;
}

void KeyboardEventInit::setKey(String* key)
{
    m_key = key;
}

String* KeyboardEventInit::code() const
{
    return m_code;
}

void KeyboardEventInit::setCode(String* code)
{
    m_code = code;
}

uint32_t KeyboardEventInit::location() const
{
    return m_location;
}

void KeyboardEventInit::setLocation(uint32_t location)
{
    m_location = location;
}

bool KeyboardEventInit::repeat() const
{
    return m_repeat;
}

void KeyboardEventInit::setRepeat(bool repeat)
{
    m_repeat = repeat;
}

bool KeyboardEventInit::isComposing() const
{
    return m_isComposing;
}

void KeyboardEventInit::setIsComposing(bool isComposing)
{
    m_isComposing = isComposing;
}

unsigned long KeyboardEvent::convertKeyCodeFromEcore(String* key)
{
    if (key->length() == 1) {
        const char* rawdata = key->utf8Data();
        if (rawdata[0] >= '0' && rawdata[0] <= '9') {
            return (unsigned)(rawdata[0] - '0') + (unsigned)KEYBOARD_KEYCODE_0;
        } else if (rawdata[0] >= 'a' && rawdata[0] <= 'z') {
            return (unsigned)(rawdata[0] - 'a') + (unsigned)KEYBOARD_KEYCODE_a;
        }
        // Note: No upper case -> expressed with shift key
    } else if (key->equals("Left")) {
        return KEYBOARD_KEYCODE_LEFT;
    } else if (key->equals("Up")) {
        return KEYBOARD_KEYCODE_UP;
    } else if (key->equals("Right")) {
        return KEYBOARD_KEYCODE_RIGHT;
    } else if (key->equals("Down")) {
        return KEYBOARD_KEYCODE_DOWN;
    } else if (key->equals("Return")) {
        return KEYBOARD_KEYCODE_RETURN;
    } else if (key->equals("BackSpace")) {
        return KEYBOARD_KEYCODE_BACKSPACE;
    } else if (key->equals("XF86Back")) {
        return KEYBOARD_KEYCODE_ESC;
    } else if (key->equals("space")) {
        return KEYBOARD_KEYCODE_SPACE;
    } else if (key->equals("Shift_L")) {
        return KEYBOARD_KEYCODE_SHIFT;
    } else if (key->equals("Control_L")) {
        return KEYBOARD_KEYCODE_CTRL_L;
    } else if (key->equals("Alt_L")) {
        return KEYBOARD_KEYCODE_ALT_L;
    }
    return KEYBOARD_KEYCODE_NONE;
}
}
