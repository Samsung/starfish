/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd

 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
#include "StarfishConfig.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/page/Window.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameGridBox.h"
#include "core/layout/FrameTableObjectBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/OverflowStatus.h"
#include "core/layout/StackingContext.h"
#include "core/style/CalcData.h"

namespace Starfish {

FrameBlockBox* blockContainer(Frame* currentFrame)
{
    Frame* f = currentFrame->layoutParent();

    if (!f || currentFrame->isFrameDocument()) {
        STARFISH_ASSERT(currentFrame->isFrameDocument());
        return currentFrame->asFrameBlockBox();
    }

    while (true) {
        if ((f->isFrameBlockBox() && !f->isAnonymous()) ||
            f->isFrameTableCellBox()) {
            return f->asFrameBlockBox();
        }
        f = f->layoutParent();
    }
}

FrameBlockBox* containingFrameBlockBox(Frame* currentFrame)
{
    FrameBlockBox* blockBox = blockContainer(currentFrame);
    if (currentFrame->isAbsolutePositioned()) {
        while (!blockBox->canBeContainingBlockOfAbsolutePositionedBox(
            currentFrame)) {
            blockBox = blockContainer(blockBox);
        }
        return blockBox;
    } else {
        return blockBox;
    }
}

FrameBox* containingBlock(Frame* currentFrame)
{
    // https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#containing-block-details
    if (currentFrame->isAbsolutePositioned()) {
        Frame* f = currentFrame->parent();
        if (!f) {
            if (currentFrame->isFrameDocument()) {
                return currentFrame->asFrameBox();
            }

            f = currentFrame->layoutParent();

            if (!f) {
                f = currentFrame->asFrameBox();
            }
        }

        while (!f->canBeContainingBlockOfAbsolutePositionedBox(currentFrame)) {
            f = f->parent();
        }

        if (f->isFrameBlockBox()) {
            return f->asFrameBlockBox();
        } else {
            STARFISH_ASSERT(f->isFrameInline());
            FrameBlockBox* c = blockContainer(f);
            FrameInline* in = f->asFrameInline();
            return c->firstInlineNonReplacedBox(in);
        }
    } else {
        FrameBlockBox* blockBox = blockContainer(currentFrame);
        return blockBox;
    }
}

FloatingBoxInfo::FloatingBoxInfo(FrameBox* box, LayoutContext* ctx)
    : m_box(box)
    , m_canLayoutParentCollapseWithMarginTop(false)
{
    STARFISH_ASSERT(box->isFloating());
    m_isLeft = box->style()->floating() == LeftFloatValue;
    for (Frame* parent = box->layoutParent(); parent;
         parent = parent->layoutParent()) {
        if (!parent->isAnonymous() && parent->isBlockLevel()) {
            if (parent->isFrameTableObjectBox() &&
                parent->asFrameTableObjectBox()->isInternalTableBox()) {
                //  Internal table elements do not have margins.
                continue;
            }
            m_canLayoutParentCollapseWithMarginTop =
                ctx->marginInfo(parent->asFrameBlockBox())
                    ->canCollapseWithMarginTop();
            break;
        }
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
    c.m_floatBoxes->erase(c.m_floatBoxes->begin() + from,
                          c.m_floatBoxes->end());
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
    FrameBox* cb = containingBlock(currentFrame);
    LayoutUnit w = cb->contentWidth();
    if (currentFrame->isAbsolutePositioned()) {
        w += cb->paddingWidth();
    }

    return w;
}

bool LayoutContext::parentHasFixedHeight(Frame* currentFrame)
{
    if (currentFrame->isAbsolutePositioned()) {
        return true;
    }
    FrameBlockBox* container = blockContainer(currentFrame);
    while (container) {
        Length height = container->style()->height();
        if (height.isDefinite(false)) {
            return true;
        }

        if (container->isAbsolutePositioned()) {
            if (height.isPercent() || height.isCalc()) {
                return true;
            }

            LengthData offset = container->style()->offset();
            if (offset.bottom().isSpecified() && offset.top().isSpecified()) {
                return true;
            }
        }

        if (height.isAuto()) {
            return false;
        }

        STARFISH_ASSERT(height.isPercent() || height.isCalc());
        container = blockContainer(container);
    }
    return false;
}

LayoutUnit LayoutContext::parentFixedHeight(Frame* currentFrame,
                                            bool isQuirksMode)
{
    if (currentFrame->isAbsolutePositioned()) {
        FrameBox* cb = containingBlock(currentFrame);
        return cb->contentHeight() + cb->paddingHeight();
    }
    FrameBlockBox* container = blockContainer(currentFrame);
    std::vector<std::pair<FrameBlockBox*, Length>> reverse;
    while (container) {
        Length height = container->style()->height();
        if (height.isDefinite(false)) {
            reverse.emplace_back(container, height);
            break;
        }

        if (container->isAbsolutePositioned()) {
            if (height.isCalc() || height.isPercent()) {
                LayoutUnit parentHeight =
                    containingBlock(container)->contentHeight();
                reverse.emplace_back(
                    container,
                    Length(Length::Fixed,
                           height.specifiedValue(parentHeight, container)));
                break;
            }

            LengthData offset = container->style()->offset();
            if (offset.top().isSpecified() && offset.bottom().isSpecified()) {
                LayoutUnit parentHeight =
                    containingBlock(container)->contentHeight();
                LayoutUnit t =
                    offset.top().specifiedValue(parentHeight, container);
                LayoutUnit b =
                    offset.bottom().specifiedValue(parentHeight, container);
                LayoutUnit height;
                if (container->style()->boxSizing() ==
                    BorderBoxBoxSizingValue) {
                    height = parentHeight - t - b;
                } else {
                    height = parentHeight - t - b - container->paddingHeight() -
                             container->borderHeight();
                }

                reverse.emplace_back(container, Length(Length::Fixed, height));
                break;
            }
        }

        if (isQuirksMode) {
            if (!height.isAuto()) {
                reverse.emplace_back(container, height);
            }
        } else {
            STARFISH_ASSERT(height.isPercent() || height.isCalc());
            reverse.emplace_back(container, height);
        }
        container = blockContainer(container);
    }
    Length height = reverse.back().second;
    container = reverse.back().first;
    LayoutUnit result;
    if (height.isDefinite(false)) {
        LayoutUnit unused;
        result = height.specifiedValue(unused, container);
    }

    result = container->contentHeightAfterApplyingBoxSizing(result);
    reverse.pop_back();
    while (reverse.size()) {
        height = reverse.back().second;
        container = reverse.back().first;
        result = height.specifiedValue(result, container);
        result = container->contentHeightAfterApplyingBoxSizing(result);
        reverse.pop_back();
    }

    return result;
}

LayoutUnit LayoutContext::specifiedVerticalValue(Frame* f, Length l)
{
    bool parentHasFixedHeight = this->parentHasFixedHeight(f);
    if (l.isDefinite(parentHasFixedHeight)) {
        LayoutUnit parentContentHeight;
        if (parentHasFixedHeight) {
            parentContentHeight = this->parentFixedHeight(f);
        }
        return l.specifiedValue(parentContentHeight, f);
    }

    return 0;
}

void LayoutContext::advanceLineBoxAscender(LayoutUnit a)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    for (size_t i = 0; i < c.m_inlineBlockBoxStack->size(); i++) {
        FrameBlockBox* blockBoxInStack = (*c.m_inlineBlockBoxStack)[i];
        auto& lineBoxAscenders = (*c.m_lineBoxAscenders);
        auto iter = lineBoxAscenders.begin();
        while (iter != lineBoxAscenders.end()) {
            iter->second += a;
            iter++;
        }
    }
}

void LayoutContext::registerLineBoxAscender(FrameBlockBox* blockBox,
                                            LineBox* lb, LayoutUnit ascender)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    for (size_t i = 0; i < c.m_inlineBlockBoxStack->size(); i++) {
        FrameBlockBox* blockBoxInStack = (*c.m_inlineBlockBoxStack)[i];
        auto& lineBoxAscenders = (*c.m_lineBoxAscenders);
        if (blockBoxInStack->style()->verticalAlign() == BaselineVAlignValue ||
            blockBoxInStack->style()->verticalAlign() == NumericVAlignValue) {
            if (blockBoxInStack->isFrameFlexibleBox() ||
                blockBoxInStack->isFrameGridBox() ||
                blockBoxInStack->isFrameTableCellBox()) {
                auto iter = lineBoxAscenders.find(blockBoxInStack);
                if (iter == lineBoxAscenders.end()) {
                    lineBoxAscenders[blockBoxInStack] =
                        lb->absolutePoint(blockBoxInStack).y() + ascender;
                }
            } else if (blockBoxInStack->style()->display() ==
                       InlineBlockDisplayValue) {
                lineBoxAscenders[blockBoxInStack] =
                    lb->absolutePoint(blockBoxInStack).y() + ascender;
            }
        }
    }

    registerFirstLineAscender(blockBox, lb, ascender);
}

Nullable<LayoutUnit> LayoutContext::lineBoxAscender(FrameBlockBox* blockBox)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    STARFISH_ASSERT(c.m_inlineBlockBoxStack->back() == blockBox);
    auto iter = c.m_lineBoxAscenders->find(blockBox);
    if (iter == c.m_lineBoxAscenders->end()) {
        return Nullable<LayoutUnit>();
    }
    LayoutUnit r = iter->second;
    c.m_lineBoxAscenders->erase(iter);
    return r;
}

void LayoutContext::registerFirstLineAscender(FrameBlockBox* owner,
                                              LineBox* lineBox,
                                              LayoutUnit ascender)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    for (size_t i = 0; i < c.m_blockBoxAligningAtFirstBaselineStack->size();
         i++) {
        FrameBlockBox* blockBox =
            (*c.m_blockBoxAligningAtFirstBaselineStack)[i];
        if ((blockBox->isFlexItem()) || (blockBox->isFrameTableCellBox() &&
                                         !owner->isFrameTableCaptionBox())) {
            auto iter = c.m_firstLineAscenders->find(blockBox);
            if (iter == c.m_firstLineAscenders->end()) {
                // TODO: Because of the table's specific implementation,
                // we can't make use of line ascender in share.
                // So here we make different version of saving ascender only.

                size_t index = 0;
                for (size_t i = 0; i < owner->lineBoxes().size(); i++) {
                    if (owner->lineBoxes()[i] == lineBox) {
                        index = i;
                        break;
                    }
                }

                AscenderInfo info;
                info.m_block = owner;
                info.m_lineIndex = index;

                (*c.m_firstLineAscenders)[blockBox] =
                    std::make_pair(info, ascender);
            }
        }
    }
}

static LineBox* findLineBox(FrameBlockBox* fb, size_t index)
{
    const auto& lb = fb->lineBoxes();
    if (index < lb.size()) {
        return lb[index];
    }
    return nullptr;
}

Nullable<std::pair<LineBox*, LayoutUnit>> LayoutContext::firstLineAscender(
    FrameBlockBox* blockBox)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    STARFISH_ASSERT(c.m_blockBoxAligningAtFirstBaselineStack->back() ==
                    blockBox);
    auto iter = c.m_firstLineAscenders->find(blockBox);
    if (iter == c.m_firstLineAscenders->end()) {
        return Nullable<std::pair<LineBox*, LayoutUnit>>();
    }

    auto l = iter->second;
    c.m_firstLineAscenders->erase(iter);

    LineBox* lb = findLineBox(l.first.m_block, l.first.m_lineIndex);

    if (lb) {
        return Nullable<std::pair<LineBox*, LayoutUnit>>(
            std::make_pair(lb, l.second));
    }

    return Nullable<std::pair<LineBox*, LayoutUnit>>();
}

void LayoutContext::tempReigsterFirstLineAscender(
    FrameTableCellBox* cellBox, std::pair<LineBox*, LayoutUnit> ascenderInfo)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();

    auto iter = (*c.m_tempAscenders).find(cellBox);

    if (iter != (*c.m_tempAscenders).end()) {
        (*c.m_tempAscenders).erase(iter);
    }

    FrameBlockBox* owner =
        ascenderInfo.first->layoutParent()->asFrameBlockBox();
    size_t index = 0;
    for (size_t i = 0; i < owner->lineBoxes().size(); i++) {
        if (owner->lineBoxes()[i] == ascenderInfo.first) {
            index = i;
            break;
        }
    }

    AscenderInfo info;
    info.m_block = owner;
    info.m_lineIndex = index;

    (*c.m_tempAscenders)
        .insert(
            std::make_pair(cellBox, std::make_pair(info, ascenderInfo.second)));
}

Nullable<std::pair<LineBox*, LayoutUnit>> LayoutContext::tempFirstLineAscender(
    FrameTableCellBox* cellBox)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    auto it = c.m_tempAscenders->find(cellBox);
    if (it == c.m_tempAscenders->end()) {
        return Nullable<std::pair<LineBox*, LayoutUnit>>();
    }

    LineBox* lb =
        findLineBox(it->second.first.m_block, it->second.first.m_lineIndex);

    if (lb) {
        return Nullable<std::pair<LineBox*, LayoutUnit>>(
            std::make_pair(lb, it->second.second));
    }

    return Nullable<std::pair<LineBox*, LayoutUnit>>();
}

Nullable<PreferredWidthValue> LayoutContext::preferredWidthInfo(
    PreferredWidthKey key)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    auto it = c.m_preferredWidthValues->find(key);
    if (it == c.m_preferredWidthValues->end()) {
        return Nullable<PreferredWidthValue>();
    }

    return it->second;
}

void LayoutContext::registerPreferredWidthInfo(PreferredWidthKey key,
                                               PreferredWidthValue value)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    (*c.m_preferredWidthValues)[key] = value;
}

void LayoutContext::registerAbsolutePositionedBox(FrameBox* box)
{
    FrameBlockBox* cb = containingFrameBlockBox(box);
    m_absolutePositionedBoxes.emplace(cb, std::vector<FrameBox*>());
    auto& vec = m_absolutePositionedBoxes[cb];
    vec.push_back(box);
}

void LayoutContext::layoutRegisteredAbsolutePositionedBoxes(
    FrameBlockBox* containingBlock)
{
    auto iter = m_absolutePositionedBoxes.find(containingBlock);
    if (iter == m_absolutePositionedBoxes.end()) {
        return;
    } else {
        const auto& boxes = iter->second;
        for (size_t i = 0; i < boxes.size(); i++) {
            FrameBox* box = boxes[i];
            box->layout(*this, Frame::LayoutWantToResolve::ResolveAll);
        }
        m_absolutePositionedBoxes.erase(iter);
    }
}

void LayoutContext::addToRelativePositionedBoxes(FrameBox* box, bool dueToSelf)
{
    FrameBlockBox* cb = containingFrameBlockBox(box);
    m_relativePositionedBoxes.emplace(
        cb, std::vector<std::pair<FrameBox*, bool>>());
    auto& vec = m_relativePositionedBoxes[cb];

    bool has = false;
    for (size_t i = 0; i < vec.size(); i++) {
        if (vec[i].first == box) {
            has = true;
            break;
        }
    }
    if (!has) {
        vec.emplace_back(box, dueToSelf);
    }
}

void LayoutContext::layoutRelativePositionedBox(FrameBox* box, bool dueToSelf)
{
    if (dueToSelf) {
        applyRelativePosition(box);
    } else {
        Element* elm = box->node()->parentElement();
        while (elm && elm->frame()->isFrameInline() &&
               elm->style()->position() == RelativePositionValue) {
            applyRelativePositionInlineCase(elm->frame(), box);
            elm = elm->parentElement();
        }
    }
}

void LayoutContext::layoutRegisteredRelativePositionedBoxes(
    FrameBlockBox* containingBlock)
{
    auto iter = m_relativePositionedBoxes.find(containingBlock);
    if (iter == m_relativePositionedBoxes.end()) {
        return;
    } else {
        const auto& boxes = iter->second;
        for (size_t i = 0; i < boxes.size(); i++) {
            FrameBox* box = boxes[i].first;
            layoutRelativePositionedBox(box, boxes[i].second);
        }
        m_relativePositionedBoxes.erase(iter);
    }
}

bool LayoutContext::checkIfThisIsFirstLineCandidate(FrameBlockBox* blockBox)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    auto& firstLineCandidates = c.m_firstLineCandidates;
    Frame* parent = blockBox->parent();

    auto it = firstLineCandidates->find(parent);
    if (it == firstLineCandidates->end()) {
        return true;
    }

    return (*it).second == blockBox;
}

void LayoutContext::registerFirstLineCandidate(FrameBlockBox* blockBox)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    auto& firstLineCandidates = c.m_firstLineCandidates;
    Frame* parent = blockBox->parent();

    auto it = firstLineCandidates->find(parent);
    if (it == firstLineCandidates->end()) {
        (*firstLineCandidates)[parent] = blockBox;
    }
}

void LayoutContext::registerContentHeight(FrameBox* box,
                                          LayoutUnit contentHeight)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    (*c.m_contentHeights)[box] = contentHeight;
}

LayoutUnit LayoutContext::contentHeight(FrameBox* box)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    auto iter = c.m_contentHeights->find(box);
    if (iter == c.m_contentHeights->end()) {
        return intMaxForLayoutUnit;
    }
    return iter->second;
}

void LayoutContext::pushIntoLineBoxPool(LineBox* b)
{
    memset(b, 0, sizeof(LineBox));
    m_lineBoxPool.push_back(b);
}

void LayoutContext::pushIntoInlineTextBoxPool(InlineTextBox* b)
{
    memset(b, 0, sizeof(InlineTextBox));
    m_inlineTextBoxPool.push_back(b);
}

void LayoutContext::pushIntoInlineNonReplacedBoxPool(InlineNonReplacedBox* b)
{
    memset(b, 0, sizeof(InlineNonReplacedBox));
    m_inlineNonReplacedBoxPool.push_back(b);
}

void LayoutContext::
    applyInvertOffsetBeforeApplyingRelativePositionInQuickLayout(FrameBox* box)
{
    STARFISH_ASSERT(box);
    STARFISH_ASSERT(box->style()->position() ==
                    PositionValue::RelativePositionValue);

    FrameBlockBox* cb = containingFrameBlockBox(box);
    m_relativePositionedBoxes.emplace(
        cb, std::vector<std::pair<FrameBox*, bool>>());
    auto& vec = m_relativePositionedBoxes[cb];

    bool has = false;
    for (size_t i = 0; i < vec.size(); i++) {
        if (vec[i].first == box) {
            has = true;
            break;
        }
    }
    if (!has) {
        LayoutUnit orgX = box->x();
        LayoutUnit orgY = box->y();

        bool dueToSelf = true;
        if (box->node() && box->node()->parentElement()) {
            Node* nd = box->node()->parentElement();
            if (nd->frame()->isFrameInline() &&
                nd->style()->position() == RelativePositionValue) {
                dueToSelf = false;
            }
        }

        layoutRelativePositionedBox(box, dueToSelf);

        box->setX(orgX - (box->x() - orgX));
        box->setY(orgY - (box->y() - orgY));
    }
}

Nullable<LayoutUnit> LayoutContext::testBasisSizeCache(
    Frame* flexItem, bool isMainAxisInInlineAxis,
    LayoutUnit availableMainCrossSize,
    bool shouldRespectPercentageWidthOnComputingBasisSize)
{
    auto iter = m_basisSizeCache.find(flexItem);
    if (iter != m_basisSizeCache.end()) {
        LayoutContext::CachedBasisSizeVector& v = iter->second;
        for (size_t i = 0; i < v.size(); i++) {
            if (std::get<0>(v[i]) == availableMainCrossSize) {
                if (!(std::get<1>(v[i]).m_seenPercentageWidth)) {
                    return std::get<2>(v[i]);
                }

                if (isMainAxisInInlineAxis) {
                    if (std::get<1>(v[i])
                            .m_shouldRespectPercentageWidthOnComputingBasisSize ==
                        shouldRespectPercentageWidthOnComputingBasisSize) {
                        return std::get<2>(v[i]);
                    }
                }
            }
        }
    }
    return Nullable<LayoutUnit>();
}

void LayoutContext::registerToBasisSizeCache(
    Frame* flexItem, LayoutUnit availableMainCrossSize,
    bool seenPercentageWidth,
    bool shouldRespectPercentageWidthOnComputingBasisSize, LayoutUnit basisSize)
{
    auto iter = m_basisSizeCache.find(flexItem);
    if (iter != m_basisSizeCache.end()) {
        LayoutContext::CachedBasisSizeVector& v = iter->second;
        v.push_back(std::make_tuple(
            availableMainCrossSize,
            CachedBasisSizeFlags(
                seenPercentageWidth,
                shouldRespectPercentageWidthOnComputingBasisSize),
            basisSize));
    } else {
        LayoutContext::CachedBasisSizeVector v;
        v.push_back(std::make_tuple(
            availableMainCrossSize,
            CachedBasisSizeFlags(
                seenPercentageWidth,
                shouldRespectPercentageWidthOnComputingBasisSize),
            basisSize));
        m_basisSizeCache.insert(std::make_pair(flexItem, std::move(v)));
    }
}

void LayoutContext::unregisterToBasisSizeCache(
    Frame* flexItem, LayoutUnit availableMainCrossSize)
{
    auto iter = m_basisSizeCache.find(flexItem);
    if (iter != m_basisSizeCache.end()) {
        LayoutContext::CachedBasisSizeVector& v = iter->second;
        auto iter2 = v.begin();
        while (iter2 != v.end()) {
            if (std::get<0>(*iter2) == availableMainCrossSize) {
                iter2 = v.erase(iter2);
            } else {
                iter2++;
            }
        }
    }
}

Nullable<LayoutUnit> LayoutContext::testGridItemPreferredWidthCache(
    Frame* gridItem, LayoutUnit availableWidth)
{
    auto iter = m_gridItemPreferredWidthCache.find(gridItem);
    if (iter != m_gridItemPreferredWidthCache.end()) {
        LayoutContext::CachedGridItemPreferredWidthVector& v = iter->second;
        for (size_t i = 0; i < v.size(); i++) {
            if (std::get<0>(v[i]) == availableWidth) {
                return std::get<1>(v[i]);
            }
        }
    }
    return Nullable<LayoutUnit>();
}

void LayoutContext::registerToGridItemPreferredWidthCache(
    Frame* gridItem, LayoutUnit availableWidth, LayoutUnit preferredWidth)
{
    auto iter = m_gridItemPreferredWidthCache.find(gridItem);
    if (iter != m_gridItemPreferredWidthCache.end()) {
        LayoutContext::CachedGridItemPreferredWidthVector& v = iter->second;
        v.push_back(std::make_tuple(availableWidth, preferredWidth));
    } else {
        LayoutContext::CachedGridItemPreferredWidthVector v;
        v.push_back(std::make_tuple(availableWidth, preferredWidth));
        m_gridItemPreferredWidthCache.insert(
            std::make_pair(gridItem, std::move(v)));
    }
}

PreferredWidthContext& PreferredWidthContext::nearestFloatContext()
{
    PreferredWidthContext* c = this;
    while (c) {
        if (c->m_frame->needToEstablishBlockFormattingContext()) {
            return *c;
        }
        c = c->m_upperContext;
    }
    return *this;
}

Frame::ComputeVisibleRectContext::ComputeVisibleRectContext(
    ComputePurpose purpose, StackingContext* sourceStackingContext,
    SkMatrix& tranformMatrix, LayoutRect& result)
    : purpose(purpose)
    , ignoreTransformOnce(purpose >= GraphicsBufferBySelf ? true : false)
    , isForSpecialValueForTableCell(false)
    , isVisibleRectCollapsible(purpose >= GraphicsBufferBySelf)
    , sourceStackingContext(sourceStackingContext)
    , sourceFrameBox(sourceStackingContext->owner())
    , tranformMatrix(tranformMatrix)
    , result(result)
{
    if (sourceStackingContext->owner()->shouldApplyOverflow()) {
        LayoutRect rt = sourceStackingContext->owner()->frameVisibleRect();

        boundMaxExtentDueToOverflow.push_back(
            std::make_tuple(computeBoxExtent(rt, SkMatrix::I()),
                            sourceStackingContext->owner()));
    }
}

void Frame::ComputeVisibleRectContext::uniteRect(const LayoutRect& r)
{
    LayoutRect tmp = computeBoxExtent(r, tranformMatrix);

    // TODO CanvasStateRestorer, CompositorStateRestorer,
    // Frame::ComputeVisibleRectContext::uniteRect have same source

    if (boundMaxExtentDueToOverflow.size()) {
        auto iter = fragmentBoxStack.rbegin();
        OverflowStatus status(*iter);

        bool shareWithStackingBuffer = true;
        while (iter != fragmentBoxStack.rend()) {
            FrameBox* f = (*iter);
            iter++;

            if (shareWithStackingBuffer && f &&
                f->asFrameBox()->stackingContext() &&
                f->asFrameBox()->stackingContext()->needsGraphicsBuffer() &&
                f->asFrameBox()->stackingContext()->isAncestorOf(
                    sourceStackingContext)) {
                shareWithStackingBuffer = false;
            }

            if (shareWithStackingBuffer) {
                if (status.canApplyOverflow(f, false)) {
#ifndef NDEBUG
                    bool finded = false;
#endif
                    for (size_t i = 0; i < boundMaxExtentDueToOverflow.size();
                         i++) {
                        if (std::get<1>(boundMaxExtentDueToOverflow[i]) == f) {
#ifndef NDEBUG
                            finded = true;
#endif
                            tmp = LayoutRect::overlappedRect(
                                tmp,
                                std::get<0>(boundMaxExtentDueToOverflow[i]));
                            break;
                        }
                    }

#ifndef NDEBUG
                    STARFISH_ASSERT(finded);
#endif
                    status.reset(f);
                }
            }
        }

        // test source has buffer & overflow
        if (sourceStackingContext && purpose >= GraphicsBufferBySelf &&
            sourceFrameBox->shouldApplyOverflow()) {
            STARFISH_ASSERT(std::get<1>(boundMaxExtentDueToOverflow[0]) ==
                            sourceFrameBox);
            tmp = LayoutRect::overlappedRect(
                tmp, std::get<0>(boundMaxExtentDueToOverflow[0]));
        }
    }

    auto prevValue = result;
    result.unite(tmp);

    if (ComputePurpose::Scrolling == purpose) {
        if (prevValue.maxX() != result.maxX()) {
            result.setWidth(result.width() + sourceFrameBox->paddingRight());
        }

        if (prevValue.maxY() != result.maxY()) {
            result.setHeight(result.height() + sourceFrameBox->paddingBottom());
        }
    }
}

Frame::ComputeVisibleRectContextFragment::ComputeVisibleRectContextFragment(
    ComputeVisibleRectContext& ctx, FrameBox* fragmentBox)
    : ctx(ctx)
    , fragmentBox(fragmentBox)
    , transformMatrixBefore(ctx.tranformMatrix)
    , shouldStopComputingBecauseMatrixInvalidFromHere(false)
    , overflowWasApplyed(false)
{
    if (ctx.fragmentBoxStack.size() &&
        ctx.fragmentBoxStack.back()->isFrameBlockBox()) {
        ctx.tranformMatrix.preTranslate(
            -ctx.fragmentBoxStack.back()->asFrameBlockBox()->scrollLeft(),
            -ctx.fragmentBoxStack.back()->asFrameBlockBox()->scrollTop());
    }
    ctx.fragmentBoxStack.push_back(fragmentBox);

    if (ctx.ignoreTransformOnce) {
        ctx.ignoreTransformOnce = false;
        return;
    }

    ComputedStyle* cs = fragmentBox->style();
    if (cs && cs->hasTransforms(fragmentBox)) {
        SkMatrix m;
        if (ctx.purpose >=
            Frame::ComputeVisibleRectContext::GraphicsBufferBySelf) {
            m = fragmentBox->stackingContext()->transformMatrix();
        } else {
            m = cs->transformsToMatrix(
                fragmentBox->width(), fragmentBox->height(), fragmentBox, true);
        }
        // If Frame has `skew, rotate, 3d-transform`, Frame must own it's
        // graphics buffer.
        // If matrix is not rect, we should ignore child visible rects from here
        SkMatrix test;
        if (!m.invert(&test)) {
            shouldStopComputingBecauseMatrixInvalidFromHere = true;
            return;
        }

        LayoutLocation to;
        if (ctx.purpose >=
            Frame::ComputeVisibleRectContext::GraphicsBufferBySelf) {
            to = fragmentBox->stackingContext()->transformOrigin();
        } else {
            StyleTransformOrigin* origin = cs->transformOrigin();
            auto od = origin->originValue();
            to.setX(od->getXAxis().specifiedValue(fragmentBox->width(),
                                                  fragmentBox));
            to.setY(od->getYAxis().specifiedValue(fragmentBox->height(),
                                                  fragmentBox));
        }

        ctx.tranformMatrix.preTranslate((float)fragmentBox->x(),
                                        (float)fragmentBox->y());
        ctx.tranformMatrix.preTranslate((float)to.x(), (float)to.y());
        ctx.tranformMatrix.preConcat(m);
        ctx.tranformMatrix.preTranslate((float)-to.x(), (float)-to.y());

        if (!ctx.tranformMatrix.rectStaysRect()) {
            shouldStopComputingBecauseMatrixInvalidFromHere = true;
            return;
        }
    } else {
        ctx.tranformMatrix.preTranslate((float)fragmentBox->x(),
                                        (float)fragmentBox->y());
    }

    if (fragmentBox->shouldApplyOverflow()) {
        overflowWasApplyed = true;
        ctx.boundMaxExtentDueToOverflow.push_back(
            std::make_tuple(computeBoxExtent(fragmentBox->frameVisibleRect(),
                                             ctx.tranformMatrix),
                            fragmentBox));
    }
}
Frame::ComputeVisibleRectContextFragment::~ComputeVisibleRectContextFragment()
{
    ctx.fragmentBoxStack.pop_back();

    ctx.tranformMatrix = transformMatrixBefore;
    if (overflowWasApplyed) {
        ctx.boundMaxExtentDueToOverflow.pop_back();
    }
}

Frame::Frame(Node* node, ComputedStyle* s)
{
    bool isAnonymous;
    if (node) {
        m_node = node;
        isAnonymous = false;
    } else if (s) {
        STARFISH_ASSERT(node == nullptr);
        m_styleWhenNodeIsAnonymous = s;
        isAnonymous = true;
    } else {
        m_node = nullptr;
        isAnonymous = true;
    }

    m_flags.m_isAnonymous = isAnonymous;
    m_flags.m_isLeftMBPCleared = false;
    m_flags.m_isRightMBPCleared = false;

    m_flags.m_needsGraphicsBuffer = false;
    m_flags.m_isFrameText = false;
    m_flags.m_heightComputed = false;
    m_flags.m_hasBiggerContentThanFrameWidth = false;
    m_flags.m_hasBiggerContentThanFrameHeight = false;
    m_flags.m_needsToComputeScrollVisbleRect = false;
    m_flags.m_isFirstLine = false;
    m_flags.m_shouldApplyOverflow = false;
    m_flags.m_seenNormalFlowBlockChild = false;
    m_flags.m_seenNonPositionedFloats = false;
    m_flags.m_seenReplacedBlock = false;
    m_flags.m_seenNormalFlowInline = false;
    m_flags.m_seenNormalFlowInlineBox = false;
    m_flags.m_seenNormalFlowInlineBlockBox = false;
    m_flags.m_seenNormalFlowInlineReplaced = false;
    m_flags.m_contentWidthDamaged = false;
    m_flags.m_paddingWidthDamaged = false;
    m_flags.m_contentHeightDamaged = false;
    m_flags.m_paddingHeightDamaged = false;
    m_flags.m_needToEstablishBlockFormattingContext = false;

    m_flags.m_isFlexItem = false;
    m_flags.m_isGridItem = false;

    computeStyleFlags();

    if (needToEstablishKindsOfFormattingContext()) {
        m_flags.m_needsLayout = true;
    }
    m_flags.m_needsPainting = true;
}

void Frame::computePaintingFlags()
{
    PaintingKind kind;

    if (isInlineLevel() || isFlexItem()) {
        kind = NormalFlowInline;
    } else if (isFloating()) {
        kind = NonPositionedFloats;
    } else if (isBlockLevel() && isFrameReplaced()) {
        kind = ReplacedBlock;
    } else {
        kind = NormalFlowBlockChild;
    }

    seenPaintingKind(kind);
    if (kind != NormalFlowInline && isFrameBlockBox() &&
        !asFrameBlockBox()->hasBlockFlow()) {
        seenPaintingKind(NormalFlowInline);
    }
}

void Frame::seenPaintingKind(PaintingKind kind)
{
    Frame* f = this;
    while (f) {
        if (kind == NormalFlowInline) {
            f->m_flags.m_seenNormalFlowInline = true;
        } else if (kind == NonPositionedFloats) {
            f->m_flags.m_seenNonPositionedFloats = true;
        } else if (kind == ReplacedBlock) {
            f->m_flags.m_seenReplacedBlock = true;
        } else {
            f->m_flags.m_seenNormalFlowBlockChild = true;
        }

        if (f->needToEstablishStackingContext()) {
            break;
        }

        f = f->layoutParent();
    }
}

void Frame::computeShouldApplyOverflow()
{
    Node* thisNode = node();
    if (thisNode /* !isAnonymous() */) {
        if (thisNode->isHTMLHtmlElement()) {
            m_flags.m_shouldApplyOverflow = false;
            return;
        } else if (thisNode->isHTMLBodyElement()) {
            HTMLHtmlElement* html = thisNode->document()->rootElement();
            if (html->style()->overflowX() == OverflowValue::VisibleOverflow &&
                html->style()->overflowY() == OverflowValue::VisibleOverflow) {
                m_flags.m_shouldApplyOverflow = false;
                return;
            }
        }
    }

    ComputedStyle* cs = style();
    if (cs) {
        m_flags.m_shouldApplyOverflow =
            (cs->overflowX() != OverflowValue::VisibleOverflow) ||
            (cs->overflowY() != OverflowValue::VisibleOverflow);
    } else {
        m_flags.m_shouldApplyOverflow = false;
    }

    if (cs && cs->display() == DisplayValue::InlineDisplayValue) {
        m_flags.m_shouldApplyOverflow = false;
    }
}

bool Frame::isRootElement() const
{
    return node() && node()->isHTMLHtmlElement();
}

bool Frame::isPositioned()
{
    ComputedStyle* style = this->style();
    if (!style) {
        return false;
    }

    return style->position() != PositionValue::StaticPositionValue;
}

bool Frame::isSpecifiedZIndex()
{
    ComputedStyle* style = this->style();
    if (!style) {
        return false;
    }

    return style->isSpecifiedZIndex();
}

bool Frame::isAbsolutePositioned()
{
    ComputedStyle* style = this->style();
    if (!style) {
        return false;
    }

    return style->isAbsolutePositioned();
}

bool Frame::isFloating()
{
    ComputedStyle* style = this->style();
    if (!style) {
        return false;
    }

    return style->floating() != FloatValue::NoneFloatValue;
}

void Frame::computeStyleFlags()
{
    Node* node = this->node();
    bool isRootElement = node && node->isHTMLHtmlElement();
    m_flags.m_needToEstablishBlockFormattingContext = isRootElement;
    m_flags.m_needToEstablishStackingContext = isRootElement;

    computeShouldApplyOverflow();

    ComputedStyle* style = Frame::style();
    if (!style) {
        return;
    }

    PositionValue position = style->position();
    bool isAbsolutePositioned =
        (position == PositionValue::AbsolutePositionValue ||
         position == PositionValue::FixedPositionValue);
    bool isFloating = (style->floating() != FloatValue::NoneFloatValue);

    // TODO add condition
    m_flags.m_needToEstablishBlockFormattingContext |= (shouldApplyOverflow());
    m_flags.m_needToEstablishBlockFormattingContext |= isFlexItem();
    m_flags.m_needToEstablishBlockFormattingContext |= isGridItem();
    m_flags.m_needToEstablishBlockFormattingContext |= isAbsolutePositioned;
    m_flags.m_needToEstablishBlockFormattingContext |= isFloating;
    m_flags.m_needToEstablishBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::InlineBlockDisplayValue);
    m_flags.m_needToEstablishBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::TableCellDisplayValue);
    m_flags.m_needToEstablishBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::TableCaptionDisplayValue);
    m_flags.m_needToEstablishBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::FlexDisplayValue);
    m_flags.m_needToEstablishBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::InlineFlexDisplayValue);
    // https://www.w3.org/TR/html5/rendering.html#the-fieldset-and-legend-elements
    m_flags.m_needToEstablishBlockFormattingContext |=
        (!isAnonymous() && node->isHTMLFieldSetElement());

    // https://www.w3.org/TR/2011/REC-CSS2-20110607/tables.html#model
    // The table wrapper box establishes a block formatting context
    m_flags.m_needToEstablishBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::TableDisplayValue);
    m_flags.m_needToEstablishBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::InlineTableDisplayValue);

    // TODO add condition
    // NOTE
    // https://www.w3.org/TR/CSS2/zindex.html
    // Appendix E. Elaborate description of Stacking Contexts
    // All positioned descendants with 'z-index: auto' or 'z-index: 0', in
    // tree order. For those with 'z-index: auto', treat the element as if
    // it created a new stacking context.
    m_flags.m_needToEstablishStackingContext |=
        (position != PositionValue::StaticPositionValue);
    m_flags.m_needToEstablishStackingContext |=
        isFlexItem() && style->isSpecifiedZIndex();
    m_flags.m_needToEstablishStackingContext |= (style->opacity() != 1);
    m_flags.m_needToEstablishStackingContext |=
        (node && node->isRunningOpacityAnimation());
    m_flags.m_needToEstablishStackingContext |= (style->hasTransforms(this));
    m_flags.m_needToEstablishStackingContext |=
        (node && node->isRunningTransformAnimation());
    m_flags.m_needToEstablishStackingContext |= (style->hasAvailableFilter());
    auto wc = style->willChange();
    if (wc && (wc->transform() || wc->opacity())) {
        m_flags.m_needToEstablishStackingContext |= true;
    }
    m_flags.m_needToEstablishStackingContext |= (style->maskLayerSize() > 0);

    // TODO add condition
    m_flags.m_needsGraphicsBuffer = (style->has3DTransforms(this));
    if (wc && (wc->transform() || wc->opacity())) {
        m_flags.m_needsGraphicsBuffer |= true;
    }
}

Node* Frame::nodeSlowCase() const
{
    STARFISH_ASSERT(isFrameText());
    FrameText* self = const_cast<Frame*>(this)->asFrameText();
    if (self->hasRareData()) {
        return self->frameTextRareData()->m_node;
    }
    if (self->isAnonymous()) {
        return nullptr;
    }
    return m_node;
}

Frame* Frame::firstLinePseudoComputedStyleOwnerFrame()
{
    STARFISH_ASSERT(isFrameBlockBox());
    Frame* firstLineFrame = this;
    bool hasPseudo = false;

    while (true) {
        if (!firstLineFrame->isAnonymous() &&
            !firstLineFrame->isFrameDocument()) {
            STARFISH_ASSERT(firstLineFrame->node()->isElement());
            hasPseudo = firstLineFrame->style()->seenPseudoElement(
                PseudoElementType::PseudoElementFirstLine);
        }
        if (hasPseudo) {
            break;
        }

        Frame* parentFrame = firstLineFrame->parent();
        if (firstLineFrame->isAtomicInlineLevel() ||
            !firstLineFrame->isNormalFlow() || !parentFrame ||
            !parentFrame->canHaveFirstLineOrFirstLetterStyle()) {
            break;
        }

        STARFISH_ASSERT(parentFrame->isFrameBlockBox());

        Frame* child = parentFrame->firstChild();
        if (child->isAnonymous() && child->firstChild()->isFrameText()) {
            String* text = child->firstChild()->asFrameText()->text();
            if (text->containsOnlyWhitespace()) {
                child = child->next();
            }
        }

        if (child != firstLineFrame) {
            break;
        }

        firstLineFrame = parentFrame;
    }

    if (!hasPseudo) {
        return nullptr;
    }
    return firstLineFrame;
}

ComputedStyle* Frame::createFirstLinePseudoComputedStyle(
    PseudoElementType pseudoId, ComputedStyle* parentStyle)
{
    STARFISH_ASSERT(node());
    STARFISH_ASSERT(node()->isElement());
    STARFISH_ASSERT(
        (pseudoId == PseudoElementType::PseudoElementFirstLine) ||
        (pseudoId == PseudoElementType::PseudoElementFirstLineInherited));

    if (!node()->style()->seenPseudoElement(pseudoId)) {
        return nullptr;
    }
    if (!parentStyle) {
        parentStyle = style();
    }

    Node* n = node();
    while (n) {
        if (n->isElement()) {
            break;
        }
        n = n->parentNode();
    }

    if (!n) {
        return nullptr;
    }

    Element* element = n->asElement();
    ComputedStyle* result = new ComputedStyle(parentStyle);
    StyleResolveContext ctx(document());
    if (pseudoId == PseudoElementType::PseudoElementFirstLine) {
        document()->styleResolver().matchAllRules(
            ctx, element, result, parentStyle,
            PseudoElementType::PseudoElementFirstLine);
    } else {
        ComputedStyle::InheritedStyles orgInheritedStyles =
            parentStyle->m_inheritedStyles;
        document()->styleResolver().matchAllRules(
            ctx, element, result, parentStyle,
            PseudoElementType::PseudoElementFirstLine);
        Unit::Color computedColor = result->color();
        result->m_inheritedStyles = orgInheritedStyles;
        if (parentStyle->m_gotInheritedColor) {
            result->m_inheritedStyles.m_color = computedColor;
        }

        result->setPseudoType(
            PseudoElementType::PseudoElementFirstLineInherited);
    }
    Length fontSize = result->fontSize();
    fontSize.changeToFixedIfNeeded(
        parentStyle->fontSize(),
        element->document()->rootElement()->style()->fontSize(),
        parentStyle->font(), element->window()->innerWidth(),
        element->window()->innerHeight(), result);
    result->setFontSize(fontSize);
    result->setDisplay(DisplayValue::InlineDisplayValue);
    result->setPosition(PositionValue::StaticPositionValue);
    result->loadResources(element);
    result->arrangeStyleValues(parentStyle, element);

    return result;
}

ComputedStyle* Frame::cachedPseudoStyle(PseudoElementType pseudo,
                                        ComputedStyle* parentStyle)
{
    if (node() && node()->isElement()) {
        if (pseudo == PseudoElementType::PseudoElementFirstLine ||
            pseudo == PseudoElementType::PseudoElementFirstLineInherited) {
            auto c = style()->cachedPseudoStyle(pseudo);
            if (c == nullptr) {
                auto s =
                    createFirstLinePseudoComputedStyle(pseudo, parentStyle);
                style()->addCachedPseudoStyle(s);
                return s;
            } else {
                return c;
            }
        }
        return style()->pseudoStyle(node()->asElement(), pseudo);
    }
    return nullptr;
}

static ComputedStyle* firstLineStyleFromCache(Frame* frame,
                                              ComputedStyle* style)
{
    Frame* f = frame;
    if (f->canHaveFirstLineOrFirstLetterStyle()) {
        if (Frame* firstLineFrame =
                f->firstLinePseudoComputedStyleOwnerFrame()) {
            return firstLineFrame->cachedPseudoStyle(
                PseudoElementType::PseudoElementFirstLine, style);
        }
    } else if (!f->isAnonymous()) {
        if (f->isInlineNonReplacedBox()) {
            return firstLineStyleFromCache(
                f->asInlineNonReplacedBox()->origin(), style);
        } else if (f->isFrameInline() &&
                   !(f->style()->seenPseudoElement(
                       PseudoElementType::PseudoElementFirstLetter))) {
            ComputedStyle* parentStyle = f->style();
            if (parentStyle != f->parent()->style()) {
                Frame* fb = f;
                while (true) {
                    if (fb->isFrameBlockBox() && fb->node()) {
                        break;
                    }
                    fb = fb->parent();
                }
                return fb->cachedPseudoStyle(
                    PseudoElementType::PseudoElementFirstLineInherited,
                    parentStyle);
            }
        }
    }

    return nullptr;
}

ComputedStyle* Frame::firstLineStyle(Frame* frame, ComputedStyle* frameStyle)
{
    if (document()->styleResolver().usesFirstLineRule()) {
        if (ComputedStyle* pseudoStyle = firstLineStyleFromCache(
                frame->isFrameText() ? frame->parent() : frame, frameStyle)) {
            return pseudoStyle;
        }
    }
    return Frame::style();
}

OverflowValue Frame::appliedOverflowX()
{
    if (node()) {
        return node()->appliedOverflowX();
    }

    return m_styleWhenNodeIsAnonymous->overflowX();
}

OverflowValue Frame::appliedOverflowY()
{
    if (node()) {
        return node()->appliedOverflowY();
    }

    return m_styleWhenNodeIsAnonymous->overflowY();
}

void Frame::updateComputedStyle(Node* refNode)
{
    STARFISH_ASSERT(isAnonymous());
    ComputedStyle* newStyle = new ComputedStyle(refNode->style());
    newStyle->setDisplay(m_styleWhenNodeIsAnonymous->display());
    newStyle->loadResources(refNode, m_styleWhenNodeIsAnonymous);
    newStyle->arrangeStyleValues(refNode->style(), refNode);
    m_styleWhenNodeIsAnonymous = newStyle;
}

void Frame::markFlexItem()
{
    // https://www.w3.org/TR/css-flexbox-1/#painting
    // Flex items paint exactly the same as inline blocks [CSS21], except
    // that order-modified document order is used in place of raw document
    // order, and z-index values other than auto create a stacking context
    // even if position is static.
    if (m_flags.m_isFlexItem) {
        return;
    }

    if (FlexFormattingContext::doesParticipateInFlexFormattingContext(this)) {
        m_flags.m_isFlexItem = true;
        m_flags.m_needToEstablishStackingContext |=
            style()->isSpecifiedZIndex();
        m_flags.m_needToEstablishBlockFormattingContext = true;
        m_flags.m_needsLayout = true;
    }
}

void Frame::markGridItem()
{
    if (m_flags.m_isGridItem) {
        return;
    }

    if (GridFormattingContext::doesParticipateInGridFormattingContext(this)) {
        m_flags.m_isGridItem = true;
        m_flags.m_needToEstablishBlockFormattingContext = true;
        markNeedsLayout();
    }
}

Element* Frame::offsetParent()
{
    // https://drafts.csswg.org/cssom-view/#dom-htmlelement-offsetparent
    if (isAnonymous() || isRootElement() || node()->isHTMLBodyElement() ||
        style()->position() == PositionValue::FixedPositionValue) {
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

        if (node->isHTMLBodyElement()) {
            break;
        }

        if (!isPositioned() &&
            (node->isHTMLTableElement() || node->isHTMLTableCellElement())) {
            break;
        }
    }

    return node && node->isElement() ? node->asElement() : nullptr;
}

LayoutLocation Frame::adjustedPositionRelativeToOffsetParent()
{
    if (node()->isHTMLBodyElement() || !parent()) {
        return LayoutLocation();
    }

    Frame* frameObj = this;
    LayoutRect result(0, 0, 0, 0);

    Node* offsetParentNode = frameObj->offsetParent();
    FrameBox* offsetParent = nullptr;

    if (!offsetParentNode) {
        offsetParent = document()->frame()->asFrameBox();
    } else if (offsetParentNode->frame()->isFrameBox()) {
        offsetParent = offsetParentNode->frame()->asFrameBox();
    } else {
        offsetParent = document()->frame()->asFrameBox();
    }

    FrameBox* box = frameObj->findNearestAssociateBox();
    LayoutRect rect = box->absoluteRect(offsetParent);
    rect.setX(rect.x() - offsetParent->paddingLeft());
    rect.setY(rect.y() - offsetParent->paddingTop());
    result.unite(rect);

    if (offsetParent->node()->isHTMLBodyElement()) {
        result.setX(result.x() + offsetParent->x());
        result.setY(result.y() + offsetParent->y());
    }

    return result.location();
}

FrameBox* Frame::findNearestAssociateBox()
{
    Frame* frameObj = this;
    FrameBox* box = nullptr;
    if (frameObj->isFrameBox()) {
        box = frameObj->asFrameBox();
    } else {
        FrameBlockBox* c = blockContainer(frameObj);
        FrameInline* in = asFrameInline();
        InlineNonReplacedBox* inrb = c->firstInlineNonReplacedBox(in);
        if (inrb && inrb->boxes().size() > 0) {
            box = inrb->boxes()[0];
        } else {
            box = c;
        }
    }
    return box;
}

Document* Frame::document()
{
    STARFISH_ASSERT(node() || parent());
    return isAnonymous() ? parent()->document() : node()->document();
}

struct LayoutDamager {
    LayoutDamager()
        : m_canPercentDamage(false)
        , m_canAutoDamage(false)
        , m_canViewportWidthDamage(false)
        , m_canViewportHeightDamage(false)
        , m_canIntrinsicDamage(false)
    {
    }

    bool m_canPercentDamage;
    bool m_canAutoDamage;
    bool m_canViewportWidthDamage;
    bool m_canViewportHeightDamage;
    bool m_canIntrinsicDamage;
};

static bool isLayoutDamaged(LayoutDamager damager, Length l)
{
    if (l.isFixed() || l.isFontPercent() || l.isInheritableNumber()) {
        return false;
    } else if (l.isPercent()) {
        return damager.m_canPercentDamage;
    } else if (l.isViewportPercent()) {
        Length::Type t = l.type();
        CSSLength::Kind k;
        if (t == Length::Vw) {
            return damager.m_canViewportWidthDamage;
        } else if (t == Length::Vh) {
            return damager.m_canViewportHeightDamage;
        } else {
            return damager.m_canViewportWidthDamage ||
                   damager.m_canViewportHeightDamage;
        }
    } else if (l.isAuto()) {
        return damager.m_canAutoDamage;
    } else if (l.isCalc()) {
        GCVector<CalcTerm*>& data = l.calcData()->terms();
        auto iter = data.begin();

        while (iter != data.end()) {
            GCVector<CalcValue>& data2 = (*iter)->values();
            auto iter2 = data2.begin();
            while (iter2 != data2.end()) {
                CalcValue& v = *iter2;
                if (v.type().isLength()) {
                    Length l2 = v.lengthValue().toLength();
                    if (isLayoutDamaged(damager, l2)) {
                        return true;
                    }
                } else if (v.type().isPercentage()) {
                    Length l2 = Length(Length::Percent, v.percentageValue());
                    if (isLayoutDamaged(damager, l2)) {
                        return true;
                    }
                }
                iter2++;
            }
            iter++;
        }

        return false;
    } else if (l.isIntrinsic()) {
        return damager.m_canIntrinsicDamage;
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return false;
    }
}

bool Frame::shouldLayout(LayoutContext& ctx, LayoutWantToResolve resolveWhat,
                         FrameBox* containingBox)
{
    if (needsLayout()) {
        return true;
    }

    ComputedStyle* style = this->style();
    Node* node = this->node();
    LayoutDamager damager;
    bool containerWidthMayBeChanged = containingBox->contentWidthDamaged();
    bool containerHeightMayBeChanged = containingBox->contentHeightDamaged();
    if (isAbsolutePositioned()) {
        containerWidthMayBeChanged |= containingBox->paddingWidthDamaged();
        containerHeightMayBeChanged |= containingBox->paddingHeightDamaged();
    }
    damager.m_canViewportWidthDamage = ctx.viewportWidthDamaged();
    damager.m_canViewportHeightDamage = ctx.viewportHeightDamaged();
    damager.m_canAutoDamage = false;
    if (resolveWhat & LayoutWantToResolve::ResolveWidth) {
        damager.m_canIntrinsicDamage = containerWidthMayBeChanged;
        damager.m_canPercentDamage = containerWidthMayBeChanged;
        if (style->width().isAuto()) {
            if (containerWidthMayBeChanged) {
                markNeedsLayout();
                return true;
            }

            LengthData margin = style->margin();
            if (isLayoutDamaged(damager, margin.left()) ||
                isLayoutDamaged(damager, margin.right())) {
                markNeedsLayout();
                return true;
            }

            BorderData border = style->border();
            if (isLayoutDamaged(damager, border.left().width()) ||
                isLayoutDamaged(damager, border.right().width())) {
                markNeedsLayout();
                return true;
            }

            LengthData padding = style->padding();
            if (isLayoutDamaged(damager, padding.left()) ||
                isLayoutDamaged(damager, padding.right())) {
                markNeedsLayout();
                return true;
            }

            if (isAbsolutePositioned()) {
                LengthData offset = style->offset();
                if ((offset.left().isSpecified() &&
                     isLayoutDamaged(damager, offset.left())) ||
                    (offset.right().isSpecified() &&
                     isLayoutDamaged(damager, offset.right()))) {
                    markNeedsLayout();
                    return true;
                } else if (offset.left().isAuto() && offset.right().isAuto()) {
                    markNeedsLayout();
                    return true;
                }
            }
        } else {
            if (isLayoutDamaged(damager, style->width())) {
                markNeedsLayout();
                return true;
            }

            if (isFrameTableBox() && style->width().isPercent()) {
                return true;
            }
        }

        if (isLayoutDamaged(damager, style->minWidth()) ||
            isLayoutDamaged(damager, style->maxWidth())) {
            markNeedsLayout();
            return true;
        }
    }

    if (resolveWhat & LayoutWantToResolve::ResolveHeight) {
        LengthData offset = style->offset();
        if (style->height().isAuto() && isAbsolutePositioned() &&
            offset.top().isSpecified() && offset.bottom().isSpecified()) {
            if (containerHeightMayBeChanged) {
                return true;
            }

            damager.m_canPercentDamage = containerWidthMayBeChanged;

            LengthData margin = style->margin();
            if (isLayoutDamaged(damager, margin.top()) ||
                isLayoutDamaged(damager, margin.bottom())) {
                return true;
            }

            BorderData border = style->border();
            if (isLayoutDamaged(damager, border.top().width()) ||
                isLayoutDamaged(damager, border.bottom().width())) {
                return true;
            }

            LengthData padding = style->padding();
            if (isLayoutDamaged(damager, padding.top()) ||
                isLayoutDamaged(damager, padding.bottom())) {
                return true;
            }

            damager.m_canPercentDamage = containerHeightMayBeChanged;
            if (isLayoutDamaged(damager, offset.top()) ||
                isLayoutDamaged(damager, offset.bottom())) {
                return true;
            }
        } else {
            damager.m_canPercentDamage = containerHeightMayBeChanged;
            if (isLayoutDamaged(damager, style->height())) {
                return true;
            }
        }

        damager.m_canPercentDamage = containerHeightMayBeChanged;
        if (isLayoutDamaged(damager, style->minHeight()) ||
            isLayoutDamaged(damager, style->maxHeight())) {
            return true;
        }

        if (isFrameTableBox()) {
            return true;
        }

        if (isFrameTableCellBox()) {
            VerticalAlignValue verticalAlign = style->verticalAlign();
            if (verticalAlign == VerticalAlignValue::BottomVAlignValue ||
                verticalAlign == VerticalAlignValue::MiddleVAlignValue) {
                return true;
            }
        }

        damager.m_canPercentDamage = containerWidthMayBeChanged;
        if (isLayoutDamaged(damager, style->textIndent())) {
            return true;
        }

        damager.m_canPercentDamage = false;
        if (isLayoutDamaged(damager, style->letterSpacing()) ||
            isLayoutDamaged(damager, style->wordSpacing())) {
            return true;
        }

        if (style->verticalAlign() == NumericVAlignValue) {
            damager.m_canPercentDamage = false;
            if (isLayoutDamaged(damager, style->verticalAlignLength())) {
                return true;
            }
        }
    }

    return false;
}

bool Frame::isRunningOpacityAnimation()
{
    if (isAnonymous()) {
        return false;
    }
    return node()->isRunningOpacityAnimation();
}

bool Frame::isRunningTransformAnimation()
{
    if (isAnonymous()) {
        return false;
    }
    return node()->isRunningTransformAnimation();
}

void Frame::markRunningOpacityAnimation()
{
    STARFISH_ASSERT(node());
    node()->markRunningOpacityAnimation();
}

void Frame::markRunningTransformAnimation()
{
    STARFISH_ASSERT(node());
    node()->markRunningTransformAnimation();
}

void Frame::clearRunningOpacityAnimation()
{
    STARFISH_ASSERT(node());
    node()->clearRunningOpacityAnimation();
}

void Frame::clearRunningTransformAnimation()
{
    STARFISH_ASSERT(node());
    node()->clearRunningTransformAnimation();
}

bool Frame::hasFrameBorderRadius()
{
    return style()->hasBorderRadius() &&
           !(isLeftMBPCleared() && isRightMBPCleared());
}

BorderRadiusData Frame::frameBorderRadius()
{
    STARFISH_ASSERT(style()->hasBorderRadius());
    BorderRadiusData data = style()->borderRadius();
    if (isLeftMBPCleared()) {
        data.m_topLeftHorizontal = Length(Length::Fixed, 0);
        data.m_topLeftVertical = Length(Length::Fixed, 0);
        data.m_bottomLeftHorizontal = Length(Length::Fixed, 0);
        data.m_bottomLeftVertical = Length(Length::Fixed, 0);
    }
    if (isRightMBPCleared()) {
        data.m_topRightHorizontal = Length(Length::Fixed, 0);
        data.m_topRightVertical = Length(Length::Fixed, 0);
        data.m_bottomRightHorizontal = Length(Length::Fixed, 0);
        data.m_bottomRightVertical = Length(Length::Fixed, 0);
    }
    return data;
}
} // namespace Starfish
