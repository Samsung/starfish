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
#include "StarFish.h"
#include "core/dom/Node.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/FrameDocument.h"

namespace StarFish {

IntrinsicSizeUsedInLayout FrameReplaced::computeIntrinsicSizeForLayout()
{
    IntrinsicSize siz = intrinsicSize();
    IntrinsicSizeUsedInLayout result;
    String* widthString = node()->asElement()->getAttributeOrEmpty(
        node()->starFish()->staticStrings()->m_width);
    String* heightString = node()->asElement()->getAttributeOrEmpty(
        node()->starFish()->staticStrings()->m_height);
    if (siz.m_isContentExists) {
        result.m_intrinsicContentSize =
            LayoutSize(siz.m_intrinsicContentSize.width(),
                       siz.m_intrinsicContentSize.height());
        bool widthIsEmpty = widthString->equals(String::emptyString);
        bool heightIsEmpty = heightString->equals(String::emptyString);
        if (widthIsEmpty && heightIsEmpty) {
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
                std::make_pair(Length(), Length());
        } else if (widthIsEmpty) {
            float h = String::parseFloat(heightString);
            bool heightIsPercent =
                heightString->lastIndexOf('%') == heightString->length() - 1;
            Length height = heightIsPercent
                                ? Length(Length::Percent, (float)h / 100)
                                : Length(Length::Fixed, h);
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
                std::make_pair(Length(), height);
        } else if (heightIsEmpty) {
            float w = String::parseFloat(widthString);
            bool widthIsPercent =
                widthString->lastIndexOf('%') == widthString->length() - 1;
            Length width = widthIsPercent
                               ? Length(Length::Percent, (float)w / 100)
                               : Length(Length::Fixed, w);
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
                std::make_pair(width, Length());
        } else {
            float w = String::parseFloat(widthString);
            float h = String::parseFloat(heightString);
            bool heightIsPercent =
                heightString->lastIndexOf('%') == heightString->length() - 1;
            bool widthIsPercent =
                widthString->lastIndexOf('%') == widthString->length() - 1;
            Length width = widthIsPercent
                               ? Length(Length::Percent, (float)w / 100)
                               : Length(Length::Fixed, w);
            Length height = heightIsPercent
                                ? Length(Length::Percent, (float)h / 100)
                                : Length(Length::Fixed, h);
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
                std::make_pair(width, height);
        }
    } else {
        result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
            std::make_pair(Length(Length::Fixed, 0), Length(Length::Fixed, 0));
        bool widthIsEmpty = widthString->equals(String::emptyString);
        bool heightIsEmpty = heightString->equals(String::emptyString);
        if (widthIsEmpty && heightIsEmpty) {
        } else if (widthIsEmpty) {
            float h = String::parseFloat(heightString);
            bool heightIsPercent =
                heightString->lastIndexOf('%') == heightString->length() - 1;
            Length height = heightIsPercent
                                ? Length(Length::Percent, (float)h / 100)
                                : Length(Length::Fixed, h);
            if (!heightIsPercent) {
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
                    std::make_pair(Length(Length::Fixed, 0), height);
            }
        } else if (heightIsEmpty) {
            float w = String::parseFloat(widthString);
            bool widthIsPercent =
                widthString->lastIndexOf('%') == widthString->length() - 1;
            Length width = widthIsPercent
                               ? Length(Length::Percent, (float)w / 100)
                               : Length(Length::Fixed, w);
            if (!widthIsPercent) {
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
                    std::make_pair(width, Length(Length::Fixed, 0));
            }
        } else {
            float w = String::parseFloat(widthString);
            float h = String::parseFloat(heightString);
            bool heightIsPercent =
                heightString->lastIndexOf('%') == heightString->length() - 1;
            bool widthIsPercent =
                widthString->lastIndexOf('%') == widthString->length() - 1;
            Length width = widthIsPercent ? Length(Length::Fixed, 0)
                                          : Length(Length::Fixed, w);
            Length height = heightIsPercent ? Length(Length::Fixed, 0)
                                            : Length(Length::Fixed, h);
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement =
                std::make_pair(width, height);
        }
    }

    if (result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.first
            .isSpecified()) {
        if (!result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.first
                 .isPositiveOrZero()) {
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.first =
                Length();
        }
    }

    if (result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.second
            .isSpecified()) {
        if (!result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.second
                 .isPositiveOrZero()) {
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.second =
                Length();
        }
    }

    return result;
}

void FrameReplaced::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    IntrinsicSizeUsedInLayout s = computeIntrinsicSizeForLayout();
    Length width = style()->width();
    Length height = style()->height();
    Length left = style()->left();
    Length right = style()->right();
    Length marginLeft = style()->marginLeft();
    Length marginRight = style()->marginRight();
    DirectionValue direction = style()->direction();

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        LayoutUnit parentContentWidth =
            ctx.containingFrameBlockBox(this)->contentWidth();
        LayoutUnit intrinsicWidth, intrinsicHeight;
        computeBorderMarginPadding(parentContentWidth);

        if (style()->position() == AbsolutePositionValue) {
            FrameBox* cb = ctx.containingBlock(this);
            LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();

            Length parentContentHeight = Length(Length::Fixed, parentHeight);
            computeIntrinsicSize(intrinsicWidth, intrinsicHeight,
                                 parentContentWidth, parentContentHeight);

            computeBorderMarginPadding(cb->contentWidth());

            DirectionValue parentDirection =
                ctx.blockContainer(this)->style()->direction();

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

            // 10.3.8 Absolutely positioned, replaced elements
            // 'left' + 'margin-left' + 'border-left-width' + 'padding-left' +
            // 'width' + 'padding-right' + 'border-right-width' + 'margin-right'
            // + 'right' = width of containing block

            if ((intrinsicWidth == 0 || intrinsicHeight == 0) &&
                (width.isAuto() || height.isAuto())) {
                setContentWidth(0);
                setContentHeight(0);
            } else if (width.isAuto() && height.isAuto()) {
                applyMinMaxValueIfNeeds(intrinsicWidth, intrinsicHeight,
                                        parentContentWidth, parentHeight);
            } else if (width.isSpecified() && height.isAuto()) {
                LayoutUnit w = width.specifiedValue(cb->contentWidth());
                LayoutUnit h = w * (intrinsicHeight / intrinsicWidth);
                applyMinMaxValueIfNeeds(w, h, parentContentWidth, parentHeight);
            } else if (width.isAuto() && height.isSpecified()) {
                LayoutUnit h = height.specifiedValue(cb->contentHeight());
                LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                applyMinMaxValueIfNeeds(w, h, parentContentWidth, parentHeight);
            } else {
                STARFISH_ASSERT(width.isSpecified() && height.isSpecified());
                if (height.isFixed()) {
                    applyMinMaxValueIfNeeds(
                        width.specifiedValue(cb->contentWidth()),
                        height.fixed(), parentContentWidth, parentHeight);
                } else {
                    if (ctx.parentHasFixedHeight(this)) {
                        applyMinMaxValueIfNeeds(
                            width.specifiedValue(cb->contentWidth()),
                            height.specifiedValue(cb->contentHeight()),
                            parentContentWidth, parentHeight);
                    } else {
                        applyMinMaxValueIfNeeds(
                            width.specifiedValue(cb->contentWidth()),
                            intrinsicHeight, parentContentWidth, parentHeight,
                            true, false);
                    }
                }
            }

            LayoutUnit containgBlockContentWidth =
                cb->contentWidth() + cb->paddingWidth();

            // If 'margin-left' or 'margin-right' is specified as 'auto' its
            // used value is determined by the rules below. If both 'left' and
            // 'right' have the value 'auto' then if the 'direction' property
            // of the element establishing the static-position containing block
            // is 'ltr', set 'left' to the static position; else if 'direction'
            // is 'rtl', set 'right' to the static position.
            if (left.isAuto() && right.isAuto()) {
                // static location computed in normal flow processing
                applyHorizontalMargin();
            } else if (!left.isAuto() && right.isAuto()) {
                LayoutUnit l = left.specifiedValue(containgBlockContentWidth);
                setAbsX(l, absX);
                applyHorizontalMargin(direction == RtlDirectionValue);
            } else if (left.isAuto() && !right.isAuto()) {
                LayoutUnit r = right.specifiedValue(containgBlockContentWidth);
                setAbsX(containgBlockContentWidth - r - FrameBox::width(),
                        absX);
                applyHorizontalMargin(direction == LtrDirectionValue);
            } else {
                bool isOpposite = false;
                if (marginLeft.isAuto() && marginRight.isAuto()) {
                    LayoutUnit l =
                        left.specifiedValue(containgBlockContentWidth);
                    LayoutUnit r =
                        right.specifiedValue(containgBlockContentWidth);
                    computeHorizontalMargin(containgBlockContentWidth - l - r);
                } else if (marginRight.isAuto() &&
                           direction == RtlDirectionValue) {
                    isOpposite = true;
                } else if (marginLeft.isAuto() &&
                           direction == LtrDirectionValue) {
                    isOpposite = true;
                }
                if ((direction == LtrDirectionValue && !isOpposite) ||
                    (direction == RtlDirectionValue && isOpposite)) {
                    LayoutUnit l =
                        left.specifiedValue(containgBlockContentWidth);
                    setAbsX(l, absX);
                } else {
                    LayoutUnit r =
                        right.specifiedValue(containgBlockContentWidth);
                    setAbsX(containgBlockContentWidth - FrameBox::width() - r,
                            absX);
                }
                applyHorizontalMargin(isOpposite);
            }

            if (left.isAuto() && right.isAuto() &&
                parentDirection == RtlDirectionValue) {
                moveX(-FrameBox::width());
            }
        } else {
            LayoutUnit parentHeight;
            Length parentContentHeight;
            bool parentHasFixedHeight = ctx.parentHasFixedHeight(this);
            if (parentHasFixedHeight) {
                parentContentHeight =
                    Length(Length::Fixed, ctx.parentFixedHeight(this));
                parentHeight = ctx.parentFixedHeight(this);
            } else {
                parentContentHeight = Length(Length::Auto);
            }
            computeIntrinsicSize(intrinsicWidth, intrinsicHeight,
                                 parentContentWidth, parentContentHeight);

            if ((intrinsicWidth == 0 || intrinsicHeight == 0) &&
                (width.isAuto() || height.isAuto())) {
                setContentWidth(0);
                setContentHeight(0);
            } else if (width.isAuto() && style()->height().isAuto()) {
                applyMinMaxValueIfNeeds(intrinsicWidth, intrinsicHeight,
                                        parentContentWidth, parentHeight, true,
                                        parentHasFixedHeight);
            } else if (style()->width().isSpecified() &&
                       style()->height().isAuto()) {
                LayoutUnit w = style()->width().specifiedValue(
                    ctx.parentContentWidth(this));
                LayoutUnit h = w * (intrinsicHeight / intrinsicWidth);
                applyMinMaxValueIfNeeds(w, h, parentContentWidth, parentHeight,
                                        true, parentHasFixedHeight);
            } else if (style()->width().isAuto() &&
                       style()->height().isSpecified()) {
                if (style()->height().isFixed()) {
                    LayoutUnit h = style()->height().fixed();
                    LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                    applyMinMaxValueIfNeeds(w, h, parentContentWidth,
                                            parentHeight, true,
                                            parentHasFixedHeight);
                } else {
                    STARFISH_ASSERT(style()->height().isPercent());
                    if (ctx.parentHasFixedHeight(this)) {
                        LayoutUnit h = style()->height().percent() *
                                       ctx.parentFixedHeight(this);
                        LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                        applyMinMaxValueIfNeeds(w, h, parentContentWidth,
                                                parentHeight);
                    } else {
                        applyMinMaxValueIfNeeds(intrinsicWidth, intrinsicHeight,
                                                parentContentWidth,
                                                parentHeight, true, false);
                    }
                }
            } else {
                STARFISH_ASSERT(width.isSpecified() && height.isSpecified());
                LayoutUnit w =
                    width.specifiedValue(ctx.parentContentWidth(this));
                if (height.isFixed()) {
                    applyMinMaxValueIfNeeds(w, height.fixed(),
                                            parentContentWidth, parentHeight,
                                            true, parentHasFixedHeight);
                } else {
                    if (ctx.parentHasFixedHeight(this)) {
                        applyMinMaxValueIfNeeds(
                            w, height.percent() * ctx.parentFixedHeight(this),
                            parentContentWidth, parentHeight);
                    } else {
                        LayoutUnit h = w * (intrinsicHeight / intrinsicWidth);
                        applyMinMaxValueIfNeeds(w, h, parentContentWidth,
                                                parentHeight, true, false);
                    }
                }
            }

            if (isNormalFlow() && style()->display() == BlockDisplayValue) {
                computeHorizontalMargin(parentContentWidth);
            }
        }
    }

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        if (style() && style()->position() == AbsolutePositionValue) {
            FrameBox* cb = ctx.containingBlock(this);
            FrameBox* parent = Frame::layoutParent()->asFrameBox();
            LayoutLocation l1, l2;
            if (cb->isAncestorOf(parent)) {
                l2 = parent->absolutePoint(cb);
            } else {
                l1 = cb->absolutePoint(ctx.frameDocument());
                l2 = parent->absolutePoint(ctx.frameDocument());
            }

            LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();
            Length top = style()->top();
            Length bottom = style()->bottom();

            LayoutUnit absY = l2.y() - l1.y() - cb->borderTop();

            if (top.isAuto() && bottom.isAuto()) {
                // static location computed in normal flow processing
            } else if (!top.isAuto() && bottom.isAuto()) {
                setAbsY(top.specifiedValue(parentHeight), absY);
            } else if (top.isAuto() && !bottom.isAuto()) {
                LayoutUnit b = bottom.specifiedValue(parentHeight);
                setAbsY(parentHeight - b - FrameBox::height(), absY);
            } else {
                LayoutUnit t = top.specifiedValue(parentHeight);
                setAbsY(t, absY);
            }

            applyVerticalMargin();
        }
    }
}

void FrameReplaced::computeIntrinsicSize(LayoutUnit& intrinsicWidth,
                                         LayoutUnit& intrinsicHeight,
                                         LayoutUnit parentContentWidth,
                                         Length parentContentHeight)
{
    IntrinsicSizeUsedInLayout s = computeIntrinsicSizeForLayout();
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
            intrinsicHeight =
                a.second.specifiedValue(parentContentHeight.fixed());
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
            intrinsicHeight =
                a.second.specifiedValue(parentContentHeight.fixed());
            intrinsicWidth = intrinsicHeight * (b.width() / b.height());
        } else {
            intrinsicWidth = s.m_intrinsicContentSize.width();
            intrinsicHeight = s.m_intrinsicContentSize.height();
        }
        intrinsicHeight = a.second.fixed();
        intrinsicWidth = intrinsicHeight * (b.width() / b.height());
    }
}

void FrameReplaced::paint(PaintingContext& ctx)
{
    if (isEstablishesStackingContext())
        return;

    if (isPositioned()) {
        if (ctx.m_paintingStage == PaintingPositionedElements) {
            paintBackgroundAndBorders(ctx.m_canvas);
            paintReplaced(ctx.m_canvas);
        }
    } else if (isFloating()) {
        if (ctx.m_paintingStage == PaintingNonPositionedFloats) {
            paintBackgroundAndBorders(ctx.m_canvas);
            paintReplaced(ctx.m_canvas);
        }
    } else {
        if (ctx.m_paintingStage == PaintingNormalFlowBlock) {
            if (style()->display() != DisplayValue::InlineDisplayValue) {
                paintBackgroundAndBorders(ctx.m_canvas);
            }
        } else if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            if (style()->display() == DisplayValue::InlineDisplayValue) {
                if (ctx.m_paintingInlineStage == PaintingInlineLevelElements) {
                    paintBackgroundAndBorders(ctx.m_canvas);
                    paintReplaced(ctx.m_canvas);
                }
            } else if (style()->display() ==
                       DisplayValue::InlineBlockDisplayValue) {
                if (ctx.m_paintingInlineStage == PaintingInlineBlock) {
                    paintReplaced(ctx.m_canvas);
                }
            } else {
                paintReplaced(ctx.m_canvas);
            }
        }
    }
}
}
