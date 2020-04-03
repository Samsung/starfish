/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishKeyboardEvent__
#define __StarfishKeyboardEvent__

#include "UIEvent.h"
#include "PlatformIntegrationData.h"

using namespace LWE;

namespace Starfish {
class PlatformKeyEventData;

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
        , m_which(keyValueToKeyCode(value))
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

    uint32_t which() const
    {
        return m_which;
    }

    void setWhich(uint32_t which)
    {
        m_which = which;
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
    uint32_t m_which;
    uint32_t m_virtualKeyCode;
};

struct KeyboardEventInit : public EventModifierInit {
    STARFISH_MAKE_STACK_ALLOCATED()
    friend class KeyboardEvent;

public:
    KeyboardEventInit()
        : EventModifierInit()
        , m_keyboardEventData()
    {
    }

    KeyboardEventInit(KeyboardEventData& kdata)
        : EventModifierInit()
        , m_keyboardEventData(kdata)
    {
    }

    KeyboardEventInit(PlatformKeyEventData& kdata);

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

    void setKeyCode(uint32_t keyCode)
    {
        m_keyboardEventData.setKeyCode(keyCode);
    }

    uint32_t charCode() const
    {
        return m_keyboardEventData.charCode();
    }

    void setCharCode(uint32_t charCode)
    {
        m_keyboardEventData.setCharCode(charCode);
    }

    uint32_t which() const
    {
        return m_keyboardEventData.which();
    }

    void setWhich(uint32_t which)
    {
        m_keyboardEventData.setWhich(which);
    }

    // Belows are not in IDL.
    uint32_t virtualKeyCode() const
    {
        return m_keyboardEventData.virtualKeyCode();
    }

    KeyValue keyValue() const
    {
        return m_keyboardEventData.keyValue();
    }

private:
    KeyboardEventData m_keyboardEventData;
};

class KeyboardEvent : public UIEvent {
public:
    KeyboardEvent(ExecutionContext* executionContext)
        : UIEvent(executionContext)
    {
    }

    KeyboardEvent(ExecutionContext* executionContext, String* eventType)
        : UIEvent(executionContext, eventType)
    {
    }

    KeyboardEvent(ExecutionContext* executionContext, String* eventType,
                  KeyboardEventInit& init)
        : UIEvent(executionContext, eventType, init)
        , m_eventModifierData(init.m_eventModifierData)
        , m_keyboardEventData(init.m_keyboardEventData)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isKeyboardEvent() const override;

    String* key() const
    {
        return m_keyboardEventData.key();
    }

    String* code() const
    {
        return m_keyboardEventData.code();
    }

    bool ctrlKey() const
    {
        return m_eventModifierData.ctrlKey();
    }

    bool shiftKey() const
    {
        return m_eventModifierData.shiftKey();
    }

    bool altKey() const
    {
        return m_eventModifierData.altKey();
    }

    bool metaKey() const
    {
        return m_eventModifierData.metaKey();
    }

    bool repeat() const
    {
        return m_keyboardEventData.repeat();
    }

    uint32_t keyCode() const
    {
        if (m_keyboardEventData.keyValue() != KeyValue::UnidentifiedKey &&
            type()->equals(String::createASCIIString("keydown"))) {
            return m_keyboardEventData.virtualKeyCode();
        }
        return m_keyboardEventData.keyCode();
    }

    uint32_t charCode() const
    {
        return m_keyboardEventData.charCode();
    }

    uint32_t which() const
    {
        return m_keyboardEventData.which();
    }

    // Not in IDL.
    KeyValue keyValue() const
    {
        return m_keyboardEventData.keyValue();
    }

private:
    EventModifierData m_eventModifierData;
    KeyboardEventData m_keyboardEventData;
};
} // namespace Starfish

#endif
