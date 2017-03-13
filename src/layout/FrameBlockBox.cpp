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
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"
#include "FrameDocument.h"
#include "FrameTableBox.h"
#include "FrameTableCellBox.h"

namespace StarFish {

class BlockFormattingContextBlock {
public:
    BlockFormattingContextBlock(Frame* frm, LayoutContext& ctx)
        : m_ctx(ctx), m_needs(false)
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
                                        LayoutUnit containgBlockContentWidth,
                                        LayoutUnit absX)
{
    Length marginLeft = style()->marginLeft();
    Length marginRight = style()->marginRight();
    Length left = style()->left();
    Length right = style()->right();
    Length width = style()->width();
    DirectionValue direction = style()->direction();

    if (width.isAuto()) {
        LayoutUnit parentWidth;
        if (style()->position() == AbsolutePositionValue) {
            if (left.isAuto() && !right.isAuto()) {
                LayoutUnit r = right.specifiedValue(containgBlockContentWidth);
                parentWidth = containgBlockContentWidth - r;
            } else if (right.isAuto() && !left.isAuto() &&
                       direction == RtlDirectionValue) {
                LayoutUnit l = left.specifiedValue(containgBlockContentWidth);
                parentWidth = containgBlockContentWidth - l;
            } else {
                if (style()->direction() == LtrDirectionValue) {
                    parentWidth = containgBlockContentWidth - absX - x();
                } else {
                    parentWidth = x() + absX;
                }
            }
        } else {
            STARFISH_ASSERT(isFloating());
            parentWidth = containgBlockContentWidth;
        }

        PreferredWidthContext p(ctx, parentWidth - mbpWidth());
        computePreferredWidth(p);
        applyMinMaxWidthIfNeeds(p.preferredWidth(), containgBlockContentWidth);
    } else if (width.isFixed()) {
        applyMinMaxWidthIfNeeds(width.fixed(), containgBlockContentWidth);
    } else if (width.isPercent()) {
        applyMinMaxWidthIfNeeds(containgBlockContentWidth * width.percent(),
                                containgBlockContentWidth);
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
    } else if (!left.isAuto() && right.isAuto()) {
        x = left.specifiedValue(parentSize.width());
    } else if (left.isAuto() && !right.isAuto()) {
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
    } else if (!top.isAuto() && bottom.isAuto()) {
        y = specifiedVerticalPosition(ctx, f, top);
    } else if (top.isAuto() && !bottom.isAuto()) {
        y = -specifiedVerticalPosition(ctx, f, bottom);
    }

    return LayoutLocation(x, y);
}

void LayoutContext::applyRelativePosition(FrameBox* box)
{
    FrameBlockBox* cb = containingFrameBlockBox(box);
    LayoutUnit parentWidth = cb->contentWidth();
    LayoutUnit parentHeight = cb->contentHeight();

    LayoutLocation loc =
        relativeLocation(*this, box, LayoutSize(parentWidth, parentHeight));

    box->moveX(loc.x());
    box->moveY(loc.y());
}

void LayoutContext::applyRelativePositionInlineCase(Frame* refF, FrameBox* box)
{
    FrameBox* cb = containingBlock(refF);
    LayoutUnit parentWidth = cb->contentWidth();
    LayoutUnit parentHeight = cb->contentHeight();

    LayoutLocation loc =
        relativeLocation(*this, refF, LayoutSize(parentWidth, parentHeight));

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
        if (isNormalFlow()) {
            // https://www.w3.org/TR/CSS2/visudet.html#the-width-property
            FrameBlockBox* cb = ctx.blockContainer(this);
            LayoutUnit parentContentWidth = cb->contentWidth();
            computeBorderMarginPadding(parentContentWidth);

            // width of containing block =
            //     'margin-left' + 'border-left-width' + 'padding-left' +
            //     'width' + 'padding-right' + 'border-right-width' +
            //     'margin-right'
            Length width = style()->width();
            Length marginLeft = style()->marginLeft();
            Length marginRight = style()->marginRight();

            if (width.isAuto()) {
                LayoutUnit remainedWidth = parentContentWidth - mbpWidth();
                if (style()->display() == InlineBlockDisplayValue) {
                    PreferredWidthContext p(ctx, remainedWidth);
                    computePreferredWidth(p);
                    applyMinMaxWidthIfNeeds(p.preferredWidth(),
                                            parentContentWidth);
                } else {
                    if (remainedWidth < 0) {
                        remainedWidth = 0;
                    }
                    applyMinMaxWidthIfNeeds(remainedWidth, parentContentWidth);
                }
            } else if (width.isFixed()) {
                if (cb->style()->width().isFixed()) {
                    applyMinMaxWidthIfNeeds(width.fixed(), parentContentWidth);
                } else {
                    applyMinMaxWidthIfNeeds(width.fixed(), width.fixed());
                }
            } else {
                STARFISH_ASSERT(width.isPercent());
                applyMinMaxWidthIfNeeds(parentContentWidth * width.percent(),
                                        parentContentWidth);
            }

            if (style()->display() == BlockDisplayValue) {
                computeHorizontalMargin(parentContentWidth);
            }
        } else if (style()->position() ==
                   PositionValue::AbsolutePositionValue) {
            FrameBox* cb = ctx.containingBlock(this);
            computeBorderMarginPadding(cb->contentWidth());

            STARFISH_ASSERT(!isAnonymous());
            FrameBox* parent = Frame::layoutParent()->asFrameBox();
            DirectionValue parentDirection =
                ctx.blockContainer(this)->style()->direction();

            LayoutLocation l1, l2;
            if (cb->isAncestorOf(parent)) {
                l2 = parent->absolutePoint(cb);
            } else {
                l1 = cb->absolutePoint(ctx.frameDocument());
                l2 = parent->absolutePoint(ctx.frameDocument());
            }
            LayoutUnit absX = l2.x() - l1.x() - cb->borderLeft();

            // 10.3.7 Absolutely positioned, non-replaced elements
            // The constraint that determines the used values for these elements
            // is:
            // width of containing block =
            //     'left' + 'margin-left' + 'border-left-width' + 'padding-left'
            //     + 'width' + 'padding-right' + 'border-right-width'
            //     + 'margin-right' + 'right'

            // Then, if the 'direction' property of the element establishing
            // the static-position containing block is 'ltr' set 'left' to
            // the static position and apply rule number three below;
            // otherwise, set 'right' to the static position and apply rule
            // number one below.

            Length marginLeft = style()->marginLeft();
            Length marginRight = style()->marginRight();
            Length left = style()->left();
            Length right = style()->right();
            Length width = style()->width();

            LayoutUnit containgBlockContentWidth =
                cb->contentWidth() + cb->paddingWidth();

            if (left.isAuto() && width.isAuto() && right.isAuto()) {
                // If all three of 'left', 'width', and 'right' are 'auto':
                // First set any 'auto' values for 'margin-left' and
                // 'margin-right' to 0
                if (marginLeft.isAuto()) {
                    setMarginLeft(0);
                }
                if (marginRight.isAuto()) {
                    setMarginRight(0);
                }

                computeContentWidth(ctx, containgBlockContentWidth, absX);
                applyHorizontalMargin();
            } else if (!left.isAuto() && !width.isAuto() && !right.isAuto()) {
                // If none of the three is 'auto':
                // If both 'margin-left' and 'margin-right' are 'auto',
                // solve the equation under the extra constraint that the two
                // margins get equal values, unless this would make them
                // negative, in which case when  direction of the containing
                // block is 'ltr' ('rtl'), set 'margin-left' ('margin-right')
                // to zero and solve for 'margin-right' ('margin-left').
                // If one of 'margin-left' or 'margin-right' is 'auto', solve
                // the equation for that value. If the values are
                // over-constrained, ignore the value for 'left' (in case the
                // 'direction' property of the containing block is 'rtl') or
                //'right' (in case 'direction' is 'ltr') and solve for that
                // value.

                if (style()->direction() == DirectionValue::LtrDirectionValue) {
                    LayoutUnit l =
                        left.specifiedValue(containgBlockContentWidth);
                    setAbsX(l, absX);
                } else {
                    LayoutUnit w =
                        width.specifiedValue(containgBlockContentWidth);
                    LayoutUnit r =
                        right.specifiedValue(containgBlockContentWidth);
                    setAbsX(containgBlockContentWidth - w - r, absX);
                }

                applyHorizontalMargin();
                computeContentWidth(ctx, containgBlockContentWidth, absX);
            } else {
                // Otherwise, set 'auto' values for 'margin-left' and
                // 'margin-right' to 0, and pick the one of the following six
                // rules that applies.
                if (marginLeft.isAuto()) {
                    setMarginLeft(0);
                }
                if (marginRight.isAuto()) {
                    setMarginRight(0);
                }
                if (left.isAuto() && width.isAuto() && !right.isAuto()) {
                    // 'left' and 'width' are 'auto' and 'right' is not 'auto',
                    // then the width is shrink-to-fit. Then solve for 'left'
                    computeContentWidth(ctx, containgBlockContentWidth, absX);
                    LayoutUnit r =
                        right.specifiedValue(containgBlockContentWidth);
                    setAbsX(containgBlockContentWidth - r - FrameBox::width(),
                            absX);
                    applyHorizontalMargin(parentDirection == LtrDirectionValue);
                } else if (left.isAuto() && !width.isAuto() && right.isAuto()) {
                    // 'left' and 'right' are 'auto' and 'width' is not 'auto',
                    // then if the 'direction' property of the element
                    // establishing the static-position containing block is
                    // 'ltr' set 'left' to the static position, otherwise set
                    // 'right' to the static position. Then solve for 'left'
                    // (if 'direction is 'rtl') or 'right' (if 'direction' is
                    // 'ltr').
                    applyHorizontalMargin();
                    computeContentWidth(ctx, containgBlockContentWidth, absX);
                } else if (!left.isAuto() && width.isAuto() && right.isAuto()) {
                    // 'width' and 'right' are 'auto' and 'left' is not 'auto',
                    // then the width is shrink-to-fit . Then solve for 'right'
                    LayoutUnit l =
                        left.specifiedValue(containgBlockContentWidth);
                    if (parentDirection == LtrDirectionValue) {
                        setAbsX(l, absX);
                        computeContentWidth(ctx, containgBlockContentWidth,
                                            absX);
                    } else {
                        computeContentWidth(ctx, containgBlockContentWidth,
                                            absX);
                        setAbsX(l, absX);
                    }
                    applyHorizontalMargin(parentDirection == RtlDirectionValue);
                } else if (left.isAuto() && !width.isAuto() &&
                           !right.isAuto()) {
                    // 'left' is 'auto', 'width' and 'right' are not 'auto',
                    // then solve for 'left'
                    applyHorizontalMargin();
                    computeContentWidth(ctx, containgBlockContentWidth, absX);
                    LayoutUnit r =
                        right.specifiedValue(containgBlockContentWidth);
                    setAbsX(containgBlockContentWidth - r - FrameBox::width(),
                            absX);
                } else if (!left.isAuto() && width.isAuto() &&
                           !right.isAuto()) {
                    // 'width' is 'auto', 'left' and 'right' are not 'auto',
                    // then solve for 'width'
                    LayoutUnit l =
                        left.specifiedValue(containgBlockContentWidth);
                    LayoutUnit r =
                        right.specifiedValue(containgBlockContentWidth);
                    LayoutUnit w =
                        containgBlockContentWidth - l - r - mbpWidth();
                    if (w < 0) {
                        w = 0;
                    }
                    Length oldWidth = width;
                    style()->setWidth(Length(Length::Fixed, w));
                    setAbsX(l, absX);
                    applyHorizontalMargin(parentDirection == RtlDirectionValue);
                    computeContentWidth(ctx, containgBlockContentWidth, absX);
                    style()->setWidth(oldWidth);
                } else {
                    // 'right' is 'auto', 'left' and 'width' are not 'auto',
                    // then solve for 'right'
                    STARFISH_ASSERT(right.isAuto() && !left.isAuto() &&
                                    !width.isAuto());
                    LayoutUnit l =
                        left.specifiedValue(containgBlockContentWidth);
                    if (parentDirection == LtrDirectionValue) {
                        setAbsX(l, absX);
                        applyHorizontalMargin();
                        computeContentWidth(ctx, containgBlockContentWidth,
                                            absX);
                    } else {
                        applyHorizontalMargin();
                        computeContentWidth(ctx, containgBlockContentWidth,
                                            absX);
                        setAbsX(l, absX);
                    }
                }
            }

            if (left.isAuto() && right.isAuto() &&
                parentDirection == DirectionValue::RtlDirectionValue) {
                moveX(-FrameBox::width());
            }
        } else {
            // 10.3.5 Floating, non-replaced elements
            FrameBox* cb = ctx.blockContainer(this);
            LayoutUnit containgBlockContentWidth = cb->contentWidth();
            computeBorderMarginPadding(containgBlockContentWidth);

            Length marginLeft = style()->marginLeft();
            Length marginRight = style()->marginRight();

            if (marginLeft.isAuto()) {
                setMarginLeft(0);
            }
            if (marginRight.isAuto()) {
                setMarginRight(0);
            }

            computeContentWidth(ctx, containgBlockContentWidth);
        }
    }

    if (!(Frame::LayoutWantToResolve::ResolveHeight & resolveWhat)) {
        return;
    }

    LayoutUnit contentHeight;

    if (hasBlockFlow()) {
        contentHeight = layoutBlock(ctx);
    } else {
        contentHeight = layoutInline(ctx);
    }

    // The contentHeight is Used when table cell contents are vertically aligned
    // in the table row.
    if (style()->display() == TableCellDisplayValue) {
        this->asFrameTableCellBox()->setActualContentHeight(contentHeight);
    }

    // Now the intrinsic height of the object is known because the children are
    // placed

    // Determine the final height
    if (style()->position() == PositionValue::AbsolutePositionValue) {
        FrameBox* cb = ctx.containingBlock(this);
        FrameBox* parent = Frame::layoutParent()->asFrameBox();

        LayoutLocation l1, l2;
        if (cb->isAncestorOf(parent)) {
            l2 = parent->absolutePoint(cb);
        } else {
            l1 = cb->absolutePoint(ctx.frameDocument());
            l2 = parent->absolutePoint(ctx.frameDocument());
        }

        LayoutUnit absY = l2.y() - l1.y() - cb->borderTop();
        LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();

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
            LayoutUnit t = top.specifiedValue(parentHeight);
            setAbsY(t, absY);
        } else if (top.isAuto() && height.isAuto() && !bottom.isAuto()) {
            // 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then
            // the height is based on the content per 10.6.7 set 'auto' values
            // for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
            LayoutUnit b = bottom.specifiedValue(parentHeight);
            applyMinMaxHeightIfNeeds(contentHeight, parentHeight);
            setAbsY(parentHeight - asFrameBox()->contentHeight() -
                        paddingHeight() - borderHeight() - b,
                    absY);
        } else if (top.isAuto() && bottom.isAuto() && !height.isAuto()) {
            // 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then
            // set 'top' to the static position set 'auto' values for
            // 'margin-top' and 'margin-bottom' to 0, and  solve for 'bottom'
        } else if (height.isAuto() && bottom.isAuto() && !top.isAuto()) {
            // 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then
            // the height is based on the content per 10.6.7, set 'auto' values
            // for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
            setAbsY(top.specifiedValue(parentHeight), absY);
        } else if (top.isAuto() && !height.isAuto() && !bottom.isAuto()) {
            // 'top' is 'auto', 'height' and 'bottom' are not 'auto', then set
            // 'auto' values for 'margin-top' and 'margin-bottom' to 0, and
            // solve for 'top'
            LayoutUnit h = height.specifiedValue(parentHeight);
            LayoutUnit b = bottom.specifiedValue(parentHeight);
            setAbsY(parentHeight - h - b - paddingHeight() - borderHeight(),
                    absY);
        } else if (height.isAuto() && !top.isAuto() && !bottom.isAuto()) {
            // 'height' is 'auto', 'top' and 'bottom' are not 'auto', then
            // 'auto' values for 'margin-top' and 'margin-bottom' are set to 0
            // and solve for 'height'
            LayoutUnit t = top.specifiedValue(parentHeight);
            LayoutUnit b = bottom.specifiedValue(parentHeight);
            height =
                Length(Length::Fixed,
                       parentHeight - t - b - paddingHeight() - borderHeight());
            setAbsY(t, absY);
        } else {
            // 'bottom' is 'auto', 'top' and 'height' are not 'auto', then set
            // 'auto' values for 'margin-top' and 'margin-bottom' to 0 and solve
            // for 'bottom'
            STARFISH_ASSERT(bottom.isAuto() && !top.isAuto() &&
                            !height.isAuto());
            setAbsY(top.specifiedValue(parentHeight), absY);
        }

        if (height.isAuto()) {
            applyMinMaxHeightIfNeeds(contentHeight, parentHeight);
        } else {
            applyMinMaxHeightIfNeeds(height.specifiedValue(parentHeight),
                                     parentHeight);
        }

        applyVerticalMargin();
    } else {
        // Normal Flow or Float box
        // 10.6.6 Complicated cases
        Length height = style()->height();
        if (height.isAuto()) {
            if (ctx.parentHasFixedHeight(this)) {
                applyMinMaxHeightIfNeeds(contentHeight,
                                         ctx.parentFixedHeight(this));
            } else {
                applyMinMaxHeightIfNeeds(contentHeight, contentHeight, false);
            }
        } else if (height.isFixed()) {
            if (ctx.parentHasFixedHeight(this)) {
                applyMinMaxHeightIfNeeds(height.fixed(),
                                         ctx.parentFixedHeight(this));
            } else {
                applyMinMaxHeightIfNeeds(height.fixed(), contentHeight, false);
            }
        } else {
            if (ctx.parentHasFixedHeight(this)) {
                applyMinMaxHeightIfNeeds(height.percent() *
                                             ctx.parentFixedHeight(this),
                                         ctx.parentFixedHeight(this));
            } else {
                applyMinMaxHeightIfNeeds(contentHeight, contentHeight, false);
            }
        }
    }

    if (style()->position() == PositionValue::RelativePositionValue) {
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

Frame* FrameBlockBox::hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                          HitTestStage s)
{
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
        return;
    }

    ctx.m_canvas->save();

    bool overflowApplied = shouldApplyOverflow();
    if (overflowApplied) {
        ctx.m_canvas->clip(Rect(0, 0, width(), height()));
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
                ctx.m_canvas->clip(Rect(
                    borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                    contentWidth(), contentHeight() + paddingBottom()));
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
                ctx.m_canvas->clip(Rect(
                    borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                    contentWidth(), contentHeight() + paddingBottom()));
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
                ctx.m_canvas->clip(Rect(
                    borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                    contentWidth(), contentHeight() + paddingBottom()));
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
            if (overflowApplied) {
                ctx.m_canvas->clip(Rect(
                    borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                    contentWidth(), contentHeight() + paddingBottom()));
            }
            paintChildrenWith(ctx);
        } else {
            paintChildrenWith(ctx);
        }
    }

    ctx.m_canvas->restore();
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
