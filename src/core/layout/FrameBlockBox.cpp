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
#include "core/layout/FrameInline.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameText.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

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
        // Table starts its own layout algorithm that has minimum
        // interaction with the existing layout algorithm.
        // In brief, after establishes a table context, we calculate
        // the width of the table, and place cells in rows and columns.
        // To do so, we calculate x positions of cells first, and then
        // calculate the y positions of cells.
        FrameTableBox* tableBox = asFrameTableBox();
        tableBox->calCellWidth(ctx);
        tableBox->calCellWidthsWithColspans();
        tableBox->layoutWidth(ctx);
    } else {
        Length left = style()->left();
        Length right = style()->right();
        Length width = style()->width();
        BoxSizingValue boxSizing = style()->boxSizing();
        LayoutUnit contentWidth;

        if (width.isAuto()) {
            if (isNormalFlow() &&
                style()->display() != InlineBlockDisplayValue) {
                // https://www.w3.org/TR/CSS2/visudet.html#the-width-property
                // width of containing block =
                //     'margin-left' + 'border-left-width' + 'padding-left' +
                //     'width' + 'padding-right' + 'border-right-width' +
                //     'margin-right'
                contentWidth = std::max(containgBlockContentWidth - mbpWidth(),
                                        LayoutUnit(0));
            } else if (isAbsolutePositioned() && left.isSpecified() &&
                       right.isSpecified()) {
                LayoutUnit l = left.specifiedValue(containgBlockContentWidth);
                LayoutUnit r = right.specifiedValue(containgBlockContentWidth);
                LayoutUnit w =
                    std::max(LayoutUnit(0),
                             containgBlockContentWidth - l - r - mbpWidth());
                contentWidth = w;
            } else {
                PreferredWidthContext p(ctx,
                                        containgBlockContentWidth - mbpWidth());
                computePreferredWidth(p);
                contentWidth = p.preferredWidth();
            }
        } else {
            if (width.isFixed()) {
                contentWidth = width.fixed();
            } else {
                STARFISH_ASSERT(width.isPercent());
                contentWidth = containgBlockContentWidth * width.percent();
            }
            contentWidth = contentWidthApplyingBoxSizing(contentWidth);
        }

        applyMinMaxWidthIfNeeds(contentWidth, containgBlockContentWidth);
    }
}

void FrameBlockBox::computeContentHeight(LayoutContext& ctx, FrameBox* cb)
{
    STARFISH_ASSERT(isAbsolutePositioned() || cb == nullptr);

    if (isFrameTableBox()) {
        asFrameTableBox()->layoutHeight(ctx);
    } else {
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
            parentHeight = cb->contentHeight() + cb->paddingWidth();
            if (height.isAuto()) {
                Length top = style()->top();
                Length bottom = style()->bottom();
                if (top.isSpecified() && bottom.isSpecified()) {
                    LayoutUnit t = top.specifiedValue(parentHeight);
                    LayoutUnit b = bottom.specifiedValue(parentHeight);
                    contentHeight =
                        parentHeight - t - b - paddingHeight() - borderHeight();
                }
            } else {
                contentHeight = height.specifiedValue(parentHeight);
                contentHeight = contentHeightApplyingBoxSizing(contentHeight);
            }

            applyMinMaxHeightIfNeeds(contentHeight, parentHeight);
        } else {
            bool parentHasFixedValue = ctx.parentHasFixedHeight(this);

            if (parentHasFixedValue) {
                parentHeight = ctx.parentFixedHeight(this);
            } else {
                parentHeight = contentHeight;
            }

            if (height.isFixed()) {
                contentHeight = height.fixed();
                contentHeight = contentHeightApplyingBoxSizing(contentHeight);
            } else if (height.isPercent() && parentHasFixedValue) {
                contentHeight = height.specifiedValue(parentHeight);
                contentHeight = contentHeightApplyingBoxSizing(contentHeight);
            }

            applyMinMaxHeightIfNeeds(contentHeight, parentHeight,
                                     parentHasFixedValue);
        }
    }
}

static LayoutUnit specifiedVerticalPosition(LayoutContext& ctx, Frame* f,
                                            const Length& l)
{
    STARFISH_ASSERT(!l.isAuto());
    if (l.isFixed()) {
        return l.fixed();
    } else {
        STARFISH_ASSERT(l.isPercent());
        if (ctx.parentHasFixedHeight(f)) {
            return l.specifiedValue(ctx.parentFixedHeight(f));
        }
        return 0;
    }
}

static LayoutLocation relativeLocation(LayoutContext& ctx, Frame* f,
                                       LayoutSize parentSize)
{
    LayoutUnit x = 0;
    LayoutUnit y = 0;
    Length left = f->style()->left();
    Length right = f->style()->right();
    Length top = f->style()->top();
    Length bottom = f->style()->bottom();

    // left, right
    if (!left.isAuto() && !right.isAuto()) {
        if (f->style()->direction() == LtrDirectionValue) {
            x = left.specifiedValue(parentSize.width());
        } else {
            x = -right.specifiedValue(parentSize.height());
        }
    } else if (!left.isAuto()) {
        x = left.specifiedValue(parentSize.width());
    } else if (!right.isAuto()) {
        x = -right.specifiedValue(parentSize.height());
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
        y = specifiedVerticalPosition(ctx, f, top);
    } else if (!top.isAuto()) {
        y = specifiedVerticalPosition(ctx, f, top);
    } else if (!bottom.isAuto()) {
        y = -specifiedVerticalPosition(ctx, f, bottom);
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

    if (box->style()->left().isAuto() && box->style()->right().isAuto()) {
        box->moveX(loc.x());
    }
    if (box->style()->top().isAuto() && box->style()->bottom().isAuto()) {
        box->moveY(loc.y());
    }
}

void FrameBlockBox::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    BlockFormattingContextBlock blockFormattingContextBlock(this, ctx);
    // Determine the horizontal margins and the width of this object.
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        FrameBox* cb = ctx.containingBlock(this);
        LayoutUnit parentContentWidth = cb->contentWidth();
        computeBorderMarginPadding(parentContentWidth);

        if (isAbsolutePositioned()) {
            // 10.3.7 Absolutely positioned, non-replaced elements
            STARFISH_ASSERT(!isAnonymous());
            DirectionValue parentDirection =
                ctx.blockContainer(this)->style()->direction();
            HorizontalDataLocToContainingBlock data =
                computeHorizontalDataToContainingBlock(ctx, cb);

            Length left = style()->left();
            Length right = style()->right();
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
                    if (parentDirection == DirectionValue::LtrDirectionValue) {
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
                computeHorizontalMargin(parentContentWidth);
            }
        }
    }

    if (!(Frame::LayoutWantToResolve::ResolveHeight & resolveWhat)) {
        return;
    }

    FrameBox* cb = nullptr;
    if (isAbsolutePositioned()) {
        cb = ctx.containingBlock(this);
    }
    computeContentHeight(ctx, cb);

    // Now the intrinsic height of the object is known because the children are
    // placed

    // Determine the final height
    if (isAbsolutePositioned()) {
        VerticalDataLocToContainingBlock data =
            computeVerticalDataToContainingBlock(ctx, cb);

        Length top = style()->top();
        Length bottom = style()->bottom();
        Length height = style()->height();

        // 10.6.4 Absolutely positioned, non-replaced elements

        // For absolutely positioned elements, the used values of the vertical
        // dimensions must satisfy this constraint:
        // 'top' + 'margin-top' + 'border-top-width' + 'padding-top' + 'height'
        // + 'padding-bottom' + 'border-bottom-width' + 'margin-bottom' +
        // 'bottom' = height of containing block

        if (top.isAuto() && height.isAuto() && bottom.isAuto()) {
            // If all three of 'top', 'height', and 'bottom' are auto, set 'top'
            // to the static position and apply rule number three below.
        } else if (!top.isAuto() && !height.isAuto() && !bottom.isAuto()) {
            // If none of the three are 'auto': If both 'margin-top' and
            // 'margin-bottom' are 'auto', solve the equation under the extra
            // constraint that the two margins get equal values. If one of
            // 'margin-top' or 'margin-bottom' is 'auto', solve the equation
            // for that value. If the values are over-constrained, ignore the
            // value for 'bottom' and solve for that value.
            setY(data.m_top - data.m_absY);
        } else if (top.isAuto() && height.isAuto() && !bottom.isAuto()) {
            // 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then
            // the height is based on the content per 10.6.7 set 'auto' values
            // for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
            setY(data.m_contentHeight - asFrameBox()->contentHeight() -
                 paddingHeight() - borderHeight() - data.m_bottom -
                 data.m_absY);
        } else if (top.isAuto() && bottom.isAuto() && !height.isAuto()) {
            // 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then
            // set 'top' to the static position set 'auto' values for
            // 'margin-top' and 'margin-bottom' to 0, and  solve for 'bottom'
        } else if (height.isAuto() && bottom.isAuto() && !top.isAuto()) {
            // 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then
            // the height is based on the content per 10.6.7, set 'auto' values
            // for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
            setY(data.m_top - data.m_absY);
        } else if (top.isAuto() && !height.isAuto() && !bottom.isAuto()) {
            // 'top' is 'auto', 'height' and 'bottom' are not 'auto', then set
            // 'auto' values for 'margin-top' and 'margin-bottom' to 0, and
            // solve for 'top'
            LayoutUnit h = height.specifiedValue(data.m_contentHeight);
            if (style()->boxSizing() == BorderBoxBoxSizingValue) {
                setY(data.m_contentHeight - h - data.m_bottom - data.m_absY);
            } else {
                setY(data.m_contentHeight - h - data.m_bottom -
                     paddingHeight() - borderHeight() - data.m_absY);
            }
        } else if (height.isAuto() && !top.isAuto() && !bottom.isAuto()) {
            // 'height' is 'auto', 'top' and 'bottom' are not 'auto', then
            // 'auto' values for 'margin-top' and 'margin-bottom' are set to 0
            // and solve for 'height'
            setY(data.m_top - data.m_absY);
        } else {
            // 'bottom' is 'auto', 'top' and 'height' are not 'auto', then set
            // 'auto' values for 'margin-top' and 'margin-bottom' to 0 and solve
            // for 'bottom'
            STARFISH_ASSERT(bottom.isAuto() && !top.isAuto() &&
                            !height.isAuto());
            setY(data.m_top - data.m_absY);
        }

        applyVerticalMarginForAbsoluteBox();
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

// This function returns nullptr if
// * This block box contains no lineboxes, OR
// * This function is called before doing layout.
LineBox* FrameBlockBox::firstLineBox()
{
    LineBox* ret = nullptr;
    if (hasBlockFlow()) {
        for (Frame* child = firstChild(); child; child = child->next()) {
            if ((ret = child->firstLineBox())) {
                return ret;
            }
        }
    } else if (!m_lineBoxes.empty()) {
        return m_lineBoxes[0];
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

void FrameBlockBox::computeVisibleRect(StackingContext* sCtx,
                                       LayoutLocation& loc)
{
    if (!tryUniteVisibleRect(sCtx, loc)) {
        return;
    }

    VisibleRectContext ctx(this, &loc);

    if (hasBlockFlow()) {
        Frame* child = firstChild();
        while (child) {
            child->computeVisibleRect(sCtx, loc);
            child = child->next();
        }
    } else {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            m_lineBoxes[i]->computeVisibleRect(sCtx, loc);
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
    if (heightLength.isPercent() && !heightLength.isZero() &&
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
    if (style()->overflowX() != OverflowValue::VisibleOverflow ||
        style()->overflowY() != OverflowValue::VisibleOverflow) {
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

    Frame* result = nullptr;
    if (isPositioned()) {
        if (stage == HitTestPositionedElements) {
            HitTestStage s = HitTestStage::HitTestPositionedElements;
            while (s != HitTestStageEnd) {
                result = hitTestChildrenWith(x, y, s);
                if (result) {
                    return result;
                }
                s = (HitTestStage)(s + 1);
            }
            return FrameBox::hitTest(x, y, stage);
        }
    } else if (style()->display() == InlineBlockDisplayValue) {
        if (stage == HitTestNormalFlowInline) {
            HitTestStage s = HitTestStage::HitTestPositionedElements;
            while (s != HitTestStageEnd) {
                result = hitTestChildrenWith(x, y, s);
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
                result = hitTestChildrenWith(x, y, s);
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
                    LayoutUnit cx = x - child->asFrameBox()->x();
                    LayoutUnit cy = y - child->asFrameBox()->y();
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
            return hitTestChildrenWith(x, y, stage);
        }
    }
    return nullptr;
}

void FrameBlockBox::paint(PaintingContext& ctx)
{
    if (isEstablishesStackingContext()) {
        ctx.m_canvas->saveByFrame(this);
        return;
    }

    ctx.m_canvas->save();

    if (!isNormalFlow() || style()->display() == InlineBlockDisplayValue ||
        style()->display() == InlineTableDisplayValue) {
        ctx.m_canvas->resetTextDecorationData();
    } else {
        ctx.m_canvas->mergeTextDecorationData(style());
    }

    bool overflowApplied = shouldApplyOverflow();
    if (overflowApplied) {
        ctx.m_canvas->clip(Unit::Rect(0, 0, width(), height()));
    }

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        ctx.m_canvas->setVisible(false);
    } else {
        ctx.m_canvas->setVisible(true);
    }

    if (isPositioned()) {
        if (ctx.m_paintingStage == PaintingPositionedElements) {
            paintBackgroundAndBorders(ctx.m_canvas);
            if (overflowApplied) {
                ctx.m_canvas->clip(Unit::Rect(borderLeft(), borderTop(),
                                              width() - borderWidth(),
                                              height() - borderHeight()));
            }
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                ctx.m_paintingStage = s;
                paintChildrenWith(ctx);
                s = (PaintingStage)(s + 1);
            }
            ctx.m_paintingStage = PaintingPositionedElements;
        }
    } else if ((style()->display() == InlineBlockDisplayValue) ||
               (style()->display() == InlineTableDisplayValue)) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline &&
            ctx.m_paintingInlineStage == PaintingInlineBlock) {
            paintBackgroundAndBorders(ctx.m_canvas);
            if (overflowApplied) {
                ctx.m_canvas->clip(Unit::Rect(borderLeft(), borderTop(),
                                              width() - borderWidth(),
                                              height() - borderHeight()));
            }
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                ctx.m_paintingStage = s;
                paintChildrenWith(ctx);
                s = (PaintingStage)(s + 1);
            }
            ctx.m_paintingStage = PaintingNormalFlowInline;
        }
    } else if (isFloating()) {
        if (ctx.m_paintingStage == PaintingNonPositionedFloats &&
            ctx.m_paintingInlineStage == PaintingInlineBlock) {
            paintBackgroundAndBorders(ctx.m_canvas);
            if (overflowApplied) {
                ctx.m_canvas->clip(Unit::Rect(borderLeft(), borderTop(),
                                              width() - borderWidth(),
                                              height() - borderHeight()));
            }
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                ctx.m_paintingStage = s;
                paintChildrenWith(ctx);
                s = (PaintingStage)(s + 1);
            }
            ctx.m_paintingStage = PaintingNonPositionedFloats;
        }
    } else {
        if (ctx.m_paintingStage == PaintingNormalFlowBlock) {
            paintBackgroundAndBorders(ctx.m_canvas);
        }
        if (overflowApplied) {
            ctx.m_canvas->clip(Unit::Rect(borderLeft(), borderTop(),
                                          width() - borderWidth(),
                                          height() - borderHeight()));
        }
        paintChildrenWith(ctx);
    }

    ctx.m_canvas->restore();
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
