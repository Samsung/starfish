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

#ifndef __StarFishTouch__
#define __StarFishTouch__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace StarFish {

class EventTarget;

class TouchData {
    friend Touch;
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    TouchData()
        : TouchData(0, 0)
    {
    }

    TouchData(double clientX, double clientY)
        : TouchData(clientX, clientY, clientX, clientY)
    {
    }

    TouchData(double clientX, double clientY, double screenX, double screenY)
        : m_target(nullptr)
        , m_clientX(clientX)
        , m_clientY(clientY)
        , m_screenX(clientX)
        , m_screenY(clientY)
    {
    }

    EventTarget* target() const
    {
        return m_target;
    }
    void setTarget(EventTarget* target)
    {
        m_target = target;
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
    EventTarget* m_target;
    double m_clientX;
    double m_clientY;
    double m_screenX;
    double m_screenY;
};

// Binding interface
// https://w3c.github.io/touch-events/#idl-def-touchinit
class TouchInit : public TouchData {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    TouchInit()
        : TouchData()
    {
    }
};

// Binding interface
// https://w3c.github.io/touch-events/#idl-def-touch
class Touch : public ScriptWrappable, public DocumentHoldable {
public:
    Touch(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_touchData()
    {
    }

    Touch(Document* document, TouchInit& init)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_touchData(init)
    {
    }

    Touch(Document* document, TouchData& data)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_touchData(data)
    {
    }

    EventTarget* target() const
    {
        return m_touchData.m_target;
    }
    void setTarget(EventTarget* target)
    {
        m_touchData.m_target = target;
    }
    double clientX() const
    {
        return m_touchData.m_clientX;
    }
    void setClientX(double clientX)
    {
        m_touchData.m_clientX = clientX;
    }
    double clientY() const
    {
        return m_touchData.m_clientY;
    }
    void setClientY(double clientY)
    {
        m_touchData.m_clientY = clientY;
    }
    double screenX() const
    {
        return m_touchData.m_screenX;
    }
    void setScreenX(double screenX)
    {
        m_touchData.m_screenX = screenX;
    }
    double screenY() const
    {
        return m_touchData.m_screenY;
    }
    void setScreenY(double screenY)
    {
        m_touchData.m_screenY = screenY;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTouch() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

private:
    TouchData m_touchData;
};
}

#endif
