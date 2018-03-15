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
#include "core/style/CounterStyle.h"

namespace StarFish {

class ComputedStyle;
class ImageResource;

class ListStyleData : public gc {
    friend ComputedStyle;

public:
    ListStyleData()
        : m_position(ListStylePositionValue::ListStylePositionOutside)
        , m_counterStyle(CounterStyle::getDiscCounter())
        , m_image(String::emptyString)
        , m_imageResource(nullptr)
    {
    }

    ListStylePositionValue position() const
    {
        return m_position;
    }

    void setPosition(ListStylePositionValue v)
    {
        m_position = v;
    }

    const CounterStyle* typeData() const
    {
        STARFISH_ASSERT(m_counterStyle);
        return m_counterStyle;
    }

    String* type() const;
    void setType(String* v);
    void setType(const CounterStyle* v);

    String* image() const
    {
        return m_image;
    }

    ImageResource* imageResource() const
    {
        return m_imageResource;
    }

    void setImage(String* v)
    {
        m_image = v;
    }

    void setImageResource(ImageResource* v)
    {
        m_imageResource = v;
    }

    friend inline bool operator==(const ListStyleData& a,
                                  const ListStyleData& b);
    friend inline bool operator!=(const ListStyleData& a,
                                  const ListStyleData& b);

private:
    ListStylePositionValue m_position;
    const CounterStyle* m_counterStyle;
    String* m_image;
    ImageResource* m_imageResource;
};

bool operator==(const ListStyleData& a, const ListStyleData& b)
{
    if (!a.m_image->equals(b.m_image)) {
        return false;
    }
    if (a.m_position != b.m_position) {
        return false;
    }
    STARFISH_ASSERT(a.m_counterStyle);
    STARFISH_ASSERT(b.m_counterStyle);
    return a.m_counterStyle->equals(b.m_counterStyle);
}

bool operator!=(const ListStyleData& a, const ListStyleData& b)
{
    return !operator==(a, b);
}
}

#endif
