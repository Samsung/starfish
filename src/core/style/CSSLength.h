
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

#ifndef __StarFishCSSLength__
#define __StarFishCSSLength__

#include "core/style/Length.h"

namespace StarFish {

class String;

// https://www.w3.org/TR/CSS21/syndata.html#value-def-length
class CSSLength {
public:
    enum Kind { PX, EM, EX, INCH, CM, MM, PT, PC, VW, VH, VMIN, VMAX, REM, CH };

    CSSLength(float f)
    {
        m_kind = PX;
        m_value = f;
    }

    CSSLength(Kind kind, float f)
    {
        m_kind = kind;
        m_value = f;
    }

    CSSLength(String* unit, float f);
    CSSLength(const CSSTokenValue& unit, float f);

    Kind kind() const
    {
        return m_kind;
    }

    float value() const
    {
        return m_value;
    }

    Length toLength() const;

    String* toString() const;

protected:
    Kind m_kind;
    float m_value;
};

inline CSSLength operator*(const CSSLength& a, const float b)
{
    return CSSLength(a.value() * b);
}

inline CSSLength operator*(const float a, const CSSLength& b)
{
    return CSSLength(a * b.value());
}

} // namespace StarFish

#endif
