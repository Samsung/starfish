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
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

IntrinsicSizeUsedInLayout FrameReplaced::computeIntrinsicSizeForLayout()
{
    IntrinsicSize siz = intrinsicSize();
    IntrinsicSizeUsedInLayout result;
    result.m_hasAspectRatio = siz.m_hasAspectRatio;
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

void FrameReplaced::computeContentWidthAndHeight(LayoutContext& ctx,
                                                 FrameBox* cb)
{
    Length width = style()->width();
    Length height = style()->height();
    LayoutUnit intrinsicWidth, intrinsicHeight;
    LayoutUnit parentContentWidth, parentContentHeight;
    Length parentHeightLength;
    bool parentHasFixedHeight;
    bool hasAspectRatio;

    if (style()->position() == AbsolutePositionValue) {
        parentContentWidth = cb->contentWidth() + cb->paddingWidth();
        parentHasFixedHeight = true;
        parentContentHeight = cb->contentHeight() + cb->paddingHeight();
    } else {
        parentContentWidth = cb->contentWidth();
        parentHasFixedHeight = ctx.parentHasFixedHeight(this);
        if (parentHasFixedHeight) {
            parentContentHeight = ctx.parentFixedHeight(this);
        }
    }

    if (parentHasFixedHeight) {
        parentHeightLength = Length(Length::Fixed, parentContentHeight);
    } else {
        parentHeightLength = Length(Length::Auto);
    }

    computeIntrinsicSize(intrinsicWidth, intrinsicHeight, hasAspectRatio,
                         parentContentWidth, parentHeightLength);

    LayoutUnit w, h;

    if ((intrinsicWidth == 0 || intrinsicHeight == 0) &&
        (width.isAuto() || height.isAuto())) {
        setContentWidth(0);
        setContentHeight(0);
        return;
    } else if (width.isAuto() && height.isAuto()) {
        w = intrinsicWidth;
        h = intrinsicHeight;
    } else if (height.isAuto()) {
        w = width.specifiedValue(parentContentWidth);
        if (hasAspectRatio) {
            h = w * (intrinsicHeight / intrinsicWidth);
        } else {
            h = intrinsicHeight;
        }
    } else if (width.isAuto()) {
        if (height.isFixed() || parentHasFixedHeight) {
            h = height.specifiedValue(parentContentHeight);
            if (hasAspectRatio) {
                w = h * (intrinsicWidth / intrinsicHeight);
            } else {
                w = intrinsicWidth;
            }
        } else {
            w = intrinsicWidth;
            h = intrinsicHeight;
        }
    } else {
        STARFISH_ASSERT(width.isSpecified() && height.isSpecified());
        w = width.specifiedValue(parentContentWidth);
        if (height.isFixed() || parentHasFixedHeight) {
            h = height.specifiedValue(parentContentHeight);
        } else {
            if (hasAspectRatio) {
                h = w * (intrinsicHeight / intrinsicWidth);
            } else {
                h = intrinsicHeight;
            }
        }
    }

    applyMinMaxValueIfNeeds(w, h, parentContentWidth, parentContentHeight,
                            parentHasFixedHeight);
}

void FrameReplaced::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        FrameBox* cb = ctx.containingBlock(this);
        LayoutUnit parentContentWidth = cb->contentWidth();
        computeBorderMarginPadding(parentContentWidth);
        computeContentWidthAndHeight(ctx, cb);

        if (style()->position() == AbsolutePositionValue) {
            DirectionValue parentDirection =
                ctx.blockContainer(this)->style()->direction();
            HorizontalDataLocToContainingBlock data =
                computeHorizontalDataToContainingBlock(ctx, cb);
            Length left = style()->left();
            Length right = style()->right();

            if (left.isAuto() && right.isAuto()) {
                // static location computed in normal flow processing
                if (parentDirection == LtrDirectionValue) {
                    moveX(FrameBox::marginLeft());
                } else {
                    // if the 'direction' property of the element establishing
                    // the static-position containing block is 'ltr' set 'left'
                    // to the static position, otherwise set 'right' to the
                    // static position. Then solve for 'left' (if 'direction
                    // is 'rtl') or 'right' (if 'direction' is 'ltr').
                    moveX(-FrameBox::width() - FrameBox::marginRight());
                }
            } else if (!left.isAuto() && !right.isAuto()) {
                Length marginLeft = style()->marginLeft();
                Length marginRight = style()->marginRight();
                bool relativeToLeft = false;

                if (marginLeft.isAuto() && marginRight.isAuto()) {
                    // If at this point both 'margin-left' and 'margin-right'
                    // are still 'auto', solve the equation under the extra
                    // constraint that the two margins must get equal values,
                    // unless this would make them negative, in which case when
                    // the direction of the containing block is 'ltr' ('rtl'),
                    // set 'margin-left' ('margin-right') to zero and solve
                    // for 'margin-right' ('margin-left').
                    computeHorizontalMargin(data.m_contentWidth - data.m_left -
                                            data.m_right);
                    relativeToLeft =
                        parentDirection == DirectionValue::LtrDirectionValue;
                } else if (marginLeft.isAuto()) {
                    relativeToLeft = false;
                } else if (marginRight.isAuto()) {
                    relativeToLeft = true;
                } else {
                    relativeToLeft =
                        parentDirection == DirectionValue::LtrDirectionValue;
                }

                if (relativeToLeft) {
                    setX(data.m_left + FrameBox::marginLeft() - data.m_absX);
                } else {
                    setX(data.m_contentWidth - FrameBox::width() -
                         data.m_right - FrameBox::marginRight() - data.m_absX);
                }
            } else {
                if (left.isSpecified()) {
                    setX(data.m_left + FrameBox::marginLeft() - data.m_absX);
                } else {
                    setX(data.m_contentWidth - data.m_right -
                         FrameBox::width() - FrameBox::marginRight() -
                         data.m_absX);
                }
            }
        } else if (isNormalFlow() && isBlockLevel()) {
            computeHorizontalMargin(parentContentWidth);
        }
    }

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        if (style()->position() == AbsolutePositionValue) {
            FrameBox* cb = ctx.containingBlock(this);
            VerticalDataLocToContainingBlock data =
                computeVerticalDataToContainingBlock(ctx, cb);
            Length top = style()->top();
            Length bottom = style()->bottom();

            if (top.isAuto() && bottom.isAuto()) {
                // static location computed in normal flow processing
            } else if (!top.isAuto() && bottom.isAuto()) {
                setY(data.m_top - data.m_absY);
            } else if (top.isAuto() && !bottom.isAuto()) {
                setY(data.m_contentHeight - data.m_bottom - FrameBox::height() -
                     data.m_absY);
            } else {
                setY(data.m_top - data.m_absY);
            }

            applyVerticalMarginForAbsoluteBox();
        }
    }
}

void FrameReplaced::computeIntrinsicSize(LayoutUnit& intrinsicWidth,
                                         LayoutUnit& intrinsicHeight,
                                         bool& hasAspectRatio,
                                         LayoutUnit parentContentWidth,
                                         Length parentContentHeight)
{
    IntrinsicSizeUsedInLayout s = computeIntrinsicSizeForLayout();
    hasAspectRatio = s.m_hasAspectRatio;
    auto a = s.m_intrinsicSizeIsSpecifiedByAttributeOfElement;
    auto b = s.m_intrinsicContentSize;
    if (a.first.isAuto() && a.second.isAuto()) {
        intrinsicWidth = s.m_intrinsicContentSize.width();
        intrinsicHeight = s.m_intrinsicContentSize.height();
    } else if (a.first.isSpecified() && a.second.isAuto()) {
        intrinsicWidth = a.first.specifiedValue(parentContentWidth);
        if (s.m_hasAspectRatio) {
            intrinsicHeight = intrinsicWidth * (b.height() / b.width());
        } else {
            intrinsicHeight = b.height();
        }
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
            if (s.m_hasAspectRatio) {
                intrinsicHeight = intrinsicWidth * (b.height() / b.width());
            } else {
                intrinsicHeight = b.height();
            }
        }
    } else if (a.first.isAuto() && a.second.isFixed()) {
        intrinsicHeight = a.second.fixed();
        if (s.m_hasAspectRatio) {
            intrinsicWidth = intrinsicHeight * (b.width() / b.height());
        } else {
            intrinsicWidth = b.width();
        }
    } else {
        STARFISH_ASSERT(a.first.isAuto() && a.second.isPercent());
        if (parentContentHeight.isFixed()) {
            intrinsicHeight =
                a.second.specifiedValue(parentContentHeight.fixed());
            if (s.m_hasAspectRatio) {
                intrinsicWidth = intrinsicHeight * (b.width() / b.height());
            } else {
                intrinsicWidth = b.width();
            }
        } else {
            intrinsicWidth = s.m_intrinsicContentSize.width();
            intrinsicHeight = s.m_intrinsicContentSize.height();
        }
        // intrinsicHeight = a.second.fixed();
        // intrinsicWidth = intrinsicHeight * (b.width() / b.height());
    }
}

void FrameReplaced::paint(PaintingContext& ctx)
{
    if (isEstablishesStackingContext()) {
        ctx.m_canvas->saveByFrame(this);
        return;
    }

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
