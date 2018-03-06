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

#ifndef __StarFishLengthData__
#define __StarFishLengthData__

namespace StarFish {

class ComputedStyle;

class LengthData : public gc {
public:
    LengthData()
        : m_top(Length(Length::Fixed, 0))
        , m_right(Length(Length::Fixed, 0))
        , m_bottom(Length(Length::Fixed, 0))
        , m_left(Length(Length::Fixed, 0))
    {
    }

    LengthData(Length length)
        : m_top(length)
        , m_right(length)
        , m_bottom(length)
        , m_left(length)
    {
    }

    Length top()
    {
        return m_top;
    }

    Length right()
    {
        return m_right;
    }

    Length bottom()
    {
        return m_bottom;
    }

    Length left()
    {
        return m_left;
    }

    void setTop(Length top)
    {
        m_top = top;
    }

    void setRight(Length right)
    {
        m_right = right;
    }

    void setBottom(Length bottom)
    {
        m_bottom = bottom;
    }

    void setLeft(Length left)
    {
        m_left = left;
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        m_left.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                     windowSize.width(), windowSize.height(),
                                     cs);
        m_right.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                      windowSize.width(), windowSize.height(),
                                      cs);
        m_top.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                    windowSize.width(), windowSize.height(),
                                    cs);
        m_bottom.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                       windowSize.width(), windowSize.height(),
                                       cs);
    }

    bool operator==(const LengthData& o)
    {
        return m_left == o.m_left && m_right == o.m_right && m_top == o.m_top &&
               m_bottom == o.m_bottom;
    }

    bool operator!=(const LengthData& o)
    {
        return !operator==(o);
    }

private:
    Length m_top;
    Length m_right;
    Length m_bottom;
    Length m_left;
};

} /* namespace StarFish */

#endif /* __StarFishLengthData__ */
