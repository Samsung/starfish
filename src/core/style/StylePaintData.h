/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
