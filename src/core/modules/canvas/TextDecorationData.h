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

#ifndef __StarFishTextDecorationData__
#define __StarFishTextDecorationData__

#include "core/style/Unit.h"

namespace StarFish {
class ComputedStyle;

class TextDecorationData {
public:
    TextDecorationData()
        : m_hasUnderLine(false)
        , m_hasLineThrough(false)
    {
    }

    TextDecorationData(TextDecorationData* other)
        : m_hasUnderLine(other->m_hasUnderLine)
        , m_hasLineThrough(other->m_hasLineThrough)
        , m_underLineColor(other->m_underLineColor)
        , m_lineThroughColor(other->m_lineThroughColor)
    {
    }

    void merge(ComputedStyle* style);

    void reset()
    {
        m_hasUnderLine = false;
        m_hasLineThrough = false;
    }

    bool hasUnderLine()
    {
        return m_hasUnderLine;
    }

    bool hasLineThrough()
    {
        return m_hasLineThrough;
    }

    Unit::Color underLineColor()
    {
        return m_underLineColor;
    }

    Unit::Color lineThroughColor()
    {
        return m_lineThroughColor;
    }

private:
    bool m_hasUnderLine;
    bool m_hasLineThrough;
    Unit::Color m_underLineColor;
    Unit::Color m_lineThroughColor;
};
}

#endif
