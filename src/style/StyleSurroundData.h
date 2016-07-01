/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishStyleSurroundData__
#define __StarFishStyleSurroundData__

#include "style/BorderData.h"
#include "style/LengthData.h"

namespace StarFish {

class StyleSurroundData : public gc {
public:
    StyleSurroundData()
        : margin(Length(Length::Fixed, 0))
        , padding(Length(Length::Fixed, 0))
        , offset(Length())
    {
    }

    virtual ~StyleSurroundData()
    {
    }

    bool operator==(const StyleSurroundData& o)
    {
        return border == o.border && margin == o.margin && padding == o.padding && offset == o.offset;
    }

    bool operator!=(const StyleSurroundData& o)
    {
        return !operator ==(o);
    }

    BorderData border;
    LengthData margin;
    LengthData padding;
    LengthData offset;
};

} /* namespace StarFish */

#endif /* __StarFishStyleSurroundData__ */
