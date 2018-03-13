/*
 * Copyright (C) 2003, 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2005 Nokia.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishUnit__
#define __StarFishUnit__

namespace StarFish {

class String;

namespace Unit {

    class Size {
    public:
        Size(float w, float h)
        {
            m_width = w;
            m_height = h;
        }

        void setWidth(float w)
        {
            m_width = w;
        }

        void setHeight(float h)
        {
            m_height = h;
        }

        float width() const
        {
            return m_width;
        }

        float height() const
        {
            return m_height;
        }

        bool isEmpty() const
        {
            return m_width <= 0 || m_height <= 0;
        }

    protected:
        float m_width, m_height;
    };

    class Location {
    public:
        Location(float x = 0, float y = 0)
        {
            m_x = x;
            m_y = y;
        }

        void setX(float x)
        {
            m_x = x;
        }

        void setY(float y)
        {
            m_y = y;
        }

        float x() const
        {
            return m_x;
        }

        float y() const
        {
            return m_y;
        }

    protected:
        float m_x, m_y;
    };

    inline Size operator-(const Location& a, const Location& b)
    {
        return Size(a.x() - b.x(), a.y() - b.y());
    }

    class Rect {
    public:
        Rect(float x = 0, float y = 0, float w = 0, float h = 0)
            : m_location(x, y)
            , m_size(w, h)
        {
        }

        float x() const
        {
            return m_location.x();
        }
        float y() const
        {
            return m_location.y();
        }
        float maxX() const
        {
            return x() + width();
        }
        float maxY() const
        {
            return y() + height();
        }
        float width() const
        {
            return m_size.width();
        }
        float height() const
        {
            return m_size.height();
        }

        void setX(float x)
        {
            m_location.setX(x);
        }
        void setY(float y)
        {
            m_location.setY(y);
        }
        void setWidth(float width)
        {
            m_size.setWidth(width);
        }
        void setHeight(float height)
        {
            m_size.setHeight(height);
        }

        bool isEmpty() const
        {
            return m_size.isEmpty();
        }

        bool contains(float px, float py) const
        {
            return px >= x() && px < (x() + width()) && py >= y() &&
                   py < (y() + height());
        }

        void unite(const Rect& other)
        {
            if (other.isEmpty()) {
                return;
            }
            if (isEmpty()) {
                *this = other;
                return;
            }

            Location newLocation(std::min(x(), other.x()),
                                 std::min(y(), other.y()));
            Location newMaxPoint(std::max(maxX(), other.maxX()),
                                 std::max(maxY(), other.maxY()));

            m_location = newLocation;
            m_size = newMaxPoint - newLocation;
        }

        bool operator==(const Rect& r) const
        {
            return m_location.x() == r.m_location.x() &&
                   m_location.y() == r.m_location.y() &&
                   m_size.width() == r.m_size.width() &&
                   m_size.height() == r.m_size.height();
        }
        bool operator!=(const Rect& r) const
        {
            return !this->operator==(r);
        }

    private:
        Location m_location;
        Size m_size;
    };

    class BoxSurroundData {
    public:
        BoxSurroundData(float top = 0, float right = 0, float bottom = 0,
                        float left = 0)
        {
            m_top = top;
            m_right = right;
            m_bottom = bottom;
            m_left = left;
        }

        float top()
        {
            return m_top;
        }

        float right()
        {
            return m_right;
        }

        float bottom()
        {
            return m_bottom;
        }

        float left()
        {
            return m_left;
        }

        void setTop(float v)
        {
            m_top = v;
        }

        void setRight(float v)
        {
            m_right = v;
        }

        void setBottom(float v)
        {
            m_bottom = v;
        }

        void setLeft(float v)
        {
            m_left = v;
        }

    protected:
        float m_top;
        float m_right;
        float m_bottom;
        float m_left;
    };

    class Color {
    public:
        Color()
            : m_r(0)
            , m_g(0)
            , m_b(0)
            , m_a(0)
            , m_isHsl(false)
        {
        }
        Color(unsigned char r, unsigned char g, unsigned char b,
              unsigned char a)
            : m_r(r)
            , m_g(g)
            , m_b(b)
            , m_a(a)
            , m_isHsl(false)
        {
        }

        bool operator==(const Color& c) const
        {
            return m_r == c.m_r && m_g == c.m_g && m_b == c.m_b && m_a == c.m_a;
        }

        bool operator!=(const Color& c) const
        {
            return !this->operator==(c);
        }

        bool isTransparent()
        {
            return m_a == 0;
        }

        String* toString() const;

        unsigned char r() const
        {
            return m_r;
        }
        unsigned char g() const
        {
            return m_g;
        }
        unsigned char b() const
        {
            return m_b;
        }
        unsigned char a() const
        {
            return m_a;
        }
        double R() const
        {
            return normalize(m_r);
        }
        double G() const
        {
            return normalize(m_g);
        }
        double B() const
        {
            return normalize(m_b);
        }
        double A() const
        {
            return normalize(m_a);
        }

        double normalize(unsigned char value) const
        {
            return (double)value / 255.0;
        }

        Color getDarkerColor()
        {
            if (m_r == 0 && m_g == 0 && m_b == 0) {
                return Color(171.0, 171.0, 171.0, m_a);
            }
            double max = std::max(R(), std::max(G(), B()));
            double mul = (max == 0.0) ? 0.0 : std::max(0.0, (max - 0.33) / max);

            return Color(mul * R() * 255.0, mul * G() * 255.0,
                         mul * B() * 255.0, m_a);
        }

        static double hue2rgb(double p, double q, double t)
        {
            if (t < 0) {
                t++;
            }
            if (t > 1) {
                t--;
            }
            if (t < 1 / 6.0) {
                return p + (q - p) * 6 * t;
            }
            if (t < 1 / 2.0) {
                return q;
            }
            if (t < 2 / 3.0) {
                return p + (q - p) * (2 / 3.0 - t) * 6;
            }
            return p;
        }

        static Color fromHsla(double h, double s, double l, unsigned char a)
        {
            double rVal = 0, gVal = 0, bVal = 0;

            if (s == 0) {
                rVal = gVal = bVal = l;
            } else {
                double q = l < 0.5f ? l * (1 + s) : l + s - l * s;
                double p = 2 * l - q;
                rVal = hue2rgb(p, q, h + 1 / 3.0);
                gVal = hue2rgb(p, q, h);
                bVal = hue2rgb(p, q, h - 1 / 3.0);
            }

            unsigned char r = (unsigned char)round(rVal * 255);
            unsigned char g = (unsigned char)round(gVal * 255);
            unsigned char b = (unsigned char)round(bVal * 255);

            Color c = Color(r, g, b, a);
            c.m_isHsl = true;
            return c;
        }

        double max3(double x, double y, double z) const
        {
            return std::max(std::max(x, y), z);
        }

        double min3(double x, double y, double z) const
        {
            return std::min(std::min(x, y), z);
        }

        void toHsl(double* h, double* s, double* l) const
        {
            double r = m_r / 255.0;
            double g = m_g / 255.0;
            double b = m_b / 255.0;

            double max = max3(r, g, b);
            double min = min3(r, g, b);
            double hVal = 0, sVal = 0, lVal = (max + min) / 2;

            if (max == min) {
                hVal = sVal = 0;
            } else {
                double d = max - min;
                sVal = lVal > 0.5 ? d / (2 - max - min) : d / (max + min);

                if (max == r) {
                    hVal = (g - b) / d + (g < b ? 6 : 0);
                } else if (max == g) {
                    hVal = (b - r) / d + 2;
                } else { // b
                    hVal = (r - g) / d + 4;
                }

                hVal = hVal / 6;
            }

            *h = hVal;
            *s = sVal;
            *l = lVal;
        }

        unsigned char m_r, m_g, m_b, m_a;
        bool m_isHsl;
    };
} // namespace Unit
} // namespace StarFish

#endif
