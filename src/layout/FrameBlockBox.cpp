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
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"
#include "FrameDocument.h"

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
        if (m_needs)
            m_ctx.removeBlockFormattingContext();
    }

    LayoutContext& m_ctx;
    bool m_needs;
};

void FrameBlockBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    BlockFormattingContextBlock blockFormattingContextBlock(this, ctx);
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        // Determine horizontal margins and width of this object.
        if (isNormalFlow()) {
            // https://www.w3.org/TR/CSS2/visudet.html#the-width-property
            LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
            computeBorderMarginPadding(parentContentWidth);

            // 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' = width of containing block
            ComputedStyle* style = Frame::style();
            if (style->width().isAuto()) {
                if (m_flags.m_shouldComputePreferredWidth) {
                    LayoutUnit mbp = ComputePreferredWidthContext::computeMinimumWidthDueToMBP(style);
                    ComputePreferredWidthContext p(ctx, parentContentWidth - mbp, 0);
                    computePreferredWidth(p);
                    setContentWidth(p.result());
                } else {
                    LayoutUnit remainWidth = parentContentWidth;
                    remainWidth -= marginWidth();
                    remainWidth -= borderWidth();
                    remainWidth -= paddingWidth();
                    if (remainWidth < 0)
                        remainWidth = 0;
                    setContentWidth(remainWidth);
                }
            } else {
                if (style->width().isFixed()) {
                    setContentWidth(style->width().fixed());
                } else {
                    STARFISH_ASSERT(style->width().isPercent());
                    setContentWidth(parentContentWidth * style->width().percent());
                }
            }

            if (style->display() == BlockDisplayValue) {
                if (style->marginLeft().isAuto() && style->marginRight().isAuto()) {
                    LayoutUnit remain = parentContentWidth;
                    remain -= contentWidth();
                    remain -= borderWidth();
                    remain -= paddingWidth();
                    if (remain > 0) {
                        setMarginLeft(remain / 2);
                        setMarginRight(remain / 2);
                    }
                } else if (style->marginLeft().isAuto() && !style->marginRight().isAuto()) {
                    LayoutUnit remain = parentContentWidth;
                    remain -= contentWidth();
                    remain -= borderWidth();
                    remain -= paddingWidth();
                    remain -= marginRight();
                    if (remain > 0) {
                        setMarginLeft(remain);
                    }
                } else if (!style->marginLeft().isAuto() && style->marginRight().isAuto()) {
                    LayoutUnit remain = parentContentWidth;
                    remain -= contentWidth();
                    remain -= borderWidth();
                    remain -= paddingWidth();
                    remain -= marginLeft();
                    if (remain > 0) {
                        setMarginRight(remain);
                    }
                }
            }
        } else {
            FrameBox* cb = ctx.containingBlock(this)->asFrameBox();
            computeBorderMarginPadding(cb->contentWidth());

            STARFISH_ASSERT(node() != nullptr);
            FrameBox* parent = Frame::layoutParent()->asFrameBox();
            DirectionValue direction = ctx.blockContainer(this)->style()->direction();

            LayoutLocation l1, l2;
            if (cb->isAncestorOf(parent)) {
                l2 = parent->absolutePoint(cb);
            } else {
                l1 = cb->absolutePoint(ctx.frameDocument());
                l2 = parent->absolutePoint(ctx.frameDocument());
            }
            LayoutLocation absLoc(l2.x() - l1.x(), l2.y() - l1.y());

            LayoutUnit absX = absLoc.x() - cb->borderLeft();
            auto setAbsX = [&](LayoutUnit x)
            {
                setX(x - absX);
            };


            // 10.3.7 Absolutely positioned, non-replaced elements
            // The constraint that determines the used values for these elements is:
            // 'left' + 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' + 'right' = width of containing block

            // Then, if the 'direction' property of the element establishing the static-position containing block is 'ltr' set 'left' to the static position and apply rule number three below;
            // otherwise, set 'right' to the static position and apply rule number one below.

            Length marginLeft = style()->marginLeft();
            Length marginRight = style()->marginRight();

            if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                if (style()->direction() == LtrDirectionValue) {
                    setMarginRight(0);
                } else {
                    setMarginLeft(0);
                }
            }

            auto applyMargin = [&](LayoutUnit parentWidth, bool isOpposite = false)
            {
                if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                    if ((style()->direction() == LtrDirectionValue && !isOpposite)
                        || (style()->direction() == RtlDirectionValue && isOpposite)) {
                        moveX(marginLeft.specifiedValue(parentWidth));
                    } else {
                        moveX(-marginRight.specifiedValue(parentWidth));
                    }
                } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
                    moveX(marginLeft.specifiedValue(parentWidth));
                } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
                    moveX(-marginRight.specifiedValue(parentWidth));
                } else {
                }
            };

            Length left = style()->left();
            Length right = style()->right();
            Length width = style()->width();

            LayoutUnit containgBlockContentWidth = cb->contentWidth() + cb->paddingWidth();

            auto computeContentWidth = [&](bool shouldUseParentWidthForComputePreferredWidthFromOutside = false, LayoutUnit parentWidthForComputePreferredWidthFromOutside = 0)
            {
                if (width.isAuto()) {
                    LayoutUnit parentWidthForComputePreferredWidth;
                    if (shouldUseParentWidthForComputePreferredWidthFromOutside) {
                        parentWidthForComputePreferredWidth = parentWidthForComputePreferredWidthFromOutside;
                    } else {
                        parentWidthForComputePreferredWidth = containgBlockContentWidth;
                        if (direction == LtrDirectionValue) {
                            parentWidthForComputePreferredWidth = parentWidthForComputePreferredWidth - absX - x();
                        } else {
                            parentWidthForComputePreferredWidth = x() + absX;
                        }
                    }

                    parentWidthForComputePreferredWidth -= marginWidth() + borderWidth() + paddingWidth();

                    if (parentWidthForComputePreferredWidth < 0)
                        parentWidthForComputePreferredWidth = 0;

                    ComputePreferredWidthContext p(ctx, parentWidthForComputePreferredWidth, 0);
                    computePreferredWidth(p);
                    setContentWidth(p.result());
                } else if (width.isFixed()) {
                    setContentWidth(width.fixed());
                } else if (width.isPercent()) {
                    setContentWidth(containgBlockContentWidth * width.percent());
                }
            };

            if (left.isAuto() && width.isAuto() && right.isAuto()) {
                // If all three of 'left', 'width', and 'right' are 'auto':
                applyMargin(containgBlockContentWidth);
                computeContentWidth();
            } else if (!left.isAuto() && !width.isAuto() && !right.isAuto()) {
                // If none of the three is 'auto':
                // If both 'margin-left' and 'margin-right' are 'auto',
                // solve the equation under the extra constraint that the two margins get equal values,
                // unless this would make them negative, in which case when direction of the containing block is 'ltr' ('rtl'),
                // set 'margin-left' ('margin-right') to zero and solve for 'margin-right' ('margin-left').
                // If one of 'margin-left' or 'margin-right' is 'auto', solve the equation for that value.
                // If the values are over-constrained, ignore the value for 'left'
                // (in case the 'direction' property of the containing block is 'rtl') or 'right' (in case 'direction' is 'ltr') and solve for that value.
                if (style()->direction() == DirectionValue::LtrDirectionValue) {
                    LayoutUnit computedLeft = left.specifiedValue(containgBlockContentWidth);
                    setAbsX(computedLeft);
                } else {
                    LayoutUnit computedRight = right.specifiedValue(containgBlockContentWidth);
                    setAbsX(containgBlockContentWidth - width.specifiedValue(containgBlockContentWidth) - computedRight);
                }

                applyMargin(containgBlockContentWidth);
                computeContentWidth();
            } else {
                // Otherwise, set 'auto' values for 'margin-left' and 'margin-right' to 0, and pick the one of the following six rules that applies.
                if (left.isAuto() && width.isAuto() && !right.isAuto()) {
                    // 'left' and 'width' are 'auto' and 'right' is not 'auto', then the width is shrink-to-fit. Then solve for 'left'
                    computeContentWidth(true, containgBlockContentWidth - right.specifiedValue(containgBlockContentWidth));
                    setAbsX(containgBlockContentWidth - right.specifiedValue(containgBlockContentWidth) - this->width());
                    if (direction == LtrDirectionValue) {
                        applyMargin(containgBlockContentWidth, true);
                    } else {
                        applyMargin(containgBlockContentWidth);
                    }
                } else if (left.isAuto() && right.isAuto() && !width.isAuto()) {
                    // 'left' and 'right' are 'auto' and 'width' is not 'auto',
                    // then if the 'direction' property of the element establishing the static-position containing block is 'ltr' set 'left' to the static position,
                    // otherwise set 'right' to the static position. Then solve for 'left' (if 'direction is 'rtl') or 'right' (if 'direction' is 'ltr').
                    applyMargin(containgBlockContentWidth);
                    computeContentWidth();
                } else if (width.isAuto() && right.isAuto() && !left.isAuto()) {
                    // 'width' and 'right' are 'auto' and 'left' is not 'auto', then the width is shrink-to-fit . Then solve for 'right'
                    if (direction == LtrDirectionValue) {
                        setAbsX(left.specifiedValue(containgBlockContentWidth));
                        applyMargin(containgBlockContentWidth);
                        computeContentWidth();
                    } else {
                        computeContentWidth(true, containgBlockContentWidth - left.specifiedValue(containgBlockContentWidth));
                        setAbsX(left.specifiedValue(containgBlockContentWidth));
                        applyMargin(containgBlockContentWidth, true);
                    }
                } else if (left.isAuto() && !width.isAuto() && !right.isAuto()) {
                    // 'left' is 'auto', 'width' and 'right' are not 'auto', then solve for 'left'
                    applyMargin(containgBlockContentWidth);
                    computeContentWidth();
                    setAbsX(containgBlockContentWidth - right.specifiedValue(containgBlockContentWidth) - this->width());
                } else if (width.isAuto() && !left.isAuto() && !right.isAuto()) {
                    // 'width' is 'auto', 'left' and 'right' are not 'auto', then solve for 'width'
                    LayoutUnit l = left.specifiedValue(containgBlockContentWidth);
                    LayoutUnit r = right.specifiedValue(containgBlockContentWidth);
                    LayoutUnit w = containgBlockContentWidth  - l - r - marginWidth() - paddingWidth() - borderWidth();
                    if (w < 0)
                        w = 0;
                    width = Length(Length::Fixed, w);
                    setAbsX(l);
                    applyMargin(containgBlockContentWidth);
                    computeContentWidth();
                } else {
                    // 'right' is 'auto', 'left' and 'width' are not 'auto', then solve for 'right'
                    STARFISH_ASSERT(right.isAuto() && !left.isAuto() && !width.isAuto());
                    if (direction == LtrDirectionValue) {
                        setAbsX(left.specifiedValue(containgBlockContentWidth));
                        applyMargin(containgBlockContentWidth);
                        computeContentWidth();
                    } else {
                        applyMargin(containgBlockContentWidth);
                        computeContentWidth(true, containgBlockContentWidth - left.specifiedValue(containgBlockContentWidth));
                        setAbsX(left.specifiedValue(containgBlockContentWidth));
                    }
                }
            }

            if (left.isAuto() && right.isAuto() && direction == DirectionValue::RtlDirectionValue) {
                moveX(-this->width());
            }
        }

        if (isFrameBlockBox() && asFrameBlockBox()->hasBlockFlow()) {
            Frame* child = firstChild();
            bool hasOnlySelfCollapsing = true;

            LayoutUnit top = paddingTop() + borderTop();
            LayoutUnit bottom = paddingBottom() + borderBottom();
            MarginInfo marginInfo(top, bottom, isEstablishesBlockFormattingContext() || isFrameDocument(), style()->height());
            LayoutUnit maxPositiveMarginTop, maxNegativeMarginTop;

            if (marginInfo.canCollapseTopWithChildren()) {
                if (marginTop() >= 0) {
                    maxPositiveMarginTop = marginTop();
                    ctx.setMaxPositiveMarginTop(std::max(ctx.maxPositiveMarginTop(), marginTop()));
                    marginInfo.setPositiveMargin(ctx.maxPositiveMarginTop());
                } else {
                    maxNegativeMarginTop = -marginTop();
                    ctx.setMaxNegativeMarginTop(std::max(ctx.maxNegativeMarginTop(), maxNegativeMarginTop));
                    marginInfo.setNegativeMargin(ctx.maxNegativeMarginTop());
                }
            } else {
                ctx.setMaxMarginTop(0, 0);
            }

            if (!child)
                marginInfo.setMargin(0, 0);

            while (child) {
                if (child->isNormalFlow()) {
                    child->asFrameBox()->setMarginCollapseResult(MarginCollapseResult());
                    child->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
                    LayoutUnit posTop = marginInfo.positiveMargin(), negTop = marginInfo.negativeMargin();
                    if (child->asFrameBox()->marginTop() >= 0)
                        posTop = std::max(child->asFrameBox()->marginTop(), posTop);
                    else
                        negTop = std::max(-child->asFrameBox()->marginTop(), negTop);

                    LayoutUnit topPosition = posTop - negTop;

                    posTop = std::max(posTop, ctx.maxPositiveMarginTop());
                    negTop = std::max(negTop, ctx.maxNegativeMarginTop());
                    if (marginInfo.canCollapseWithMarginTop() && marginInfo.atTopSideOfBlock()) {
                        // NOTE: max[Positive/Negative]MarginTop has the collapsed margin between ancestors and first child
                        maxPositiveMarginTop = std::max(maxPositiveMarginTop, posTop);
                        maxNegativeMarginTop = std::max(maxNegativeMarginTop, negTop);
                    } else if (child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                        MarginCollapseResult r = child->asFrameBox()->marginCollapseResult();
                        r.m_advanceY += topPosition;
                        child->asFrameBox()->setMarginCollapseResult(r);
                    }

                    if (child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                        if (child->asFrameBox()->marginBottom() >= 0)
                            posTop = std::max(posTop, child->asFrameBox()->marginBottom());
                        else
                            negTop = std::max(negTop, -child->asFrameBox()->marginBottom());
                    }
                    LayoutUnit logicalTop = posTop - negTop;
                    if (child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                        marginInfo.setMargin(posTop, negTop);
                    } else {
                        hasOnlySelfCollapsing = false;
                        if (!marginInfo.atTopSideOfBlock() || !marginInfo.canCollapseWithMarginTop()) {
                            MarginCollapseResult r = child->asFrameBox()->marginCollapseResult();
                            r.m_advanceY += logicalTop;
                            child->asFrameBox()->setMarginCollapseResult(r);
                        }
                        marginInfo.setMargin(child->asFrameBox()->marginBottom());
                        marginInfo.setPositiveMargin(std::max(marginInfo.positiveMargin(), ctx.maxPositiveMarginBottom()));
                        marginInfo.setNegativeMargin(std::max(marginInfo.negativeMargin(), ctx.maxNegativeMarginBottom()));
                    }
                    ctx.setMaxMarginTop(marginInfo.positiveMargin(), marginInfo.negativeMargin());

                    if (marginInfo.atTopSideOfBlock() && !child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                        marginInfo.setAtTopSideOfBlock(false);
                    }
                }
                child = child->next();
            }

            // NOTE: At this point, ctx.max[P/N]MarginTop has the collapsed margin
            // between collapsible ancestors and first descendants.
            ctx.setMaxMarginTop(maxPositiveMarginTop, maxNegativeMarginTop);
            // NOTE: At this point, ctx.max[P/N]MarginBottom has the collapsed margin
            // between this block and last descendants.
            ctx.setMaxMarginBottom(std::max(ctx.maxPositiveMarginBottom(), marginInfo.positiveMargin()),
                std::max(ctx.maxNegativeMarginBottom(), marginInfo.negativeMargin()));
            if (!marginInfo.canCollapseWithMarginBottom()) {
                m_marginCollapseResult.m_normalFlowHeightAdvance = ctx.maxPositiveMarginBottom() - ctx.maxNegativeMarginBottom();
                ctx.setMaxMarginBottom(0, 0);
            }
        }
    }

    if (!(Frame::LayoutWantToResolve::ResolveHeight & resolveWhat)) {
        return;
    }

    LayoutUnit contentHeight;
    LayoutRect visibleRect(0, 0, 0, 0);
    if (hasBlockFlow()) {
        contentHeight = layoutBlock(ctx);
    } else {
        contentHeight = layoutInline(ctx);
    }

    // Now the intrinsic height of the object is known because the children are placed

    // Determine the final height
    if (isNormalFlow()) {
        if (style()->height().isAuto()) {
            setContentHeight(contentHeight);
        } else if (style()->height().isFixed()) {
            setContentHeight(style()->height().fixed());
        } else {
            if (ctx.parentHasFixedHeight(this)) {
                setContentHeight(style()->height().percent() * ctx.parentFixedHeight(this));
            } else {
                setContentHeight(contentHeight);
            }
        }
    } else {
        FrameBox* cb = ctx.containingBlock(this)->asFrameBox();
        FrameBox* parent = Frame::layoutParent()->asFrameBox();

        LayoutLocation l1, l2;
        if (cb->isAncestorOf(parent)) {
            l2 = parent->absolutePoint(cb);
        } else {
            l1 = cb->absolutePoint(ctx.frameDocument());
            l2 = parent->absolutePoint(ctx.frameDocument());
        }
        LayoutLocation absLoc(l2.x() - l1.x(), l2.y() - l1.y());

        LayoutUnit absY = absLoc.y() - cb->borderTop();
        auto setAbsY = [&](LayoutUnit y)
        {
            setY(y - absY);
        };
        LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();

        Length marginTop = style()->marginTop();
        Length marginBottom = style()->marginBottom();
        Length top = style()->top();
        Length bottom = style()->bottom();
        Length height = style()->height();

        // 10.6.4 Absolutely positioned, non-replaced elements

        // For absolutely positioned elements, the used values of the vertical dimensions must satisfy this constraint:
        // 'top' + 'margin-top' + 'border-top-width' + 'padding-top' + 'height' + 'padding-bottom' + 'border-bottom-width' + 'margin-bottom' + 'bottom' = height of containing block

        if (top.isAuto() && height.isAuto() && bottom.isAuto()) {
            // If all three of 'top', 'height', and 'bottom' are auto, set 'top' to the static position and apply rule number three below.
        } else if (!top.isAuto() && !height.isAuto() && !bottom.isAuto()) {
            // If none of the three are 'auto': If both 'margin-top' and 'margin-bottom' are 'auto',
            // solve the equation under the extra constraint that the two margins get equal values.
            // If one of 'margin-top' or 'margin-bottom' is 'auto', solve the equation for that value.
            // If the values are over-constrained, ignore the value for 'bottom' and solve for that value.
            setAbsY(top.specifiedValue(parentHeight));
        } else if (top.isAuto() && height.isAuto() && !bottom.isAuto()) {
            // 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then the height is based on the content per 10.6.7
            // set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
            setContentHeight(contentHeight);
            setAbsY(parentHeight - contentHeight - paddingHeight() - borderHeight() - bottom.specifiedValue(parentHeight));
        } else if (top.isAuto() && bottom.isAuto() && !height.isAuto()) {
            // 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then set 'top' to the static position
            // set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
        } else if (height.isAuto() && bottom.isAuto() && !top.isAuto()) {
            // 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then the height is based on the content per 10.6.7,
            // set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
            setAbsY(top.specifiedValue(parentHeight));
        } else if (top.isAuto() && !height.isAuto() && !bottom.isAuto()) {
            // 'top' is 'auto', 'height' and 'bottom' are not 'auto', then set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
            setAbsY(parentHeight - height.specifiedValue(parentHeight) - paddingHeight() - borderHeight() - bottom.specifiedValue(parentHeight));
        } else if (height.isAuto() && !top.isAuto() && !bottom.isAuto()) {
            // 'height' is 'auto', 'top' and 'bottom' are not 'auto', then 'auto' values for 'margin-top' and 'margin-bottom' are set to 0 and solve for 'height'
            LayoutUnit t = top.specifiedValue(parentHeight);
            LayoutUnit b = bottom.specifiedValue(parentHeight);
            LayoutUnit h = -t - b + parentHeight;
            h = h - paddingHeight() - borderHeight();
            height = Length(Length::Fixed, h);
            setAbsY(t);
        } else {
            // 'bottom' is 'auto', 'top' and 'height' are not 'auto', then set 'auto' values for 'margin-top' and 'margin-bottom' to 0 and solve for 'bottom'
            STARFISH_ASSERT(bottom.isAuto() && !top.isAuto() && !height.isAuto());
            setAbsY(top.specifiedValue(parentHeight));
        }

        if (height.isAuto()) {
            setContentHeight(contentHeight);
        } else {
            setContentHeight(height.specifiedValue(parentHeight));
        }

        if (!marginTop.isAuto() && !marginBottom.isAuto()) {
            moveY(FrameBox::marginTop());
        } else if (!marginTop.isAuto() && marginBottom.isAuto()) {
            moveY(FrameBox::marginTop());
        } else if (marginTop.isAuto() && !marginBottom.isAuto()) {
            moveY(-FrameBox::marginBottom());
        } else {
        }
    }

    if (style()->position() == PositionValue::RelativePositionValue)
        ctx.registerRelativePositionedFrames(this, true);

    // layout absolute positioned blocks
    ctx.layoutRegisteredAbsolutePositionedFrames(this, [&](const std::vector<Frame*>& frames) {
        for (size_t i = 0; i < frames.size(); i ++) {
            Frame* f = frames[i];
            f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
        }
    });

    auto applyRelativePosition = [&](FrameBox* f, Length left, Length right, Length top, Length bottom, LayoutUnit parentWidth, LayoutUnit parentHeight)
    {
        LayoutUnit mX = 0;
        LayoutUnit mY = 0;
        // left, right
        if (!left.isAuto() && !right.isAuto()) {
            if (f->style()->direction() == LtrDirectionValue) {
                mX = left.specifiedValue(parentWidth);
            } else {
                mX = -right.specifiedValue(parentWidth);
            }
        } else if (!left.isAuto() && right.isAuto()) {
            mX = left.specifiedValue(parentWidth);
        } else if (left.isAuto() && !right.isAuto()) {
            mX = -right.specifiedValue(parentWidth);
        }

        // NOTE //
        // In latest css spec, relative position is decided after
        // the size of containing block is fixed with recursive calculation,
        // while the position is decided "before" the containing block's size is fixed in nodewebkit or Chrome.
        // we can compute percentage of top, bottom offset correctly
        // but, modern browsers(webkit, blink, gecko..) are not compute position correctly yet
        // we change implementations follow modern browsers

        // top, bottom
        /*
        // original code
        if (!top.isAuto() && !bottom.isAuto()) {
            mY = top.specifiedValue(parentHeight);
        } else if (!top.isAuto() && bottom.isAuto()) {
            mY = top.specifiedValue(parentHeight);
        } else if (top.isAuto() && !bottom.isAuto()) {
            mY = -bottom.specifiedValue(parentHeight);
        }*/

        auto computeVerticalPosition = [&](const Length& l) -> LayoutUnit
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
        };

        if (!top.isAuto() && !bottom.isAuto()) {
            mY = computeVerticalPosition(top);
        } else if (!top.isAuto() && bottom.isAuto()) {
            mY = computeVerticalPosition(top);
        } else if (top.isAuto() && !bottom.isAuto()) {
            mY = -computeVerticalPosition(bottom);
        }

        f->moveX(mX);
        f->moveY(mY);
    };

    auto applyRelativePositionInlineCase = [&](Frame* ref_f, FrameBox* f, LayoutUnit parentWidth)
    {

        LayoutUnit mX = 0;
        LayoutUnit mY = 0;

        Length left = ref_f->style()->left();
        Length right = ref_f->style()->right();
        Length top = ref_f->style()->top();
        Length bottom = ref_f->style()->bottom();

        // left, right
        if (!left.isAuto() && !right.isAuto()) {
            if (f->style()->direction() == LtrDirectionValue) {
                mX = left.specifiedValue(parentWidth);
            } else {
                mX = -right.specifiedValue(parentWidth);
            }
        } else if (!left.isAuto() && right.isAuto()) {
            mX = left.specifiedValue(parentWidth);
        } else if (left.isAuto() && !right.isAuto()) {
            mX = -right.specifiedValue(parentWidth);
        }

        auto computeVerticalPosition = [&](const Length& l) -> LayoutUnit
        {
            STARFISH_ASSERT(!l.isAuto());
            if (l.isFixed()) {
                return l.fixed();
            } else {
                STARFISH_ASSERT(l.isPercent());
                if (ctx.parentHasFixedHeight(ref_f)) {
                    return l.specifiedValue(ctx.parentFixedHeight(ref_f));
                }
                return 0;
            }
        };

        if (!top.isAuto() && !bottom.isAuto()) {
            mY = computeVerticalPosition(top);
        } else if (!top.isAuto() && bottom.isAuto()) {
            mY = computeVerticalPosition(top);
        } else if (top.isAuto() && !bottom.isAuto()) {
            mY = -computeVerticalPosition(bottom);
        }

        if (f->style()->left().isAuto() && f->style()->right().isAuto()) {
            f->moveX(mX);
        }
        if (f->style()->top().isAuto() && f->style()->bottom().isAuto()) {
            f->moveY(mY);
        }

    };

    // layout relative positioned blocks
    ctx.layoutRegisteredRelativePositionedFrames(this, [&](const std::vector<std::pair<Frame*, bool> >& frames) {
        for (size_t i = 0; i < frames.size(); i ++) {
            Frame* f = frames[i].first;

            if (frames[i].second) {
                Frame* cb = ctx.containingFrameBlockBox(f);
                applyRelativePosition(f->asFrameBox(), f->style()->left(), f->style()->right(), f->style()->top(), f->style()->bottom(), cb->asFrameBox()->contentWidth(), cb->asFrameBox()->contentHeight());
            } else {
                Node* nd = f->node()->parentElement();
                while (nd && nd->frame()->isFrameInline() && nd->style()->position() == RelativePositionValue) {
                    Frame* cb = ctx.containingBlock(nd->frame());
                    applyRelativePositionInlineCase(nd->frame(), f->asFrameBox(),
                        cb->asFrameBox()->contentWidth());
                    nd = nd->parentElement();
                }
            }
        }
    });

    if (node() && node()->parentElement()) {
        Node* nd = node()->parentElement();
        if (nd->frame()->isFrameInline() && nd->style()->position() == RelativePositionValue) {
            ctx.registerRelativePositionedFrames(this, false);
        }
    }
}

Frame* FrameBlockBox::hitTestChildrenWith(LayoutUnit x, LayoutUnit y, HitTestStage s)
{
    Frame* result = nullptr;
    if (hasBlockFlow()) {
        Frame* child = lastChild();
        while (child) {
            LayoutUnit cx = x - child->asFrameBox()->x();
            LayoutUnit cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx, cy, s);
            if (result)
                return result;
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
                result = childBox->hitTest(cx2, cy2,  s);
                if (result)
                    return result;
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
    if (isPositionedElement() && stage == HitTestPositionedElements) {
        HitTestStage s = HitTestStage::HitTestPositionedElements;
        while (s != HitTestStageEnd) {
            result = hitTestChildrenWith(x, y, s);
            if (result)
                return result;
            s = (HitTestStage)(s + 1);
        }
        return FrameBox::hitTest(x, y, stage);
    } else if (style()->display() == InlineBlockDisplayValue) {
        if (stage == HitTestNormalFlowInline) {
            HitTestStage s = HitTestStage::HitTestPositionedElements;
            while (s != HitTestStageEnd) {
                result = hitTestChildrenWith(x, y, s);
                if (result)
                    return result;
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
                    if (result)
                        return result;
                    child = child->previous();
                }

                return FrameBox::hitTest(x, y, stage);
            } else {
                return node() ? FrameBox::hitTest(x, y, stage) : nullptr;
            }
        } else {
            return hitTestChildrenWith(x, y, stage);
        }
    }
    return nullptr;
}

void FrameBlockBox::paint(Canvas* canvas, PaintingStage stage)
{
    if (isEstablishesStackingContext()) {
        return;
    }

    canvas->save();
    bool hiddenApplied = shouldApplyOverflow();
    if (hiddenApplied) {
        if (PaintingStage::PaintingNormalFlowBlock == stage) {
            canvas->clip(Rect(0, 0, width(), height()));
        } else {
            canvas->clip(Rect(borderLeft() + paddingLeft(), borderTop() + paddingTop(), contentWidth(), contentHeight()));
        }

    }

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        canvas->setVisible(false);
    } else {
        canvas->setVisible(true);
    }

    if (isPositionedElement()) {
        if (stage == PaintingPositionedElements) {
            paintBackgroundAndBorders(canvas);
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                paintChildrenWith(canvas, s);
                s = (PaintingStage)(s + 1);
            }
        }
    } else if (style()->display() == InlineBlockDisplayValue) {
        if (stage == PaintingNormalFlowInline) {
            paintBackgroundAndBorders(canvas);
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                paintChildrenWith(canvas, s);
                s = (PaintingStage)(s + 1);
            }
        }
    } else {
        if (stage == PaintingNormalFlowBlock) {
            paintBackgroundAndBorders(canvas);
            paintChildrenWith(canvas, stage);
        } else {
            paintChildrenWith(canvas, stage);
        }
    }

    canvas->restore();
}
#ifdef STARFISH_ENABLE_TEST
void FrameBlockBox::dump(int depth)
{
    FrameBox::dump(depth);
    if (!hasBlockFlow()) {
        if (m_lineBoxes.size() && m_lineBoxes[0]->m_boxes.size()) {
            for (size_t i = 0; i < m_lineBoxes.size(); i++) {
                puts("");
                for (int k = 0; k < depth + 1; k++)
                    printf("  ");
                printf("LineBox(%g,%g,%g,%g)\n", (float)m_lineBoxes[i]->m_frameRect.x(), (float)m_lineBoxes[i]->m_frameRect.y(), (float)m_lineBoxes[i]->m_frameRect.width(), (float)m_lineBoxes[i]->m_frameRect.height());

                LineBox& lb = *m_lineBoxes[i];
                for (size_t k = 0; k < lb.m_boxes.size(); k++) {
                    FrameBox* childBox = lb.m_boxes[k];
                    for (int j = 0; j < depth + 2; j++)
                        printf("  ");
                    printf("%s", childBox->name());
                    childBox->dump(depth + 3);
                    if (k != lb.m_boxes.size() - 1)
                        puts("");
                }
            }
        }
    }
}
#endif
}
