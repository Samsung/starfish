/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "core/event/EventModifierData.h"
#include "core/event/KeyBoardEventData.h"

namespace StarFish {
class PlatformKeyEventData {
    friend class KeyboardEventInit;
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
