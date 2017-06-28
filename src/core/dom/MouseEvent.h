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
struct MouseEventInit : EventModifierInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    MouseEventInit()
        : MouseEventInit(0, 0, 0, 0)
    {
    }

    MouseEventInit(double clientX, double clientY)
        : MouseEventInit(clientX, clientY, clientX, clientY)
    {
    }

    MouseEventInit(double clientX, double clientY, double screenX,
                   double screenY)
        : EventModifierInit()
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

private:
    double m_clientX;
    double m_clientY;
    double m_screenX;
    double m_screenY;
};

class MouseEvent : public UIEvent {
public:
    MouseEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_clientX(0)
        , m_clientY(0)
        , m_screenX(0)
        , m_screenY(0)
    {
    }

    MouseEvent(Document* document, String* eventType, MouseEventInit& init)
        : UIEvent(document, eventType, init)
        , m_clientX(init.clientX())
        , m_clientY(init.clientY())
        , m_screenX(init.screenX())
        , m_screenY(init.screenY())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMouseEvent() const override;

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

private:
    double m_clientX;
    double m_clientY;
    double m_screenX;
    double m_screenY;
};
}

#endif
