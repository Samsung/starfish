
/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCSSAngle__
#define __StarfishCSSAngle__

#include "core/style/Angle.h"

namespace Starfish {

class String;
// https://www.w3.org/TR/css3-values/#angles
class CSSAngle {
public:
    enum Kind { UNSPECIFIED, DEG, GRAD, RAD, TURN };

    CSSAngle()
    {
        m_kind = UNSPECIFIED;
        m_value = 0;
    }

    CSSAngle(float f)
    {
        m_kind = UNSPECIFIED;
        m_value = f;
    }

    CSSAngle(Kind kind, float f)
    {
        m_kind = kind;
        m_value = f;
    }

    CSSAngle(String* str, float f);
    CSSAngle(const CSSTokenValue& str, float f);

    Kind kind() const
    {
        return m_kind;
    }

    float value() const
    {
        return m_value;
    }

    float toDegreeValue() const;

    Angle toAngle() const;

    String* toString() const;

    bool operator==(const CSSAngle& other) const
    {
        return m_kind == other.m_kind && m_value == other.m_value;
    }

    bool operator!=(const CSSAngle& other) const
    {
        return !(operator==(other));
    }

protected:
    Kind m_kind;
    float m_value;
};

inline CSSAngle operator+(const CSSAngle& a, const CSSAngle& b)
{
    return CSSAngle(a.toDegreeValue() + b.toDegreeValue());
}

inline CSSAngle& operator+=(CSSAngle& a, const CSSAngle& b)
{
    a = a + b;
    return a;
}

inline CSSAngle operator-(const CSSAngle& a, const CSSAngle& b)
{
    return CSSAngle(a.toDegreeValue() - b.toDegreeValue());
}

inline CSSAngle operator*(const float a, const CSSAngle& b)
{
    return CSSAngle(a * b.toDegreeValue());
}

inline CSSAngle operator*(const CSSAngle& a, const float b)
{
    return CSSAngle(a.toDegreeValue() * b);
}

inline CSSAngle& operator*=(CSSAngle& a, float b)
{
    a = a * b;
    return a;
}

inline CSSAngle operator/(const CSSAngle& a, const float b)
{
    return CSSAngle(a.toDegreeValue() / b);
}

inline CSSAngle& operator/=(CSSAngle& a, float b)
{
    a = a / b;
    return a;
}
} // namespace Starfish

#endif
