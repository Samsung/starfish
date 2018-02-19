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
#include "core/event/KeyBoardEventData.h"

namespace StarFish {
class PlatformKeyEventData;

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
    KeyboardEvent(Document* document)
        : UIEvent(document)
    {
    }

    KeyboardEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
    {
    }

    KeyboardEvent(Document* document, String* eventType,
                  KeyboardEventInit& init)
        : UIEvent(document, eventType, init)
    {
        m_eventModifierData = init.m_eventModifierData;
        m_keyboardEventData = init.m_keyboardEventData;
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
#ifndef NDEBUG
#if !defined(PORT_GRAPHIC_BACKEND_EFL) && \
    !defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
#endif
#endif
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

    // Not in IDL.
    KeyValue keyValue() const
    {
        return m_keyboardEventData.keyValue();
    }

private:
    EventModifierData m_eventModifierData;
    KeyboardEventData m_keyboardEventData;
};
} // namespace StarFish

#endif
