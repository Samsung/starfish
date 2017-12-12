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
#include "core/dom/Element.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameText.h"
#include "core/layout/StackingContext.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

void* FrameBlockBoxRareData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameBlockBoxRareData)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBoxRareData, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBoxRareData, m_stackingContext));
#if defined(PORT_GRAPHIC_BACKEND_EFL)
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameBlockBoxRareData,
                                              m_bufferForBorderRadius));
#endif
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameBlockBoxRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

LayoutUnit FrameBlockBox::scrollLeft()
{
    if (node()) {
        return node()->asElement()->scrollLeft(false);
    }
    return 0;
}

LayoutUnit FrameBlockBox::scrollTop()
{
    if (node()) {
        return node()->asElement()->scrollTop(false);
    }
    return 0;
}

class BlockFormattingContextBlock {
public:
    BlockFormattingContextBlock(Frame* frm, LayoutContext& ctx)
        : m_ctx(ctx)
        , m_needs(false)
    {
        if (frm->isEstablishesBlockFormattingContext()) {
            m_needs = true;
            m_ctx.establishBlockFormattingContext(frm->isNormalFlow());
        }
    }
    ~BlockFormattingContextBlock()
    {
        if (m_needs) {
            m_ctx.removeBlockFormattingContext();
        }
    }

    LayoutContext& m_ctx;
    bool m_needs;
};

void FrameBlockBox::computeContentWidth(LayoutContext& ctx,
                                        LayoutUnit containgBlockContentWidth)
{
    if (isFrameTableBox()) {
        FrameTableBox* tableBox = asFrameTableBox();
        tableBox->computeTableWidth(ctx);
    } else {
        LengthData offset = style()->offset();
        Length left = offset.left();
        Length right = offset.right();
        Length width = style()->width();
        BoxSizingValue boxSizing = style()->boxSizing();
        LayoutUnit contentWidth;

        if (width.isAuto()) {
            if (isNormalFlow() && !isAtomicInlineLevel()) {
                // https://www.w3.org/TR/CSS2/visudet.html#the-width-property
                // width of containing block =
                //     'margin-left' + 'border-left-width' + 'padding-left' +
                //     'width' + 'padding-right' + 'border-right-width' +
                //     'margin-right'
                contentWidth = std::max(containgBlockContentWidth - mbpWidth(),
                                        LayoutUnit(0));
            } else if (isAbsolutePositioned() && left.isSpecified() &&
                       right.isSpecified()) {
                LayoutUnit l =
                    left.specifiedValue(containgBlockContentWidth, this);
                LayoutUnit r =
                    right.specifiedValue(containgBlockContentWidth, this);
                LayoutUnit w =
                    std::max(LayoutUnit(0),
                             containgBlockContentWidth - l - r - mbpWidth());
                contentWidth = w;
            } else {
                PreferredWidthContext p(ctx, this,
                                        containgBlockContentWidth - mbpWidth());
                p.computePreferredWidth();
                contentWidth = p.preferredWidth();
            }
        } else {
            contentWidth =
                width.specifiedValue(containgBlockContentWidth, this);
            contentWidth = contentWidthApplyingBoxSizing(contentWidth);
        }

        applyMinMaxWidthIfNeeds(ctx, contentWidth, containgBlockContentWidth);
    }
}

void FrameBlockBox::computeContentHeight(LayoutContext& ctx, FrameBox* cb)
{
    if (isFrameTableBox()) {
        asFrameTableBox()->layoutTable(ctx);
    } else if (isFrameFlexibleBox()) {
        asFrameFlexibleBox()->layoutFlex(ctx);
    } else {
        STARFISH_ASSERT(isAbsolutePositioned() || cb == nullptr);

        LayoutUnit contentHeight;
        LayoutUnit parentHeight;
        Length height = style()->height();
        BoxSizingValue boxSizing = style()->boxSizing();

        if (hasBlockFlow()) {
            contentHeight = layoutBlock(ctx);
        } else {
            contentHeight = layoutInline(ctx);
        }

        // The contentHeight is used when table cell contents are vertically
        // aligned in the table row.
        if (isFrameTableCellBox()) {
            asFrameTableCellBox()->setActualContentHeight(contentHeight);
        }

        if (isAbsolutePositioned()) {
            parentHeight = cb->contentHeight() + cb->paddingHeight();
            if (height.isAuto()) {
                LengthData offset = style()->offset();
                Length top = offset.top();
                Length bottom = offset.bottom();
                if (top.isSpecified() && bottom.isSpecified()) {
                    LayoutUnit t = top.specifiedValue(parentHeight, this);
                    LayoutUnit b = bottom.specifiedValue(parentHeight, this);
                    contentHeight =
                        parentHeight - t - b - paddingHeight() - borderHeight();
                }
            } else {
                contentHeight = height.specifiedValue(parentHeight, this);
                contentHeight = contentHeightApplyingBoxSizing(contentHeight);
            }

            applyMinMaxHeightIfNeeds(ctx, contentHeight, parentHeight);
        } else {
            computeContentHeight(ctx, contentHeight);
        }
    }

    if (isFlexItem()) {
        ctx.registerContentHeight(this, contentHeight());
    }
}

void FrameBlockBox::computeContentHeight(LayoutContext& ctx,
                                         LayoutUnit contentHeight)
{
    bool parentHasFixedHeight = ctx.parentHasFixedHeight(this);
    LayoutUnit parentHeight;
    if (parentHasFixedHeight) {
        parentHeight = ctx.parentFixedHeight(this);
    } else {
        parentHeight = contentHeight;
    }

    Length height = style()->height();
    if (height.isDefinite(parentHasFixedHeight)) {
        contentHeight = height.specifiedValue(parentHeight, this);
        contentHeight = contentHeightApplyingBoxSizing(contentHeight);
    }

    applyMinMaxHeightIfNeeds(ctx, contentHeight, parentHeight,
                             parentHasFixedHeight);
}

static LayoutLocation relativeLocation(LayoutContext& ctx, Frame* f,
                                       LayoutSize parentSize)
{
    LayoutUnit x = 0;
    LayoutUnit y = 0;
    LengthData offset = f->style()->offset();
    Length left = offset.left();
    Length right = offset.right();
    Length top = offset.top();
    Length bottom = offset.bottom();

    // left, right
    if (!left.isAuto() && !right.isAuto()) {
        if (f->style()->direction() == LtrDirectionValue) {
            x = left.specifiedValue(parentSize.width(), f);
        } else {
            x = -right.specifiedValue(parentSize.width(), f);
        }
    } else if (!left.isAuto()) {
        x = left.specifiedValue(parentSize.width(), f);
    } else if (!right.isAuto()) {
        x = -right.specifiedValue(parentSize.width(), f);
    }

    // NOTE: In latest css spec, relative position is decided after the size of
    // containing block is fixed with recursive calculation, while the position
    // is decided "before" the containing block's size is fixed in nodewebkit
    // or Chrome. we can compute percentage of top, bottom offset correctly
    // but, modern browsers(webkit, blink, gecko..) are not compute position
    // correctly yet we change implementations follow modern browsers

    // top, bottom
    /*
    // original code
    if (!top.isAuto() && !bottom.isAuto()) {
        y = top.specifiedValue(parentHeight);
    } else if (!top.isAuto() && bottom.isAuto()) {
        y = top.specifiedValue(parentHeight);
    } else if (top.isAuto() && !bottom.isAuto()) {
        y = -bottom.specifiedValue(parentHeight);
    }*/

    if (!top.isAuto() && !bottom.isAuto()) {
        y = ctx.specifiedVerticalValue(f, top);
    } else if (!top.isAuto()) {
        y = ctx.specifiedVerticalValue(f, top);
    } else if (!bottom.isAuto()) {
        y = -ctx.specifiedVerticalValue(f, bottom);
    }

    return LayoutLocation(x, y);
}

void LayoutContext::applyRelativePosition(FrameBox* box)
{
    FrameBox* bc = blockContainer(box);
    LayoutUnit parentWidth = bc->contentWidth();
    LayoutUnit parentHeight = bc->contentHeight();

    LayoutLocation loc =
        relativeLocation(*this, box, LayoutSize(parentWidth, parentHeight));

    box->moveX(loc.x());
    box->moveY(loc.y());
}

void LayoutContext::applyRelativePositionInlineCase(Frame* origin,
                                                    FrameBox* box)
{
    FrameBox* bc = blockContainer(origin);
    LayoutUnit parentWidth = bc->contentWidth();
    LayoutUnit parentHeight = bc->contentHeight();

    LayoutLocation loc =
        relativeLocation(*this, origin, LayoutSize(parentWidth, parentHeight));
    LengthData offset = box->style()->offset();

    if (offset.left().isAuto() && offset.right().isAuto()) {
        box->moveX(loc.x());
    }
    if (offset.top().isAuto() && offset.bottom().isAuto()) {
        box->moveY(loc.y());
    }
}

LayoutRect FrameBlockBox::computeVisibleRectForScroll()
{
    LayoutRect visibleRect = LayoutRect(0, 0, width(), height());
    SkMatrix loc = SkMatrix::I();
    Frame::ComputeVisibleRectContext vctx(
        Frame::ComputeVisibleRectContext::Scrolling, nullptr, loc, visibleRect);
    if (hasBlockFlow()) {
        Frame* child = firstChild();
        while (child) {
            child->computeVisibleRect(vctx);
            child = child->next();
        }
    } else {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            m_lineBoxes[i]->computeVisibleRect(vctx);
        }
    }
    return visibleRect;
}

void FrameBlockBox::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    Frame::computePaintingFlags(ctx, resolveWhat);

    BlockFormattingContextBlock blockFormattingContextBlock(this, ctx);
    // Determine the horizontal margins and the width of this object.
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        FrameBox* cb = containingBlock(this);
        LayoutUnit parentContentWidth = cb->contentWidth();
        DirectionValue parentDirection;
        if (isAbsolutePositioned()) {
            parentDirection = blockContainer(this)->style()->direction();
        } else {
            parentDirection = cb->style()->direction();
        }
        computeBorderMarginPadding(ctx, parentContentWidth);

        if (isAbsolutePositioned()) {
            // 10.3.7 Absolutely positioned, non-replaced elements
            STARFISH_ASSERT(!isAnonymous());
            HorizontalDataLocToContainingBlock data =
                computeHorizontalDataToContainingBlock(ctx, cb);

            LengthData offset = style()->offset();
            Length left = offset.left();
            Length right = offset.right();
            Length width = style()->width();

            if (left.isAuto() && right.isAuto()) {
                if (width.isAuto()) {
                    if (parentDirection == LtrDirectionValue) {
                        computeContentWidth(ctx, data.m_contentWidth -
                                                     data.m_absX - x());
                    } else {
                        computeContentWidth(ctx, x() + data.m_absX);
                    }
                } else {
                    computeContentWidth(ctx, data.m_contentWidth);
                }

                if (parentDirection == LtrDirectionValue) {
                    moveX(FrameBox::marginLeft());
                } else {
                    // if the 'direction' property of the element
                    // establishing the static-position containing block is
                    // 'ltr' set 'left' to the static position, otherwise set
                    // 'right' to the static position. Then solve for 'left'
                    // (if 'direction is 'rtl') or 'right' (if 'direction' is
                    // 'ltr').
                    moveX(-FrameBox::width() - FrameBox::marginRight());
                }
            } else if (!left.isAuto() && !right.isAuto()) {
                computeContentWidth(ctx, data.m_contentWidth);
                if (width.isAuto()) {
                    setX(data.m_left + FrameBox::marginLeft() - data.m_absX);
                } else {
                    // If none of the three is 'auto':
                    // If both 'margin-left' and 'margin-right' are 'auto',
                    // solve the equation under the extra constraint that the
                    // two margins get equal values, unless this would make
                    // them negative, in which case when  direction of the
                    // containing block is 'ltr' ('rtl'), set 'margin-left'
                    // ('margin-right') to zero and solve for 'margin-right'
                    // ('margin-left'). If one of 'margin-left' or
                    // 'margin-right' is 'auto', solve the equation for that
                    // value. If the values are over-constrained, ignore the
                    // value for 'left' (in case the 'direction' property of
                    // the containing block is 'rtl') or 'right' (in case
                    // 'direction' is 'ltr') and solve for that value.
                    computeHorizontalMargin(data.m_contentWidth - data.m_left -
                                                data.m_right,
                                            parentDirection);
                    LengthData margin = style()->margin();
                    Length marginLeft = margin.left();
                    Length marginRight = margin.right();
                    bool relativeToLeft = false;

                    if (marginLeft.isAuto() && marginRight.isAuto()) {
                        relativeToLeft = parentDirection ==
                                         DirectionValue::LtrDirectionValue;
                    } else if (marginLeft.isAuto()) {
                        relativeToLeft = false;
                    } else if (marginRight.isAuto()) {
                        relativeToLeft = true;
                    } else {
                        relativeToLeft = parentDirection ==
                                         DirectionValue::LtrDirectionValue;
                    }

                    if (relativeToLeft) {
                        setX(data.m_left + FrameBox::marginLeft() -
                             data.m_absX);
                    } else {
                        setX(parentContentWidth - FrameBox::width() -
                             data.m_right - FrameBox::marginRight() -
                             data.m_absX);
                    }
                }
            } else {
                if (width.isAuto()) {
                    computeContentWidth(ctx, data.m_contentWidth - data.m_left -
                                                 data.m_right);
                } else {
                    computeContentWidth(ctx, data.m_contentWidth);
                }

                if (left.isSpecified()) {
                    setX(data.m_left + FrameBox::marginLeft() - data.m_absX);
                } else {
                    setX(data.m_contentWidth - data.m_right -
                         FrameBox::width() - FrameBox::marginRight() -
                         data.m_absX);
                }
            }
        } else {
            // 10.3.3 Block-level, non-replaced elements in normal flow
            // 10.3.5 Floating, non-replaced elements
            computeContentWidth(ctx, parentContentWidth);
            if (isNormalFlow() && isBlockLevel()) {
                computeHorizontalMargin(parentContentWidth, parentDirection);
            }
        }
    }

    if (!(Frame::LayoutWantToResolve::ResolveHeight & resolveWhat)) {
        return;
    }

    FrameBox* cb = nullptr;
    if (isAbsolutePositioned()) {
        cb = containingBlock(this);
    }

    computeContentHeight(ctx, cb);

    // Now the intrinsic height of the object is known because the children are
    // placed

    // Determine the final height
    if (isAbsolutePositioned()) {
        VerticalDataLocToContainingBlock data =
            computeVerticalDataToContainingBlock(ctx, cb);

        LengthData offset = style()->offset();
        Length top = offset.top();
        Length bottom = offset.bottom();

        // 10.6.4 Absolutely positioned, non-replaced elements

        // For absolutely positioned elements, the used values of the vertical
        // dimensions must satisfy this constraint:
        // 'top' + 'margin-top' + 'border-top-width' + 'padding-top' + 'height'
        // + 'padding-bottom' + 'border-bottom-width' + 'margin-bottom' +
        // 'bottom' = height of containing block

        if (top.isAuto() && bottom.isAuto()) {
            // static location computed in normal flow processing
            moveY(marginTop());
        } else if (!top.isAuto() && bottom.isAuto()) {
            setY(data.m_top - data.m_absY + marginTop());
        } else if (top.isAuto() && !bottom.isAuto()) {
            setY(data.m_contentHeight - data.m_bottom - outerHeight() -
                 data.m_absY + marginTop());
        } else {
            computeVerticalMargin(data.m_contentHeight - data.m_top -
                                  data.m_bottom);
            setY(data.m_top - data.m_absY + marginTop());
        }
    } else if (style()->position() == PositionValue::RelativePositionValue) {
        ctx.registerRelativePositionedBox(this, true);
    }

    // layout absolute positioned blocks
    ctx.layoutRegisteredAbsolutePositionedBoxes(this);

    // layout relative positioned blocks
    ctx.layoutRegisteredRelativePositionedBoxes(this);

    if (node() && node()->parentElement()) {
        Node* nd = node()->parentElement();
        if (nd->frame()->isFrameInline() &&
            nd->style()->position() == RelativePositionValue) {
            ctx.registerRelativePositionedBox(this, false);
        }
    }

    if (m_flags.m_hasBiggerContentThanFrameWidth) {
        frameBlockBoxRareData()->m_scrollWidth = 0;
        m_flags.m_hasBiggerContentThanFrameWidth = false;
    }
    if (m_flags.m_hasBiggerContentThanFrameHeight) {
        frameBlockBoxRareData()->m_scrollHeight = 0;
        m_flags.m_hasBiggerContentThanFrameHeight = false;
    }

    m_flags.m_needsToComputeScrollVisbleRect = true;

    auto overflowX = appliedOverflowX();
    if (isFrameDocument()) {
        overflowX = OverflowValue::AutoOverflow;
    }
    auto overflowY = appliedOverflowY();
    if (isFrameDocument()) {
        overflowY = OverflowValue::AutoOverflow;
    }

    if (overflowX >= AutoOverflow || overflowY >= AutoOverflow) {
        updateScrollWidthAndHeightIfNeeds(overflowX, overflowY);
    }

    if (overflowX >= AutoOverflow) {
        LayoutUnit* u;
        if (node()->isElement()) {
            u = &node()->asElement()->ensureRareElementMembers()->m_scrollLeft;
        } else {
            STARFISH_ASSERT(node()->isDocument());
            u = &asFrameDocument()->m_scrollLeft;
        }
        LayoutUnit scrollWidth = this->scrollWidth();
        if (*u > scrollWidth - width()) {
            *u = scrollWidth - width();
            if (*u < 0) {
                *u = 0;
            }
        }
    } else {
        if (node() && node()->isElement() && node()->hasRareMembers()) {
            node()->asElement()->ensureRareElementMembers()->m_scrollLeft = 0;
        }
    }

    if (overflowY >= AutoOverflow) {
        LayoutUnit* u;
        if (node()->isElement()) {
            u = &node()->asElement()->ensureRareElementMembers()->m_scrollTop;
        } else {
            STARFISH_ASSERT(node()->isDocument());
            u = &asFrameDocument()->m_scrollTop;
        }
        LayoutUnit scrollHeight = this->scrollHeight();
        if (*u > scrollHeight - height()) {
            *u = scrollHeight - height();
            if (*u < 0) {
                *u = 0;
            }
        }
    } else {
        if (node() && node()->isElement() && node()->hasRareMembers()) {
            node()->asElement()->ensureRareElementMembers()->m_scrollTop = 0;
        }
    }
}

void FrameBlockBox::updateScrollWidthAndHeightIfNeeds()
{
    auto overflowX = appliedOverflowX();
    if (isFrameDocument()) {
        overflowX = OverflowValue::AutoOverflow;
    }
    auto overflowY = appliedOverflowY();
    if (isFrameDocument()) {
        overflowY = OverflowValue::AutoOverflow;
    }
    updateScrollWidthAndHeightIfNeeds(overflowX, overflowY);
}

void FrameBlockBox::updateScrollWidthAndHeightIfNeeds(OverflowValue overflowX,
                                                      OverflowValue overflowY)
{
    if (m_flags.m_needsToComputeScrollVisbleRect) {
        m_flags.m_needsToComputeScrollVisbleRect = false;

        if (isFrameDocument()) {
            overflowX = OverflowValue::AutoOverflow;
            overflowY = OverflowValue::AutoOverflow;
        }

        LayoutRect visibleRect = computeVisibleRectForScroll();
        LayoutUnit scrollWidth = visibleRect.width();
        if (visibleRect.x() < 0) {
            scrollWidth += visibleRect.x();
        }

        if (scrollWidth > width() &&
            overflowX != OverflowValue::HiddenOverflow) {
            m_flags.m_hasBiggerContentThanFrameWidth = true;
            ensureFrameBoxRareData();
            frameBlockBoxRareData()->m_scrollWidth = scrollWidth;
        }

        LayoutUnit scrollHeight = visibleRect.height();
        if (visibleRect.y() < 0) {
            scrollHeight += visibleRect.y();
        }

        if (scrollHeight > height() &&
            overflowY != OverflowValue::HiddenOverflow) {
            m_flags.m_hasBiggerContentThanFrameHeight = true;
            ensureFrameBoxRareData();
            frameBlockBoxRareData()->m_scrollHeight = scrollHeight;
        }
    }
}

InlineNonReplacedBox* FrameBlockBox::firstInlineNonReplacedBox(FrameInline* f)
{
    InlineNonReplacedBox* ret = nullptr;

    if (hasBlockFlow()) {
        Frame* child = firstChild();
        while (child) {
            if ((ret = child->firstInlineNonReplacedBox(f))) {
                return ret;
            }

            child = child->next();
        }
    } else {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            if ((ret = m_lineBoxes[i]->firstInlineNonReplacedBox(f))) {
                return ret;
            }
        }
    }

    return ret;
}

void FrameBlockBox::establishesStackingContextIfNeeds()
{
    FrameBox::establishesStackingContextIfNeeds();

    if (hasBlockFlow()) {
        Frame* child = firstChild();
        while (child) {
            child->establishesStackingContextIfNeeds();
            child = child->next();
        }
    } else {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            m_lineBoxes[i]->establishesStackingContextIfNeeds();
        }
    }
}

void FrameBlockBox::computeVisibleRect(Frame::ComputeVisibleRectContext& ctx)
{
    Frame::ComputeVisibleRectContextFragment f(ctx, this);

    if (!tryUniteVisibleRect(ctx)) {
        return;
    }

    if (hasBlockFlow()) {
        Frame* child = firstChild();
        while (child) {
            child->computeVisibleRect(ctx);
            child = child->next();
        }
    } else {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            m_lineBoxes[i]->computeVisibleRect(ctx);
        }
    }
}

bool FrameBlockBox::isSelfCollapsingBlock(LayoutContext& ctx)
{
    if (isEstablishesBlockFormattingContext()) {
        return false;
    }

    if (!isNecessaryBlockBox()) {
        return true;
    }

    if (heightComputed()) {
        return false;
    }

    if (paddingHeight() || borderHeight()) {
        return false;
    }

    Length heightLength = style()->height();
    // NOTE: In case of percentage height,
    // if containing blocks' height is fixed, the block is not
    // self-collapsing block.
    // TODO: should we handle the case : calc(100% - 100% + 10px)
    if (((heightLength.isPercent() && !heightLength.isZero()) ||
         (heightLength.isCalc() && !heightLength.isCalcAndLengthOfType())) &&
        ctx.parentHasFixedHeight(this)) {
        return false;
    }

    if (heightLength.isAuto() || heightLength.isZero()) {
        Frame* child = firstChild();
        while (child) {
            if (!child->isNormalFlow()) {
                child = child->next();
                continue;
            }
            if (!child->isSelfCollapsingBlock(ctx)) {
                return false;
            }
            child = child->next();
        }
        return true;
    }
    return false;
}

Frame* FrameBlockBox::hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                          HitTestStage s)
{
    if (shouldApplyOverflow()) {
        if (FrameBox::hitTest(x, y, HitTestStageEnd) == nullptr) {
            return nullptr;
        }
    }

    Frame* result = nullptr;
    if (hasBlockFlow()) {
        Frame* child = lastChild();
        while (child) {
            LayoutUnit cx = x - child->asFrameBox()->x();
            LayoutUnit cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx, cy, s);
            if (result) {
                return result;
            }
            child = child->previous();
        }
    } else {
        auto iterL = m_lineBoxes.rbegin();
        while (iterL != m_lineBoxes.rend()) {
            LineBox& b = **iterL;
            LayoutUnit cx = x - b.m_frameRect.x();
            LayoutUnit cy = y - b.m_frameRect.y();
            auto iter = b.m_boxes.rbegin();
            while (iter != b.m_boxes.rend()) {
                FrameBox* childBox = *iter;
                LayoutUnit cx2 = cx - childBox->x();
                LayoutUnit cy2 = cy - childBox->y();
                result = childBox->hitTest(cx2, cy2, s);
                if (result) {
                    return result;
                }
                iter++;
            }
            iterL++;
        }
    }

    return result;
}

Frame* FrameBlockBox::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
{
    if (isEstablishesStackingContext()) {
        return nullptr;
    }

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        return nullptr;
    }

    LayoutUnit childX = x + scrollLeft();
    LayoutUnit childY = y + scrollTop();

    Frame* result = nullptr;
    if (isPositioned()) {
        if (stage == HitTestPositionedElements) {
            HitTestStage s = HitTestStage::HitTestPositionedElements;
            while (s != HitTestStageEnd) {
                result = hitTestChildrenWith(childX, childY, s);
                if (result) {
                    return result;
                }
                s = (HitTestStage)(s + 1);
            }
            return FrameBox::hitTest(x, y, stage);
        }
    } else if (isInlineLevel() || isFlexItem()) {
        if (stage == HitTestNormalFlowInline) {
            HitTestStage s = HitTestStage::HitTestPositionedElements;
            while (s != HitTestStageEnd) {
                result = hitTestChildrenWith(childX, childY, s);
                if (result) {
                    return result;
                }
                s = (HitTestStage)(s + 1);
            }
            return FrameBox::hitTest(x, y, stage);
        }
    } else if (isFloating()) {
        if (stage == HitTestNonPositionedFloats) {
            HitTestStage s = HitTestStage::HitTestPositionedElements;
            while (s != HitTestStageEnd) {
                result = hitTestChildrenWith(childX, childY, s);
                if (result) {
                    return result;
                }
                s = (HitTestStage)(s + 1);
            }
            return FrameBox::hitTest(x, y, stage);
        }
    } else {
        if (stage == HitTestNormalFlowBlock) {
            if (hasBlockFlow()) {
                Frame* result = nullptr;
                Frame* child = lastChild();
                while (child) {
                    LayoutUnit cx = childX - child->asFrameBox()->x();
                    LayoutUnit cy = childY - child->asFrameBox()->y();
                    result = child->hitTest(cx, cy, stage);
                    if (result) {
                        return result;
                    }
                    child = child->previous();
                }

                return FrameBox::hitTest(x, y, stage);
            } else {
                return isAnonymous() ? nullptr : FrameBox::hitTest(x, y, stage);
            }
        } else {
            return hitTestChildrenWith(childX, childY, stage);
        }
    }
    return nullptr;
}

void* FrameBlockBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameBlockBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameBlockBox, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameBlockBox, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameBlockBox, m_lineBoxes));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameBlockBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

#ifdef STARFISH_ENABLE_TEST
void FrameBlockBox::dump(int depth)
{
    FrameBox::dump(depth);
    if (!hasBlockFlow()) {
        if (m_lineBoxes.size() && m_lineBoxes[0]->m_boxes.size()) {
            for (size_t i = 0; i < m_lineBoxes.size(); i++) {
                puts("");
                for (int k = 0; k < depth + 1; k++) {
                    printf("  ");
                }
                printf("LineBox (%g,%g,%g,%g)\n",
                       (float)m_lineBoxes[i]->m_frameRect.x(),
                       (float)m_lineBoxes[i]->m_frameRect.y(),
                       (float)m_lineBoxes[i]->m_frameRect.width(),
                       (float)m_lineBoxes[i]->m_frameRect.height());

                LineBox& lb = *m_lineBoxes[i];
                for (size_t k = 0; k < lb.m_boxes.size(); k++) {
                    FrameBox* childBox = lb.m_boxes[k];
                    for (int j = 0; j < depth + 2; j++) {
                        printf("  ");
                    }
                    childBox->dump(depth + 3);
                    if (k != lb.m_boxes.size() - 1) {
                        puts("");
                    }
                }
            }
        }
    }
}
#endif
}
