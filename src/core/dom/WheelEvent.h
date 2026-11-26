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

#ifndef __StarfishWheelEvent__
#define __StarfishWheelEvent__

#include "MouseEvent.h"

namespace Starfish {

struct WheelDelta {
    double deltaX() const
    {
        return m_deltaX;
    }
    double deltaY() const
    {
        return m_deltaY;
    }
    double deltaZ() const
    {
        return m_deltaZ;
    }
    uint32_t deltaMode() const
    {
        return m_deltaMode;
    }
    void setDeltaX(double value)
    {
        m_deltaX = value;
    }
    void setDeltaY(double value)
    {
        m_deltaY = value;
    }
    void setDeltaZ(double value)
    {
        m_deltaZ = value;
    }
    void setDeltaMode(uint32_t value)
    {
        m_deltaMode = value;
    }

private:
    double m_deltaX{ 0 };
    double m_deltaY{ 0 };
    double m_deltaZ{ 0 };
    uint32_t m_deltaMode{ 0 };
};

struct WheelEventInit : public MouseEventInit, public WheelDelta {};

class WheelEvent : public MouseEvent, public WheelDelta {
public:
    using MouseEvent::MouseEvent;
    WheelEvent(ExecutionContext* context, String* type, WheelEventInit& init)
        : MouseEvent(context, type, init)
        , WheelDelta(init)
    {
    }

    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWheelEvent() const override;
};
} // namespace Starfish

#endif
