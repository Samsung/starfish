/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishStylePaintData__
#define __StarFishStylePaintData__

#include "core/style/Style.h"

namespace StarFish {

class StylePaintData : public gc {
public:
    StylePaintData(NamedColor::NamedColorValue e)
        : m_hasCurrentColorValue(true)
        , m_color(Unit::Color(0, 0, 0, 0))
    {
        STARFISH_ASSERT(e == NamedColor::currentColor);
    }
    StylePaintData(Unit::Color clr = Unit::Color(0, 0, 0, 0))
        : m_hasCurrentColorValue(false)
        , m_color(clr)
    {
    }

    bool operator==(const StylePaintData& o)
    {
        return m_color == o.m_color &&
               m_hasCurrentColorValue == o.m_hasCurrentColorValue;
    }

    bool operator!=(const StylePaintData& o)
    {
        return !operator==(o);
    }

    Unit::Color color() const
    {
        STARFISH_ASSERT(!m_hasCurrentColorValue);
        return m_color;
    }

    void updateCurrentColorToFixedColorIfNeeds(Unit::Color clr)
    {
        if (m_hasCurrentColorValue) {
            m_hasCurrentColorValue = false;
            m_color = clr;
        }
    }

private:
    // TODO add fill functions
    bool m_hasCurrentColorValue;
    Unit::Color m_color;
};
}

#endif
