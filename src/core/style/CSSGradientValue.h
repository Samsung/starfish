/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCSSGradientValue__
#define __StarFishCSSGradientValue__

#include "core/style/CSSAngle.h"
#include "core/style/CSSLength.h"

namespace StarFish {

class CSSLinearGradientValue;

enum class CSSGradientType { LinearGradient, RadialGradient };

enum SideOrConer {
    toLeft = 1 << 0,
    toRight = 1 << 1,
    toTop = 1 << 2,
    toBottom = 1 << 3
};

class ColorStopOffsetType {
public:
    enum class ValueType { None, Invalid, Length, Percentage };

    STARFISH_MAKE_STACK_ALLOCATED();

    ColorStopOffsetType()
        : m_type(ValueType::None)
    {
    }

    ColorStopOffsetType(ValueType type)
        : m_type(type)
    {
    }

    bool isNone() const
    {
        return m_type == ValueType::None;
    }

    bool isInvalid() const
    {
        return m_type == ValueType::Invalid;
    }

    bool isLength() const
    {
        return m_type == ValueType::Length;
    }

    bool isPercentage() const
    {
        return m_type == ValueType::Percentage;
    }

    bool operator==(ColorStopOffsetType& other) const
    {
        return m_type == other.m_type;
    }

    bool operator!=(ColorStopOffsetType& other) const
    {
        return !(operator==(other));
    }
    ValueType m_type;
};

class ColorStopOffsetValue {
public:
    union ColorStopOffsetData {
        float m_numberData;
        CSSLength m_lengthData;

        ColorStopOffsetData(float data)
            : m_numberData(data)
        {
        }

        ColorStopOffsetData(CSSLength data)
            : m_lengthData(data)
        {
        }
    };

    ColorStopOffsetValue()
        : m_type(ColorStopOffsetType::ValueType::None)
        , m_data(0)
    {
    }

    ColorStopOffsetValue(ColorStopOffsetType::ValueType type)
        : m_type(type)
        , m_data(0)
    {
    }

    ColorStopOffsetValue(float data)
        : m_type(ColorStopOffsetType::ValueType::Percentage)
        , m_data(data)
    {
    }

    ColorStopOffsetValue(CSSLength data)
        : m_type(ColorStopOffsetType::ValueType::Length)
        , m_data(data)
    {
    }

    void setType(ColorStopOffsetType::ValueType type)
    {
        m_type = type;
    }

    ColorStopOffsetType type() const
    {
        return m_type;
    }

    void setValue(ColorStopOffsetData data)
    {
        m_data = data;
    }

    float percentageValue() const
    {
        STARFISH_ASSERT(m_type.isPercentage());
        return m_data.m_numberData;
    }

    CSSLength lengthValue() const
    {
        STARFISH_ASSERT(m_type.isLength());
        return m_data.m_lengthData;
    }

    String* toString()
    {
        if (m_type.isLength()) {
            return m_data.m_lengthData.toString();
        } else if (m_type.isPercentage()) {
            StringBuilder builder;
            builder.appendString(
                String::fromFloat(m_data.m_numberData * 100.f));
            builder.appendChar('%');
            return builder.finalize();
        }
        return String::emptyString;
    }

private:
    ColorStopOffsetType m_type;
    ColorStopOffsetData m_data;
};

class ColorStop : public gc {
public:
    ColorStop()
        : m_color()
        , m_offset()
    {
    }

    Unit::Color color()
    {
        return m_color;
    }

    void setColor(Unit::Color color)
    {
        m_color = color;
    }

    ColorStopOffsetValue offset()
    {
        return m_offset;
    }

    void setOffset(ColorStopOffsetValue offset)
    {
        m_offset = offset;
    }

private:
    Unit::Color m_color;
    ColorStopOffsetValue m_offset;
};

class CSSGradientValue : public gc {
public:
    CSSGradientValue(CSSGradientType gradientType)
        : m_gradientType(gradientType)
        , m_colorStopList()
    {
    }

    CSSGradientType type()
    {
        return m_gradientType;
    }

    CSSLinearGradientValue* asCSSLinearGradientValue()
    {
        STARFISH_ASSERT(m_gradientType == CSSGradientType::LinearGradient);
        return (CSSLinearGradientValue*)this;
    }

    virtual String* toString() = 0;

    GCVector<ColorStop*>& colorStopList()
    {
        return m_colorStopList;
    }

protected:
    CSSGradientType m_gradientType;
    GCVector<ColorStop*> m_colorStopList;
};

class CSSLinearGradientValue : public CSSGradientValue {
public:
    CSSLinearGradientValue(CSSAngle angle = CSSAngle(180))
        : CSSGradientValue(CSSGradientType::LinearGradient)
        , m_angle(angle)
        , m_sc(0)
    {
    }

    CSSAngle angle()
    {
        return m_angle;
    }

    void setAngle(CSSAngle val)
    {
        m_angle = val;
    }

    void setSideOrConter(uint8_t sc)
    {
        m_sc = sc;
    }

    uint8_t SideOrConer()
    {
        return m_sc;
    }

    bool computeEndPoints(const int width, const int height, float& x1,
                          float& y1, float& x2, float& y2);

    virtual String* toString() override;

private:
    CSSAngle m_angle;
    uint8_t m_sc;
};
} // namespace StarFish

#endif
