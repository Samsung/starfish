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

#include "core/style/CSSGradientValue.h"

namespace StarFish {

class LinearGradientData;

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
        Length m_lengthData;

        ColorStopOffsetData(float data)
            : m_numberData(data)
        {
        }

        ColorStopOffsetData(Length data)
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

    ColorStopOffsetValue(Length data)
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

    Length lengthValue() const
    {
        STARFISH_ASSERT(m_type.isLength());
        return m_data.m_lengthData;
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

    void setColor(Unit::Color& color)
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

class GradientData : public gc {
public:
    GradientData(CSSGradientType gradientType)
        : m_type(gradientType)
        , m_colorStopList()
    {
    }

    CSSGradientType type()
    {
        return m_type;
    }

    LinearGradientData* asLinearGradientData()
    {
        STARFISH_ASSERT(m_type == CSSGradientType::LinearGradient);
        return (LinearGradientData*)this;
    }

    GCVector<ColorStop*>& colorStopList()
    {
        return m_colorStopList;
    }

    virtual CSSGradientValue* convertToCSSGradientValue() = 0;

protected:
    CSSGradientType m_type;
    GCVector<ColorStop*> m_colorStopList;
};

class LinearGradientData : public GradientData {
public:
    LinearGradientData(float angleDeg = 180.0f)
        : GradientData(CSSGradientType::LinearGradient)
        , m_angleDeg(angleDeg)
        , m_sc(0)
    {
    }

    float angle()
    {
        return m_angleDeg;
    }

    void setAngle(float val)
    {
        m_angleDeg = val;
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

    virtual CSSGradientValue* convertToCSSGradientValue() override;

private:
    float m_angleDeg;
    uint8_t m_sc;
};
} // namespace StarFish
