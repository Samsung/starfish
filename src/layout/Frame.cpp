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

    if (!f) {
        return currentFrame;
    }

    while (true) {
        if (f->isFrameBlockBox() && !f->isAnonymous()) {
            return f;
        }
        f = f->layoutParent();
    }
}

Frame* LayoutContext::containingFrameBlockBox(Frame* currentFrame)
{
    Frame* block = blockContainer(currentFrame);
    if (currentFrame->style()->position() == AbsolutePositionValue) {
        while (!block->isFrameDocument() && !block->isPositioned()) {
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
        while (!block->isFrameDocument() && !block->isPositioned()) {
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
                    if (box->isInlineBox() &&
                        box->asInlineBox()->isInlineNonReplacedBox()) {
                        if (box->asInlineBox()
                                ->asInlineNonReplacedBox()
                                ->origin() == in) {
                            first =
                                box->asInlineBox()->asInlineNonReplacedBox();
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

FloatingBoxInfo::FloatingBoxInfo(FrameBox* box, LayoutContext* ctx) : m_box(box)
{
    STARFISH_ASSERT(box->isFloating());
    m_isLeft = box->style()->floating() == LeftFloatValue;
    Frame* parent = box->layoutParent();
    while (parent) {
        if (parent->isFrameBlockBox() && !parent->isAnonymous()) {
            m_canLayoutParentCollapseWithMarginTop =
                parent->asFrameBlockBox()
                    ->marginInfo()
                    ->canCollapseWithMarginTop();
            break;
        }

        parent = parent->layoutParent();
    }
    reCache(ctx);
}

void FloatingBoxInfo::reCache(LayoutContext* ctx)
{
    m_loc = m_box->absolutePoint(ctx->frameDocument());
    m_top = m_loc.y() - m_box->marginTop();
    m_bottom = m_loc.y() + m_box->height() + m_box->marginBottom();
    if (m_isLeft) {
        m_horizontalBoundary =
            m_loc.x() + m_box->width() + m_box->marginRight();
    } else {
        m_horizontalBoundary = m_loc.x() - m_box->marginLeft();
    }
}

void LayoutContext::registerFloatingBox(FrameBox* box)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    FloatingBoxInfo fbi = FloatingBoxInfo(box, this);
    c.m_floatBoxes->push_back(fbi);
    c.m_topLocOfFloatBox = fbi.top();
}

void LayoutContext::unregisterFloatingBoxes(size_t from)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    auto iter = c.m_floatBoxes->begin() + from;
    while (iter != c.m_floatBoxes->end()) {
        iter = c.m_floatBoxes->erase(iter);
    }
}

static bool floatAffected(LayoutUnit yPosition, LayoutUnit height,
                          FloatingBoxInfo& f)
{
    if (height == 0) {
        // height of Linebox can be 0 for the first time.
        return f.top() <= yPosition && yPosition < f.bottom();
    } else {
        if (f.top() >= yPosition + height) {
            return false;
        } else if (f.bottom() <= yPosition) {
            return false;
        }
        return true;
    }
}

LayoutUnit LayoutContext::clearedDistanceToFloatBottom(LayoutUnit yPosition,
                                                       ClearValue clearValue,
                                                       size_t* idx)
{
    bool hasLeft = false, hasRight = false;
    LayoutUnit clearedDistanceToLeftFloatBottom;
    LayoutUnit clearedDistanceToRightFloatBottom;
    size_t leftIdx = 0, rightIdx = 0;
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    if (clearValue == BothClearValue) {
        for (size_t i = 0; i < c.m_floatBoxes->size(); i++) {
            FloatingBoxInfo& f = (*c.m_floatBoxes)[i];
            if (f.isLeft()) {
                if (!hasLeft) {
                    clearedDistanceToLeftFloatBottom = f.bottom();
                    hasLeft = true;
                    leftIdx = i;
                } else {
                    if (clearedDistanceToLeftFloatBottom < f.bottom()) {
                        leftIdx = i;
                        clearedDistanceToLeftFloatBottom = f.bottom();
                    }
                }
            } else {
                if (!hasRight) {
                    clearedDistanceToRightFloatBottom = f.bottom();
                    hasRight = true;
                    rightIdx = i;
                } else {
                    if (clearedDistanceToRightFloatBottom < f.bottom()) {
                        rightIdx = i;
                        clearedDistanceToRightFloatBottom = f.bottom();
                    }
                }
            }
        }

        if (hasLeft) {
            if (hasRight) {
                if (clearedDistanceToLeftFloatBottom <
                    clearedDistanceToRightFloatBottom) {
                    if (idx) {
                        *idx = rightIdx;
                    }
                    return clearedDistanceToRightFloatBottom - yPosition;
                } else {
                    if (idx) {
                        *idx = leftIdx;
                    }
                    return clearedDistanceToLeftFloatBottom - yPosition;
                }
            } else {
                if (idx) {
                    *idx = leftIdx;
                }
                return clearedDistanceToLeftFloatBottom - yPosition;
            }
        } else {
            if (hasRight) {
                if (idx) {
                    *idx = rightIdx;
                }
                return clearedDistanceToRightFloatBottom - yPosition;
            } else {
                if (idx) {
                    *idx = SIZE_MAX;
                }
                return 0;
            }
        }
    } else if (clearValue == LeftClearValue) {
        for (size_t i = 0; i < c.m_floatBoxes->size(); i++) {
            FloatingBoxInfo& f = (*c.m_floatBoxes)[i];
            if (f.isLeft()) {
                if (!hasLeft) {
                    clearedDistanceToLeftFloatBottom = f.bottom();
                    hasLeft = true;
                    leftIdx = i;
                } else {
                    if (clearedDistanceToLeftFloatBottom < f.bottom()) {
                        leftIdx = i;
                        clearedDistanceToLeftFloatBottom = f.bottom();
                    }
                }
            }
        }

        if (hasLeft) {
            if (idx) {
                *idx = leftIdx;
            }
            return clearedDistanceToLeftFloatBottom - yPosition;
        } else {
            if (idx) {
                *idx = SIZE_MAX;
            }
            return 0;
        }
    } else if (clearValue == RightClearValue) {
        for (size_t i = 0; i < c.m_floatBoxes->size(); i++) {
            FloatingBoxInfo& f = (*c.m_floatBoxes)[i];
            if (!f.isLeft()) {
                if (!hasRight) {
                    clearedDistanceToRightFloatBottom = f.bottom();
                    hasRight = true;
                    rightIdx = i;
                } else {
                    if (clearedDistanceToRightFloatBottom < f.bottom()) {
                        rightIdx = i;
                        clearedDistanceToRightFloatBottom = f.bottom();
                    }
                }
            }
        }

        if (hasRight) {
            if (idx) {
                *idx = rightIdx;
            }
            return clearedDistanceToRightFloatBottom - yPosition;
        } else {
            if (idx) {
                *idx = SIZE_MAX;
            }
            return 0;
        }
    } else {
        if (idx) {
            *idx = SIZE_MAX;
        }
        return 0;
    }
}

LayoutUnit LayoutContext::nextDistanceToFloatBottom(LayoutUnit yPosition,
                                                    LayoutUnit height)
{
    bool hasLeft = false, hasRight = false;
    LayoutUnit lastDistanceToLeftFloatBottom;
    LayoutUnit lastDistanceToRightFloatBottom;
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

#ifndef NDEBUG
    LayoutUnit leftX, rightX, leftY, rightY;
#endif

    for (size_t i = 0; i < c.m_floatBoxes->size(); i++) {
        FloatingBoxInfo& f = (*c.m_floatBoxes)[i];
        if (floatAffected(yPosition, height, f)) {
            if (f.isLeft()) {
#ifndef NDEBUG
                if (hasLeft) {
                    if (leftY == f.top()) {
                        STARFISH_ASSERT(f.loc().x() >= leftX);
                    } else {
                        leftY = f.top();
                        leftX = f.loc().x() - f.box()->marginLeft();
                    }
                }
                leftY = f.top();
                leftX = f.loc().x() - f.box()->marginLeft();
#endif
                hasLeft = true;
                lastDistanceToLeftFloatBottom = f.bottom();
            } else {
#ifndef NDEBUG
                if (hasRight) {
                    if (rightY == f.top()) {
                        STARFISH_ASSERT(f.loc().x() <= rightX);
                    } else {
                        rightY = f.top();
                        rightX = f.loc().x() - f.box()->marginLeft();
                    }
                }
                rightY = f.top();
                rightX = f.loc().x() - f.box()->marginLeft();
#endif
                hasRight = true;
                lastDistanceToRightFloatBottom = f.bottom();
            }
        }
    }

    if (hasLeft) {
        if (hasRight) {
            return std::min(lastDistanceToLeftFloatBottom,
                            lastDistanceToRightFloatBottom) -
                   yPosition;
        } else {
            return lastDistanceToLeftFloatBottom - yPosition;
        }
    } else {
        if (hasRight) {
            return lastDistanceToRightFloatBottom - yPosition;
        } else {
            return 0;
        }
    }
}

std::pair<LayoutUnit, LayoutUnit>
LayoutContext::horizontalBoundaryBetweenFloatingBoxes(LayoutUnit yPosition,
                                                      LayoutUnit height,
                                                      LayoutUnit left,
                                                      LayoutUnit right)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    for (size_t i = 0; i < c.m_floatBoxes->size(); i++) {
        FloatingBoxInfo& f = (*c.m_floatBoxes)[i];
        if (floatAffected(yPosition, height, f)) {
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

bool LayoutContext::isCollidedWithFloatingBoxes(LayoutLocation loc,
                                                FrameBox* box,
                                                LayoutUnit leftBoundary,
                                                LayoutUnit rightBoundary)
{
    std::pair<LayoutUnit, LayoutUnit> boundaries =
        horizontalBoundaryBetweenFloatingBoxes(loc.y(), box->height(),
                                               leftBoundary, rightBoundary);

    return (leftBoundary != boundaries.first && loc.x() < boundaries.first) ||
           (rightBoundary != boundaries.second &&
            loc.x() + box->width() > boundaries.second);
}

void LayoutContext::resetLastTopLoc()
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    if (c.m_floatBoxes->size() == 0) {
        return;
    }

    c.m_topLocOfFloatBox = (*c.m_floatBoxes->rbegin()).top();
}

LayoutUnit LayoutContext::lastTopLoc()
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    return c.m_topLocOfFloatBox;
}

size_t LayoutContext::floatingBoxesSize()
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    return c.m_floatBoxes->size();
}

void LayoutContext::reCacheFloatingBoxes(size_t from)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    for (size_t i = from; i < c.m_floatBoxes->size(); i++) {
        FloatingBoxInfo& fbi = (*c.m_floatBoxes)[i];
        fbi.reCache(this);
    }

    resetLastTopLoc();
}

bool LayoutContext::canFloatCollapseWithMarginTop(size_t idx)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    if (idx == SIZE_MAX) {
        return false;
    }

    FloatingBoxInfo& fbi = (*c.m_floatBoxes)[idx];
    return fbi.canLayoutParentCollapseWithMarginTop();
}

LayoutUnit LayoutContext::parentContentWidth(Frame* currentFrame)
{
    return blockContainer(currentFrame)->asFrameBox()->contentWidth();
}

bool LayoutContext::parentHasFixedHeight(Frame* currentFrame)
{
    Frame* container = blockContainer(currentFrame);
    if (currentFrame->style()->position() ==
        PositionValue::AbsolutePositionValue) {
        return true;
    }
    while (container) {
        if (container->style()->height().isFixed()) {
            return true;
        } else if (container->style()->position() ==
                       PositionValue::AbsolutePositionValue &&
                   container->style()->height().isPercent()) {
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
        } else if (container->style()->position() ==
                       PositionValue::AbsolutePositionValue &&
                   container->style()->height().isPercent()) {
            reverse.emplace_back(
                Length::Fixed,
                container->style()->height().specifiedValue(
                    containingBlock(container)->asFrameBox()->contentHeight()));
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

void LayoutContext::registerYPositionPerVAInlineBlock(LineBox* lb)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    for (size_t i = 0; i < c.m_inlineBlockBoxStack->size(); i++) {
        (*c.m_registeredYPositionPerVAInlineBlock)[(
            *c.m_inlineBlockBoxStack)[i]] =
            lb->absolutePoint((*c.m_inlineBlockBoxStack)[i]).y() +
            lb->ascender();
    }
}

std::pair<bool, LayoutUnit> LayoutContext::registeredLastLineBoxYPosition(
    FrameBlockBox* box)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    STARFISH_ASSERT(c.m_inlineBlockBoxStack->back() == box);
    auto iter = (*c.m_registeredYPositionPerVAInlineBlock).find(box);
    if (iter == c.m_registeredYPositionPerVAInlineBlock->end()) {
        return std::pair<bool, LayoutUnit>(false, 0);
    }
    LayoutUnit r = iter->second;
    c.m_registeredYPositionPerVAInlineBlock->erase(iter);
    return std::pair<bool, LayoutUnit>(true, r);
}

void LayoutContext::registerAbsolutePositionedBox(Frame* frm)
{
    Frame* cb = containingFrameBlockBox(frm);
    m_absolutePositionedBoxes.emplace(cb, std::vector<FrameBox*>());
    auto& vec = m_absolutePositionedBoxes[cb];
    STARFISH_ASSERT(std::find(vec.begin(), vec.end(), frm) == vec.end());
    vec.push_back(frm->asFrameBox());
}

void LayoutContext::registerRelativePositionedBox(Frame* frm, bool dueToSelf)
{
    Frame* cb = containingFrameBlockBox(frm);
    m_relativePositionedBoxes.emplace(
        cb, std::vector<std::pair<FrameBox*, bool>>());
    auto& vec = m_relativePositionedBoxes[cb];
    vec.emplace_back(frm->asFrameBox(), dueToSelf);
}

Element* Frame::offsetParent()
{
    if (isDocumentElement() || isBodyElement()) {
        return nullptr;
    }

    Node* node = nullptr;
    for (Frame* parent = layoutParent(); parent;
         parent = parent->layoutParent()) {
        node = parent->node();

        if (!node) {
            continue;
        }

        if (parent->isPositioned()) {
            break;
        }

        if (node->isElement() && node->asElement()->isHTMLElement() &&
            node->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            break;
        }
    }

    return node && node->isElement() ? node->asElement() : nullptr;
}
}
