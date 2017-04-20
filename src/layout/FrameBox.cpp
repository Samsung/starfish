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

#include "StarFish.h"
#include "dom/Document.h"
#include "layout/FrameBox.h"
#include "layout/StackingContext.h"
#include "platform/window/Window.h"

namespace StarFish {

void FrameBox::paintBackgroundAndBorders(Canvas* canvas)
{
    do {
        if (node() && node()->isHTMLHtmlElement()) {
            break;
        }

        if (node() && node()->isHTMLBodyElement()) {
            if (!node()->document()->window()->hasRootElementBackground()) {
                break;
            }
        }

        LayoutRect bgRect(borderLeft(), borderTop(),
                          m_frameRect.width() - borderWidth(),
                          m_frameRect.height() - borderHeight());
        paintBackground(canvas, style(), bgRect,
                        LayoutRect(0, 0, width(), height()), false);

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
            canvas->drawRect(LayoutRect(width() - borderRight(), 0,
                                        borderRight(), height()));
            // bottom
            canvas->setColor(style()->borderBottomColor());
            canvas->drawRect(LayoutRect(0, height() - borderBottom(), width(),
                                        borderBottom()));
            // left
            canvas->setColor(style()->borderLeftColor());
            canvas->drawRect(LayoutRect(0, 0, borderLeft(), height()));
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
}
