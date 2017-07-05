/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

class KeyboardData {
    STARFISH_MAKE_STACK_ALLOCATED()
    friend KeyboardEvent;

public:
    KeyboardData()
        : KeyboardData(String::emptyString)
    {
    }
    KeyboardData(String* key)
        : KeyboardData(key, String::emptyString)
    {
    }
    KeyboardData(String* key, String* code)
        : KeyboardData(key, code, 0)
    {
    }
    KeyboardData(String* key, String* code, uint32_t keyCode)
        : m_key(key)
        , m_code(code)
        , m_keyCode(keyCode)
        , m_ctrlKey(false)
        , m_shiftKey(false)
        , m_altKey(false)
        , m_metaKey(false)
    {
    }
    String* key()
    {
        return m_key;
    }
    String* code()
    {
        return m_code;
    }
    uint32_t keyCode()
    {
        return m_keyCode;
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

protected:
    String* m_key;
    String* m_code;
    uint32_t m_keyCode;
    bool m_ctrlKey;
    bool m_shiftKey;
    bool m_altKey;
    bool m_metaKey;
};

class KeyboardEvent : public UIEvent {
public:
    KeyboardEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_keyboardData()
    {
    }

    KeyboardEvent(Document* document, String* eventType, KeyboardData& data)
        : UIEvent(document, eventType)
        , m_keyboardData(data)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isKeyboardEvent() const override;

    String* key()
    {
        return m_keyboardData.m_key;
    }
    String* code()
    {
        return m_keyboardData.m_code;
    }
    uint32_t keyCode()
    {
        return m_keyboardData.m_keyCode;
    }
    bool ctrlKey()
    {
        return m_keyboardData.m_ctrlKey;
    }
    bool shiftKey()
    {
        return m_keyboardData.m_shiftKey;
    }
    bool altKey()
    {
        return m_keyboardData.m_altKey;
    }
    bool metaKey()
    {
        return m_keyboardData.m_metaKey;
    }

private:
    KeyboardData m_keyboardData;
};
}

#endif
