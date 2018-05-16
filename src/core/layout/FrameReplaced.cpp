/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Node.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

void* FrameReplaced::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameReplaced)] = { 0 };
        FrameReplaced::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameReplaced));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

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

std::pair<LayoutUnit, LayoutUnit>
FrameReplaced::minMaxWidthAndHeightAppliedIfNeeds(
    LayoutContext& ctx, LayoutUnit w, LayoutUnit h, LayoutUnit parentWidth,
    LayoutUnit parentHeight, bool hasAspectRatio,
    bool parentHeightHasFixedValue)
{
    LayoutUnit newWidth = w;
    LayoutUnit newHeight = h;
    Length width = style()->width();
    Length height = style()->height();
    Length minWidth = style()->minWidth();
    Length maxWidth = style()->maxWidth();
    Length minHeight = style()->minHeight();
    Length maxHeight = style()->maxHeight();
    bool canApplyMinHeight = minHeight.isDefinite(parentHeightHasFixedValue);
    bool canApplyMaxHeight = maxHeight.isDefinite(parentHeightHasFixedValue);

    if (minWidth.isSpecified()) {
        newWidth = std::max(w, contentWidthApplyingBoxSizing(
                                   minWidth.specifiedValue(parentWidth, this)));
        if (canApplyMinHeight) {
            newHeight =
                std::max(h, contentHeightApplyingBoxSizing(
                                minHeight.specifiedValue(parentHeight, this)));
            if (width.isAuto() && height.isAuto()) {
                if (hasAspectRatio) {
                    if (newWidth > newHeight) {
                        newHeight = newWidth * (h / w);
                    } else if (newWidth < newHeight) {
                        newWidth = newHeight * (w / h);
                    }
                }
            } else if (width.isAuto()) {
                if (hasAspectRatio && newWidth < newHeight) {
                    newWidth = newHeight * (w / h);
                }
            } else if (height.isAuto()) {
                if (hasAspectRatio && newWidth > newHeight) {
                    newHeight = newWidth * (h / w);
                }
            }
        } else if (canApplyMaxHeight) {
            // in the case minWidth and maxHeight, then apply values
            // respectively.
            newHeight =
                std::min(h, contentHeightApplyingBoxSizing(
                                maxHeight.specifiedValue(parentHeight, this)));
        } else {
            if (hasAspectRatio && height.isAuto()) {
                newHeight = newWidth * (h / w);
            }
        }
    } else if (maxWidth.isSpecified()) {
        newWidth = std::min(w, contentWidthApplyingBoxSizing(
                                   maxWidth.specifiedValue(parentWidth, this)));
        if (canApplyMinHeight) {
            // in the case maxWidth and minHeight, then apply values
            // respectively.
            newHeight =
                std::max(h, contentHeightApplyingBoxSizing(
                                minHeight.specifiedValue(parentHeight, this)));
        } else if (canApplyMaxHeight) {
            newHeight =
                std::min(h, contentHeightApplyingBoxSizing(
                                maxHeight.specifiedValue(parentHeight, this)));
            if (width.isAuto() && height.isAuto()) {
                if (hasAspectRatio) {
                    if (newWidth > newHeight) {
                        newWidth = newHeight * (w / h);
                    } else if (newWidth < newHeight) {
                        newHeight = newWidth * (h / w);
                    }
                }
            } else if (width.isAuto()) {
                if (hasAspectRatio && newWidth > newHeight) {
                    newWidth = newHeight * (w / h);
                } else if (hasAspectRatio && width.isAuto() &&
                           height.isAuto()) {
                    newHeight = newWidth * (h / w);
                }
            } else if (height.isAuto()) {
                if (hasAspectRatio && newWidth < newHeight) {
                    newHeight = newWidth * (h / w);
                } else if (hasAspectRatio && width.isAuto() &&
                           height.isAuto()) {
                    newWidth = newHeight * (w / h);
                }
            }
        } else {
            if (hasAspectRatio && height.isAuto()) {
                newHeight = newWidth * (h / w);
            }
        }
    } else {
        if (canApplyMinHeight) {
            newHeight =
                std::max(h, contentHeightApplyingBoxSizing(
                                minHeight.specifiedValue(parentHeight, this)));
            if (hasAspectRatio && width.isAuto()) {
                newWidth = newHeight * (w / h);
            }
        } else if (canApplyMaxHeight) {
            newHeight =
                std::min(h, contentHeightApplyingBoxSizing(
                                maxHeight.specifiedValue(parentHeight, this)));
            if (hasAspectRatio && width.isAuto()) {
                newWidth = newHeight * (w / h);
            }
        }
    }

    return std::make_pair(newWidth, newHeight);
}

void FrameReplaced::computeContentWidthAndHeight(LayoutContext& ctx,
                                                 FrameBox* cb)
{
    STARFISH_ASSERT(cb);
    if (isEstablishesBlockFormattingContext()) {
        if (!shouldLayout(ctx, Frame::ResolveAll, cb)) {
            return;
        }
    }
    Length width = style()->width();
    Length height = style()->height();
    LayoutUnit intrinsicWidth, intrinsicHeight;
    LayoutUnit parentContentWidth, parentContentHeight;
    Length parentHeightLength;
    bool parentHasFixedHeight;
    bool hasAspectRatio;

    parentContentWidth = cb->contentWidth() + cb->paddingWidth();
    parentHasFixedHeight = ctx.parentHasFixedHeight(this);
    if (parentHasFixedHeight) {
        parentContentHeight = ctx.parentFixedHeight(this);
        parentHeightLength = Length(Length::Fixed, parentContentHeight);
    } else {
        parentHeightLength = Length(Length::Auto);
    }

    computeIntrinsicSize(ctx, intrinsicWidth, intrinsicHeight, hasAspectRatio,
                         parentContentWidth, parentHeightLength);

    LayoutUnit w, h;

    if (node()->isHTMLImageElement() &&
        (node()->asHTMLImageElement()->imageData() ==
         node()->document()->brokenImage())) {
        if (width.isAuto() || height.isAuto()) {
            width = Length(Length::Type::Fixed, intrinsicWidth);
            height = Length(Length::Type::Fixed, intrinsicHeight);
        }
    }

    if ((intrinsicWidth == 0 || intrinsicHeight == 0) &&
        (width.isAuto() || height.isAuto())) {
        setContentWidth(0);
        setContentHeight(0);
        return;
    } else if (width.isAuto() && height.isAuto()) {
        w = intrinsicWidth;
        h = intrinsicHeight;
    } else if (height.isAuto()) {
        w = width.specifiedValue(parentContentWidth, this);
        w = contentWidthApplyingBoxSizing(w);
        if (hasAspectRatio) {
            h = w * (intrinsicHeight / intrinsicWidth);
        } else {
            h = intrinsicHeight;
        }
    } else if (width.isAuto()) {
        if (height.isDefinite(parentHasFixedHeight)) {
            h = height.specifiedValue(parentContentHeight, this);
            h = contentHeightApplyingBoxSizing(h);
            if (hasAspectRatio && intrinsicHeight) {
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
        w = width.specifiedValue(parentContentWidth, this);
        w = contentWidthApplyingBoxSizing(w);
        if (height.isDefinite(parentHasFixedHeight)) {
            h = height.specifiedValue(parentContentHeight, this);
            h = contentHeightApplyingBoxSizing(h);
        } else {
            if (hasAspectRatio && intrinsicWidth) {
                h = w * (intrinsicHeight / intrinsicWidth);
            } else {
                h = intrinsicHeight;
            }
        }
    }

    applyMinMaxWidthAndHeightIfNeeds(ctx, w, h, parentContentWidth,
                                     parentContentHeight, hasAspectRatio,
                                     parentHasFixedHeight);

    if (isFlexItem()) {
        ctx.registerContentHeight(this, contentHeight());
    }
}

void FrameReplaced::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    Frame::computePaintingFlags(ctx, resolveWhat);

    FrameBox* cb = containingBlock(this);
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        clearContentWidthDamaged();
        clearContentHeightDamaged();
        LayoutUnit parentContentWidth = cb->contentWidth();
        DirectionValue parentDirection =
            blockContainer(this)->style()->direction();
        computeBorderMarginPadding(ctx, parentContentWidth);
        LayoutUnit oldContentWidth = contentWidth();
        LayoutUnit oldContentHeight = contentHeight();

        computeContentWidthAndHeight(ctx, cb);

        if (oldContentWidth != contentWidth()) {
            markContentWidthDamaged();
        } else {
            clearContentWidthDamaged();
        }

        if (oldContentHeight != contentHeight()) {
            markContentHeightDamaged();
        } else {
            clearContentHeightDamaged();
        }

        if (isAbsolutePositioned()) {
            HorizontalDataLocToContainingBlock data =
                computeHorizontalDataToContainingBlock(ctx, cb);
            LengthData offset = style()->offset();
            Length left = offset.left();
            Length right = offset.right();

            if (left.isAuto() && right.isAuto()) {
                if (parent()->isAnonymous() &&
                    parent()->parent()->isFrameFlexibleBox() &&
                    !parent()->isFlexItem()) {
                    moveToStaticPositionForAbsolutedPositionedBoxHorizontally(
                        this);
                } else {
                    if (parentDirection == LtrDirectionValue) {
                        moveX(FrameBox::marginLeft());
                    } else {
                        moveX(-FrameBox::width() - FrameBox::marginRight());
                    }
                }
            } else if (!left.isAuto() && !right.isAuto()) {
                computeHorizontalMargin(data.m_contentWidth - data.m_left -
                                            data.m_right,
                                        parentDirection);
                LengthData margin = style()->margin();
                Length marginLeft = margin.left();
                Length marginRight = margin.right();
                bool relativeToLeft = false;

                if (marginLeft.isAuto() && marginRight.isAuto()) {
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
        } else if (isNormalFlow() && isBlockLevel() && !isFlexItem()) {
            computeHorizontalMargin(parentContentWidth, parentDirection);
        }
    }

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        if (isAbsolutePositioned()) {
            VerticalDataLocToContainingBlock data =
                computeVerticalDataToContainingBlock(ctx, cb);
            LengthData offset = style()->offset();
            Length top = offset.top();
            Length bottom = offset.bottom();

            if (top.isAuto() && bottom.isAuto()) {
                // static location computed in normal flow processing
                if (parent()->isAnonymous() &&
                    parent()->parent()->isFrameFlexibleBox() &&
                    !parent()->isFlexItem()) {
                    moveToStaticPositionForAbsolutedPositionedBoxVertically(
                        this);
                } else {
                    moveY(marginTop());
                }
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
        }

        clearNeedsLayout();
    }
}

void FrameReplaced::computeIntrinsicSize(LayoutContext& ctx,
                                         LayoutUnit& intrinsicWidth,
                                         LayoutUnit& intrinsicHeight,
                                         bool& hasAspectRatio,
                                         LayoutUnit parentContentWidth,
                                         Length parentContentHeight)
{
    IntrinsicSizeUsedInLayout s = computeIntrinsicSizeForLayout();
    hasAspectRatio = s.m_hasAspectRatio;
    auto a = s.m_intrinsicSizeIsSpecifiedByAttributeOfElement;
    auto b = s.m_intrinsicContentSize;

    if (a.first.isAuto() || parentContentWidth == intMaxForLayoutUnit) {
        if (a.second.isAuto()) {
            intrinsicWidth = s.m_intrinsicContentSize.width();
            intrinsicHeight = s.m_intrinsicContentSize.height();
        } else if (a.second.isDefinite(false) && !a.first.isDefinite(false)) {
            LayoutUnit unused;
            intrinsicHeight = a.second.specifiedValue(unused, this);
            if (s.m_hasAspectRatio) {
                intrinsicWidth = intrinsicHeight * (b.width() / b.height());
            } else {
                intrinsicWidth = b.width();
            }
        } else if (a.second.isDefinite(false) && a.first.isDefinite(false)) {
            LayoutUnit unused;
            intrinsicHeight = a.second.specifiedValue(unused, this);
            intrinsicWidth = a.first.specifiedValue(unused, this);
        } else {
            STARFISH_ASSERT(a.second.isPercent() || a.second.isCalc());
            if (parentContentHeight.isFixed()) {
                intrinsicHeight =
                    a.second.specifiedValue(parentContentHeight.fixed(), this);
                if (s.m_hasAspectRatio) {
                    intrinsicWidth = intrinsicHeight * (b.width() / b.height());
                } else {
                    intrinsicWidth = b.width();
                }
            } else {
                intrinsicWidth = s.m_intrinsicContentSize.width();
                intrinsicHeight = s.m_intrinsicContentSize.height();
            }
        }
    } else {
        STARFISH_ASSERT(parentContentWidth != intMaxForLayoutUnit &&
                        a.first.isSpecified());
        intrinsicWidth = a.first.specifiedValue(parentContentWidth, this);
        if (a.second.isAuto()) {
            if (s.m_hasAspectRatio) {
                intrinsicHeight = intrinsicWidth * (b.height() / b.width());
            } else {
                intrinsicHeight = b.height();
            }
        } else if (a.second.isDefinite(false)) {
            LayoutUnit unused;
            intrinsicHeight = a.second.specifiedValue(unused, this);
        } else {
            if ((parentContentHeight.isFixed())) {
                intrinsicHeight =
                    a.second.specifiedValue(parentContentHeight.fixed(), this);
            } else {
                if (s.m_hasAspectRatio) {
                    intrinsicHeight = intrinsicWidth * (b.height() / b.width());
                } else {
                    intrinsicHeight = b.height();
                }
            }
        }
    }
}

LayoutRect FrameReplaced::computeObjectFit(const LayoutUnit& w,
                                           const LayoutUnit& h)
{
    LayoutRect contentRect =
        LayoutRect(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                   contentWidth(), contentHeight());
    if (style()->objectSizing() == ObjectSizingData()) {
        return contentRect;
    }

    LayoutSize intrinsicSize = LayoutSize(w, h);
    if (!intrinsicSize.width() || !intrinsicSize.height()) {
        return contentRect;
    }

    LayoutRect rect = contentRect;
    ObjectFitValue objectFit = style()->objectFit();
    switch (objectFit) {
    case ObjectFitValue::ContainObjectFitValue:
    case ObjectFitValue::ScaledownObjectFitValue:
    case ObjectFitValue::CoverObjectFitValue:
        rect.setSize(rect.size().fitToAspectRatio(
            intrinsicSize, objectFit == ObjectFitValue::CoverObjectFitValue
                               ? GrowAspectRatioFit
                               : ShrinkAspectRatioFit));
        if (objectFit != ObjectFitValue::ScaledownObjectFitValue ||
            rect.width() <= intrinsicSize.width()) {
            break;
        }
    case ObjectFitValue::NoneObjectFitValue:
        rect.setSize(intrinsicSize);
        break;
    case ObjectFitValue::FillObjectFitValue:
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    LayoutUnit offsetX = style()->objectPositionX().specifiedValue(
        contentRect.width() - rect.width(), this);
    LayoutUnit offsetY = style()->objectPositionY().specifiedValue(
        contentRect.height() - rect.height(), this);
    rect.setX(rect.x() + offsetX);
    rect.setY(rect.y() + offsetY);

    return rect;
}

void FrameReplaced::paintContent(PaintingContext& ctx)
{
    if (canSkipPaintingStage(ctx)) {
        return;
    }

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        ctx.m_canvas->setVisible(false);
    } else {
        ctx.m_canvas->setVisible(true);
    }

    ctx.m_canvas->save();
    if (isFlexItem()) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            paintBackgroundAndBorders(ctx.m_canvas);
            paintReplaced(ctx.m_canvas);
            paintOutline(ctx.m_canvas);
        }
    } else if (isFloating()) {
        if (ctx.m_paintingStage == PaintingNonPositionedFloats) {
            paintBackgroundAndBorders(ctx.m_canvas);
            paintReplaced(ctx.m_canvas);
        } else if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            paintOutline(ctx.m_canvas);
        }
    } else if (isBlockLevel()) {
        if (ctx.m_paintingStage == PaintingReplacedBlock) {
            paintBackgroundAndBorders(ctx.m_canvas);
            paintReplaced(ctx.m_canvas);
            paintOutline(ctx.m_canvas);
        }
    } else if (isInlineLevel()) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            paintBackgroundAndBorders(ctx.m_canvas);
            paintReplaced(ctx.m_canvas);
            paintOutline(ctx.m_canvas);
        }
    }
    ctx.m_canvas->restore();
}

Frame* FrameReplaced::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
{
    if (isEstablishesStackingContext()) {
        return nullptr;
    }

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        return nullptr;
    }

    return FrameBox::hitTest(x, y, stage);
}
}
