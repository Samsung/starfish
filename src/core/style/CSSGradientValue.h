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

enum CSSGradientType { LinearGradient, RadialGradient };

class CSSGradientValue : public gc {
public:
    CSSGradientValue(CSSGradientType gradientType)
        : m_gradientType(gradientType)
        , m_startingX(0)
        , m_startingY(0)
        , m_endingX(0)
        , m_endingY(0)
    {
    }

    CSSGradientType type()
    {
        return m_gradientType;
    }

    float startingX()
    {
        return m_startingX;
    }

    void setStartingX(float val)
    {
        m_startingX = val;
    }

    float startingY()
    {
        return m_startingY;
    }

    void setStartingY(float val)
    {
        m_startingY = val;
    }

    float endingX()
    {
        return m_endingX;
    }

    void setEndingX(float val)
    {
        m_endingX = val;
    }

    float endingY()
    {
        return m_endingY;
    }

    void setEndingY(float val)
    {
        m_endingY = val;
    }

    String* toString();

protected:
    CSSGradientType m_gradientType;
    float m_startingX;
    float m_startingY;
    float m_endingX;
    float m_endingY;
    // TODO: Consider <color-stop-list>
};

class CSSLinearGradientValue : public CSSGradientValue {
public:
    CSSLinearGradientValue(CSSAngle angle)
        : CSSGradientValue(CSSGradientType::LinearGradient)
        , m_angle(angle)
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

private:
    CSSAngle m_angle;
};

} /* namespace StarFish */

#endif /* __StarFishCSSGradientValue__ */
