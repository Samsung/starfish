/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAngle__
#define __StarfishAngle__

#include "core/layout/LayoutUtil.h"
#include "core/style/Unit.h"

namespace Starfish {

class String;
class Font;
class CalcData;
class node;
class Frame;
class Element;
class ComputedStyle;

class Angle {
public:
    enum Type {
        Auto,
        Fixed,

        InheritableNumber,
        Calc,
    };

    STARFISH_MAKE_STACK_ALLOCATED();

    Angle(Type type = Auto, float data = 0.f)
        : m_data(data)
        , m_type(type)
    {
        STARFISH_ASSERT(!isCalc());
    }

    Angle(CalcData* data)
        : m_data(data)
        , m_type(Calc)
    {
        STARFISH_ASSERT(!isCalc());
    }

    bool isSpecified() const
    {
        return isFixed() || isCalc();
    }

    bool isDefinite(bool canApplyPercentage) const
    {
        if (isSpecified()) {
            if (isFixed()) {
                return true;
            } else {
                return canApplyPercentage;
            }
        }

        return false;
    }

    bool isAuto() const
    {
        return m_type == Auto;
    }

    bool isFixed() const
    {
        return m_type == Fixed;
    }

    bool isInheritableNumber() const
    {
        return m_type == InheritableNumber;
    }

    bool isCalc() const
    {
        return m_type == Calc;
    }

    Type type() const
    {
        return m_type;
    }

    float fixed() const
    {
        STARFISH_ASSERT(isFixed());
        return m_data.m_numberData;
    }

    CalcData* calcData() const
    {
        STARFISH_ASSERT(m_type == Calc);
        return m_data.m_calcData;
    }

    float specifiedValue() const;

protected:
    union ValueData {
        float m_numberData;
        CalcData* m_calcData;

        ValueData(float data)
            : m_numberData(data)
        {
        }

        ValueData(CalcData* data)
            : m_calcData(data)
        {
        }
    };

    ValueData m_data;
    Type m_type;
};
} // namespace Starfish

#endif
