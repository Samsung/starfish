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

namespace StarFish {

class Font;
class LinearGradientData;
class CSSGradientValue;
class FrameBox;

enum class CSSGradientType;

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
        float m_percentageData;
        Length m_lengthData;

        ColorStopOffsetData(float data)
            : m_percentageData(data)
        {
        }

        ColorStopOffsetData(Length data)
            : m_lengthData(data)
        {
        }
    };

    ColorStopOffsetValue()
        : m_valueType(ColorStopOffsetType::ValueType::None)
        , m_valueData(0)
    {
    }

    ColorStopOffsetValue(ColorStopOffsetType::ValueType type)
        : m_valueType(type)
        , m_valueData(0)
    {
    }

    ColorStopOffsetValue(float data)
        : m_valueType(ColorStopOffsetType::ValueType::Percentage)
        , m_valueData(data)
    {
    }

    ColorStopOffsetValue(Length data)
        : m_valueType(ColorStopOffsetType::ValueType::Length)
        , m_valueData(data)
    {
    }

    void setType(ColorStopOffsetType::ValueType type)
    {
        m_valueType = type;
    }

    ColorStopOffsetType type() const
    {
        return m_valueType;
    }

    void setValue(ColorStopOffsetData data)
    {
        m_valueData = data;
    }

    float percentageValue() const
    {
        STARFISH_ASSERT(m_valueType.isPercentage());
        return m_valueData.m_percentageData;
    }

    Length lengthValue() const
    {
        STARFISH_ASSERT(m_valueType.isLength());
        return m_valueData.m_lengthData;
    }

    bool operator==(ColorStopOffsetValue& other) const
    {
        if (m_valueType != other.m_valueType) {
            return false;
        }

        if (m_valueType.m_type == ColorStopOffsetType::ValueType::None) {
            return true;
        } else if (m_valueType.m_type ==
                   ColorStopOffsetType::ValueType::Length) {
            return m_valueData.m_lengthData == other.m_valueData.m_lengthData;
        } else if (m_valueType.m_type ==
                   ColorStopOffsetType::ValueType::Percentage) {
            return m_valueData.m_percentageData ==
                   other.m_valueData.m_percentageData;
        }
        return false;
    }

    bool operator!=(ColorStopOffsetValue& other) const
    {
        return !(operator==(other));
    }

private:
    ColorStopOffsetType m_valueType;
    ColorStopOffsetData m_valueData;
};

class ColorStop : public gc {
public:
    ColorStop()
        : m_color()
        , m_offset()
        , m_specified(false)
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

    bool specified()
    {
        return m_specified;
    }

    void setSpecified(bool value)
    {
        m_specified = value;
    }

    bool equals(ColorStop* other) const
    {
        if (m_color != other->m_color) {
            return false;
        }
        if (m_offset != other->m_offset) {
            return false;
        }
        return true;
    }

private:
    Unit::Color m_color;
    ColorStopOffsetValue m_offset;
    bool m_specified;
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

    LinearGradientData* asLinearGradientData();

    GCVector<ColorStop*>& colorStopList()
    {
        return m_colorStopList;
    }

    virtual CSSGradientValue* convertToCSSGradientValue() = 0;

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs) = 0;

    void makeSpecifiedColorStops(GCVector<ColorStop*>& out, float& x1,
                                 float& y1, float& x2, float& y2,
                                 FrameBox* owner);

    virtual bool equals(GradientData* other) const;

protected:
    CSSGradientType m_type;
    GCVector<ColorStop*> m_colorStopList;
};

class LinearGradientData : public GradientData {
public:
    LinearGradientData(float angleDeg = 180.0f);

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

    bool computeEndPoints(const Unit::Rect& rect, float& x1, float& y1,
                          float& x2, float& y2);

    virtual CSSGradientValue* convertToCSSGradientValue() override;
    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs) override;
    virtual bool equals(GradientData* other) const override;

private:
    float m_angleDeg;
    uint8_t m_sc;
};
} // namespace StarFish
