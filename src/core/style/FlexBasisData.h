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

#ifndef __StarFishFlexBasisData__
#define __StarFishFlexBasisData__

#include "core/style/Length.h"

namespace StarFish {
class FlexBasisData {
    enum Type { Content, Width };

    Type m_type;
    Length m_width;

public:
    FlexBasisData()
        : m_type(Content)
        , m_width(Length())
    {
    }

    FlexBasisData(bool isContent, Length width = Length())
        : m_width(width)
    {
        if (isContent) {
            m_type = Content;
        } else {
            m_type = Width;
        }
    }

    bool isContent() const
    {
        return m_type == Content;
    }

    bool isWidth() const
    {
        return m_type == Width;
    }

    Length width() const
    {
        return m_width;
    }
};
}

#endif
