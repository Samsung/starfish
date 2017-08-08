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
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameDocument.h"

namespace StarFish {

FrameBlockBox* LayoutContext::blockContainer(Frame* currentFrame)
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

FrameBlockBox* LayoutContext::containingFrameBlockBox(Frame* currentFrame)
{
    FrameBlockBox* blockBox = blockContainer(currentFrame);
    if (currentFrame->isAbsolutePositioned()) {
        bool isFixed = currentFrame->style()->position() == FixedPositionValue;
        while (!blockBox->isFrameDocument() &&
               (isFixed || !blockBox->isPositioned()) &&
               !blockBox->style()->hasTransforms(blockBox)) {
            blockBox = blockContainer(blockBox);
        }
        return blockBox;
    } else {
        return blockBox;
    }
}

FrameBox* LayoutContext::containingBlock(Frame* currentFrame)
{
    // https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#containing-block-details
    if (currentFrame->isAbsolutePositioned()) {
        bool isFixed = currentFrame->style()->position() == FixedPositionValue;
        Frame* f = currentFrame->parent();
        while (!f->isFrameDocument() && (isFixed || !f->isPositioned()) &&
               !f->style()->hasTransforms(f)) {
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
    return blockContainer(currentFrame)->contentWidth();
}

bool LayoutContext::parentHasFixedHeight(Frame* currentFrame)
{
    if (currentFrame->isAbsolutePositioned()) {
        return true;
    }
    FrameBlockBox* container = blockContainer(currentFrame);
    while (container) {
        Length height = container->style()->height();
        if (height.isFixed() || height.isViewportPercent()) {
            return true;
        } else if (container->isAbsolutePositioned() &&
                   (height.isPercent() ||
                    (container->style()->bottom().isSpecified() &&
                     container->style()->top().isSpecified()))) {
            return true;
        } else if (height.isAuto()) {
            return false;
        } else {
            STARFISH_ASSERT(height.isPercent());
            container = blockContainer(container);
        }
    }
    return false;
}

LayoutUnit LayoutContext::parentFixedHeight(Frame* currentFrame)
{
    FrameBlockBox* container = blockContainer(currentFrame);
    std::vector<std::pair<FrameBox*, Length>> reverse;
    while (container) {
        Length height = container->style()->height();
        if (height.isFixed() || height.isViewportPercent()) {
            reverse.emplace_back(container, height);
            break;
        }

        if (container->isAbsolutePositioned()) {
            if (height.isPercent()) {
                LayoutUnit parentHeight =
                    containingBlock(container)->contentHeight();
                reverse.emplace_back(
                    container,
                    Length(Length::Fixed, height.percentValue(parentHeight)));
                break;
            } else if (container->style()->top().isSpecified() &&
                       container->style()->bottom().isSpecified()) {
                LayoutUnit parentHeight =
                    containingBlock(container)->contentHeight();
                LayoutUnit viewportHeight = LayoutContext::viewportHeight();
                LayoutUnit t = container->style()->top().specifiedValue(
                    parentHeight, viewportHeight);
                LayoutUnit b = container->style()->bottom().specifiedValue(
                    parentHeight, viewportHeight);
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

        STARFISH_ASSERT(height.isPercent());
        reverse.emplace_back(container, height);
        container = blockContainer(container);
    }
    Length height = reverse.back().second;
    LayoutUnit result;
    if (height.isFixed()) {
        result = height.fixed();
    } else {
        STARFISH_ASSERT(height.isViewportPercent());
        result = height.viewportPercentValue(viewportHeight());
    }

    result = reverse.back().first->contentHeightApplyingBoxSizing(result);
    reverse.pop_back();
    while (reverse.size()) {
        result = reverse.back().second.percentValue(result);
        result = reverse.back().first->contentHeightApplyingBoxSizing(result);
        reverse.pop_back();
    }

    return result;
}

void LayoutContext::registerYPositionPerVAInlineBlock(LineBox* lb,
                                                      LayoutUnit ascender)
{
    BlockFormattingContext& c = m_blockFormattingContextInfo.back();
    for (size_t i = 0; i < c.m_inlineBlockBoxStack->size(); i++) {
        (*c.m_registeredYPositionPerVAInlineBlock)[(
            *c.m_inlineBlockBoxStack)[i]] =
            lb->absolutePoint((*c.m_inlineBlockBoxStack)[i]).y() + ascender;
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

void LayoutContext::registerFirstLineAscender(LineBox* l, LayoutUnit a)
{
    m_firstLineAscender[l] = a;
}

LayoutUnit LayoutContext::firstLineAscender(LineBox* l)
{
    STARFISH_ASSERT(m_firstLineAscender.find(l) != m_firstLineAscender.end());
    return m_firstLineAscender[l];
}

bool LayoutContext::checkIfThisIsFirstLineCandidate(Frame* parent,
                                                    FrameBlockBox* child)
{
    if (!child->isNecessaryBlockBox()) {
        return false;
    }

    auto it = m_firstLineCandidates.find(parent);
    if (it == m_firstLineCandidates.end()) {
        m_firstLineCandidates[parent] = child;
        return true;
    }

    return (*it).second == child;
}

LayoutUnit LayoutContext::viewportWidth()
{
    return frameDocument()->style()->width().fixed();
}

LayoutUnit LayoutContext::viewportHeight()
{
    return frameDocument()->style()->height().fixed();
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
    m_flags.m_heightComputed = false;
    m_flags.m_hasBiggerContentThanFrameWidth = false;
    m_flags.m_hasBiggerContentThanFrameHeight = false;
    m_flags.m_isFirstLine = false;

    computeStyleFlags();
}

bool Frame::shouldApplyOverflow()
{
    if (!isAnonymous()) {
        if (m_node->isHTMLHtmlElement()) {
            return false;
        } else if (m_node->isHTMLBodyElement()) {
            HTMLHtmlElement* html =
                m_node->parentElement()->asHTMLHtmlElement();
            if (html->style()->overflowX() == OverflowValue::VisibleOverflow &&
                html->style()->overflowY() == OverflowValue::VisibleOverflow) {
                return false;
            }
        }
    }

    return (style()->overflowX() != OverflowValue::VisibleOverflow) ||
           (style()->overflowY() != OverflowValue::VisibleOverflow);
}

void Frame::computeStyleFlags()
{
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
    // https://www.w3.org/TR/CSS21/visuren.html#block-formatting
    // Block formatting context is established when the element is either
    // float, absolute positioned, block containers (such as inline-blocks,
    // table-cells, and table-captions) that are not block boxes,
    // or block boxes with 'overflow' other than 'visible'.
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
    // https://www.w3.org/TR/html5/rendering.html#the-fieldset-and-legend-elements
    m_flags.m_isEstablishesBlockFormattingContext |=
        (!isAnonymous() && m_node->isHTMLFieldSetElement());

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
    // it created a new stacking context,
    m_flags.m_isEstablishesStackingContext |= m_flags.m_isPositioned;
    m_flags.m_isEstablishesStackingContext |= (style->opacity() != 1);
    m_flags.m_isEstablishesStackingContext |= (style->hasTransforms(this));

    // TODO add condition
    m_flags.m_needsGraphicsBuffer |= (style->hasComplexTransforms(this));
}

ComputedStyle* Frame::style()
{
    if (UNLIKELY(isAnonymous())) {
        return m_styleWhenNodeIsAnonymous;
    } else {
        return node()->style();
    }
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
            hasPseudo = firstLineFrame->node()->asElement()->hasPseudoElement(
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

    if (!node()->asElement()->hasPseudoElement(pseudoId)) {
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
    if (pseudoId == StyleResolver::PseudoElementType::PseudoElementFirstLine) {
        document()->styleResolver().matchAllRules(
            element, result, parentStyle,
            StyleResolver::PseudoElementType::PseudoElementFirstLine);
    } else {
        document()->styleResolver().matchAllRules(element, result, parentStyle);
        result->setPseudoType(
            StyleResolver::PseudoElementType::PseudoElementFirstLineInherited);
    }

    result->setDisplay(DisplayValue::InlineDisplayValue);
    result->setPosition(PositionValue::StaticPositionValue);
    result->loadResources(element);
    result->arrangeStyleValues(parentStyle, element);

    return result;
}

ComputedStyle* Frame::cachedPseudoStyle(StyleResolver::PseudoElementType pseudo,
                                        ComputedStyle* parentStyle)
{
    if (!(node()->isElement() &&
          node()->asElement()->hasPseudoElement(pseudo))) {
        return nullptr;
    }

    ComputedStyle* cachedStyle = style()->cachedPseudoStyle(pseudo);
    if (cachedStyle) {
        return cachedStyle;
    }

    ComputedStyle* result = pseudoStyleForFirstLine(pseudo, parentStyle);
    return style()->addCachedPseudoStyle(result);
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
                   !(f->node()->asElement()->hasPseudoElement(
                       StyleResolver::PseudoElementType::
                           PseudoElementFirstLetter))) {
            ComputedStyle* parentStyle =
                f->parent()->firstLineStyle(f->parent(), f->parent()->style());
            if (parentStyle != f->parent()->style()) {
                f->node()->asElement()->setPseudoElement(
                    StyleResolver::PseudoElementType::
                        PseudoElementFirstLineInherited);
                return f->cachedPseudoStyle(StyleResolver::PseudoElementType::
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

void Frame::updateComputedStyle(Node* refNode)
{
    STARFISH_ASSERT(isAnonymous());
    ComputedStyle* newStyle = new ComputedStyle(refNode->style());
    newStyle->setDisplay(m_styleWhenNodeIsAnonymous->display());
    newStyle->loadResources(refNode, m_styleWhenNodeIsAnonymous);
    newStyle->arrangeStyleValues(refNode->style(), refNode);
    m_styleWhenNodeIsAnonymous = newStyle;
}

bool Frame::isDocumentElement() const
{
    return !isAnonymous() && m_node->document() == m_node;
}

Element* Frame::offsetParent()
{
    if (isDocumentElement() ||
        (!isAnonymous() && m_node->isHTMLBodyElement())) {
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
    }

    return node && node->isElement() ? node->asElement() : nullptr;
}

Document* Frame::document()
{
    STARFISH_ASSERT(node() || parent());
    return isAnonymous() ? parent()->document() : m_node->document();
}
}
