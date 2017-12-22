/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd

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
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/page/Window.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/StackingContext.h"

namespace StarFish {

FrameBlockBox* blockContainer(Frame* currentFrame)
{
    Frame* f = currentFrame->layoutParent();

    if (!f || currentFrame->isFrameDocument()) {
        STARFISH_ASSERT(currentFrame->isFrameDocument());
        return currentFrame->asFrameBlockBox();
    }

    while (true) {
        if (f->isFrameBlockBox() && !f->isAnonymous()) {
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
            STARFISH_ASSERT(currentFrame->isFrameDocument());
            return currentFrame->asFrameBox();
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

bool LayoutPredictionStatus::predict(bool isHorizontal, bool isMinMax, Length l)
{
    if (childrenOrSelfChanged()) {
        return true;
    }

    if (l.isFixed()) {
        return false;
    } else if (l.isPercent()) {
        // TODO:
        return true;
        /* if (isHorizontal) {
            return containerWidthMaybeChanged();
        } else {
            return containerHeightMaybeChanged();
        } */
    } else if (l.isFontPercent()) {
        // TODO:
        return true;
    } else if (l.isViewportPercent()) {
        Length::Type t = l.type();
        CSSLength::Kind k;
        if (t == Length::Vw) {
            return viewportWidthChanged();
        } else if (t == Length::Vh) {
            return viewportHeightChanged();
        } else {
            // TODO:
            return viewportWidthChanged() || viewportHeightChanged();
        }
    } else if (l.isAuto()) {
        return !isMinMax;
    } else if (l.isCalc()) {
        // TODO:
        return true;
    } else if (l.isInheritableNumber()) {
        // TODO:
        return true;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

void LayoutPredictionContext::makeState()
{
    LayoutPredictionStatus state;

    if (m_states.size() > 0) {
        auto& lastState = this->state();

        if (lastState.viewportWidthChanged()) {
            state.markViewportWidthChanged();
        }

        if (lastState.viewportHeightChanged()) {
            state.markViewportHeightChanged();
        }
    }
    m_index++;
    m_states.push_back(state);
}

bool LayoutPredictionContext::shouldLayout(Frame* f)
{
    auto& state = this->state();

    Length width = f->style()->width();
    Length height = f->style()->height();
    LengthData margin = f->style()->margin();
    Length marginTop = margin.top();
    Length marginBottom = margin.bottom();
    Length marginLeft = margin.left();
    Length marginRight = margin.right();
    BorderData border = f->style()->border();
    Length borderTop = border.top().width();
    Length borderBottom = border.bottom().width();
    Length borderLeft = border.left().width();
    Length borderRight = border.right().width();
    LengthData padding = f->style()->padding();
    Length paddingTop = padding.top();
    Length paddingBottom = padding.bottom();
    Length paddingLeft = padding.left();
    Length paddingRight = padding.right();
    Length minWidth = f->style()->minWidth();
    Length maxWidth = f->style()->maxWidth();
    Length minHeight = f->style()->minHeight();
    Length maxHeight = f->style()->maxHeight();
    Length top, bottom, left, right;
    if (f->isAbsolutePositioned()) {
        top = f->style()->top();
        bottom = f->style()->bottom();
        left = f->style()->left();
        right = f->style()->right();
    }

    return state.predict(true, false, width) ||
           state.predict(false, false, height) ||
           state.predict(true, false, marginTop) ||
           state.predict(true, false, marginBottom) ||
           state.predict(true, false, marginLeft) ||
           state.predict(true, false, marginRight) ||
           state.predict(true, false, borderTop) ||
           state.predict(true, false, borderBottom) ||
           state.predict(true, false, borderLeft) ||
           state.predict(true, false, borderRight) ||
           state.predict(true, false, paddingTop) ||
           state.predict(true, false, paddingBottom) ||
           state.predict(true, false, paddingLeft) ||
           state.predict(true, false, paddingRight) ||
           state.predict(true, true, minWidth) ||
           state.predict(true, true, maxWidth) ||
           state.predict(false, true, minHeight) ||
           state.predict(false, true, maxHeight) ||
           (f->isAbsolutePositioned() && (state.predict(false, false, top) ||
                                          state.predict(false, false, bottom) ||
                                          state.predict(true, false, left) ||
                                          state.predict(true, false, right)));
}

FloatingBoxInfo::FloatingBoxInfo(FrameBox* box, LayoutContext* ctx)
    : m_box(box)
{
    STARFISH_ASSERT(box->isFloating());
    m_isLeft = box->style()->floating() == LeftFloatValue;
    Frame* parent = box->layoutParent();
    while (parent) {
        if (!parent->isAnonymous() && parent->isBlockLevel()) {
            m_canLayoutParentCollapseWithMarginTop =
                ctx->marginInfo(parent->asFrameBlockBox())
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

LayoutUnit LayoutContext::parentFixedHeight(Frame* currentFrame)
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

        STARFISH_ASSERT(height.isPercent() || height.isCalc());
        reverse.emplace_back(container, height);
        container = blockContainer(container);
    }
    Length height = reverse.back().second;
    container = reverse.back().first;
    LayoutUnit result;
    if (height.isDefinite(false)) {
        LayoutUnit unused;
        result = height.specifiedValue(unused, container);
    }

    result = container->contentHeightApplyingBoxSizing(result);
    reverse.pop_back();
    while (reverse.size()) {
        height = reverse.back().second;
        container = reverse.back().first;
        result = height.specifiedValue(result, container);
        result = container->contentHeightApplyingBoxSizing(result);
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

void LayoutContext::registerLineBoxAscender(FrameBlockBox* blockBox,
                                            LineBox* lb, LayoutUnit ascender)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    for (size_t i = 0; i < c.m_inlineBlockBoxStack->size(); i++) {
        FrameBlockBox* blockBox = (*c.m_inlineBlockBoxStack)[i];
        auto& lineBoxAscenders = (*c.m_lineBoxAscenders);
        if (blockBox->style()->verticalAlign() == BaselineVAlignValue ||
            blockBox->style()->verticalAlign() == NumericVAlignValue) {
            if (blockBox->isFrameFlexibleBox() ||
                blockBox->isFrameTableCellBox()) {
                auto iter = lineBoxAscenders.find(blockBox);
                if (iter == lineBoxAscenders.end()) {
                    lineBoxAscenders[blockBox] =
                        lb->absolutePoint(blockBox).y() + ascender;
                }
            } else if (blockBox->style()->display() ==
                       InlineBlockDisplayValue) {
                lineBoxAscenders[blockBox] =
                    lb->absolutePoint(blockBox).y() + ascender;
            }
        }
    }

    registerFirstLineAscender(blockBox, lb, ascender);
}

Nullable<LayoutUnit> LayoutContext::lineBoxAscender(FrameBlockBox* blockBox)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    STARFISH_ASSERT(c.m_inlineBlockBoxStack->back() == blockBox);
    auto iter = (*c.m_lineBoxAscenders).find(blockBox);
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
                (*c.m_firstLineAscenders)[blockBox] =
                    std::make_pair(lineBox, ascender);
            }
        }
    }
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
    return l;
}

void LayoutContext::tempReigsterFirstLineAscender(
    FrameTableCellBox* cellBox, std::pair<LineBox*, LayoutUnit> ascenderInfo)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    (*c.m_tempAscenders)[cellBox] = ascenderInfo;
}

Nullable<std::pair<LineBox*, LayoutUnit>> LayoutContext::tempFirstLineAscender(
    FrameTableCellBox* cellBox)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    auto it = c.m_tempAscenders->find(cellBox);
    if (it == c.m_tempAscenders->end()) {
        return Nullable<std::pair<LineBox*, LayoutUnit>>();
    }

    return it->second;
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

void LayoutContext::registerRelativePositionedBox(FrameBox* box, bool dueToSelf)
{
    FrameBlockBox* cb = containingFrameBlockBox(box);
    m_relativePositionedBoxes.emplace(
        cb, std::vector<std::pair<FrameBox*, bool>>());
    auto& vec = m_relativePositionedBoxes[cb];
    vec.emplace_back(box, dueToSelf);
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
            if (boxes[i].second) {
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
        m_relativePositionedBoxes.erase(iter);
    }
}

bool LayoutContext::checkIfThisIsFirstLineCandidate(FrameBlockBox* blockBox)
{
    if (!blockBox->isNecessaryBlockBox()) {
        return false;
    }

    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    auto& firstLineCandidates = c.m_firstLineCandidates;
    Frame* parent = blockBox->parent();

    auto it = firstLineCandidates->find(parent);
    if (it == firstLineCandidates->end()) {
        (*firstLineCandidates)[parent] = blockBox;
        return true;
    }

    return (*it).second == blockBox;
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

void Frame::ComputeVisibleRectContext::uniteRect(const LayoutRect& r)
{
    SkMatrix m = tranformMatrix;

    // If Frame has `skew, rotate, 3d-transform`, Frame must own it's graphics
    // buffer.
    // If matrix is not rect, we should ignore child visible rects from here
    if (!m.rectStaysRect()) {
        return;
    }
    SkRect skRect = SkRect::MakeXYWH((float)r.x(), (float)r.y(),
                                     (float)r.width(), (float)r.height());

    m.mapRect(&skRect);
    skRect.sort();
    LayoutRect tmp =
        LayoutRect(skRect.x(), skRect.y(), skRect.width(), skRect.height());

    for (size_t i = 0; i < boundMaxExtentDueToOverflow.size(); i++) {
        LayoutRect rt = std::get<0>(boundMaxExtentDueToOverflow[i]);

        if (std::get<1>(boundMaxExtentDueToOverflow[i])) { // x
            if (tmp.x() < rt.x()) {
                tmp =
                    LayoutRect(rt.x(), tmp.y(),
                               tmp.width() - (rt.x() - tmp.x()), tmp.height());
            }

            if (tmp.maxX() > rt.maxX()) {
                tmp.setWidth(tmp.width() - (tmp.maxX() - rt.maxX()));
            }
        }

        if (std::get<2>(boundMaxExtentDueToOverflow[i])) { // y
            if (tmp.y() < rt.y()) {
                tmp = LayoutRect(tmp.x(), rt.y(), tmp.width(),
                                 tmp.height() - (rt.y() - tmp.y()));
            }
            if (tmp.maxY() > rt.maxY()) {
                tmp.setHeight(tmp.height() - (tmp.maxY() - rt.maxY()));
            }
        }
    }

    result.unite(tmp);
}

Frame::ComputeVisibleRectContextFragment::ComputeVisibleRectContextFragment(
    ComputeVisibleRectContext& ctx, FrameBox* fragmentBox)
    : ctx(ctx)
    , fragmentBox(fragmentBox)
    , transformMatrixBefore(ctx.tranformMatrix)
    , shouldStopComputingBecauseMatrixInvalidFromHere(false)
    , overflowXWasApplyed(false)
    , overflowYWasApplyed(false)
{
    if (ctx.ignoreTransformOnce) {
        ctx.ignoreTransformOnce = false;
        return;
    }

    ComputedStyle* cs = fragmentBox->style();
    if (ctx.purpose == Frame::ComputeVisibleRectContext::GraphicsBuffer && cs &&
        cs->hasTransforms(fragmentBox)) {
        // transform Frame must own StackingContext
        SkMatrix m = fragmentBox->stackingContext()->transformMatrix();
        // If Frame has `skew, rotate, 3d-transform`, Frame must own it's
        // graphics buffer.
        // If matrix is not rect, we should ignore child visible rects from here
        if (!m.rectStaysRect()) {
            shouldStopComputingBecauseMatrixInvalidFromHere = true;
            return;
        }
        STARFISH_ASSERT(m.rectStaysRect());

        auto to = fragmentBox->stackingContext()->transformOrigin();

        ctx.tranformMatrix.postTranslate((float)fragmentBox->x(),
                                         (float)fragmentBox->y());
        ctx.tranformMatrix.postTranslate((float)to.x(), (float)to.y());
        ctx.tranformMatrix.preConcat(m);
        ctx.tranformMatrix.postTranslate((float)-to.x(), (float)-to.y());

        if (!ctx.tranformMatrix.rectStaysRect()) {
            shouldStopComputingBecauseMatrixInvalidFromHere = true;
            return;
        }
    } else {
        ctx.tranformMatrix.postTranslate((float)fragmentBox->x(),
                                         (float)fragmentBox->y());
    }

    if (fragmentBox->shouldApplyOverflow()) {
        overflowXWasApplyed = cs->overflowX() != OverflowValue::VisibleOverflow;
        overflowYWasApplyed = cs->overflowY() != OverflowValue::VisibleOverflow;

        if (overflowXWasApplyed || overflowYWasApplyed) {
            LayoutRect rt = fragmentBox->frameVisibleRect();

            SkRect skRect =
                SkRect::MakeXYWH((float)rt.x(), (float)rt.y(),
                                 (float)rt.width(), (float)rt.height());

            ctx.tranformMatrix.mapRect(&skRect);
            skRect.sort();
            LayoutRect tmp = LayoutRect(skRect.x(), skRect.y(), skRect.width(),
                                        skRect.height());

            ctx.boundMaxExtentDueToOverflow.push_back(
                std::make_tuple(tmp, overflowXWasApplyed, overflowYWasApplyed));
        }
    }
}
Frame::ComputeVisibleRectContextFragment::~ComputeVisibleRectContextFragment()
{
    ctx.tranformMatrix = transformMatrixBefore;
    if (overflowXWasApplyed || overflowYWasApplyed) {
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

    m_flags.m_needsLayout = true;
    m_flags.m_isAnonymous = isAnonymous;

    bool isRootElement = node && node->isHTMLHtmlElement();
    m_flags.m_isRootElement = isRootElement;

    m_flags.m_isLeftMBPCleared = false;
    m_flags.m_isRightMBPCleared = false;

    m_flags.m_isEstablishesBlockFormattingContext = isRootElement;
    m_flags.m_isPositioned = false;
    m_flags.m_isEstablishesStackingContext = isRootElement;
    m_flags.m_needsGraphicsBuffer = false;
    m_flags.m_isNormalFlow = true;
    m_flags.m_isAbsolutePositioned = false;
    m_flags.m_isFloating = false;
    m_flags.m_isFrameText = false;
    m_flags.m_heightComputed = false;
    m_flags.m_hasBiggerContentThanFrameWidth = false;
    m_flags.m_hasBiggerContentThanFrameHeight = false;
    m_flags.m_needsToComputeScrollVisbleRect = false;
    m_flags.m_isFirstLine = false;
    m_flags.m_isRunningTransformAnimation = false;
    m_flags.m_shouldApplyOverflow = false;
    m_flags.m_seenNormalFlowBlockChild = false;
    m_flags.m_seenNonPositionedFloats = false;
    m_flags.m_seenReplacedBlock = false;
    m_flags.m_seenNormalFlowInline = false;
    m_flags.m_seenNormalFlowInlineBox = false;
    m_flags.m_seenNormalFlowInlineBlockBox = false;
    m_flags.m_seenNormalFlowInlineReplaced = false;

    computeStyleFlags();
}

void Frame::predictLayout(LayoutPredictionContext& ctx, PredictionStage stage)
{
    if (stage == PredictionStage::Collect) {
        bool ret = needsLayout();
        clearNeedsLayout();

        if (node()) {
            ret |= node()->needsLayout();
            node()->clearNeedsLayout();
        }

        if (ret) {
            ctx.state().markChildrenOrSelfChanged();
        }
    }
}

void Frame::computePaintingFlags(LayoutContext& ctx,
                                 LayoutWantToResolve resolveWhat)
{
    if (resolveWhat & LayoutWantToResolve::ResolveWidth) {
        m_flags.m_seenNormalFlowBlockChild = false;
        m_flags.m_seenNonPositionedFloats = false;
        m_flags.m_seenReplacedBlock = false;
        m_flags.m_seenNormalFlowInline = false;
    }

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
    if (isFrameBlockBox() && !asFrameBlockBox()->hasBlockFlow()) {
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

        if (f->isEstablishesStackingContext()) {
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
        ;
    }
}

void Frame::computeStyleFlags()
{
    computeShouldApplyOverflow();

    ComputedStyle* style = Frame::style();
    if (!style) {
        return;
    }

    m_flags.m_isPositioned =
        (style->position() != PositionValue::StaticPositionValue);
    m_flags.m_isAbsolutePositioned |=
        (style->position() == PositionValue::AbsolutePositionValue);
    m_flags.m_isAbsolutePositioned |=
        (style->position() == PositionValue::FixedPositionValue);
    m_flags.m_isFloating = (style->floating() != FloatValue::NoneFloatValue);

    m_flags.m_isNormalFlow = !m_flags.m_isAbsolutePositioned;
    m_flags.m_isNormalFlow &= !m_flags.m_isFloating;

    // TODO add condition
    m_flags.m_isEstablishesBlockFormattingContext |= (shouldApplyOverflow());
    m_flags.m_isEstablishesBlockFormattingContext |=
        m_flags.m_isAbsolutePositioned;
    m_flags.m_isEstablishesBlockFormattingContext |= m_flags.m_isFloating;
    m_flags.m_isEstablishesBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::InlineBlockDisplayValue);
    m_flags.m_isEstablishesBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::TableCellDisplayValue);
    m_flags.m_isEstablishesBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::TableCaptionDisplayValue);
    m_flags.m_isEstablishesBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::FlexDisplayValue);
    m_flags.m_isEstablishesBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::InlineFlexDisplayValue);
    // https://www.w3.org/TR/html5/rendering.html#the-fieldset-and-legend-elements
    m_flags.m_isEstablishesBlockFormattingContext |=
        (!isAnonymous() && node()->isHTMLFieldSetElement());

    // https://www.w3.org/TR/2011/REC-CSS2-20110607/tables.html#model
    // The table wrapper box establishes a block formatting context
    m_flags.m_isEstablishesBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::TableDisplayValue);
    m_flags.m_isEstablishesBlockFormattingContext |=
        (style->originalDisplay() == DisplayValue::InlineTableDisplayValue);

    // TODO add condition
    // NOTE
    // https://www.w3.org/TR/CSS2/zindex.html
    // Appendix E. Elaborate description of Stacking Contexts
    // All positioned descendants with 'z-index: auto' or 'z-index: 0', in
    // tree order. For those with 'z-index: auto', treat the element as if
    // it created a new stacking context.
    m_flags.m_isEstablishesStackingContext |= m_flags.m_isPositioned;
    m_flags.m_isEstablishesStackingContext |= (style->opacity() != 1);
    m_flags.m_isEstablishesStackingContext |= (style->hasTransforms(this));

    // TODO add condition
    m_flags.m_needsGraphicsBuffer |= (style->hasComplexTransforms(this));
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

Frame* Frame::enclosingFirstLineStyle()
{
    STARFISH_ASSERT(isFrameBlockBox());
    Frame* firstLineFrame = this;
    bool hasPseudo = false;

    while (true) {
        if (!firstLineFrame->isAnonymous() &&
            !firstLineFrame->isFrameDocument()) {
            STARFISH_ASSERT(firstLineFrame->node()->isElement());
            hasPseudo = firstLineFrame->style()->seenPseudoElement(
                StyleResolver::PseudoElementType::PseudoElementFirstLine);
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

ComputedStyle* Frame::pseudoStyleForFirstLine(
    StyleResolver::PseudoElementType pseudoId, ComputedStyle* parentStyle)
{
    STARFISH_ASSERT(node());
    STARFISH_ASSERT(node()->isElement());

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
    if (pseudoId == StyleResolver::PseudoElementType::PseudoElementFirstLine) {
        document()->styleResolver().matchAllRules(
            ctx, element, result, parentStyle,
            StyleResolver::PseudoElementType::PseudoElementFirstLine);
    } else {
        ComputedStyle::InheritedStyles orgInheritedStyles =
            parentStyle->m_inheritedStyles;
        document()->styleResolver().matchAllRules(
            ctx, element, result, parentStyle,
            StyleResolver::PseudoElementType::PseudoElementFirstLine);
        Unit::Color computedColor = result->color();
        result->m_inheritedStyles = orgInheritedStyles;
        if (parentStyle->m_gotInheritedColor) {
            result->m_inheritedStyles.m_color = computedColor;
        }

        result->setPseudoType(
            StyleResolver::PseudoElementType::PseudoElementFirstLineInherited);
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

ComputedStyle* Frame::cachedPseudoStyle(StyleResolver::PseudoElementType pseudo,
                                        ComputedStyle* parentStyle)
{
    if (node() && node()->isElement()) {
        if (pseudo == StyleResolver::PseudoElementFirstLine ||
            pseudo == StyleResolver::PseudoElementFirstLineInherited) {
            auto c = style()->cachedPseudoStyle(pseudo);
            if (c == nullptr) {
                auto s = pseudoStyleForFirstLine(pseudo, parentStyle);
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
        if (Frame* firstLineFrame = f->enclosingFirstLineStyle()) {
            return firstLineFrame->cachedPseudoStyle(
                StyleResolver::PseudoElementType::PseudoElementFirstLine,
                style);
        }
    } else if (!f->isAnonymous()) {
        if (f->isInlineNonReplacedBox()) {
            return firstLineStyleFromCache(
                f->asInlineNonReplacedBox()->origin(), style);
        } else if (f->isFrameInline() &&
                   !(f->style()->seenPseudoElement(
                       StyleResolver::PseudoElementType::
                           PseudoElementFirstLetter))) {
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
                    StyleResolver::PseudoElementType::
                        PseudoElementFirstLineInherited,
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
    m_flags.m_isFlexItem = true;
    // https://www.w3.org/TR/css-flexbox-1/#painting
    // Flex items paint exactly the same as inline blocks [CSS21], except
    // that order-modified document order is used in place of raw document
    // order, and z-index values other than auto create a stacking context
    // even if position is static.
    if (!FlexFormattingContext::isAnonymousFlexItemContainingOnlyWhitespace(
            this)) {
        m_flags.m_isEstablishesStackingContext = true;
        m_flags.m_isEstablishesBlockFormattingContext = true;
    }
}

bool Frame::isDocumentElement() const
{
    return !isAnonymous() && node()->document() == node();
}

Element* Frame::offsetParent() const
{
    if (isDocumentElement() ||
        (!isAnonymous() && node()->isHTMLBodyElement())) {
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
}
