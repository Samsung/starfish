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

#ifndef __StarFishRectData__
#define __StarFishRectData__

namespace StarFish {

class RectData : public gc {
public:
    RectData()
    {
    }

    RectData(Length top, Length right, Length bottom, Length left)
        : m_top(top)
        , m_right(right)
        , m_bottom(bottom)
        , m_left(left)
    {
    }

    Length& top()
    {
        return m_top;
    }

    Length& right()
    {
        return m_right;
    }

    Length& bottom()
    {
        return m_bottom;
    }

    Length& left()
    {
        return m_left;
    }

    String* toString()
    {
        StringBuilder builder;
        builder.appendString("rect(");
        String* top = m_top.dumpString();
        builder.appendString(top);
        builder.appendString("px");
        builder.appendString(", ");
        String* right = m_right.dumpString();
        builder.appendString(right);
        builder.appendString("px");
        builder.appendString(", ");
        String* bottom = m_bottom.dumpString();
        builder.appendString(bottom);
        builder.appendString("px");
        builder.appendString(", ");
        String* left = m_left.dumpString();
        builder.appendString(left);
        builder.appendString("px");
        builder.appendString(")");

        return builder.finalize();
    }

    bool operator==(const RectData& src)
    {
        return m_top == src.m_top && m_right == src.m_right &&
               m_bottom == src.m_bottom && m_left == src.m_left;
    }

private:
    Length m_top;
    Length m_right;
    Length m_bottom;
    Length m_left;
};

} /* namespace StarFish */

#endif /* __StarFishRectData__ */
