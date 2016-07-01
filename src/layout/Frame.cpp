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
#include "Frame.h"
#include "FrameBox.h"

namespace StarFish {

Frame* LayoutContext::blockContainer(Frame* currentFrame)
{
    Frame* f = currentFrame->layoutParent();

    if (!f)
        return currentFrame;

    while (true) {
        if (f->isFrameBlockBox() && f->node() != nullptr) {
            return f;
        }
        f = f->layoutParent();
    }
}

Frame* LayoutContext::containingBlock(Frame* currentFrame)
{
    // https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#containing-block-details
    Frame* block = blockContainer(currentFrame);
    if (currentFrame->style()->position() == AbsolutePositionValue) {
        while (!block->isFrameDocument() && !block->isPositionedElement()) {
            block = blockContainer(block);
        }
        return block;
    } else {
        return block;
    }
}

LayoutUnit LayoutContext::parentContentWidth(Frame* currentFrame)
{
    return blockContainer(currentFrame)->asFrameBox()->contentWidth();
}

bool LayoutContext::parentHasFixedHeight(Frame* currentFrame)
{
    Frame* container = blockContainer(currentFrame);
    if (currentFrame->style()->position() == PositionValue::AbsolutePositionValue) {
        return true;
    }
    while (container) {
        if (container->style()->height().isFixed()) {
            return true;
        } else if (container->style()->position() == PositionValue::AbsolutePositionValue && container->style()->height().isPercent()) {
            return true;
        } else if (container->style()->height().isAuto()) {
            return false;
        } else {
            STARFISH_ASSERT(container->style()->height().isPercent());
            container = blockContainer(container);
        }
    }
    return false;
}

LayoutUnit LayoutContext::parentFixedHeight(Frame* currentFrame)
{
    Frame* container = blockContainer(currentFrame);
    std::vector<Length> reverse;
    while (container) {
        if (container->style()->height().isFixed()) {
            reverse.push_back(container->style()->height());
            break;
        } else if (container->style()->position() == PositionValue::AbsolutePositionValue && container->style()->height().isPercent()) {
            reverse.push_back(Length(Length::Fixed, container->style()->height().specifiedValue(containingBlock(container)->asFrameBox()->contentHeight())));
            break;
        } else {
            STARFISH_ASSERT(container->style()->height().isPercent());
            reverse.push_back(container->style()->height());
            container = blockContainer(container);
        }
    }
    LayoutUnit result = reverse.back().fixed();
    reverse.pop_back();
    while (reverse.size()) {
        result = result * reverse.back().percent();
        reverse.pop_back();
    }

    return result;
}

}
