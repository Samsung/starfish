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
#include "FrameBlockBox.h"

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

Frame* LayoutContext::containingFrameBlockBox(Frame* currentFrame)
{
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

Frame* LayoutContext::containingBlock(Frame* currentFrame)
{
    // https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#containing-block-details
    if (currentFrame->style()->position() == AbsolutePositionValue) {
        Frame* block = currentFrame->parent();
        while (!block->isFrameDocument() && !block->isPositionedElement()) {
            block = block->parent();
        }

        if (block->isFrameBox()) {
            return block;
        } else {
            STARFISH_ASSERT(block->isFrameInline());
            FrameBlockBox* c = blockContainer(block)->asFrameBlockBox();
            bool finded = false;
            FrameBox* first = nullptr;
            FrameInline* in = block->asFrameInline();
            c->iterateChildBoxes([&finded, &first, &in](FrameBox* box) {
                if (!finded) {
                    if (box->isInlineBox() && box->asInlineBox()->isInlineNonReplacedBox()) {
                        if (box->asInlineBox()->asInlineNonReplacedBox()->origin() == in) {
                            first = box->asInlineBox()->asInlineNonReplacedBox();
                            finded = true;
                        }
                    }
                }
            });
            STARFISH_ASSERT(first && finded);
            return first;
        }
    } else {
        Frame* block = blockContainer(currentFrame);
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

void LayoutContext::registerYPositionForVerticalAlignInlineBlock(LineBox* lb)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    for (size_t i = 0; i < c.m_inlineBlockBoxStack->size(); i ++) {
        (*c.m_registeredYPositionForVerticalAlignInlineBlock)[c.m_inlineBlockBoxStack->at(i)] = lb->absolutePoint(c.m_inlineBlockBoxStack->at(i)).y() + lb->ascender();
    }
}

std::pair<bool, LayoutUnit> LayoutContext::readRegisteredLastLineBoxYPos(FrameBlockBox* box)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    STARFISH_ASSERT(c.m_inlineBlockBoxStack->back() == box);
    auto iter = (*c.m_registeredYPositionForVerticalAlignInlineBlock).find(box);
    if (iter == c.m_registeredYPositionForVerticalAlignInlineBlock->end()) {
        return std::pair<bool, LayoutUnit>(false, 0);
    }
    LayoutUnit r = iter->second;
    c.m_registeredYPositionForVerticalAlignInlineBlock->erase(iter);
    return std::pair<bool, LayoutUnit>(true, r);
}

}
