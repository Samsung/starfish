
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

#ifndef __StarFishCSSTime__
#define __StarFishCSSTime__

namespace StarFish {

class String;

class CSSTime {
public:
    enum Kind { S, MS };

    CSSTime()
    {
        m_kind = MS;
        m_value = 0;
    }

    CSSTime(double time)
    {
        m_kind = MS;
        m_value = time;
    }

    CSSTime(const CSSTokenValue& str, float f);
    CSSTime(String* str, float f);

    Kind kind() const
    {
        return m_kind;
    }

    bool isZero() const
    {
        return m_value == 0;
    }

    float value() const
    {
        return m_value;
    }

    double toTimeValue() const;

    String* toString() const;

    bool operator==(const CSSTime& t) const
    {
        if (m_kind != t.m_kind) {
            return false;
        }

        if (m_value != t.m_value) {
            return false;
        }
        return true;
    }

    bool operator!=(const CSSTime& t) const
    {
        return !this->operator==(t);
    }

protected:
    Kind m_kind;
    double m_value; // ms
};

inline CSSTime operator+(const CSSTime& a, const CSSTime& b)
{
    return CSSTime(a.toTimeValue() + b.toTimeValue());
}

inline CSSTime& operator+=(CSSTime& a, const CSSTime& b)
{
    a = a + b;
    return a;
}

inline CSSTime operator-(const CSSTime& a, const CSSTime& b)
{
    return CSSTime(a.toTimeValue() - b.toTimeValue());
}

inline CSSTime operator*(const float a, const CSSTime& b)
{
    return CSSTime(a * b.toTimeValue());
}

inline CSSTime operator*(const CSSTime& a, const float b)
{
    return CSSTime(a.toTimeValue() * b);
}

inline CSSTime& operator*=(CSSTime& a, float b)
{
    a = a * b;
    return a;
}

inline CSSTime operator/(const CSSTime& a, const float b)
{
    return CSSTime(a.toTimeValue() / b);
}

inline CSSTime& operator/=(CSSTime& a, float b)
{
    a = a / b;
    return a;
}
} // namespace StarFish

#endif
