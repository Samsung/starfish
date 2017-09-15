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
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/ImageData.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace StarFish {

void* FrameBoxRareData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameBoxRareData)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBoxRareData, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBoxRareData, m_stackingContext));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameBoxRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

LayoutLocation FrameBox::absolutePointIncludingScroll(FrameBox* top)
{
    LayoutLocation l(0, 0);
    Frame* p = this;
    while (top != p) {
        l.setX(l.x() + p->asFrameBox()->x());
        l.setY(l.y() + p->asFrameBox()->y());
        if (p->isFrameBlockBox() && p != this) {
            l.setX(l.x() - p->asFrameBlockBox()->scrollLeft());
            l.setY(l.y() - p->asFrameBlockBox()->scrollTop());
        }
        p = p->layoutParent();
    }
    return l;
}

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
                h = style->bgSizeValue().height().specifiedValue(
                    bh, canvas->viewportHeight());
                w = h * id->width() / id->height();
            } else if (!style->bgSizeValue().width().isAuto() &&
                       style->bgSizeValue().height().isAuto()) {
                w = style->bgSizeValue().width().specifiedValue(
                    bw, canvas->viewportWidth());
                h = w * id->height() / id->width();
            } else {
                w = style->bgSizeValue().width().specifiedValue(
                    bw, canvas->viewportWidth());
                h = style->bgSizeValue().height().specifiedValue(
                    bh, canvas->viewportHeight());
            }
        } else {
            STARFISH_ASSERT(style->bgSizeType() ==
                            BackgroundSizeType::SizeNone);
            STARFISH_ASSERT_NOT_REACHED();
        }

        LayoutUnit x = style->backgroundPositionX().specifiedValue(
            bw - w, canvas->viewportWidth());
        LayoutUnit y = style->backgroundPositionY().specifiedValue(
            bh - h, canvas->viewportHeight());

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

void FrameBox::computeBorderMarginPadding(LayoutContext& ctx,
                                          LayoutUnit parentContentWidth)
{
    LayoutUnit viewportWidth = ctx.viewportWidth();
    // padding
    if (style()->paddingLeft().isSpecified() && !m_flags.m_isLeftMBPCleared) {
        setPaddingLeft(style()->paddingLeft().specifiedValue(parentContentWidth,
                                                             viewportWidth));
    } else {
        setPaddingLeft(0);
    }
    if (style()->paddingTop().isSpecified()) {
        setPaddingTop(style()->paddingTop().specifiedValue(parentContentWidth,
                                                           viewportWidth));
    } else {
        setPaddingTop(0);
    }
    if (style()->paddingRight().isSpecified() && !m_flags.m_isRightMBPCleared) {
        setPaddingRight(style()->paddingRight().specifiedValue(
            parentContentWidth, viewportWidth));
    } else {
        setPaddingRight(0);
    }
    if (style()->paddingBottom().isSpecified()) {
        setPaddingBottom(style()->paddingBottom().specifiedValue(
            parentContentWidth, viewportWidth));
    } else {
        setPaddingBottom(0);
    }

    // border
    if (style()->hasBorderStyle()) {
        if (style()->borderLeftWidth().isSpecified() &&
            !m_flags.m_isLeftMBPCleared) {
            setBorderLeft(style()->borderLeftWidth().specifiedValue(
                parentContentWidth, viewportWidth));
        } else {
            setBorderLeft(0);
        }
        if (style()->borderTopWidth().isSpecified()) {
            setBorderTop(style()->borderTopWidth().specifiedValue(
                parentContentWidth, viewportWidth));
        } else {
            setBorderTop(0);
        }
        if (style()->borderRightWidth().isSpecified() &&
            !m_flags.m_isRightMBPCleared) {
            setBorderRight(style()->borderRightWidth().specifiedValue(
                parentContentWidth, viewportWidth));
        } else {
            setBorderRight(0);
        }
        if (style()->borderBottomWidth().isSpecified()) {
            setBorderBottom(style()->borderBottomWidth().specifiedValue(
                parentContentWidth, viewportWidth));
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
        setMarginLeft(style()->marginLeft().specifiedValue(parentContentWidth,
                                                           viewportWidth));
    } else {
        setMarginLeft(0);
    }
    if (style()->marginTop().isSpecified()) {
        setMarginTop(style()->marginTop().specifiedValue(parentContentWidth,
                                                         viewportWidth));
    } else {
        setMarginTop(0);
    }
    if (style()->marginRight().isSpecified() && !m_flags.m_isRightMBPCleared) {
        setMarginRight(style()->marginRight().specifiedValue(parentContentWidth,
                                                             viewportWidth));
    } else {
        setMarginRight(0);
    }
    if (style()->marginBottom().isSpecified()) {
        setMarginBottom(style()->marginBottom().specifiedValue(
            parentContentWidth, viewportWidth));
    } else {
        setMarginBottom(0);
    }
}

HorizontalDataLocToContainingBlock
FrameBox::computeHorizontalDataToContainingBlock(LayoutContext& ctx,
                                                 FrameBox* cb)
{
    STARFISH_ASSERT(cb);
    DirectionValue parentDirection = blockContainer(this)->style()->direction();

    FrameBox* parent = layoutParent()->asFrameBox();

    LayoutLocation l1, l2;
    if (cb->isAncestorOf(parent)) {
        l2 = parent->absolutePoint(cb);
    } else {
        l1 = cb->absolutePoint(ctx.frameDocument());
        l2 = parent->absolutePoint(ctx.frameDocument());
    }
    LayoutUnit absX = l2.x() - l1.x() - cb->borderLeft();

    LayoutUnit containgBlockContentWidth =
        cb->contentWidth() + cb->paddingWidth();
    LayoutUnit viewportWidth = ctx.viewportWidth();

    LayoutUnit l, r;
    Length left = style()->left();
    Length right = style()->right();
    if (left.isSpecified()) {
        l = left.specifiedValue(containgBlockContentWidth, viewportWidth);
    }

    if (right.isSpecified()) {
        r = right.specifiedValue(containgBlockContentWidth, viewportWidth);
    }

    return HorizontalDataLocToContainingBlock(containgBlockContentWidth, absX,
                                              l, r);
}

VerticalDataLocToContainingBlock FrameBox::computeVerticalDataToContainingBlock(
    LayoutContext& ctx, FrameBox* cb)
{
    STARFISH_ASSERT(cb);
    FrameBox* parent = layoutParent()->asFrameBox();
    LayoutLocation l1, l2;
    if (cb->isAncestorOf(parent)) {
        l2 = parent->absolutePoint(cb);
    } else {
        l1 = cb->absolutePoint(ctx.frameDocument());
        l2 = parent->absolutePoint(ctx.frameDocument());
    }
    LayoutUnit containgBlockContentHeight =
        cb->contentHeight() + cb->paddingHeight();
    LayoutUnit viewportHeight = ctx.viewportHeight();

    LayoutUnit absY = l2.y() - l1.y() - cb->borderTop();

    LayoutUnit t, b;
    Length top = style()->top();
    Length bottom = style()->bottom();
    if (top.isSpecified()) {
        t = top.specifiedValue(containgBlockContentHeight, viewportHeight);
    }

    if (bottom.isSpecified()) {
        b = bottom.specifiedValue(containgBlockContentHeight, viewportHeight);
    }

    return VerticalDataLocToContainingBlock(containgBlockContentHeight, absY, t,
                                            b);
}

void FrameBox::computeHorizontalMargin(LayoutUnit parentContentWidth,
                                       DirectionValue parentDirection)
{
    Length marginLeft = style()->marginLeft();
    Length marginRight = style()->marginRight();
    LayoutUnit remainingWidth = parentContentWidth - width();

    if (marginLeft.isAuto() && marginRight.isAuto()) {
        if (remainingWidth > 0) {
            setMarginLeft(remainingWidth / 2);
            setMarginRight(remainingWidth / 2);
        } else if (isAbsolutePositioned()) {
            if (parentDirection == LtrDirectionValue) {
                setMarginRight(remainingWidth);
            } else {
                setMarginLeft(remainingWidth);
            }
        }
    } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
        remainingWidth -= FrameBox::marginRight();
        if (isAbsolutePositioned() || remainingWidth > 0) {
            setMarginLeft(remainingWidth);
        }
    } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
        remainingWidth -= FrameBox::marginLeft();
        if (isAbsolutePositioned() || remainingWidth > 0) {
            setMarginRight(remainingWidth);
        }
    }
}

void FrameBox::computeVerticalMargin(LayoutUnit parentContentHeight)
{
    STARFISH_ASSERT(isAbsolutePositioned());
    Length marginTop = style()->marginTop();
    Length marginBottom = style()->marginBottom();
    LayoutUnit remainingHeight = parentContentHeight - height();

    if (marginTop.isAuto() && marginBottom.isAuto()) {
        if (isAbsolutePositioned() || remainingHeight > 0) {
            setMarginTop(remainingHeight / 2);
            setMarginBottom(remainingHeight / 2);
        }
    } else if (marginTop.isAuto() && !marginBottom.isAuto()) {
        remainingHeight -= FrameBox::marginBottom();
        if (isAbsolutePositioned() || remainingHeight > 0) {
            setMarginTop(remainingHeight);
        }
    } else if (!marginTop.isAuto() && marginBottom.isAuto()) {
        remainingHeight -= FrameBox::marginTop();
        if (isAbsolutePositioned() || remainingHeight > 0) {
            setMarginBottom(remainingHeight);
        }
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
            style()->surround()->border.top().width().specifiedValue(
                height(), canvas->viewportHeight());
        double bImgWidth =
            style()->surround()->border.image().widths().top().specifiedValue(
                bWidth, canvas->viewportHeight());
        double bImgSlice =
            style()->surround()->border.image().slices().top().specifiedValue(
                height(), canvas->viewportHeight());

        size_t imgWidth =
            style()->surround()->border.image().imageData()->width();
        size_t imgHeight =
            style()->surround()->border.image().imageData()->height();

        size_t lSlice =
            style()->surround()->border.image().slices().left().specifiedValue(
                width(), canvas->viewportWidth());
        size_t tSlice =
            style()->surround()->border.image().slices().top().specifiedValue(
                height(), canvas->viewportHeight());
        size_t rSlice =
            style()->surround()->border.image().slices().right().specifiedValue(
                width(), canvas->viewportWidth());
        size_t bSlice = style()
                            ->surround()
                            ->border.image()
                            .slices()
                            .bottom()
                            .specifiedValue(height(), canvas->viewportHeight());

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

        if (style()->isFourSideBorderStyleValueSolid() &&
            (style()->borderTopColor() == style()->borderRightColor()) &&
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
            if (style()->borderTopStyle() ==
                BorderStyleValue::InsetBorderStyleValue) {
                canvas->setColor(style()->borderTopColor().getDarkerColor());
            } else {
                canvas->setColor(style()->borderTopColor());
            }
            canvas->drawRect(
                LayoutLocation(0, 0), LayoutLocation(width(), 0),
                LayoutLocation(width() - borderRight(), borderTop()),
                LayoutLocation(borderLeft(), borderTop()));

            // right
            if ((style()->borderRightStyle() ==
                 BorderStyleValue::InsetBorderStyleValue) &&
                (style()->borderRightColor().r() == 0 &&
                 style()->borderRightColor().g() == 0 &&
                 style()->borderRightColor().b() == 0)) {
                canvas->setColor(Unit::Color(238, 238, 238,
                                             style()->borderRightColor().a()));
            } else {
                canvas->setColor(style()->borderRightColor());
            }
            canvas->drawRect(
                LayoutLocation(width() - borderRight(), borderTop()),
                LayoutLocation(width(), 0), LayoutLocation(width(), height()),
                LayoutLocation(width() - borderRight(),
                               height() - borderBottom()));

            // bottom
            if ((style()->borderBottomStyle() ==
                 BorderStyleValue::InsetBorderStyleValue) &&
                (style()->borderBottomColor().r() == 0 &&
                 style()->borderBottomColor().g() == 0 &&
                 style()->borderBottomColor().b() == 0)) {
                canvas->setColor(Unit::Color(238, 238, 238,
                                             style()->borderBottomColor().a()));
            } else {
                canvas->setColor(style()->borderBottomColor());
            }
            canvas->drawRect(
                LayoutLocation(borderLeft(), height() - borderBottom()),
                LayoutLocation(width() - borderRight(),
                               height() - borderBottom()),
                LayoutLocation(width(), height()), LayoutLocation(0, height()));

            // left
            if (style()->borderLeftStyle() ==
                BorderStyleValue::InsetBorderStyleValue) {
                canvas->setColor(style()->borderLeftColor().getDarkerColor());
            } else {
                canvas->setColor(style()->borderLeftColor());
            }
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
    ctx.m_paintingInlineStage = PaintingInlineBox;
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
        STARFISH_ASSERT(isRootElement() || stackingContext() == nullptr);
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
                    } else if (p->style()->position() == FixedPositionValue) {
                        break;
                    } else if ((p->isPositioned() || p->isFlexItem()) &&
                               p->style()->IsSpecifiedZIndex()) {
                        break;
                    } else if (p->style()->opacity() != 1) {
                        break;
                    } else if (p->style()->overflowX() !=
                                   OverflowValue::VisibleOverflow ||
                               p->style()->overflowY() !=
                                   OverflowValue::VisibleOverflow) {
                        break;
                    }
                }
                p = p->layoutParent()->asFrameBox();
            }
            ensureFrameBoxRareData()->m_stackingContext =
                new StackingContext(this, p->stackingContext());
        } else {
            ensureFrameBoxRareData()->m_stackingContext =
                new StackingContext(this, nullptr);
        }
    }
}

bool FrameBox::tryUniteVisibleRect(StackingContext* sCtx, LayoutLocation& loc,
                                   LayoutRect& result)
{
    if (sCtx && (this != sCtx->owner() && stackingContext() &&
                 stackingContext()->needsOwnBuffer())) {
        return false;
    }

    LayoutRect r = frameRect();
    r.setX(r.x() + loc.x());
    r.setY(r.y() + loc.y());
    result.unite(r);

    return !shouldApplyOverflow();
}

void FrameBox::computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc,
                                  LayoutRect& result)
{
    tryUniteVisibleRect(sCtx, loc, result);
}

void FrameBox::clearStackingContextIfNeeds(bool shouldDetachNativeBuffer)
{
    if (stackingContext()) {
        stackingContext()->clearOwnBuffer(shouldDetachNativeBuffer);
        frameBoxRareData()->m_stackingContext = nullptr;
    }
}

LayoutUnit FrameBox::minMaxWidthAppliedIfNeeds(
    LayoutContext& ctx, LayoutUnit width, LayoutUnit parentWidth,
    LayoutUnit viewportWidth, bool underComputingPreferredWidth)
{
    ComputedStyle* style = Frame::style();
    if (style->minWidth().isSpecified()) {
        if (!(underComputingPreferredWidth && style->minWidth().isPercent())) {
            LayoutUnit minWidth =
                style->minWidth().specifiedValue(parentWidth, viewportWidth);

            minWidth = contentWidthApplyingBoxSizing(minWidth);

            if (minWidth > width) {
                return minWidth;
            }
        }
    } else if (isFlexItem()) {
        LayoutUnit minWidth = intMaxForLayoutUnit;
        if (style->width().isSpecified()) {
            if (!(underComputingPreferredWidth && style->width().isPercent())) {
                LayoutUnit width =
                    style->width().specifiedValue(parentWidth, viewportWidth);

                width = contentWidthApplyingBoxSizing(width);

                minWidth = width;
            }
        }

        if (!underComputingPreferredWidth &&
            layoutParent()->asFrameFlexibleBox()->isMainAxisInInlineAxis() &&
            appliedOverflowX() == VisibleOverflow) {
            PreferredWidthContext p(ctx, this, parentWidth - mbpWidth());
            p.computePreferredWidth();
            minWidth = std::min(minWidth, p.preferredMinWidth());
        }

        if (minWidth != intMaxForLayoutUnit && minWidth > width) {
            return minWidth;
        }
    }
    if (style->maxWidth().isSpecified()) {
        if (!(underComputingPreferredWidth && style->maxWidth().isPercent())) {
            LayoutUnit maxWidth =
                style->maxWidth().specifiedValue(parentWidth, viewportWidth);

            maxWidth = contentWidthApplyingBoxSizing(maxWidth);

            if (maxWidth >= 0 && maxWidth < width) {
                return maxWidth;
            }
        }
    }
    return width;
}

LayoutUnit FrameBox::minMaxHeightAppliedIfNeeds(LayoutUnit height,
                                                LayoutUnit parentHeight,
                                                LayoutUnit viewportHeight,
                                                bool parentHasFixedValue)
{
    ComputedStyle* style = Frame::style();
    if (style->minHeight().isSpecified()) {
        if (!parentHasFixedValue && style->minHeight().isPercent()) {
            return height;
        }

        LayoutUnit minHeight =
            style->minHeight().specifiedValue(parentHeight, viewportHeight);

        minHeight = contentHeightApplyingBoxSizing(minHeight);

        if (minHeight > height) {
            return minHeight;
        }
    } else if (isFlexItem()) {
        LayoutUnit minHeight;
        if (!layoutParent()->asFrameFlexibleBox()->isMainAxisInInlineAxis() &&
            appliedOverflowY() == VisibleOverflow) {
            if (!(style->height().isAuto() ||
                  (style->height().isPercent() && !parentHasFixedValue))) {
                // TODO: should compare `min-content` size of flex-item, too.
                minHeight =
                    std::min(height, LayoutUnit(style->height().specifiedValue(
                                         parentHeight, viewportHeight)));
            }
        }

        if (minHeight > height) {
            return minHeight;
        }
    }

    if (style->maxHeight().isSpecified()) {
        if (!parentHasFixedValue && style->maxHeight().isPercent()) {
            return height;
        }

        LayoutUnit maxHeight =
            style->maxHeight().specifiedValue(parentHeight, viewportHeight);

        maxHeight = contentHeightApplyingBoxSizing(maxHeight);

        if (maxHeight >= 0 && maxHeight < height) {
            return maxHeight;
        }
    }
    return height;
}
}
