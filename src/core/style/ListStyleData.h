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

#ifndef __StarFishListStyleData__
#define __StarFishListStyleData__

#include "core/style/Style.h"
#include "core/style/StyleRuleCounterStyle.h"

namespace StarFish {

class ComputedStyle;

class ListStyleData : public gc {
    friend ComputedStyle;

public:
    ListStyleData()
        : m_position(ListStylePositionValue::ListStylePositionOutside)
        , m_counterStyle(nullptr)
    {
    }

    ListStylePositionValue position()
    {
        return m_position;
    }

    void setPosition(ListStylePositionValue v)
    {
        m_position = v;
    }

    String* type();
    void setType(String* v);
    void setType(StyleRuleCounterStyle* v);

    friend inline bool operator==(const ListStyleData& a,
                                  const ListStyleData& b);
    friend inline bool operator!=(const ListStyleData& a,
                                  const ListStyleData& b);

private:
    ListStylePositionValue m_position;
    const StyleRuleCounterStyle* m_counterStyle;
};

bool operator==(const ListStyleData& a, const ListStyleData& b)
{
    if (a.m_position != b.m_position) {
        return false;
    }
    if (a.m_counterStyle) {
        return a.m_counterStyle->equals(b.m_counterStyle);
    }
    return !b.m_counterStyle;
}

bool operator!=(const ListStyleData& a, const ListStyleData& b)
{
    return !operator==(a, b);
}
}

#endif
