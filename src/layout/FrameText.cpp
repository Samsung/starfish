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

#include "StarFishConfig.h"
#include "FrameText.h"

#include "FrameBlockBoxInlineLayout.h"

namespace StarFish {

struct PreferedMinWidthComputer {
    PreferedMinWidthComputer()
        : m_maxWidthSoFar(0) { }

    void handleTextToken(TextToken& token)
    {
        m_maxWidthSoFar = std::max(m_maxWidthSoFar, token.width());
    }

    LayoutUnit m_maxWidthSoFar;
};

LayoutUnit FrameText::preferredMinWidth(LayoutContext& ctx)
{
    // We measure the width of each word in the text, and get the maximum
    // length among these words
    PreferedMinWidthComputer c;
    tokenizeText(ctx.starFish(), this, &c);
    return c.m_maxWidthSoFar;
}

struct PreferedWidthComputer {
    PreferedWidthComputer()
        : m_widthSoFar(0) { }

    void handleTextToken(TextToken& token)
    {
        m_widthSoFar += token.width();
    }

    LayoutUnit m_widthSoFar;
};

LayoutUnit FrameText::preferredWidth(LayoutContext& ctx)
{
    // We measure the width of the text, as if no line breaks, except where
    // explicit, occurred.
    PreferedWidthComputer c;
    tokenizeText(ctx.starFish(), this, &c);
    return c.m_widthSoFar;
}

}
