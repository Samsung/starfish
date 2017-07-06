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

#ifndef __StarFishMouseEvent__
#define __StarFishMouseEvent__

#include "UIEvent.h"

namespace StarFish {

// https://w3c.github.io/uievents/#idl-mouseevent
// https://w3c.github.io/uievents/#idl-mouseeventinit
class MouseData {
    friend MouseEvent;
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    enum MouseButtonValue {
        NoButton = 0,
        LeftButton = 0,
        MiddleButton = 1,
        RightButton = 2
    };

    enum MouseButtonsValue {
        NoButtonDown = 0,
        LeftButtonDown = 1,
        RightButtonDown = 1 << 1,
        MiddleButtonDown = 1 << 2,
    };

    MouseData()
        : MouseData(MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown,
                    0, 0)
    {
    }

    MouseData(unsigned char button, unsigned char buttons, double clientX,
              double clientY)
        : MouseData(button, buttons, clientX, clientY, clientX, clientY)
    {
    }

    MouseData(unsigned char button, unsigned char buttons, double clientX,
              double clientY, double screenX, double screenY)
        : m_button(button)
        , m_buttons(buttons)
        , m_clientX(clientX)
        , m_clientY(clientY)
        , m_screenX(screenX)
        , m_screenY(screenY)
    {
    }

    double clientX() const
    {
        return m_clientX;
    }

    void setClientX(double clientX)
    {
        m_clientX = clientX;
    }

    double clientY() const
    {
        return m_clientY;
    }

    void setClientY(double clientY)
    {
        m_clientY = clientY;
    }

    double screenX() const
    {
        return m_screenX;
    }

    void setScreenX(double screenX)
    {
        m_screenX = screenX;
    }

    double screenY() const
    {
        return m_screenY;
    }

    void setScreenY(double screenY)
    {
        m_screenY = screenY;
    }

    unsigned char button() const
    {
        return m_button;
    }

    void setButton(unsigned char button)
    {
        m_button = button;
    }

    unsigned char buttons() const
    {
        return m_buttons;
    }

    void setButtons(unsigned char buttons)
    {
        m_buttons = buttons;
    }

protected:
    unsigned char m_button;
    unsigned char m_buttons;

    double m_clientX;
    double m_clientY;
    double m_screenX;
    double m_screenY;
};

// Binding interface
// https://w3c.github.io/uievents/#idl-mouseeventinit
class MouseEventInit : public EventModifierInit, public MouseData {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    MouseEventInit()
        : EventModifierInit()
        , MouseData()
    {
    }
};

// Binding interface
class MouseEvent : public UIEvent {
public:
    MouseEvent(Document* document)
        : UIEvent(document)
        , m_mouseData()
    {
    }

    MouseEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_mouseData()
    {
    }

    MouseEvent(Document* document, String* eventType, MouseData& data)
        : UIEvent(document, eventType)
        , m_mouseData(data)
    {
    }

    MouseEvent(Document* document, String* eventType, MouseEventInit& init)
        : UIEvent(document, eventType, init)
        , m_mouseData(init)
    {
    }

    double clientX() const
    {
        return m_mouseData.m_clientX;
    }

    double clientY() const
    {
        return m_mouseData.m_clientY;
    }

    double screenX() const
    {
        return m_mouseData.m_screenX;
    }

    double screenY() const
    {
        return m_mouseData.m_screenY;
    }

    short button() const
    {
        return m_mouseData.button();
    }

    unsigned short buttons() const
    {
        return m_mouseData.buttons();
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMouseEvent() const override;

private:
    MouseData m_mouseData;
};
}

#endif
