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

#ifndef __StarFishBorderImageLength__
#define __StarFishBorderImageLength__

#include "core/style/Length.h"

namespace StarFish {

class ComputedStyle;

class BorderImageLength {
public:
    enum Type { LengthType, NumberType };

    BorderImageLength()
        : m_type(NumberType)
        , m_number(1.0)
    {
    }

    BorderImageLength(Length length)
        : m_type(LengthType)
        , m_length(length)
    {
    }

    BorderImageLength(double number)
        : m_type(NumberType)
        , m_number(number)
    {
    }

    STARFISH_MAKE_STACK_ALLOCATED();

    void setValue(Length length)
    {
        m_type = LengthType;
        m_length = length;
    }

    void setValue(double number)
    {
        m_type = NumberType;
        m_number = number;
    }

    bool isLength()
    {
        return m_type == LengthType;
    }

    bool isNumber()
    {
        return m_type == NumberType;
    }

    bool isSpecified()
    {
        return isLength() || isNumber();
    }

    Length& length()
    {
        STARFISH_ASSERT(m_type == LengthType);
        return m_length;
    }

    double number()
    {
        STARFISH_ASSERT(m_type == NumberType);
        return m_number;
    }

    float specifiedValue(LayoutUnit parentLength, Frame* f)
    {
        STARFISH_ASSERT(isSpecified());
        if (isLength()) {
            return length().specifiedValue(parentLength, f);
        } else {
            return parentLength * number();
        }
    }

    String* dumpString()
    {
        if (m_type == LengthType) {
            return m_length.dumpString();
        } else if (m_type == NumberType) {
            char temp[100];
            snprintf(temp, sizeof(temp), "%f", m_number);
            return String::fromUTF8(temp);
        }
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        if (m_type == LengthType) {
            m_length.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                           windowSize.width(),
                                           windowSize.height(), cs);
        }
    }

    bool operator==(const BorderImageLength& o)
    {
        return m_type == o.m_type && m_length == o.m_length &&
               m_number == o.m_number;
    }

    bool operator!=(const BorderImageLength& o)
    {
        return !operator==(o);
    }

    Type m_type;
    Length m_length;
    double m_number;
};

class BorderImageLengthBox {
public:
    BorderImageLengthBox()
    {
    }

    BorderImageLengthBox(Length l)
        : m_left(l)
        , m_right(l)
        , m_top(l)
        , m_bottom(l)
    {
    }

    BorderImageLengthBox(double num)
        : m_left(num)
        , m_right(num)
        , m_top(num)
        , m_bottom(num)
    {
    }

    BorderImageLengthBox(BorderImageLength& l, BorderImageLength& r,
                         BorderImageLength& t, BorderImageLength& b)
        : m_left(l)
        , m_right(r)
        , m_top(t)
        , m_bottom(b)
    {
    }

    STARFISH_MAKE_STACK_ALLOCATED();

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        m_left.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
        m_right.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
        m_top.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
        m_bottom.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
    }

    BorderImageLength& left()
    {
        return m_left;
    }
    BorderImageLength& right()
    {
        return m_right;
    }
    BorderImageLength& top()
    {
        return m_top;
    }
    BorderImageLength& bottom()
    {
        return m_bottom;
    }

    bool operator==(const BorderImageLengthBox& o)
    {
        return m_left == o.m_left && m_right == o.m_right && m_top == o.m_top &&
               m_bottom == o.m_bottom;
    }

    bool operator!=(const BorderImageLengthBox& o)
    {
        return !operator==(o);
    }

private:
    BorderImageLength m_left;
    BorderImageLength m_right;
    BorderImageLength m_top;
    BorderImageLength m_bottom;
};
}

#endif
