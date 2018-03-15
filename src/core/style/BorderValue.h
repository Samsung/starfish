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

#ifndef __StarFishBorderValue__
#define __StarFishBorderValue__

#include "core/style/Style.h"

namespace StarFish {

class ComputedStyle;

class BorderValue {
public:
    BorderValue()
        : m_style(BorderStyleValue::NoneBorderStyleValue)
        , m_hasBorderColor(false)
        , m_width(Length(Length::Fixed, 3))
        , m_color(Unit::Color(0, 0, 0, 255))
    {
    }

    BorderStyleValue style()
    {
        return m_style;
    }

    STARFISH_MAKE_STACK_ALLOCATED();

    Length width()
    {
        return m_width;
    }

    Unit::Color color()
    {
        return m_color;
    }

    void setStyle(BorderStyleValue style)
    {
        m_style = style;
    }

    void setWidth(Length length)
    {
        m_width = length;
    }

    void setColor(Unit::Color color)
    {
        m_color = color;
        m_hasBorderColor = true;
    }

    void clearColor()
    {
        m_hasBorderColor = false;
    }

    bool hasBorderColor()
    {
        return m_hasBorderColor;
    }

    bool hasBorderStyle()
    {
        return style() != BorderStyleValue::NoneBorderStyleValue;
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        m_width.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                      windowSize.width(), windowSize.height(),
                                      cs);
        m_width.roundBorderWidth();
    }

    bool operator==(const BorderValue& o)
    {
        return this->m_style == o.m_style && this->m_width == o.m_width &&
               this->m_color == o.m_color &&
               this->m_hasBorderColor == o.m_hasBorderColor;
    }

    bool operator!=(const BorderValue& o)
    {
        return !operator==(o);
    }

protected:
    BorderStyleValue m_style : 4;
    bool m_hasBorderColor : 1;

    Length m_width;
    Unit::Color m_color;
};

} /* namespace StarFish */

#endif /* __StarFishBorderValue__ */
