/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPointerEvent__
#define __StarfishPointerEvent__

#include "StarfishConfig.h"
#include "MouseEvent.h"
using namespace LWE;

namespace Starfish {

class Window;

class PointerData {
    friend class PointerEvent;
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    PointerData()
        : m_pointerId(0)
        , m_pointerType(String::createASCIIString("mouse"))
    {
    }

    int32_t pointerId() const
    {
        return m_pointerId;
    }

    String* pointerType() const
    {
        return m_pointerType;
    }

protected:
    int32_t m_pointerId;
    String* m_pointerType;
};

// https://w3c.github.io/pointerevents/#dom-pointereventinit
class PointerEventInit : public MouseEventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    PointerEventInit()
        : MouseEventInit()
        , m_pointerId(0)
        , m_pointerType(String::createASCIIString("mouse"))
    {
    }

    void setPointerId(int32_t pointerId)
    {
        m_pointerId = pointerId;
    }

    int32_t pointerId() const
    {
        return m_pointerId;
    }

    void setPointerType(String* pointerType)
    {
        m_pointerType = pointerType;
    }

    String* pointerType() const
    {
        return m_pointerType;
    }

private:
    int32_t m_pointerId;
    String* m_pointerType;
};

// Binding interface
class PointerEvent : public MouseEvent {
public:
    PointerEvent(ExecutionContext* executionContext)
        : MouseEvent(executionContext)
    {
    }

    PointerEvent(ExecutionContext* executionContext, String* eventType)
        : MouseEvent(executionContext, eventType)
    {
    }

    PointerEvent(ExecutionContext* executionContext, String* eventType,
                 MouseData& data)
        : MouseEvent(executionContext, eventType, data)
    {
    }

    PointerEvent(ExecutionContext* executionContext, String* eventType,
                 PointerEventInit& init)
        : MouseEvent(executionContext, eventType, init)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isPointerEvent() const override;
    virtual uint32_t which() const
    {
        return buttons();
    }

    int32_t pointerId() const
    {
        return m_pointerData.pointerId();
    }

    String* pointerType() const
    {
        return m_pointerData.pointerType();
    }

private:
    PointerData m_pointerData;
};
} // namespace Starfish

#endif
