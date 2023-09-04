/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMRectInit__
#define __StarfishDOMRectInit__

namespace Starfish {

struct DOMRectInit {
    DOMRectInit(double x, double y, double width, double height)
        : m_x(x)
        , m_hasX(true)
        , m_y(y)
        , m_hasY(true)
        , m_width(width)
        , m_hasWidth(true)
        , m_height(height)
        , m_hasHeight(true)
    {
    }

    DOMRectInit()
        : m_x(0)
        , m_y(0)
        , m_width(0)
        , m_height(0)
    {
    }

    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, x, X);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, y, Y);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, width, Width);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, height, Height);

    DEFINE_MEMBER_WITH_HASFLAG(double, x, X);
    DEFINE_MEMBER_WITH_HASFLAG(double, y, Y);
    DEFINE_MEMBER_WITH_HASFLAG(double, width, Width);
    DEFINE_MEMBER_WITH_HASFLAG(double, height, Height);

    bool equals(const DOMRectInit& other) const
    {
        return m_x == other.m_x && m_y == other.m_y &&
               m_width == other.m_width && m_height == other.m_height;
    }
};

} // namespace Starfish

#endif
