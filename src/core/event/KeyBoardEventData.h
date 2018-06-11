/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishKeyBoardEventData__
#define __StarFishKeyBoardEventData__

#include "PlatformIntegrationData.h"
using namespace LWE;

namespace StarFish {

String* keyValueToKey(KeyValue v);
String* keyValueToCode(KeyValue v);
uint32_t keyValueToKeyCode(KeyValue v, bool isForVirtualKeyCode = false);
uint32_t keyValueToCharCode(KeyValue v);

class KeyboardEventData {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    KeyboardEventData(KeyValue value = KeyValue::UnidentifiedKey)
        : m_keyValue(value)
        , m_key(keyValueToKey(value))
        , m_code(keyValueToCode(value))
        , m_location(0)
        , m_repeat(false)
        , m_isComposing(false)
        , m_keyCode(keyValueToKeyCode(value))
        , m_charCode(keyValueToCharCode(value))
        , m_virtualKeyCode(keyValueToKeyCode(value, true))
    {
    }

    KeyValue keyValue() const
    {
        return m_keyValue;
    }

    String* key() const
    {
        return m_key;
    }

    void setKey(String* key)
    {
        m_key = key;
    }

    String* code() const
    {
        return m_code;
    }

    void setCode(String* code)
    {
        m_code = code;
    }

    uint32_t location() const
    {
        return m_location;
    }

    void setLocation(uint32_t location)
    {
        m_location = location;
    }

    bool repeat() const
    {
        return m_repeat;
    }

    void setRepeat(bool repeat)
    {
        m_repeat = repeat;
    }

    bool isComposing() const
    {
        return m_isComposing;
    }

    void setIsComposing(bool isComposing)
    {
        m_isComposing = isComposing;
    }

    uint32_t keyCode() const
    {
        return m_keyCode;
    }

    void setKeyCode(uint32_t keyCode)
    {
        m_keyCode = keyCode;
    }

    uint32_t charCode() const
    {
        return m_charCode;
    }

    void setCharCode(uint32_t charCode)
    {
        m_charCode = charCode;
    }

    uint32_t virtualKeyCode() const
    {
        return m_virtualKeyCode;
    }

private:
    KeyValue m_keyValue;
    String* m_key;
    String* m_code;
    uint32_t m_location;
    bool m_repeat;
    bool m_isComposing;
    uint32_t m_keyCode;
    uint32_t m_charCode;
    uint32_t m_virtualKeyCode;
};
} // namespace StarFish
#endif
