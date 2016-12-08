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
#include "FrameDocument.h"

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
            c->iterateChildBoxes([&finded, &first, &in](FrameBox* box) -> bool {
                if (!finded) {
                    if (box->isInlineBox() && box->asInlineBox()->isInlineNonReplacedBox()) {
                        if (box->asInlineBox()->asInlineNonReplacedBox()->origin() == in) {
                            first = box->asInlineBox()->asInlineNonReplacedBox();
                            finded = true;
                            return false;
                        }
                    }
                }
                return true;
            });
            STARFISH_ASSERT(first && finded);
            return first;
        }
    } else {
        Frame* block = blockContainer(currentFrame);
        return block;
    }
}

FloatingBoxInfo::FloatingBoxInfo(FrameBlockBox* box, LayoutLocation loc)
    : m_box(box)
    , m_loc(loc)
{
    STARFISH_ASSERT(box->style()->floating() != NoneFloatValue);
    m_isLeft = box->style()->floating() == LeftFloatValue;
    m_bottom = m_loc.y() + m_box->height() + m_box->marginBottom();
    if (m_isLeft) {
        m_horizontalBoundary = m_loc.x() + m_box->contentWidth() + m_box->borderRight() + m_box->paddingRight() + m_box->marginRight();
    } else {
        m_horizontalBoundary = m_loc.x() - m_box->marginLeft();
    }
}

void LayoutContext::registerFloatingBoxes(FrameBlockBox* box)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    LayoutLocation loc = box->absolutePoint(m_frameDocument);
    FloatingBoxInfo fbi = FloatingBoxInfo(box, loc);
    c.m_floatBoxes->push_back(fbi);
    if (fbi.isLeft()) {
        c.m_lastLeftTopFloatBoxLoc = fbi.loc().y();
    } else {
        c.m_lastRightFloatTopLoc = fbi.loc().y();
    }
}

LayoutUnit LayoutContext::maxHeightDueTofloatingBoxes(LayoutUnit yPosition)
{
    bool hasLeft = false, hasRight = false;
    LayoutUnit maxLeftHeight;
    LayoutUnit maxRightHeight;
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

#ifndef NDEBUG
    LayoutUnit leftX, rightX;
#endif

    for (size_t i = 0; i < c.m_floatBoxes->size(); i ++) {
        FloatingBoxInfo f = c.m_floatBoxes->at(i);
        if (f.loc().y() <= yPosition && yPosition < f.bottom()) {
            if (f.isLeft()) {
#ifndef NDEBUG
                if (hasLeft) {
                    STARFISH_ASSERT(f.loc().x() > leftX);
                }
                leftX = f.loc().x();
#endif
                if (!hasLeft) {
                    maxLeftHeight = f.bottom();
                    hasLeft = true;
                } else {
                    maxLeftHeight = std::max(maxLeftHeight, f.bottom());
                }
            } else {
#ifndef NDEBUG
                if (hasRight) {
                    STARFISH_ASSERT(f.loc().x() < rightX);
                }
                rightX = f.loc().x();
#endif
                if (!hasRight) {
                    maxRightHeight = f.bottom();
                    hasRight = true;
                } else {
                    maxRightHeight = std::max(maxRightHeight, f.bottom());
                }
            }
        }
    }

    if (hasLeft) {
        if (hasRight) {
            return std::max(maxLeftHeight, maxRightHeight) - yPosition;
        } else {
            return maxLeftHeight - yPosition;
        }
    } else {
        if (hasRight) {
            return maxRightHeight - yPosition;
        } else {
            return 0;
        }
    }
}

LayoutUnit LayoutContext::heightDueTofloatingBoxes(LayoutUnit yPosition)
{
    bool hasLeft = false, hasRight = false;
    LayoutUnit lastLeftHeight;
    LayoutUnit lastRightHeight;
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

#ifndef NDEBUG
    LayoutUnit leftX, rightX;
#endif

    for (size_t i = 0; i < c.m_floatBoxes->size(); i ++) {
        FloatingBoxInfo f = c.m_floatBoxes->at(i);
        if (f.loc().y() <= yPosition && yPosition < f.bottom()) {
            if (f.isLeft()) {
#ifndef NDEBUG
                if (hasLeft) {
                    STARFISH_ASSERT(f.loc().x() > leftX);
                }
                leftX = f.loc().x();
#endif
                hasLeft = true;
                lastLeftHeight = f.bottom();
            } else {
#ifndef NDEBUG
                if (hasRight) {
                    STARFISH_ASSERT(f.loc().x() < rightX);
                }
                rightX = f.loc().x();
#endif
                hasRight = true;
                lastRightHeight = f.bottom();
            }
        }
    }

    if (hasLeft) {
        if (hasRight) {
            return std::min(lastLeftHeight, lastRightHeight) - yPosition;
        } else {
            return lastLeftHeight - yPosition;
        }
    } else {
        if (hasRight) {
            return lastRightHeight - yPosition;
        } else {
            return 0;
        }
    }
}

std::pair<LayoutUnit, LayoutUnit> LayoutContext::floatingBoxBoundary(LayoutUnit yPosition, LayoutUnit left, LayoutUnit right)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    for (size_t i = 0; i < c.m_floatBoxes->size(); i ++) {
        FloatingBoxInfo f = c.m_floatBoxes->at(i);
        if (f.loc().y() <= yPosition && yPosition < f.bottom()) {
            LayoutUnit x = f.horizontalBoundary();
            if (f.isLeft()) {
                if (x > left) {
                    left = x;
                }
            } else {
                if (x < right) {
                    right = x;
                }
            }
        }
    }

    return std::make_pair(left, right);
}

LayoutUnit LayoutContext::lastTopLoc(FloatValue floating)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    if (floating == LeftFloatValue) {
        return c.m_lastLeftTopFloatBoxLoc;
    } else {
        return c.m_lastRightFloatTopLoc;
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

Element* Frame::offsetParent()
{
    if (isDocumentElement() || isBody())
        return nullptr;

    Node* node = nullptr;
    for (Frame* ancestor = layoutParent(); ancestor; ancestor = ancestor->layoutParent()) {

        node = ancestor->node();

        if (!node)
            continue;

        if (ancestor->isPositionedElement())
            break;

        if (node->isElement() && node->asElement()->isHTMLElement() && node->asElement()->asHTMLElement()->isHTMLBodyElement())
            break;
    }

    return node && node->isElement() ? node->asElement() : nullptr;
}

}
