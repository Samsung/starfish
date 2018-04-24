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

#include "core/style/Style.h"
namespace StarFish {

class CSSLinearGradientValue;
class GradientData;
class ColorStop;

enum class GradientType { LinearGradient, RadialGradient };

enum SideOrConer {
    toLeft = 1 << 0,
    toRight = 1 << 1,
    toTop = 1 << 2,
    toBottom = 1 << 3
};

class CSSColorStop : public gc {
public:
    CSSColorStop()
        : m_color()
        , m_offset()
    {
    }

    CSSStyleValuePair color()
    {
        return m_color;
    }

    void setColor(CSSStyleValuePair& color)
    {
        m_color = color;
    }

    CSSStyleValuePair offset()
    {
        return m_offset;
    }

    void setOffset(CSSStyleValuePair& offset)
    {
        m_offset = offset;
    }

private:
    CSSStyleValuePair m_color;
    CSSStyleValuePair m_offset;
};

class CSSGradientValue : public gc {
public:
    CSSGradientValue(GradientType gradientType)
        : m_gradientType(gradientType)
        , m_cssColorStopList()
    {
    }

    GradientType type()
    {
        return m_gradientType;
    }

    virtual String* toString() = 0;
    virtual GradientData* convertToGradientData() = 0;

    GCVector<CSSColorStop*>& cssColorStopList()
    {
        return m_cssColorStopList;
    }

protected:
    void convertCSSColorStopsToColorStops(GCVector<ColorStop*>& out);
    String* colorStopListToString();

    GradientType m_gradientType;
    GCVector<CSSColorStop*> m_cssColorStopList;
};

class CSSLinearGradientValue : public CSSGradientValue {
public:
    CSSLinearGradientValue(CSSAngle angle = CSSAngle(180))
        : CSSGradientValue(GradientType::LinearGradient)
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
    virtual String* toString() override;
    virtual GradientData* convertToGradientData() override;

private:
    CSSAngle m_angle;
    uint8_t m_sc;
};

enum class RadialGradientSizeKeyword {
    None,
    ClosetSide,
    FarthestSide,
    ClosetCorner,
    FarthestCorner
};

class CSSRadialGradientSize : public gc {
public:
    CSSRadialGradientSize()
        : m_firstRadius()
        , m_secondRadius()
        , m_keyword(RadialGradientSizeKeyword::None)
    {
    }

    CSSStyleValuePair firstRadius()
    {
        return m_firstRadius;
    }

    void setFirstRadius(CSSStyleValuePair firstRadius)
    {
        m_firstRadius = firstRadius;
    }

    CSSStyleValuePair secondRadius()
    {
        return m_secondRadius;
    }

    void setSecondRadius(CSSStyleValuePair secondRadius)
    {
        m_secondRadius = secondRadius;
    }

    bool hasValue()
    {
        return hasFirstRadius() || hasSecondRadius() || hasKeyword();
    }

    bool hasFirstRadius()
    {
        const auto& valueKind = m_firstRadius.valueKind();
        return (valueKind == CSSStyleValuePair::ValueKind::Percentage) ||
               (valueKind == CSSStyleValuePair::ValueKind::Length);
    }

    bool hasSecondRadius()
    {
        const auto& valueKind = m_secondRadius.valueKind();
        return (valueKind == CSSStyleValuePair::ValueKind::Percentage) ||
               (valueKind == CSSStyleValuePair::ValueKind::Length);
    }

    bool hasKeyword()
    {
        return m_keyword != RadialGradientSizeKeyword::None;
    }

    RadialGradientSizeKeyword keyword()
    {
        return m_keyword;
    }

    void setKeyword(RadialGradientSizeKeyword keyword)
    {
        m_keyword = keyword;
    }

private:
    CSSStyleValuePair m_firstRadius;
    CSSStyleValuePair m_secondRadius;
    RadialGradientSizeKeyword m_keyword;
};

enum class RadialGradientShape { None, Circle, Elipse };
class CSSRadialGradientValue : public CSSGradientValue {
public:
    CSSRadialGradientValue(
        RadialGradientShape shape = RadialGradientShape::Circle)
        : CSSGradientValue(GradientType::RadialGradient)
        , m_shape(shape)
        , m_size()
        , m_positionX()
        , m_positionY()
    {
    }

    CSSRadialGradientValue(RadialGradientShape shape,
                           CSSRadialGradientSize& size,
                           CSSStyleValuePair& positionX,
                           CSSStyleValuePair& positionY)
        : CSSGradientValue(GradientType::RadialGradient)
        , m_shape(shape)
        , m_size(size)
        , m_positionX(positionX)
        , m_positionY(positionY)
    {
    }

    RadialGradientShape shape()
    {
        return m_shape;
    }

    void setShape(RadialGradientShape shape)
    {
        m_shape = shape;
    }

    CSSRadialGradientSize size()
    {
        return m_size;
    }

    void setSize(CSSRadialGradientSize& size)
    {
        m_size = size;
    }

    CSSStyleValuePair positionX()
    {
        return m_positionX;
    }

    void setPositionX(CSSStyleValuePair& positionX)
    {
        m_positionX = positionX;
    }

    CSSStyleValuePair positionY()
    {
        return m_positionY;
    }

    void setPositionY(CSSStyleValuePair& positionY)
    {
        m_positionY = positionY;
    }

    virtual String* toString() override;
    virtual GradientData* convertToGradientData() override;

private:
    RadialGradientShape m_shape;
    CSSRadialGradientSize m_size; // size of the gradient's ending shape
    CSSStyleValuePair m_positionX;
    CSSStyleValuePair m_positionY;
};
} // namespace StarFish

#endif
