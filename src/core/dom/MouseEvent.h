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
    MouseData()
        : MouseData(0, 0)
    {
    }

    MouseData(double clientX, double clientY)
        : MouseData(clientX, clientY, clientX, clientY)
    {
    }

    MouseData(double clientX, double clientY, double screenX, double screenY)
        : m_clientX(clientX)
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

protected:
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

    void setClientX(double clientX)
    {
        m_mouseData.m_clientX = clientX;
    }

    double clientY() const
    {
        return m_mouseData.m_clientY;
    }

    void setClientY(double clientY)
    {
        m_mouseData.m_clientY = clientY;
    }

    double screenX() const
    {
        return m_mouseData.m_screenX;
    }

    void setScreenX(double screenX)
    {
        m_mouseData.m_screenX = screenX;
    }

    double screenY() const
    {
        return m_mouseData.m_screenY;
    }

    void setScreenY(double screenY)
    {
        m_mouseData.m_screenY = screenY;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMouseEvent() const override;

private:
    MouseData m_mouseData;
};
}

#endif
