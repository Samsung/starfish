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
#include "FrameReplaced.h"
#include "FrameDocument.h"

namespace StarFish {

void FrameReplaced::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    IntrinsicSizeUsedInLayout s = intrinsicSize();

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        LayoutUnit parentContentWidth = ctx.containingFrameBlockBox(this)->asFrameBox()->contentWidth();
        LayoutUnit intrinsicWidth, intrinsicHeight;
        computeBorderMarginPadding(parentContentWidth);

        if (isNormalFlow()) {
            Length parentContentHeight;
            if (ctx.parentHasFixedHeight(this)) {
                parentContentHeight = Length(Length::Fixed, ctx.parentFixedHeight(this));
            } else {
                parentContentHeight = Length(Length::Auto);
            }
            computeIntrinsicSize(intrinsicWidth, intrinsicHeight, parentContentWidth, parentContentHeight);

            if ((intrinsicWidth == 0 || intrinsicHeight == 0) && (style()->width().isAuto() || style()->height().isAuto())) {
                setContentWidth(0);
                setContentHeight(0);
            } else if (style()->width().isAuto() && style()->height().isAuto()) {
                setContentWidth(intrinsicWidth);
                setContentHeight(intrinsicHeight);
            } else if (style()->width().isSpecified() && style()->height().isAuto()) {
                LayoutUnit w = style()->width().specifiedValue(ctx.parentContentWidth(this));
                LayoutUnit h = w * (intrinsicHeight / intrinsicWidth);
                setContentWidth(w);
                setContentHeight(h);
            } else if (style()->width().isAuto() && style()->height().isSpecified()) {
                if (style()->height().isFixed()) {
                    LayoutUnit h = style()->height().fixed();
                    LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                    setContentWidth(w);
                    setContentHeight(h);
                } else {
                    STARFISH_ASSERT(style()->height().isPercent());
                    if (ctx.parentHasFixedHeight(this)) {
                        LayoutUnit h = style()->height().percent() * ctx.parentFixedHeight(this);
                        LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                        setContentWidth(w);
                        setContentHeight(h);
                    } else {
                        setContentWidth(intrinsicWidth);
                        setContentHeight(intrinsicHeight);
                    }
                }
            } else {
                STARFISH_ASSERT(style()->width().isSpecified() && style()->height().isSpecified());
                setContentWidth(style()->width().specifiedValue(ctx.parentContentWidth(this)));
                if (style()->height().isFixed()) {
                    setContentHeight(style()->height().fixed());
                } else {
                    if (ctx.parentHasFixedHeight(this))
                        setContentHeight(style()->height().percent() * ctx.parentFixedHeight(this));
                    else
                        setContentHeight(intrinsicHeight);
                }
            }

            if (style()->display() == BlockDisplayValue && style()->marginLeft().isAuto() && style()->marginRight().isAuto()) {
                LayoutUnit remain = parentContentWidth;
                remain -= contentWidth();
                remain -= borderWidth();
                remain -= paddingWidth();
                if (remain > 0) {
                    setMarginLeft(remain / 2);
                    setMarginRight(remain / 2);
                }
            }

            if (style()->display() == BlockDisplayValue) {
                if (style()->marginLeft().isAuto() && style()->marginRight().isAuto()) {
                    LayoutUnit remain = parentContentWidth;
                    remain -= contentWidth();
                    remain -= borderWidth();
                    remain -= paddingWidth();
                    if (remain > 0) {
                        setMarginLeft(remain / 2);
                        setMarginRight(remain / 2);
                    }
                } else if (style()->marginLeft().isAuto() && !style()->marginRight().isAuto()) {
                    LayoutUnit remain = parentContentWidth;
                    remain -= contentWidth();
                    remain -= borderWidth();
                    remain -= paddingWidth();
                    remain -= marginRight();
                    if (remain > 0) {
                        setMarginLeft(remain);
                    }
                } else if (!style()->marginLeft().isAuto() && style()->marginRight().isAuto()) {
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
            LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();

            Length parentContentHeight;
            parentContentHeight = Length(Length::Fixed, parentHeight);
            computeIntrinsicSize(intrinsicWidth, intrinsicHeight, parentContentWidth, parentContentHeight);

            computeBorderMarginPadding(cb->contentWidth());

            DirectionValue direction = ctx.blockContainer(this)->style()->direction();

            FrameBox* parent = Frame::layoutParent()->asFrameBox();

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

            // 10.3.8 Absolutely positioned, replaced elements
            // 'left' + 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' + 'right' = width of containing block

            Length marginLeft = style()->marginLeft();
            Length marginRight = style()->marginRight();

            if ((intrinsicWidth == 0 || intrinsicHeight == 0) && (style()->width().isAuto() || style()->height().isAuto())) {
                setContentWidth(0);
                setContentHeight(0);
            } else if (style()->width().isAuto() && style()->height().isAuto()) {
                setContentWidth(intrinsicWidth);
                setContentHeight(intrinsicHeight);
            } else if (style()->width().isSpecified() && style()->height().isAuto()) {
                LayoutUnit w = style()->width().specifiedValue(cb->contentWidth());
                LayoutUnit h = w * (intrinsicHeight / intrinsicWidth);
                setContentWidth(w);
                setContentHeight(h);
            } else if (style()->width().isAuto() && style()->height().isSpecified()) {
                LayoutUnit h = style()->height().specifiedValue(cb->contentHeight());
                LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                setContentWidth(w);
                setContentHeight(h);
            } else {
                STARFISH_ASSERT(style()->width().isSpecified() && style()->height().isSpecified());
                setContentWidth(style()->width().specifiedValue(cb->contentWidth()));
                if (style()->height().isFixed()) {
                    setContentHeight(style()->height().fixed());
                } else {
                    if (ctx.parentHasFixedHeight(this))
                        setContentHeight(style()->height().specifiedValue(cb->contentHeight()));
                    else
                        setContentHeight(intrinsicHeight);
                }
            }

            LayoutUnit containgBlockContentWidth = cb->contentWidth() + cb->paddingWidth();
            bool needOppositeMargin = false;

            // If 'margin-left' or 'margin-right' is specified as 'auto' its used value is determined by the rules below.
            // If both 'left' and 'right' have the value 'auto'
            // then if the 'direction' property of the element establishing the static-position containing block is 'ltr', set 'left' to the static position;
            // else if 'direction' is 'rtl', set 'right' to the static position.
            if (style()->left().isAuto() && style()->right().isAuto()) {
                // static location computed in normal flow processing
            } else if (!style()->left().isAuto() && style()->right().isAuto()) {
                if (style()->direction() == RtlDirectionValue)
                    needOppositeMargin = true;
                setAbsX(style()->left().specifiedValue(containgBlockContentWidth));
            } else if (style()->left().isAuto() && !style()->right().isAuto()) {
                if (style()->direction() == LtrDirectionValue)
                    needOppositeMargin = true;
                LayoutUnit r = style()->right().specifiedValue(containgBlockContentWidth);
                setAbsX(containgBlockContentWidth - r - width());
            } else {
                LayoutUnit computedLeft = style()->left().specifiedValue(containgBlockContentWidth);
                LayoutUnit computedRight = style()->right().specifiedValue(containgBlockContentWidth);
                if (marginLeft.isAuto() && marginRight.isAuto()) {
                    LayoutUnit remain = containgBlockContentWidth;
                    remain -= contentWidth();
                    remain -= borderWidth();
                    remain -= paddingWidth();
                    remain -= computedLeft + computedRight;
                    if (remain > 0) {
                        setMarginLeft(remain / 2);
                        setMarginRight(remain / 2);
                    }
                } else if (marginRight.isAuto() && style()->direction() == RtlDirectionValue) {
                    needOppositeMargin = true;
                } else if (marginLeft.isAuto() && style()->direction() == LtrDirectionValue) {
                    needOppositeMargin = true;
                }
                if ((style()->direction() == LtrDirectionValue && !needOppositeMargin)
                    || (style()->direction() == RtlDirectionValue && needOppositeMargin)) {
                    setAbsX(computedLeft);
                } else {
                    setAbsX(containgBlockContentWidth - FrameBox::width() - computedRight);
                }
            }

            if ((style()->direction() == LtrDirectionValue && !needOppositeMargin)
                || (style()->direction() == RtlDirectionValue && needOppositeMargin)) {
                moveX(asFrameBox()->marginLeft());
            } else {
                moveX(-asFrameBox()->marginRight());
            }

            if (style()->left().isAuto() && style()->right().isAuto() && direction == DirectionValue::RtlDirectionValue) {
                moveX(-this->width());
            }
        }
    }

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        if (isNormalFlow()) {

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

            LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();
            Length marginTop = style()->marginTop();
            Length marginBottom = style()->marginBottom();

            LayoutUnit absY = absLoc.y() - cb->borderTop();
            auto setAbsY = [&](LayoutUnit y)
            {
                setY(y - absY);
            };

            if (style()->top().isAuto() && style()->bottom().isAuto()) {
                // static location computed in normal flow processing
            } else if (!style()->top().isAuto() && style()->bottom().isAuto()) {
                setAbsY(style()->top().specifiedValue(parentHeight));
            } else if (style()->top().isAuto() && !style()->bottom().isAuto()) {
                LayoutUnit b = style()->bottom().specifiedValue(parentHeight);
                setAbsY(parentHeight - b - height());
            } else {
                setAbsY(style()->top().specifiedValue(parentHeight));
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
    }
}

void FrameReplaced::computePreferredWidth(ComputePreferredWidthContext& ctx)
{
    LayoutUnit parentContentWidth = ctx.layoutContext().blockContainer(this)->asFrameBox()->contentWidth();
    LayoutUnit intrinsicWidth, intrinsicHeight;
    Length parentContentHeight;
    if (ctx.layoutContext().parentHasFixedHeight(this)) {
        parentContentHeight = Length(Length::Fixed, ctx.layoutContext().parentFixedHeight(this));
    } else {
        parentContentHeight = Length(Length::Auto);
    }
    computeIntrinsicSize(intrinsicWidth, intrinsicHeight, parentContentWidth, parentContentHeight);

    if (style()->width().isAuto() && style()->height().isAuto()) {
        ctx.setResult(intrinsicWidth);
        ctx.setMinimumWidth(intrinsicWidth);
    } else if (style()->width().isSpecified()) {
        if (style()->width().isFixed()) {
            ctx.setResult(style()->width().fixed());
            ctx.setMinimumWidth(style()->width().fixed());
        } else {
            ctx.setResult(intrinsicWidth);
            ctx.setMinimumWidth(intrinsicWidth);
        }
    } else if (style()->height().isSpecified()) {
        if (style()->height().isFixed()) {
            LayoutUnit h = style()->height().fixed();
            LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
            ctx.setResult(w);
            ctx.setMinimumWidth(w);
        } else {
            ctx.setResult(intrinsicWidth);
            ctx.setMinimumWidth(intrinsicWidth);
        }
    } else {
        ctx.setResult(intrinsicWidth);
    }
}

void FrameReplaced::computeIntrinsicSize(LayoutUnit& intrinsicWidth, LayoutUnit& intrinsicHeight, LayoutUnit parentContentWidth, Length parentContentHeight)
{
    IntrinsicSizeUsedInLayout s = intrinsicSize();
    auto a = s.m_intrinsicSizeIsSpecifiedByAttributeOfElement;
    auto b = s.m_intrinsicContentSize;
    if (a.first.isAuto() && a.second.isAuto()) {
        intrinsicWidth = s.m_intrinsicContentSize.width();
        intrinsicHeight = s.m_intrinsicContentSize.height();
    } else if (a.first.isSpecified() && a.second.isAuto()) {
        intrinsicWidth = a.first.specifiedValue(parentContentWidth);
        intrinsicHeight = intrinsicWidth * (b.height() / b.width());
    } else if (a.first.isSpecified() && a.second.isFixed()) {
        intrinsicWidth = a.first.specifiedValue(parentContentWidth);
        intrinsicHeight = a.second.fixed();
    } else if (a.first.isSpecified() && a.second.isPercent()) {
        if (parentContentHeight.isFixed()) {
            intrinsicWidth = a.first.specifiedValue(parentContentWidth);
            intrinsicHeight = a.second.specifiedValue(parentContentHeight.fixed());
        } else {
            intrinsicWidth = a.first.specifiedValue(parentContentWidth);
            intrinsicHeight = intrinsicWidth * (b.height() / b.width());
        }
    } else if (a.first.isAuto() && a.second.isFixed()) {
        intrinsicHeight = a.second.fixed();
        intrinsicWidth = intrinsicHeight * (b.width() / b.height());
    } else {
        STARFISH_ASSERT(a.first.isAuto() && a.second.isPercent());
        if (parentContentHeight.isFixed()) {
            intrinsicHeight = a.second.specifiedValue(parentContentHeight.fixed());
            intrinsicWidth = intrinsicHeight * (b.width() / b.height());
        } else {
            intrinsicWidth = s.m_intrinsicContentSize.width();
            intrinsicHeight = s.m_intrinsicContentSize.height();
        }
        intrinsicHeight = a.second.fixed();
        intrinsicWidth = intrinsicHeight * (b.width() / b.height());
    }
}

}
