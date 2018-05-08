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

#include "core/event/EventModifierData.h"
#include "core/event/KeyBoardEventData.h"

namespace StarFish {
class PlatformKeyEventData {
    friend struct KeyboardEventInit;
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    PlatformKeyEventData()
        : PlatformKeyEventData(KeyValue::UnidentifiedKey)
    {
    }

    PlatformKeyEventData(KeyValue value)
        : m_eventModifierData()
        , m_keyboardEventData(value)
    {
    }

    bool ctrlKey() const
    {
        return m_eventModifierData.ctrlKey();
    }

    void setCtrlKey(bool ctrlKey)
    {
        m_eventModifierData.setCtrlKey(ctrlKey);
    }

    bool shiftKey() const
    {
        return m_eventModifierData.shiftKey();
    }
    void setShiftKey(bool shiftKey)
    {
        m_eventModifierData.setShiftKey(shiftKey);
    }

    bool altKey() const
    {
        return m_eventModifierData.altKey();
    }

    void setAltKey(bool altKey)
    {
        m_eventModifierData.setAltKey(altKey);
    }

    bool metaKey() const
    {
        return m_eventModifierData.metaKey();
    }

    void setMetaKey(bool metaKey)
    {
        m_eventModifierData.setMetaKey(metaKey);
    }

    KeyValue keyValue() const
    {
        return m_keyboardEventData.keyValue();
    }

    String* key() const
    {
        return m_keyboardEventData.key();
    }

    void setKey(String* key)
    {
        m_keyboardEventData.setKey(key);
    }

    String* code() const
    {
        return m_keyboardEventData.code();
    }

    void setCode(String* code)
    {
        m_keyboardEventData.setCode(code);
    }

    uint32_t location() const
    {
        return m_keyboardEventData.location();
    }

    void setLocation(uint32_t location)
    {
        m_keyboardEventData.setLocation(location);
    }

    bool repeat() const
    {
        return m_keyboardEventData.repeat();
    }

    void setRepeat(bool repeat)
    {
        m_keyboardEventData.setRepeat(repeat);
    }

    bool isComposing() const
    {
        return m_keyboardEventData.isComposing();
    }

    void setIsComposing(bool isComposing)
    {
        m_keyboardEventData.setIsComposing(isComposing);
    }

    uint32_t keyCode() const
    {
        return m_keyboardEventData.keyCode();
    }

    uint32_t charCode() const
    {
        return m_keyboardEventData.charCode();
    }

    void setCharCode(uint32_t charCode)
    {
        return m_keyboardEventData.setCharCode(charCode);
    }

    uint32_t virtualKeyCode() const
    {
        return m_keyboardEventData.virtualKeyCode();
    }

private:
    EventModifierData m_eventModifierData;
    KeyboardEventData m_keyboardEventData;
};
} // namespace StarFish
