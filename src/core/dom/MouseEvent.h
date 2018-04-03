/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishMouseEvent__
#define __StarFishMouseEvent__

#include "UIEvent.h"

namespace StarFish {

class Window;

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
                    0, 0, 0)
    {
    }

    MouseData(unsigned char button, unsigned char buttons, double clientX,
              double clientY, int32_t clickCount,
              EventTarget* relatedTarget = nullptr)
        : MouseData(button, buttons, clientX, clientY, clientX, clientY,
                    clickCount, relatedTarget)
    {
    }

    MouseData(unsigned char button, unsigned char buttons, double clientX,
              double clientY, double screenX, double screenY,
              int32_t clickCount, EventTarget* relatedTarget = nullptr)
        : m_isDefaultPrevented(false)
        , m_button(button)
        , m_buttons(buttons)
        , m_clientX(clientX)
        , m_clientY(clientY)
        , m_screenX(screenX)
        , m_screenY(screenY)
        , m_clickCount(clickCount)
        , m_relatedTarget(relatedTarget)
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

    int32_t clickCount() const
    {
        return m_clickCount;
    }

    void setClickCount(int32_t clickCount)
    {
        m_clickCount = clickCount;
    }

    EventTarget* relatedTarget() const
    {
        return m_relatedTarget;
    }

    void setRelatedTarget(EventTarget* relatedTarget)
    {
        m_relatedTarget = relatedTarget;
    }

    bool isDefaultPrevented()
    {
        return m_isDefaultPrevented;
    }

    void setDefaultPrevented()
    {
        m_isDefaultPrevented = true;
    }

protected:
    bool m_isDefaultPrevented;
    unsigned char m_button;
    unsigned char m_buttons;

    double m_clientX;
    double m_clientY;
    double m_screenX;
    double m_screenY;

    int32_t m_clickCount;
    EventTarget* m_relatedTarget;
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
        setDetail(data.clickCount());
        if (data.isDefaultPrevented()) {
            setDefaultPrevented(true);
        }
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

    EventTarget* relatedTarget() const
    {
        return m_mouseData.relatedTarget();
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMouseEvent() const override;

    void initMouseEvent(String* type, bool bubbles, bool cancelable,
                        Window* view, int32_t detail, double screenX,
                        double screenY, double clientX, double clientY,
                        unsigned char button, bool ctrlKey, bool altKey,
                        bool shiftKey, bool metaKey, EventTarget* relatedTarget)
    {
        // use ctrlKey, altKey, shiftKey, metaKey
        setType(type);
        setBubbles(bubbles);
        setCancelable(cancelable);
        setView(view);
        setDetail(detail);
        m_mouseData.setScreenX(screenX);
        m_mouseData.setScreenY(screenY);
        m_mouseData.setClientX(clientX);
        m_mouseData.setClientY(clientY);
        m_mouseData.setButton(button);
        m_mouseData.setRelatedTarget(relatedTarget);
    }

private:
    MouseData m_mouseData;
};
}

#endif
