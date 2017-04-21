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

#ifndef __StarFishKeyboardEvent__
#define __StarFishKeyboardEvent__

#include "UIEvent.h"

namespace StarFish {

#define KEYBOARD_KEYCODE_NONE 0
#define KEYBOARD_KEYCODE_BACKSPACE 8
#define KEYBOARD_KEYCODE_RETURN 13
#define KEYBOARD_KEYCODE_SHIFT 16
#define KEYBOARD_KEYCODE_CTRL_L 17
#define KEYBOARD_KEYCODE_ALT_L 18
#define KEYBOARD_KEYCODE_ALT_R 21
#define KEYBOARD_KEYCODE_CTRL_R 25
#define KEYBOARD_KEYCODE_ESC 27
#define KEYBOARD_KEYCODE_SPACE 32
#define KEYBOARD_KEYCODE_LEFT 37
#define KEYBOARD_KEYCODE_UP 38
#define KEYBOARD_KEYCODE_RIGHT 39
#define KEYBOARD_KEYCODE_DOWN 40
#define KEYBOARD_KEYCODE_0 48
#define KEYBOARD_KEYCODE_a 65

struct KeyboardEventInit : EventModifierInit {
public:
    KeyboardEventInit();

    // Constructor for internal use
    KeyboardEventInit(bool bubbles);
    KeyboardEventInit(bool bubbles, bool cancelable);

    String* key() const;
    void setKey(String* key);

    String* code() const;
    void setCode(String* code);

    uint32_t location() const;
    void setLocation(uint32_t location);

    bool repeat() const;
    void setRepeat(bool repeat);

    bool isComposing() const;
    void setIsComposing(bool isComposing);

private:
    String* m_key;
    String* m_code;
    uint32_t m_location;
    bool m_repeat;
    bool m_isComposing;
};

class KeyboardEvent : public UIEvent {
public:
    KeyboardEvent(String* eventType, String* key,
                  const KeyboardEventInit& init = KeyboardEventInit())
        : UIEvent(eventType, init)
        , m_metaKey(false)
    {
        m_keyCode = convertKeyCodeFromEcore(key);
        m_ctrlKey = ((m_keyCode == KEYBOARD_KEYCODE_CTRL_L) ||
                     (m_keyCode == KEYBOARD_KEYCODE_CTRL_R));
        m_shiftKey = (m_keyCode == KEYBOARD_KEYCODE_SHIFT);
        m_altKey = ((m_keyCode == KEYBOARD_KEYCODE_ALT_L) ||
                    (m_keyCode == KEYBOARD_KEYCODE_ALT_R));
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isKeyboardEvent() const override
    {
        return true;
    }

    unsigned long keyCode()
    {
        return (unsigned)m_keyCode;
    }
    bool ctrlKey()
    {
        return m_ctrlKey;
    }
    bool shiftKey()
    {
        return m_shiftKey;
    }
    bool altKey()
    {
        return m_altKey;
    }
    bool metaKey()
    {
        return m_metaKey;
    }
    void setCtrlKey()
    {
        m_ctrlKey = true;
    }
    void setShiftKey()
    {
        m_shiftKey = true;
    }
    void setAltKey()
    {
        m_altKey = true;
    }
    void setMetaKey()
    {
        m_metaKey = true;
    }

    static unsigned long convertKeyCodeFromEcore(String* key);

private:
    // String* m_key;
    // String* m_code;
    unsigned long m_keyCode;
    bool m_ctrlKey;
    bool m_shiftKey;
    bool m_altKey;
    bool m_metaKey;
};
}

#endif
