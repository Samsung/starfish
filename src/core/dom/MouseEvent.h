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

#ifndef __StarfishMouseEvent__
#define __StarfishMouseEvent__

#include "UIEvent.h"
#include "PlatformIntegrationData.h"
using namespace LWE;

namespace Starfish {

class Window;

// https://w3c.github.io/uievents/#idl-mouseevent
// https://w3c.github.io/uievents/#idl-mouseeventinit
class MouseData {
    friend class MouseEvent;
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    MouseData()
        : MouseData(MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown,
                    0, 0, 0, timestamp())
    {
    }

    MouseData(unsigned char button, unsigned char buttons, double clientX,
              double clientY, int32_t clickCount,
              DOMTimeStamp timeStamp = timestamp(),
              EventTarget* relatedTarget = nullptr)
        : MouseData(button, buttons, clientX, clientY, clientX, clientY,
                    clickCount, timeStamp, relatedTarget)
    {
    }

    MouseData(unsigned char button, unsigned char buttons, double clientX,
              double clientY, double screenX, double screenY,
              int32_t clickCount, DOMTimeStamp timeStamp = timestamp(),
              EventTarget* relatedTarget = nullptr)
        : m_isDefaultPrevented(false)
        , m_button(button)
        , m_buttons(buttons)
        , m_clientX(clientX)
        , m_clientY(clientY)
        , m_screenX(screenX)
        , m_screenY(screenY)
        , m_clickCount(clickCount)
        , m_timeStamp(timeStamp)
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

    DOMTimeStamp timeStamp() const
    {
        return m_timeStamp;
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
    DOMTimeStamp m_timeStamp;
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
    MouseEvent(ExecutionContext* executionContext)
        : UIEvent(executionContext)
        , m_mouseData()
    {
    }

    MouseEvent(ExecutionContext* executionContext, String* eventType)
        : UIEvent(executionContext, eventType)
        , m_mouseData()
    {
    }

    MouseEvent(ExecutionContext* executionContext, String* eventType,
               MouseData& data)
        : UIEvent(executionContext, eventType)
        , m_mouseData(data)
    {
        setTimeStamp(data.m_timeStamp);
        setDetail(data.clickCount());
        if (data.isDefaultPrevented()) {
            setDefaultPrevented(true);
        }
    }

    MouseEvent(ExecutionContext* executionContext, String* eventType,
               MouseEventInit& init)
        : UIEvent(executionContext, eventType, init)
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

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMouseEvent() const override;

    void initMouseEvent(String* type, bool bubbles, bool cancelable,
                        Window* view, int32_t detail, double screenX,
                        double screenY, double clientX, double clientY,
                        unsigned char button, bool ctrlKey, bool altKey,
                        bool shiftKey, bool metaKey, EventTarget* relatedTarget)
    {
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
        m_eventModifierData.setCtrlKey(ctrlKey);
        m_eventModifierData.setAltKey(altKey);
        m_eventModifierData.setShiftKey(shiftKey);
        m_eventModifierData.setMetaKey(metaKey);
    }

private:
    EventModifierData m_eventModifierData;
    MouseData m_mouseData;
};
}

#endif
