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

enum class CSSGradientType { LinearGradient, RadialGradient };

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
    CSSGradientValue(CSSGradientType gradientType)
        : m_gradientType(gradientType)
        , m_cssColorStopList()
    {
    }

    CSSGradientType type()
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
    CSSGradientType m_gradientType;
    GCVector<CSSColorStop*> m_cssColorStopList;
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
    virtual String* toString() override;
    virtual GradientData* convertToGradientData() override;

private:
    CSSAngle m_angle;
    uint8_t m_sc;
};
} // namespace StarFish

#endif
