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

LayoutUnit FrameText::minimumContentWidth(LayoutContext& ctx)
{
    // We measure the width of each word in the text, and get the maximum
    // length among these words
    LayoutUnit maxWidthSoFar = 0;
    textDividerForLayout(ctx.starFish(), text(),
                         [&](String* srcTxt, size_t offset, size_t nextOffset,
                             bool isWhiteSpace, bool canBreak) {
        maxWidthSoFar =
            std::max(maxWidthSoFar,
                     style()->font()->measureText(StringView(srcTxt,
                                                             offset, nextOffset)));
    });

    return maxWidthSoFar;
}

LayoutUnit FrameText::maximumContentWidth(LayoutContext& ctx)
{
    // We measure the width of the text, as if no line breaks, except where
    // explicit, occurred.
    LayoutUnit maxWidthSoFar = 0;

    // TODO
    return style()->font()->measureText(StringView(text()));
}


}
