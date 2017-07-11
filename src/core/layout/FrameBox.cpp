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
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/layout/FrameBox.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/ImageData.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace StarFish {

void FrameBox::paintChildrenWith(PaintingContext& ctx)
{
    Frame* child = firstChild();
    while (child) {
        ctx.m_canvas->save();
        ctx.m_canvas->translate(child->asFrameBox()->x(),
                                child->asFrameBox()->y());
        child->paint(ctx);
        ctx.m_canvas->restore();
        child = child->next();
    }
}

void FrameBox::paintBackground(Canvas* canvas, ComputedStyle* style,
                               LayoutRect imageRect, LayoutRect colorRect,
                               bool isRootElement,
                               bool needsToFillBgColorAtBorderBox)
{
    if (!style->backgroundColor().isTransparent() &&
        style->visibility() == VisibilityValue::VisibleVisibilityValue) {
        canvas->save();
        canvas->setColor(style->backgroundColor());
        if (isRootElement || needsToFillBgColorAtBorderBox) {
            canvas->drawRect(colorRect);
        } else {
            canvas->drawRect(imageRect);
        }
        canvas->restore();
    }

    ImageData* id = style->backgroundImageData();
    if (id && id->width() && id->height()) {
        canvas->save();
        if (isRootElement) {
            canvas->translate(0, 0);
            canvas->clip(
                Unit::Rect(0, 0, colorRect.width(), colorRect.height()));
        } else {
            canvas->translate(imageRect.x(), imageRect.y());
            canvas->clip(
                Unit::Rect(0, 0, imageRect.width(), imageRect.height()));
        }

        float bw = imageRect.width();
        float bh = imageRect.height();

        float w = bw;
        float h = bh;

        float boxR = bw / bh;
        float imgR = id->width() / (float)id->height();
        if (style->bgSizeType() == BackgroundSizeType::Cover) {
            if (boxR < imgR) {
                w = bh * imgR;
            } else {
                h = bw / imgR;
            }
        } else if (style->bgSizeType() == BackgroundSizeType::Contain) {
            if (boxR > imgR) {
                w = bh * imgR;
            } else {
                h = bw / imgR;
            }
        } else if (style->bgSizeType() == BackgroundSizeType::SizeValue) {
            if (style->bgSizeValue().width().isAuto() &&
                style->bgSizeValue().height().isAuto()) {
                w = id->width();
                h = id->height();
            } else if (style->bgSizeValue().width().isAuto() &&
                       !style->bgSizeValue().height().isAuto()) {
                h = style->bgSizeValue().height().specifiedValue(bh);
                w = h * id->width() / id->height();
            } else if (!style->bgSizeValue().width().isAuto() &&
                       style->bgSizeValue().height().isAuto()) {
                w = style->bgSizeValue().width().specifiedValue(bw);
                h = w * id->height() / id->width();
            } else {
                w = style->bgSizeValue().width().specifiedValue(bw);
                h = style->bgSizeValue().height().specifiedValue(bh);
            }
        } else {
            STARFISH_ASSERT(style->bgSizeType() ==
                            BackgroundSizeType::SizeNone);
            STARFISH_ASSERT_NOT_REACHED();
        }

        LayoutUnit x = style->backgroundPositionX().specifiedValue(bw - w);
        LayoutUnit y = style->backgroundPositionY().specifiedValue(bh - h);

        if (isRootElement) {
            x += imageRect.x();
            y += imageRect.y();
            bw = colorRect.width();
            bh = colorRect.height();
        }

        auto repeatX = style->backgroundRepeatX();
        auto repeatY = style->backgroundRepeatY();
        if (repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
            repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
            canvas->drawRepeatImage(id, Unit::Rect(x, y, bw, bh), w, h, true,
                                    true, isRootElement);
        } else if (repeatX == BackgroundRepeatValue::NoRepeatRepeatValue &&
                   repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
            canvas->drawRepeatImage(id, Unit::Rect(x, y, w, bh), w, h, false,
                                    true, isRootElement);
        } else if (repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
                   repeatY == BackgroundRepeatValue::NoRepeatRepeatValue) {
            canvas->drawRepeatImage(id, Unit::Rect(x, y, bw, h), w, h, true,
                                    false, isRootElement);
        } else {
            canvas->drawImage(id, Unit::Rect(x, y, w, h));
        }

        canvas->restore();
    }
}

void FrameBox::computeBorderMarginPadding(LayoutUnit parentContentWidth)
{
    // padding
    if (style()->paddingLeft().isSpecified() && !m_flags.m_isLeftMBPCleared) {
        setPaddingLeft(
            style()->paddingLeft().specifiedValue(parentContentWidth));
    } else {
        setPaddingLeft(0);
    }
    if (style()->paddingTop().isSpecified()) {
        setPaddingTop(style()->paddingTop().specifiedValue(parentContentWidth));
    } else {
        setPaddingTop(0);
    }
    if (style()->paddingRight().isSpecified() && !m_flags.m_isRightMBPCleared) {
        setPaddingRight(
            style()->paddingRight().specifiedValue(parentContentWidth));
    } else {
        setPaddingRight(0);
    }
    if (style()->paddingBottom().isSpecified()) {
        setPaddingBottom(
            style()->paddingBottom().specifiedValue(parentContentWidth));
    } else {
        setPaddingBottom(0);
    }

    // border
    if (style()->hasBorderStyle()) {
        if (style()->borderLeftWidth().isSpecified() &&
            !m_flags.m_isLeftMBPCleared) {
            setBorderLeft(
                style()->borderLeftWidth().specifiedValue(parentContentWidth));
        } else {
            setBorderLeft(0);
        }
        if (style()->borderTopWidth().isSpecified()) {
            setBorderTop(
                style()->borderTopWidth().specifiedValue(parentContentWidth));
        } else {
            setBorderTop(0);
        }
        if (style()->borderRightWidth().isSpecified() &&
            !m_flags.m_isRightMBPCleared) {
            setBorderRight(
                style()->borderRightWidth().specifiedValue(parentContentWidth));
        } else {
            setBorderRight(0);
        }
        if (style()->borderBottomWidth().isSpecified()) {
            setBorderBottom(style()->borderBottomWidth().specifiedValue(
                parentContentWidth));
        } else {
            setBorderBottom(0);
        }
    } else {
        setBorderLeft(0);
        setBorderTop(0);
        setBorderRight(0);
        setBorderBottom(0);
    }

    // margin
    if (style()->marginLeft().isSpecified() && !m_flags.m_isLeftMBPCleared) {
        setMarginLeft(style()->marginLeft().specifiedValue(parentContentWidth));
    } else {
        setMarginLeft(0);
    }
    if (style()->marginTop().isSpecified()) {
        setMarginTop(style()->marginTop().specifiedValue(parentContentWidth));
    } else {
        setMarginTop(0);
    }
    if (style()->marginRight().isSpecified() && !m_flags.m_isRightMBPCleared) {
        setMarginRight(
            style()->marginRight().specifiedValue(parentContentWidth));
    } else {
        setMarginRight(0);
    }
    if (style()->marginBottom().isSpecified()) {
        setMarginBottom(
            style()->marginBottom().specifiedValue(parentContentWidth));
    } else {
        setMarginBottom(0);
    }
}

void FrameBox::computeHorizontalMargin(LayoutUnit parentContentWidth)
{
    Length marginLeft = style()->marginLeft();
    Length marginRight = style()->marginRight();

    LayoutUnit remainedWidth = parentContentWidth - FrameBox::width();
    if (marginLeft.isAuto() && marginRight.isAuto()) {
        if (remainedWidth > 0) {
            setMarginLeft(remainedWidth / 2);
            setMarginRight(remainedWidth / 2);
        }
    } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
        remainedWidth -= FrameBox::marginRight();
        if (remainedWidth > 0) {
            setMarginLeft(remainedWidth);
        }
    } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
        remainedWidth -= FrameBox::marginLeft();
        if (remainedWidth > 0) {
            setMarginRight(remainedWidth);
        }
    }
}

void FrameBox::applyVerticalMarginForAbsoluteBox()
{
    STARFISH_ASSERT(style()->position() == AbsolutePositionValue);
    Length marginTop = style()->marginTop();
    Length marginBottom = style()->marginBottom();

    if (!marginTop.isAuto() && !marginBottom.isAuto()) {
        moveY(FrameBox::marginTop());
    } else if (!marginTop.isAuto() && marginBottom.isAuto()) {
        moveY(FrameBox::marginTop());
    } else if (marginTop.isAuto() && !marginBottom.isAuto()) {
        moveY(-FrameBox::marginBottom());
    }
}

void FrameBox::paintBackgroundAndBorders(Canvas* canvas)
{
    do {
        if (node() && node()->isHTMLHtmlElement()) {
            break;
        }

        if (node() && node()->isHTMLBodyElement()) {
            if (!node()
                     ->window()
                     ->browsingContext()
                     ->hasRootElementBackground()) {
                break;
            }
        }

        LayoutRect bgRect(borderLeft(), borderTop(),
                          m_frameRect.width() - borderWidth(),
                          m_frameRect.height() - borderHeight());

        if (style()->hasBorderStyle() &&
            (style()->borderTopColor() == style()->borderRightColor()) &&
            (style()->borderRightColor() == style()->borderBottomColor()) &&
            (style()->borderBottomColor() == style()->borderLeftColor()) &&
            borderWidth() && borderHeight() && !style()->hasBorderImageData()) {
            paintBackground(canvas, style(), bgRect,
                            LayoutRect(0, 0, width(), height()), false, false);
        } else {
            paintBackground(canvas, style(), bgRect,
                            LayoutRect(0, 0, width(), height()), false, true);
        }

    } while (false);

    canvas->save();

    // draw border-image
    if (style()->hasBorderImageData()) {
        double bWidth =
            style()->surround()->border.top().width().specifiedValue(height());
        double bImgWidth =
            style()->surround()->border.image().widths().top().specifiedValue(
                bWidth);
        double bImgSlice =
            style()->surround()->border.image().slices().top().specifiedValue(
                height());

        size_t imgWidth =
            style()->surround()->border.image().imageData()->width();
        size_t imgHeight =
            style()->surround()->border.image().imageData()->height();

        size_t lSlice =
            style()->surround()->border.image().slices().left().specifiedValue(
                width());
        size_t tSlice =
            style()->surround()->border.image().slices().top().specifiedValue(
                height());
        size_t rSlice =
            style()->surround()->border.image().slices().right().specifiedValue(
                width());
        size_t bSlice = style()
                            ->surround()
                            ->border.image()
                            .slices()
                            .bottom()
                            .specifiedValue(height());

        ImageData* imgData = style()->surround()->border.image().imageData();

        if (bImgSlice > imgWidth || bImgSlice > imgHeight) {
            bImgSlice = std::min(imgWidth, imgHeight);
        }

        double value = std::min((float)width() / (bImgWidth * 2),
                                (float)height() / (bImgWidth * 2));
        if (value < 1) {
            bImgWidth *= value;
        }

        double scale = bImgWidth / bImgSlice;
        bool isFill = false;

        if ((lSlice + rSlice > imgWidth) || (tSlice + bSlice > imgHeight)) {
            float drawRect = std::min((float)width(), (float)height()) / 2.0;

            if (drawRect > bImgWidth) {
                drawRect = bImgWidth;
            }

            // left-top
            canvas->drawBorderImage(imgData,
                                    Unit::Rect(0, 0, drawRect, drawRect),
                                    lSlice, tSlice, 0, 0, scale, isFill);
            // right-top
            canvas->drawBorderImage(
                imgData,
                Unit::Rect((float)width() - drawRect, 0, drawRect, drawRect), 0,
                tSlice, rSlice, 0, scale, isFill);
            // right-bottom
            canvas->drawBorderImage(imgData,
                                    Unit::Rect((float)width() - drawRect,
                                               (float)height() - drawRect,
                                               drawRect, drawRect),
                                    0, 0, rSlice, bSlice, scale, isFill);
            // left-bottom
            canvas->drawBorderImage(
                imgData,
                Unit::Rect(0, (float)height() - drawRect, drawRect, drawRect),
                lSlice, 0, 0, bSlice, scale, isFill);
        } else {
            isFill = style()->surround()->border.image().sliceFill();
            canvas->drawBorderImage(imgData,
                                    Unit::Rect(0, 0, width(), height()), lSlice,
                                    tSlice, rSlice, bSlice, scale, isFill);
        }
    } else if (style()->hasBorderStyle()) {
        // draw border
        // TODO border-join

        if ((style()->borderTopColor() == style()->borderRightColor()) &&
            (style()->borderRightColor() == style()->borderBottomColor()) &&
            (style()->borderBottomColor() == style()->borderLeftColor())) {
            // if 4-colors are same.

            // top
            canvas->setColor(style()->borderTopColor());
            canvas->drawRect(LayoutRect(0, 0, width(), borderTop()));
            // right
            canvas->setColor(style()->borderRightColor());
            canvas->drawRect(LayoutRect(width() - borderRight(), borderTop(),
                                        borderRight(),
                                        height() - borderHeight()));
            // bottom
            canvas->setColor(style()->borderBottomColor());
            canvas->drawRect(LayoutRect(0, height() - borderBottom(), width(),
                                        borderBottom()));
            // left
            canvas->setColor(style()->borderLeftColor());
            canvas->drawRect(LayoutRect(0, borderTop(), borderLeft(),
                                        height() - borderHeight()));
        } else {
            // top
            canvas->setColor(style()->borderTopColor());
            canvas->drawRect(
                LayoutLocation(0, 0), LayoutLocation(width(), 0),
                LayoutLocation(width() - borderRight(), borderTop()),
                LayoutLocation(borderLeft(), borderTop()));

            // right
            canvas->setColor(style()->borderRightColor());
            canvas->drawRect(
                LayoutLocation(width() - borderRight(), borderTop()),
                LayoutLocation(width(), 0), LayoutLocation(width(), height()),
                LayoutLocation(width() - borderRight(),
                               height() - borderBottom()));

            // bottom
            canvas->setColor(style()->borderBottomColor());
            canvas->drawRect(
                LayoutLocation(borderLeft(), height() - borderBottom()),
                LayoutLocation(width() - borderRight(),
                               height() - borderBottom()),
                LayoutLocation(width(), height()), LayoutLocation(0, height()));

            // left
            canvas->setColor(style()->borderLeftColor());
            canvas->drawRect(
                LayoutLocation(0, 0), LayoutLocation(borderLeft(), borderTop()),
                LayoutLocation(borderLeft(), height() - borderBottom()),
                LayoutLocation(0, height()));
        }
    }

    canvas->restore();
}

void FrameBox::paintStackingContextContent(Canvas* canvas)
{
    PaintingContext ctx(canvas);
    // the in-flow, non-inline-level, non-positioned descendants.
    ctx.m_paintingStage = PaintingNormalFlowBlock;
    ctx.m_paintingInlineStage = PaintingInlineLevelElements;
    paintChildrenWith(ctx);

    // the non-positioned float
    ctx.m_paintingStage = PaintingNonPositionedFloats;
    paintChildrenWith(ctx);

    // the in-flow, inline-level, non-positioned descendants, including inline
    // tables and inline blocks.
    ctx.m_paintingStage = PaintingNormalFlowInline;
    paintChildrenWith(ctx);

    // the child stacking contexts with stack level 0 and the positioned
    // descendants with stack level 0.
    ctx.m_paintingStage = PaintingPositionedElements;
    paintChildrenWith(ctx);
}

void FrameBox::establishesStackingContextIfNeeds()
{
    if (isEstablishesStackingContext()) {
        STARFISH_ASSERT(isRootElement() || m_stackingContext == nullptr);
        if (!isRootElement() ||
            (isRootElement() &&
             !node()->document()->browsingContext()->isMainBrowsingContext())) {
            FrameBox* p;
            if (!isRootElement()) {
                p = layoutParent()->asFrameBox();
            } else {
                p = node()
                        ->document()
                        ->browsingContext()
                        ->sourceElement()
                        ->frame()
                        ->layoutParent()
                        ->asFrameBox();
            }
            while (true) {
                if (p->isEstablishesStackingContext()) {
                    if (p->isRootElement()) {
                        break;
                    } else if (p->needsGraphicsBuffer()) {
                        break;
                    } else if (!p->isPositioned()) {
                        break;
                    } else if (p->style()->IsSpecifiedZIndex()) {
                        break;
                    } else if (p->style()->opacity() != 1) {
                        break;
                    } else if (!p->isPositioned()) {
                        break;
                    } else if (p->style()->overflow() !=
                               OverflowValue::VisibleOverflow) {
                        break;
                    }
                }
                p = p->layoutParent()->asFrameBox();
            }
            m_stackingContext = new StackingContext(this, p->stackingContext());
        } else {
            m_stackingContext = new StackingContext(this, nullptr);
        }
    }
}

bool FrameBox::tryUniteVisibleRect(StackingContext* sCtx, LayoutLocation& loc)
{
    if (this != sCtx->owner() && stackingContext() &&
        stackingContext()->needsOwnBuffer()) {
        return false;
    }

    LayoutRect r = frameRect();
    r.setX(r.x() + loc.x());
    r.setY(r.y() + loc.y());
    sCtx->unite(r);

    if (style()->overflow() == OverflowValue::HiddenOverflow) {
        return false;
    }
    return true;
}

void FrameBox::computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc)
{
    tryUniteVisibleRect(sCtx, loc);
}

void FrameBox::clearStackingContextIfNeeds(bool shouldDetachNativeBuffer)
{
    if (m_stackingContext) {
        m_stackingContext->clearOwnBuffer(shouldDetachNativeBuffer);
        m_stackingContext = nullptr;
    }
}

LayoutUnit FrameBox::minMaxWidthAppliedIfNeeds(LayoutUnit width,
                                               LayoutUnit parentWidth)
{
    ComputedStyle* style = Frame::style();
    if (style->minWidth().isSpecified()) {
        LayoutUnit minWidth = style->minWidth().specifiedValue(parentWidth);
        if (minWidth > width) {
            return minWidth;
        }
    }
    if (style->maxWidth().isSpecified()) {
        LayoutUnit maxWidth = style->maxWidth().specifiedValue(parentWidth);
        if (maxWidth < width) {
            return maxWidth;
        }
    }
    return width;
}

LayoutUnit FrameBox::minMaxHeightAppliedIfNeeds(LayoutUnit height,
                                                LayoutUnit parentHeight,
                                                bool parentHasFixedValue)
{
    ComputedStyle* style = Frame::style();
    if (style->minHeight().isSpecified()) {
        if (!parentHasFixedValue && style->minHeight().isPercent()) {
            return height;
        }
        LayoutUnit minHeight = style->minHeight().specifiedValue(parentHeight);
        if (minHeight > height) {
            return minHeight;
        }
    }
    if (style->maxHeight().isSpecified()) {
        if (!parentHasFixedValue && style->maxHeight().isPercent()) {
            return height;
        }
        LayoutUnit maxHeight = style->maxHeight().specifiedValue(parentHeight);
        if (maxHeight < height) {
            return maxHeight;
        }
    }
    return height;
}
}
