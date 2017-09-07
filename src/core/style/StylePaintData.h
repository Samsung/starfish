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
    StylePaintData(Unit::Color clr = Unit::Color(0, 0, 0, 0))
        : m_color(clr)
    {
    }

    bool operator==(const StylePaintData& o)
    {
        return m_color == o.m_color;
    }

    bool operator!=(const StylePaintData& o)
    {
        return !operator==(o);
    }

    Unit::Color color() const
    {
        return m_color;
    }

private:
    // TODO add fill functions
    Unit::Color m_color;
};
}

#endif
