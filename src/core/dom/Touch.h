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

struct TouchInit {
public:
    TouchInit()
        : TouchInit(0, 0, 0, 0)
    {
    }

    TouchInit(double clientX, double clientY)
        : TouchInit(clientX, clientY, clientX, clientY)
    {
    }

    TouchInit(double clientX, double clientY, double screenX, double screenY)
        : m_target(nullptr)
        , m_clientX(clientX)
        , m_clientY(clientY)
        , m_screenX(screenX)
        , m_screenY(screenY)
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

private:
    EventTarget* m_target;
    double m_clientX;
    double m_clientY;
    double m_screenX;
    double m_screenY;

    // TODO Implement
    // int32_t m_identifier;
    // double m_pageX;
    // double m_pageY;
    // double m_radiusX;
    // double m_radiusY;
    // double m_rotationAngle;
    // double m_force;
};

class Touch : public ScriptWrappable, public DocumentHoldable {
public:
    Touch(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_target(nullptr)
        , m_clientX(0)
        , m_clientY(0)
        , m_screenX(0)
        , m_screenY(0)
    {
    }

    // JS binding interface
    Touch(Document* document, TouchInit& init)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_target(init.target())
        , m_clientX(init.clientX())
        , m_clientY(init.clientY())
        , m_screenX(init.screenX())
        , m_screenY(init.screenY())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTouch() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

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

private:
    EventTarget* m_target;
    double m_clientX;
    double m_clientY;
    double m_screenX;
    double m_screenY;
};
}

#endif
