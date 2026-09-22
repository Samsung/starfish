/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/style/FilterFunctions.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/layout/ComputeOverflow.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/CanvasShadowData.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/modules/canvas/Compositor.h"
#include "core/layout/PaintPassMemo.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/GradientData.h"
#include "platform/loader/ResourceLoader.h"

namespace Starfish {

static void updateBorderRadiusByFactor(BorderRadiusFixedData& v, float factor)
{
    v.m_topLeftVertical *= factor;
    v.m_topLeftHorizontal *= factor;
    v.m_topRightHorizontal *= factor;
    v.m_topRightVertical *= factor;
    v.m_bottomLeftVertical *= factor;
    v.m_bottomLeftHorizontal *= factor;
    v.m_bottomRightHorizontal *= factor;
    v.m_bottomRightVertical *= factor;
}

static void reduceBorderRadiusToFit(const LayoutRect& rect,
                                    BorderRadiusFixedData& v)
{
    const float width = rect.width();
    const float height = rect.height();

    const float top = v.m_topLeftHorizontal + v.m_topRightHorizontal;
    if (top > width) {
        updateBorderRadiusByFactor(v, width / top);
    }
    const float right = v.m_topRightVertical + v.m_bottomRightVertical;
    if (right > height) {
        updateBorderRadiusByFactor(v, height / right);
    }
    const float bottom = v.m_bottomRightHorizontal + v.m_bottomLeftHorizontal;
    if (bottom > width) {
        updateBorderRadiusByFactor(v, width / bottom);
    }
    const float left = v.m_bottomLeftVertical + v.m_topLeftVertical;
    if (left > height) {
        updateBorderRadiusByFactor(v, height / left);
    }
}

void* FlexItemMeasureMemo::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FlexItemMeasureMemo));
    // Plain data, no GC pointers inside.
    return GC_MALLOC_ATOMIC(size);
}

void* FrameBoxRareData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameBoxRareData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameBoxRareData)] = { 0 };
        FrameBoxRareData::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameBoxRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* FrameBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameBox)] = { 0 };
        FrameBox::fillGCDescriptor(obj_bitmap);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

LayoutLocation FrameBox::absolutePointIncludingScroll(
    FrameBox* top, bool includesScrollPropertyOfTop)
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

    if (top->isFrameBlockBox() && includesScrollPropertyOfTop) {
        l.setX(l.x() - top->asFrameBlockBox()->scrollLeft());
        l.setY(l.y() - top->asFrameBlockBox()->scrollTop());
    }

    return l;
}

void FrameBox::computeBorderMarginPadding(LayoutContext& ctx,
                                          LayoutUnit parentContentWidth)
{
    LayoutUnit oldPaddingWidth = paddingWidth();
    LayoutUnit oldPaddingHeight = paddingHeight();

    Node* node = nearstNotAnonymousNode();
    auto mbp = style()->marginBorderPadding();
    // padding
    auto padding = std::get<2>(mbp);
    if (padding) {
        if (padding->left().isSpecified() && !m_flags.m_isLeftMBPCleared) {
            setPaddingLeft(
                padding->left().specifiedValue(parentContentWidth, node));
        } else {
            setPaddingLeft(0);
        }
        if (padding->top().isSpecified()) {
            setPaddingTop(
                padding->top().specifiedValue(parentContentWidth, node));
        } else {
            setPaddingTop(0);
        }
        if (padding->right().isSpecified() && !m_flags.m_isRightMBPCleared) {
            setPaddingRight(
                padding->right().specifiedValue(parentContentWidth, node));
        } else {
            setPaddingRight(0);
        }
        if (padding->bottom().isSpecified()) {
            setPaddingBottom(
                padding->bottom().specifiedValue(parentContentWidth, node));
        } else {
            setPaddingBottom(0);
        }
    } else {
        if (hasRareData()) {
            auto& p = frameBoxRareData()->m_padding;
            p.setTop(0);
            p.setRight(0);
            p.setBottom(0);
            p.setLeft(0);
        }
    }

    if (oldPaddingWidth != paddingWidth()) {
        markPaddingWidthDamaged();
    } else {
        clearPaddingWidthDamaged();
    }

    if (oldPaddingHeight != paddingHeight()) {
        markPaddingHeightDamaged();
    } else {
        clearPaddingHeightDamaged();
    }

    // border
    auto border = std::get<1>(mbp);
    if (border && border->hasBorderStyle()) {
        if (border->left().width().isSpecified() &&
            !m_flags.m_isLeftMBPCleared) {
            setBorderLeft(border->left().width().specifiedValue(
                parentContentWidth, node));
        } else {
            setBorderLeft(0);
        }
        if (border->top().width().isSpecified()) {
            setBorderTop(
                border->top().width().specifiedValue(parentContentWidth, node));
        } else {
            setBorderTop(0);
        }
        if (border->right().width().isSpecified() &&
            !m_flags.m_isRightMBPCleared) {
            setBorderRight(border->right().width().specifiedValue(
                parentContentWidth, node));
        } else {
            setBorderRight(0);
        }
        if (border->bottom().width().isSpecified()) {
            setBorderBottom(border->bottom().width().specifiedValue(
                parentContentWidth, node));
        } else {
            setBorderBottom(0);
        }
    } else {
        if (hasRareData()) {
            auto& p = frameBoxRareData()->m_border;
            p.setTop(0);
            p.setRight(0);
            p.setBottom(0);
            p.setLeft(0);
        }
    }

    // margin
    auto margin = std::get<0>(mbp);
    if (margin) {
        if (margin->left().isSpecified() && !m_flags.m_isLeftMBPCleared) {
            setMarginLeft(
                margin->left().specifiedValue(parentContentWidth, node));
        } else {
            setMarginLeft(0);
        }
        if (margin->top().isSpecified()) {
            setMarginTop(
                margin->top().specifiedValue(parentContentWidth, node));
        } else {
            setMarginTop(0);
        }
        if (margin->right().isSpecified() && !m_flags.m_isRightMBPCleared) {
            setMarginRight(
                margin->right().specifiedValue(parentContentWidth, node));
        } else {
            setMarginRight(0);
        }
        if (margin->bottom().isSpecified()) {
            setMarginBottom(
                margin->bottom().specifiedValue(parentContentWidth, node));
        } else {
            setMarginBottom(0);
        }
    } else {
        if (hasRareData()) {
            auto& p = frameBoxRareData()->m_margin;
            p.setTop(0);
            p.setRight(0);
            p.setBottom(0);
            p.setLeft(0);
        }
    }
}

HorizontalInfoForAbsoluteBlockBox
FrameBox::calHorizontalInfoRelativeToContainingBlock(LayoutContext& ctx,
                                                     FrameBox* cb)
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
    LayoutUnit absX = l2.x() - l1.x() - cb->borderLeft();

    LayoutUnit containgBlockContentWidth =
        cb->contentWidth() + cb->paddingWidth();

    LayoutUnit l, r;
    LengthData offset = style()->offset();
    Length left = offset.left();
    Length right = offset.right();
    if (left.isSpecified()) {
        l = left.specifiedValue(containgBlockContentWidth, this);
    }

    if (right.isSpecified()) {
        r = right.specifiedValue(containgBlockContentWidth, this);
    }

    return HorizontalInfoForAbsoluteBlockBox(containgBlockContentWidth, absX, l,
                                             r);
}

VerticalInfoForAbsoluteBlockBox
FrameBox::calVerticalInfoRelativeToContainingBlock(LayoutContext& ctx,
                                                   FrameBox* cb)
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

    LayoutUnit absY = l2.y() - l1.y() - cb->borderTop();

    LayoutUnit t, b;
    LengthData offset = style()->offset();
    Length top = offset.top();
    Length bottom = offset.bottom();
    if (top.isSpecified()) {
        t = top.specifiedValue(containgBlockContentHeight, this);
    }

    if (bottom.isSpecified()) {
        b = bottom.specifiedValue(containgBlockContentHeight, this);
    }

    return VerticalInfoForAbsoluteBlockBox(containgBlockContentHeight, absY, t,
                                           b);
}

void FrameBox::moveToStaticPositionForAbsolutedPositionedBoxHorizontally(
    FrameBox* flexItem)
{
    FrameFlexibleBox* flexibleBox = parent()->parent()->asFrameFlexibleBox();
    LayoutUnit offset;
    if (flexibleBox->isMainAxisInInlineAxis()) {
        JustifyContentValue justifyContent =
            flexibleBox->style()->justifyContent();
        switch (justifyContent) {
        case JustifyContentValue::NormalJustifyContentValue:
        case JustifyContentValue::StartJustifyContentValue:
        case JustifyContentValue::FlexStartJustifyContentValue:
        case JustifyContentValue::SpaceBetweenJustifyContentValue:
        case JustifyContentValue::StretchJustifyContentValue:
            break;
        case JustifyContentValue::CenterJustifyContentValue:
        case JustifyContentValue::SpaceAroundJustifyContentValue:
            offset = (flexibleBox->contentWidth() - outerWidth()) / 2;
            break;
        case JustifyContentValue::EndJustifyContentValue:
        case JustifyContentValue::FlexEndJustifyContentValue:
            offset = flexibleBox->contentWidth() - outerWidth();
            break;
        }
    } else {
        AlignItemValue alignSelf = style()->alignSelf();
        switch (alignSelf) {
        case AlignItemValue::FlexStartAlignItemValue:
        case AlignItemValue::BaselineAlignItemValue:
        case AlignItemValue::StretchAlignItemValue:
            break;
        case AlignItemValue::CenterAlignItemValue:
            offset = (flexibleBox->contentWidth() - outerWidth()) / 2;
            break;
        case AlignItemValue::FlexEndAlignItemValue:
            offset = flexibleBox->contentWidth() - outerWidth();
            break;
        default:
            STARFISH_UNSUPPORTED("css property (flex): align-items %d",
                                 alignSelf);
            break;
        }
    }

    if (flexibleBox->isLtrDirection()) {
        setX(offset + flexibleBox->borderLeft() + flexibleBox->paddingLeft() +
             FrameBox::marginLeft());
    } else {
        setX(flexibleBox->contentWidth() - offset + flexibleBox->borderLeft() +
             flexibleBox->paddingLeft() - FrameBox::width() -
             FrameBox::marginRight());
    }
}

void FrameBox::moveToStaticPositionForAbsolutedPositionedBoxVertically(
    FrameBox* box)
{
    FrameFlexibleBox* flexibleBox = parent()->parent()->asFrameFlexibleBox();
    LayoutUnit offset;
    if (flexibleBox->isMainAxisInInlineAxis()) {
        AlignItemValue alignSelf = style()->alignSelf();
        switch (alignSelf) {
        case AlignItemValue::FlexStartAlignItemValue:
        case AlignItemValue::BaselineAlignItemValue:
        case AlignItemValue::StretchAlignItemValue:
            break;
        case AlignItemValue::CenterAlignItemValue:
            offset = (flexibleBox->contentHeight() - outerHeight()) / 2;
            break;
        case AlignItemValue::FlexEndAlignItemValue:
            offset = flexibleBox->contentHeight() - outerHeight();
            break;
        default:
            STARFISH_UNSUPPORTED("css property (flex): align-items %d",
                                 alignSelf);
            break;
        }
    } else {
        JustifyContentValue justifyContent =
            flexibleBox->style()->justifyContent();
        switch (justifyContent) {
        case JustifyContentValue::NormalJustifyContentValue:
        case JustifyContentValue::StartJustifyContentValue:
        case JustifyContentValue::FlexStartJustifyContentValue:
        case JustifyContentValue::SpaceBetweenJustifyContentValue:
        case JustifyContentValue::StretchJustifyContentValue:
            break;
        case JustifyContentValue::CenterJustifyContentValue:
        case JustifyContentValue::SpaceAroundJustifyContentValue:
            offset = (flexibleBox->contentHeight() - outerHeight()) / 2;
            break;
        case JustifyContentValue::EndJustifyContentValue:
        case JustifyContentValue::FlexEndJustifyContentValue:
            offset = flexibleBox->contentHeight() - outerHeight();
            break;
        }
    }

    if (flexibleBox->isTtbDirection()) {
        setY(offset + flexibleBox->borderTop() + flexibleBox->paddingTop() +
             FrameBox::marginTop());
    } else {
        setY(flexibleBox->contentHeight() - offset + flexibleBox->borderTop() +
             flexibleBox->paddingTop() - FrameBox::height() -
             FrameBox::marginBottom());
    }
}

void FrameBox::computeHorizontalMargin(LayoutUnit parentContentWidth,
                                       DirectionValue parentDirection)
{
    LengthData margin = style()->margin();
    Length marginLeft = margin.left();
    Length marginRight = margin.right();
    LayoutUnit remainingWidth = parentContentWidth - width();

    if (remainingWidth > 0 && !isAbsolutePositioned() &&
        (style()->orignalTextAlign() ==
         TextAlignValue::WebKitCenterTextAlignValue)) {
        LayoutUnit dX;
        dX += FrameBox::marginLeft();
        dX -= FrameBox::marginRight();
        dX /= 2;
        setMarginLeft(remainingWidth / 2 + dX);
        setMarginRight(remainingWidth / 2 + dX);
        return;
    }

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
    LengthData margin = style()->margin();
    Length marginTop = margin.top();
    Length marginBottom = margin.bottom();
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

void FrameBox::paintOutline(Canvas* canvas)
{
    auto s = style()->outlineStyle();
    if (s != BorderStyleValue::NoneBorderStyleValue) {
        canvas->save();
        canvas->resetClip();
        auto u = absolutePointIncludingScroll(
            node()->document()->frame()->asFrameBox(), false);
        u.setX(u.x() -
               node()->document()->frame()->asFrameBlockBox()->scrollLeft());
        u.setY(u.y() -
               node()->document()->frame()->asFrameBlockBox()->scrollTop());
        canvas->clip(Unit::Rect(-u.x(), -u.y(), node()->window()->innerWidth(),
                                node()->window()->innerHeight()));

        LayoutUnit cbContentWidth = containingBlock(this)->contentWidth();
        LayoutUnit outlineWidth =
            style()->outlineWidth().specifiedValue(cbContentWidth, this);
        LayoutUnit outlineOffset =
            style()->outlineOffset().specifiedValue(cbContentWidth, this);
        LayoutUnit offset = outlineWidth + outlineOffset;

        LayoutRect rect = frameRect();

        rect.setX(-offset);
        rect.setY(-offset);

        rect.setWidth(rect.width() + offset * 2);
        rect.setHeight(rect.height() + offset * 2);

        if (s != BorderStyleValue::SolidBorderStyleValue) {
            STARFISH_UNSUPPORTED("css property: border (solid only)");
        }

        canvas->setFillColor(style()->outlineColor());

        if (hasFrameBorderRadius()) {
            canvas->beginPath();

            applyBorderRadius(canvas,
                              LayoutRect(rect.x().floor(), rect.y().floor(),
                                         (rect.maxX() + outlineWidth).floor(),
                                         (rect.maxY() + outlineWidth).floor()));
            applyBorderRadius(canvas,
                              LayoutRect((rect.x() + outlineWidth).floor(),
                                         (rect.y() + outlineWidth).floor(),
                                         (rect.maxX() - outlineWidth).floor(),
                                         (rect.maxY() - outlineWidth).floor()));

            canvas->setFillRule(false);
            canvas->fill();
            canvas->restore();
        } else {
            canvas->beginPath();

            canvas->moveTo(rect.x().floor(), rect.y().floor());
            canvas->lineTo(rect.maxX().floor(), rect.y().floor());
            canvas->lineTo(rect.maxX().floor(), rect.maxY().floor());
            canvas->lineTo(rect.x().floor(), rect.maxY().floor());
            canvas->lineTo(rect.x().floor(), rect.y().floor());

            canvas->lineTo((rect.x() + outlineWidth).floor(),
                           (rect.y() + outlineWidth).floor());
            canvas->lineTo((rect.x() + outlineWidth).floor(),
                           (rect.maxY() - outlineWidth).floor());
            canvas->lineTo((rect.maxX() - outlineWidth).floor(),
                           (rect.maxY() - outlineWidth).floor());
            canvas->lineTo((rect.maxX() - outlineWidth).floor(),
                           (rect.y() + outlineWidth).floor());
            canvas->lineTo((rect.x() + outlineWidth).floor(),
                           (rect.y() + outlineWidth).floor());
            canvas->lineTo(rect.x().floor(), rect.y().floor());
            canvas->fill();

            canvas->restore();
        }
    }
}

void FrameBox::computeBorderRadiusProperties(
    const BorderRadiusData& br, const LayoutRect& rect,
    float& topLeftHorizontal, float& topRightHorizontal, float& topLeftVertical,
    float& bottomLeftVertical, float& topRightVertical,
    float& bottomRightVertical, float& bottomLeftHorizontal,
    float& bottomRightHorizontal)
{
    topLeftHorizontal =
        br.m_topLeftHorizontal.specifiedValue(rect.width(), this);
    topRightHorizontal =
        br.m_topRightHorizontal.specifiedValue(rect.width(), this);

    topLeftVertical = br.m_topLeftVertical.specifiedValue(rect.height(), this);
    bottomLeftVertical =
        br.m_bottomLeftVertical.specifiedValue(rect.height(), this);

    topRightVertical =
        br.m_topRightVertical.specifiedValue(rect.height(), this);
    bottomRightVertical =
        br.m_bottomRightVertical.specifiedValue(rect.height(), this);

    bottomLeftHorizontal =
        br.m_bottomLeftHorizontal.specifiedValue(rect.width(), this);
    bottomRightHorizontal =
        br.m_bottomRightHorizontal.specifiedValue(rect.width(), this);
}

BorderRadiusFixedData FrameBox::computeFixedBorderRadius(const LayoutRect& rect,
                                                         float spreadDistance,
                                                         bool inset)
{
    BorderRadiusData br = frameBorderRadius();
    BorderRadiusFixedData fixed(br, rect.width(), rect.height(), this);

    if (inset) {
#define APPLY_BORDER_WIDTH(POS, BORDER_SIDE) \
    if (POS) {                               \
        POS -= BORDER_SIDE();                \
    }
        APPLY_BORDER_WIDTH(fixed.m_topLeftHorizontal, borderLeft);
        APPLY_BORDER_WIDTH(fixed.m_topRightHorizontal, borderRight);
        APPLY_BORDER_WIDTH(fixed.m_topLeftVertical, borderTop);
        APPLY_BORDER_WIDTH(fixed.m_bottomLeftVertical, borderBottom);
        APPLY_BORDER_WIDTH(fixed.m_topRightVertical, borderTop);
        APPLY_BORDER_WIDTH(fixed.m_bottomRightVertical, borderBottom);
        APPLY_BORDER_WIDTH(fixed.m_bottomLeftHorizontal, borderLeft);
        APPLY_BORDER_WIDTH(fixed.m_bottomRightHorizontal, borderRight);

#undef APPLY_BORDER_WIDTH
    }

    float r, m;
    if (spreadDistance != 0.0f) {
#define APPLY_SPREAD_DISTANCE(POS)                       \
    if (0 < POS) {                                       \
        if (POS < spreadDistance) {                      \
            r = POS / spreadDistance;                    \
            m = spreadDistance * (1 + pow(r - 1.0f, 3)); \
        } else {                                         \
            m = spreadDistance;                          \
        }                                                \
        if (inset) {                                     \
            POS -= m;                                    \
        } else {                                         \
            POS += m;                                    \
        }                                                \
    }
        APPLY_SPREAD_DISTANCE(fixed.m_topLeftHorizontal);
        APPLY_SPREAD_DISTANCE(fixed.m_topRightHorizontal);
        APPLY_SPREAD_DISTANCE(fixed.m_topLeftVertical);
        APPLY_SPREAD_DISTANCE(fixed.m_bottomLeftVertical);
        APPLY_SPREAD_DISTANCE(fixed.m_topRightVertical);
        APPLY_SPREAD_DISTANCE(fixed.m_bottomRightVertical);
        APPLY_SPREAD_DISTANCE(fixed.m_bottomLeftHorizontal);
        APPLY_SPREAD_DISTANCE(fixed.m_bottomRightHorizontal);
#undef APPLY_SPREAD_DISTANCE
    }

    reduceBorderRadiusToFit(rect, fixed);
    return fixed;
}

template <typename T>
static void emitBorderRadiusPath(T canvas, const LayoutRect& rect,
                                 const BorderRadiusFixedData& fixed)
{
    const float topLeftVertical = fixed.m_topLeftVertical;
    const float topLeftHorizontal = fixed.m_topLeftHorizontal;
    const float topRightHorizontal = fixed.m_topRightHorizontal;
    const float topRightVertical = fixed.m_topRightVertical;
    const float bottomLeftVertical = fixed.m_bottomLeftVertical;
    const float bottomLeftHorizontal = fixed.m_bottomLeftHorizontal;
    const float bottomRightHorizontal = fixed.m_bottomRightHorizontal;
    const float bottomRightVertical = fixed.m_bottomRightVertical;

    float arcR;
    // border-left
    {
        if (topLeftHorizontal > 0 && topLeftVertical > 0) {
            canvas->save();
            canvas->translate(topLeftHorizontal + rect.x().toFloat(),
                              topLeftVertical + rect.y().toFloat());
            if (topLeftVertical > topLeftHorizontal) {
                canvas->scale(1 * (topLeftHorizontal / topLeftVertical), 1);
                arcR = topLeftVertical;
            } else {
                canvas->scale(1, 1 * (topLeftVertical / topLeftHorizontal));
                arcR = topLeftHorizontal;
            }
            canvas->arcNegative(0, 0, arcR, M_PI + M_PI / 4, M_PI);
            canvas->restore();
        } else {
            canvas->moveTo(rect.x(), rect.y());
        }

        if (bottomLeftHorizontal > 0 && bottomLeftVertical > 0) {
            canvas->save();
            canvas->translate(rect.x() + bottomLeftHorizontal,
                              rect.maxY() - bottomLeftVertical);

            if (bottomLeftVertical > bottomLeftHorizontal) {
                canvas->scale(1 * (bottomLeftHorizontal / bottomLeftVertical),
                              1);
                arcR = bottomLeftVertical;
            } else {
                canvas->scale(1,
                              1 * (bottomLeftVertical / bottomLeftHorizontal));
                arcR = bottomLeftHorizontal;
            }

            canvas->arcNegative(0, 0, arcR, M_PI, M_PI - M_PI / 2 + M_PI / 4);
            canvas->restore();
        } else {
            canvas->lineTo(rect.x(), rect.maxY());
        }
    }

    // border-bottom
    {
        if (bottomLeftHorizontal > 0 && bottomLeftVertical > 0) {
            canvas->save();
            canvas->translate(rect.x() + bottomLeftHorizontal,
                              rect.maxY() - bottomLeftVertical);
            if (bottomLeftVertical > bottomLeftHorizontal) {
                canvas->scale(1 * (bottomLeftHorizontal / bottomLeftVertical),
                              1);
                arcR = bottomLeftVertical;
            } else {
                canvas->scale(1,
                              1 * (bottomLeftVertical / bottomLeftHorizontal));
                arcR = bottomLeftHorizontal;
            }
            canvas->arcNegative(0, 0, arcR, M_PI + M_PI / 4 - M_PI / 2,
                                M_PI - M_PI / 2);
            canvas->restore();
        } else {
            canvas->lineTo(rect.x(), rect.maxY());
        }

        if (bottomRightHorizontal > 0 && bottomRightVertical > 0) {
            canvas->save();
            canvas->translate(rect.maxX() - bottomRightHorizontal,
                              rect.maxY() - bottomRightVertical);
            if (bottomRightVertical > bottomRightHorizontal) {
                canvas->scale(1 * (bottomRightHorizontal / bottomRightVertical),
                              1);
                arcR = bottomRightVertical;
            } else {
                canvas->scale(
                    1, 1 * (bottomRightVertical / bottomRightHorizontal));
                arcR = bottomRightHorizontal;
            }
            canvas->arcNegative(0, 0, arcR, M_PI / 2, M_PI / 4);
            canvas->restore();
        } else {
            canvas->lineTo(rect.maxX(), rect.maxY());
        }
    }

    // border-right
    {
        if (bottomRightHorizontal > 0 && bottomRightVertical > 0) {
            canvas->save();
            canvas->translate(rect.maxX() - bottomRightHorizontal,
                              rect.maxY() - bottomRightVertical);
            if (bottomRightVertical > bottomRightHorizontal) {
                canvas->scale(1 * (bottomRightHorizontal / bottomRightVertical),
                              1);
                arcR = bottomRightVertical;
            } else {
                canvas->scale(
                    1, 1 * (bottomRightVertical / bottomRightHorizontal));
                arcR = bottomRightHorizontal;
            }
            canvas->arcNegative(0, 0, arcR, M_PI / 4, 0);
            canvas->restore();
        } else {
            canvas->lineTo(rect.maxX(), rect.maxY());
        }

        if (topRightHorizontal > 0 && topRightVertical > 0) {
            canvas->save();
            canvas->translate(rect.maxX().toFloat() - topRightHorizontal,
                              rect.y().toFloat() + topRightVertical);
            if (topRightVertical > topRightHorizontal) {
                canvas->scale(1 * (topRightHorizontal / topRightVertical), 1);
                arcR = topRightVertical;
            } else {
                canvas->scale(1, 1 * (topRightVertical / topRightHorizontal));
                arcR = topRightHorizontal;
            }
            canvas->arcNegative(0, 0, arcR, M_PI / 2 - M_PI / 2,
                                M_PI / 4 - M_PI / 2);
            canvas->restore();
        } else {
            canvas->lineTo(rect.maxX(), rect.y());
        }
    }

    // border-top
    {
        if (topRightHorizontal > 0 && topRightVertical > 0) {
            canvas->save();
            canvas->translate(-topRightHorizontal + rect.maxX().toFloat(),
                              topRightVertical + rect.y().toFloat());

            if (topRightVertical > topRightHorizontal) {
                canvas->scale(1 * (topRightHorizontal / topRightVertical), 1);
                arcR = topRightVertical;
            } else {
                canvas->scale(1, 1 * (topRightVertical / topRightHorizontal));
                arcR = topRightHorizontal;
            }

            canvas->arcNegative(0, 0, arcR, M_PI / 4 - M_PI / 2, -M_PI / 2);
            canvas->restore();
        } else {
            canvas->lineTo(rect.maxX(), rect.y());
        }

        if (topLeftHorizontal > 0 && topLeftVertical > 0) {
            canvas->save();
            canvas->translate(topLeftHorizontal + rect.x().toFloat(),
                              topLeftVertical + rect.y().toFloat());
            if (topLeftVertical > topLeftHorizontal) {
                canvas->scale(1 * (topLeftHorizontal / topLeftVertical), 1);
                arcR = topLeftVertical;
            } else {
                canvas->scale(1, 1 * (topLeftVertical / topLeftHorizontal));
                arcR = topLeftHorizontal;
            }
            canvas->arcNegative(0, 0, arcR, M_PI + M_PI / 2, M_PI + M_PI / 4);
            canvas->restore();
        } else {
            canvas->lineTo(rect.x(), rect.y());
        }
    }
}

template <typename T>
void FrameBox::applyBorderRadius(T canvas, const LayoutRect& inputRect,
                                 float spreadDistance, bool inset)
{
    // apply clip if border-radius exists
    if (hasFrameBorderRadius()) {
        LayoutRect rect = inputRect.snapSizeToPixel();
        emitBorderRadiusPath(
            canvas, rect,
            computeFixedBorderRadius(rect, spreadDistance, inset));
    }
}

template void FrameBox::applyBorderRadius<Canvas*>(Canvas*,
                                                   const LayoutRect& rect,
                                                   float spreadDistance,
                                                   bool inset);
template void FrameBox::applyBorderRadius<Compositor*>(Compositor*,
                                                       const LayoutRect& rect,
                                                       float spreadDistance,
                                                       bool inset);

// A rounded-corner clip differs from a plain rect clip only inside the four
// corner boxes. When the current clip cannot reach any of them, clipping to
// the rect is exactly equivalent and skips building the eight-arc path, which
// is a large part of the per-tile paint cost while scrolling.
static bool clipAvoidsBorderRadiusCorners(Canvas* canvas,
                                          const LayoutRect& rect,
                                          const BorderRadiusFixedData& fixed)
{
    const float corners[4][4] = {
        { rect.x().toFloat(), rect.y().toFloat(), fixed.m_topLeftHorizontal,
          fixed.m_topLeftVertical },
        { rect.maxX().toFloat() - fixed.m_topRightHorizontal,
          rect.y().toFloat(), fixed.m_topRightHorizontal,
          fixed.m_topRightVertical },
        { rect.x().toFloat(),
          rect.maxY().toFloat() - fixed.m_bottomLeftVertical,
          fixed.m_bottomLeftHorizontal, fixed.m_bottomLeftVertical },
        { rect.maxX().toFloat() - fixed.m_bottomRightHorizontal,
          rect.maxY().toFloat() - fixed.m_bottomRightVertical,
          fixed.m_bottomRightHorizontal, fixed.m_bottomRightVertical },
    };

    for (size_t i = 0; i < 4; i++) {
        if (corners[i][2] <= 0 || corners[i][3] <= 0) {
            continue;
        }
        if (!canvas->canRejectPainting(LayoutRect(
                corners[i][0], corners[i][1], corners[i][2], corners[i][3]))) {
            return false;
        }
    }
    return true;
}

// The compositor has no clip-extents query, so it always takes the path.
static bool clipAvoidsBorderRadiusCorners(Compositor*, const LayoutRect&,
                                          const BorderRadiusFixedData&)
{
    return false;
}

template <typename T>
void FrameBox::applyBorderRadiusClippingIfNeeds(T canvas,
                                                const LayoutRect& rect,
                                                float spreadDistance,
                                                bool inset)
{
    if (hasFrameBorderRadius()) {
        LayoutRect snapped = rect.snapSizeToPixel();
        BorderRadiusFixedData fixed =
            computeFixedBorderRadius(snapped, spreadDistance, inset);
        if (clipAvoidsBorderRadiusCorners(canvas, snapped, fixed)) {
            canvas->clip(Unit::Rect(snapped.x(), snapped.y(), snapped.width(),
                                    snapped.height()));
            return;
        }
        emitBorderRadiusPath(canvas, snapped, fixed);
        canvas->clipPath();
    }
}

template void FrameBox::applyBorderRadiusClippingIfNeeds<Canvas*>(
    Canvas*, const LayoutRect& rect, float spreadDistance, bool inset);
template void FrameBox::applyBorderRadiusClippingIfNeeds<Compositor*>(
    Compositor*, const LayoutRect& rect, float spreadDistance, bool inset);

void FrameBox::paintBackgroundAndBorders(Canvas* canvas)
{
    if (canvas->canRejectPainting(frameVisibleRect())) {
        return;
    }

    canvas->save();

    paintBoxShadows(canvas);
    const LayoutRect rect(0, 0, width(), height());
    applyBorderRadiusClippingIfNeeds(canvas, rect);

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

        paintBackground(canvas, this, nullptr);
    } while (false);

    paintInsetBoxShadows(canvas);

    paintBorders(canvas, rect);

    canvas->restore();
}

void FrameBox::applyBorderShapeClippingUsedInPaintingBoxShadow(
    const Unit::Rect& shadowRect, const Unit::Rect& borderRect,
    const Unit::Rect& imageRect, Canvas* canvas)
{
    bool intersect = borderRect.intersects(imageRect);
    if (intersect) {
        Unit::Rect exteriorRect;
        exteriorRect.unite(borderRect);
        exteriorRect.unite(imageRect);

        canvas->beginPath();
        canvas->moveTo(exteriorRect.x(), exteriorRect.y());
        canvas->lineTo(exteriorRect.x() + exteriorRect.width(),
                       exteriorRect.y());
        canvas->lineTo(exteriorRect.x() + exteriorRect.width(),
                       exteriorRect.y() + exteriorRect.height());
        canvas->lineTo(exteriorRect.x(),
                       exteriorRect.y() + exteriorRect.height());
        canvas->closePath();
        if (hasFrameBorderRadius()) {
            const LayoutRect rect(0, 0, width(), height());
            applyBorderRadiusClippingIfNeeds(canvas, rect);
        } else {
            canvas->setFillRule(false);
            canvas->clip(borderRect);
        }
    }
}

enum class BoxShadowImageKind {
    Outer,
    Inset,
    OuterCorner,
    InsetCorner,
};

// Every input the full-size painting paths below read while drawing and
// blurring a shadow mask, so equal keys mean equal pixels. The colour is not
// one of them: it is put on when the mask is drawn. See BoxShadowImageKey.
// The border radii go in as the style lengths, not resolved values: the
// paths resolve percentages against rects of their own. A calc() radius
// without a percentage resolves to the same length against any rect and goes
// in as that length; one with a percentage has no flat encoding, so such a
// box gets no key and paints uncached.
// boxWidth and boxHeight are the border box the mask is made for, which a
// strip (see drawShadowMaskStrip) makes shorter than the frame's own.
static Optional<BoxShadowImageKey> boxShadowImageKey(
    FrameBox* frame, const CanvasShadowData& shadow, BoxShadowImageKind kind,
    float dpr, LayoutUnit boxWidth, LayoutUnit boxHeight)
{
    BoxShadowImageKey key;
    memset(key.words, 0, sizeof(key.words));
    size_t i = 0;
    auto putFloat = [&key, &i](float f) {
        int32_t v;
        memcpy(&v, &f, sizeof(v));
        key.words[i++] = v;
    };
    key.words[i++] = (int32_t)kind;
    putFloat(dpr);
    key.words[i++] = boxWidth.rawValue();
    key.words[i++] = boxHeight.rawValue();
    key.words[i++] = frame->borderLeft().rawValue();
    key.words[i++] = frame->borderTop().rawValue();
    key.words[i++] = frame->borderRight().rawValue();
    key.words[i++] = frame->borderBottom().rawValue();
    putFloat(shadow.offsetX());
    putFloat(shadow.offsetY());
    putFloat(shadow.radius());
    putFloat(shadow.spreadDistance());
    bool hasRadius = frame->hasFrameBorderRadius();
    key.words[i++] = hasRadius;
    if (hasRadius) {
        BorderRadiusData br = frame->frameBorderRadius();
        const Length* lengths[8] = {
            &br.m_topLeftHorizontal,     &br.m_topLeftVertical,
            &br.m_topRightHorizontal,    &br.m_topRightVertical,
            &br.m_bottomRightHorizontal, &br.m_bottomRightVertical,
            &br.m_bottomLeftHorizontal,  &br.m_bottomLeftVertical,
        };
        for (size_t j = 0; j < 8; j++) {
            const Length& l = *lengths[j];
            if (l.isFixed()) {
                key.words[i++] = (int32_t)Length::Fixed;
                putFloat(l.fixed());
            } else if (l.isPercent()) {
                key.words[i++] = (int32_t)Length::Percent;
                putFloat(l.percent());
            } else if (l.isCalc() && !l.hasPercent()) {
                key.words[i++] = (int32_t)Length::Fixed;
                putFloat(l.specifiedValue(LayoutUnit(0), frame));
            } else {
                return Optional<BoxShadowImageKey>();
            }
        }
    }
    STARFISH_ASSERT(i <= BoxShadowImageKey::maxWords);
    return key;
}

// A shadow is kept as a blurred mask of its shape, one byte of coverage per
// pixel, and the colour is put on when it is drawn. Shadows that differ only
// in colour share one, and it blurs and stores at a quarter of the size of
// an image. draw() paints the shape onto a canvas of width x height device
// pixels; only its coverage is kept.
template <typename Draw>
static BufferedNativeImageData* blurredShadowMask(WebView* webView,
                                                  size_t width, size_t height,
                                                  float stdDeviation,
                                                  const Draw& draw)
{
    BufferedNativeImageData* shape =
        BufferedNativeImageData::create(width, height);
    Canvas* cv = Canvas::create(webView, shape);
    cv->clearColor(Unit::Color(0, 0, 0, 0));
    cv->setFillColor(Unit::Color(255, 255, 255, 255));
    draw(cv);
    cv->flush();
    delete cv;

    BufferedNativeImageData* mask =
        BufferedNativeImageData::createAlphaMask(width, height);
    for (size_t y = 0; y < height; y++) {
        const uint8_t* s = shape->data() + y * shape->stride() + 3;
        uint8_t* d = mask->data() + y * mask->stride();
        for (size_t x = 0; x < width; x++) {
            d[x] = s[x * 4];
        }
    }
    delete shape;

    ShadowBlur sb(mask->data(), mask->width(), mask->height(), mask->stride(),
                  1);
    sb.process(stdDeviation);
    return mask;
}

// Clears from the mask what the shape draw() paints. It takes the place of
// clipping the drawn mask to the outside of that shape: a mask under a clip
// with a hole does not composite right (see Canvas::fillWithImageAlpha),
// and a shape cut out once costs nothing when the mask is drawn again.
template <typename Draw>
static void cutShapeFromMask(WebView* webView, BufferedNativeImageData* mask,
                             const Draw& draw)
{
    BufferedNativeImageData* shape =
        BufferedNativeImageData::create(mask->width(), mask->height());
    Canvas* cv = Canvas::create(webView, shape);
    cv->clearColor(Unit::Color(0, 0, 0, 0));
    cv->setFillColor(Unit::Color(255, 255, 255, 255));
    draw(cv);
    cv->flush();
    delete cv;

    for (size_t y = 0; y < mask->height(); y++) {
        const uint8_t* s = shape->data() + y * shape->stride() + 3;
        uint8_t* d = mask->data() + y * mask->stride();
        for (size_t x = 0; x < mask->width(); x++) {
            const uint32_t t = d[x] * (255 - s[x * 4]) + 128;
            d[x] = (t + (t >> 8)) >> 8;
        }
    }
    delete shape;
}

// Draws piece turned by deg over dst, from what src marks out of it in the
// piece's own units, before the device scale.
//
// part is the part of dst worth painting, the rest of it being hidden, and
// src is cut to match. The turn is a whole number of quarters, so a
// rectangle of dst is a rectangle of the piece as well. A src one unit
// across is a run the piece is stretched along: cutting the run would only
// resample the same profile, so it is kept whole and only dst moves in.
static void drawBoxShadowRotatePieceImage(Canvas* canvas, Unit::Rect src,
                                          const Unit::Rect& dst,
                                          const Unit::Rect& part, float deg,
                                          NativeImageData* piece,
                                          const Unit::Color& color, float dpr)
{
    Unit::Rect local(0, 0, dst.width(), dst.height());
    if (!(part == dst)) {
        // The part as a fraction of dst, turned back into the piece's frame:
        // a quarter turn clockwise takes (u, v) there to (1 - v, u) here.
        float u0 = (part.x() - dst.x()) / dst.width();
        float u1 = (part.maxX() - dst.x()) / dst.width();
        float v0 = (part.y() - dst.y()) / dst.height();
        float v1 = (part.maxY() - dst.y()) / dst.height();
        for (int turns = ((int)(deg / 90.f)) & 3; turns > 0; turns--) {
            const float u[2] = { v0, v1 };
            const float v[2] = { 1 - u1, 1 - u0 };
            u0 = u[0];
            u1 = u[1];
            v0 = v[0];
            v1 = v[1];
        }

        // The piece is cut on whole device pixels, which is how it is
        // copied, and dst keeps whatever the cut leaves over.
        if (src.width() > 1) {
            const float w = src.width() * dpr;
            const float x0 = u0 > 0 ? floorf(u0 * w) : 0;
            const float x1 = u1 < 1 ? std::min(ceilf(u1 * w), w) : w;
            u0 = x0 / w;
            u1 = x1 / w;
            src.setX(src.x() + x0 / dpr);
            src.setWidth((x1 - x0) / dpr);
        }
        if (src.height() > 1) {
            const float h = src.height() * dpr;
            const float y0 = v0 > 0 ? floorf(v0 * h) : 0;
            const float y1 = v1 < 1 ? std::min(ceilf(v1 * h), h) : h;
            v0 = y0 / h;
            v1 = y1 / h;
            src.setY(src.y() + y0 / dpr);
            src.setHeight((y1 - y0) / dpr);
        }
        local = Unit::Rect(u0 * dst.width(), v0 * dst.height(),
                           (u1 - u0) * dst.width(), (v1 - v0) * dst.height());
    }

    src.setX(src.x() * dpr);
    src.setY(src.y() * dpr);
    src.setWidth(src.width() * dpr);
    src.setHeight(src.height() * dpr);

    canvas->save();
    canvas->translate(dst.x(), dst.y());
    canvas->translate(dst.width() / 2.f, dst.height() / 2.f);
    canvas->rotate(UnitHelper::convertFromDegToRad(deg));
    canvas->translate(-dst.width() / 2.f, -dst.height() / 2.f);
    canvas->fillWithImageAlpha(piece, src, local, color);
    canvas->restore();
}

// What of a shadow will show where it is painted: the clip it is drawn
// under, and the parts of it nothing shows of - an outer shadow is clipped
// out of its own border box, so whatever of it falls inside is painted for
// nothing. A piece wholly hidden is dropped, and one a hidden part takes a
// whole band off the edge of is cut down to the rest of it.
class BoxShadowVisibility {
public:
    void clipTo(const Unit::Rect& rect)
    {
        m_clip = rect;
        m_hasClip = true;
    }

    void hide(const Unit::Rect& rect)
    {
        if (rect.isEmpty() || m_hiddenCount >= 2) {
            return;
        }
        m_hidden[m_hiddenCount++] = rect;
    }

    // False when nothing of rect shows; otherwise part is what of it does.
    bool visiblePart(const Unit::Rect& rect, Unit::Rect& part) const
    {
        Unit::Rect r = rect;
        if (m_hasClip) {
            r.intersect(m_clip);
        }
        for (size_t i = 0; i < m_hiddenCount && !r.isEmpty(); i++) {
            Unit::Rect hidden = r;
            hidden.intersect(m_hidden[i]);
            if (hidden.isEmpty()) {
                continue;
            }
            // Only a band along one edge leaves a rectangle behind.
            if (hidden.width() == r.width()) {
                if (hidden.height() == r.height()) {
                    return false;
                }
                if (hidden.y() == r.y()) {
                    r = Unit::Rect(r.x(), hidden.maxY(), r.width(),
                                   r.maxY() - hidden.maxY());
                } else if (hidden.maxY() == r.maxY()) {
                    r.setHeight(hidden.y() - r.y());
                }
            } else if (hidden.height() == r.height()) {
                if (hidden.x() == r.x()) {
                    r = Unit::Rect(hidden.maxX(), r.y(),
                                   r.maxX() - hidden.maxX(), r.height());
                } else if (hidden.maxX() == r.maxX()) {
                    r.setWidth(hidden.x() - r.x());
                }
            }
        }
        if (r.isEmpty()) {
            return false;
        }
        part = r;
        return true;
    }

    // Whether nothing of rect shows at all, which a shape that stays inside
    // it - the notch fill - then need not be painted either.
    bool isHidden(const Unit::Rect& rect) const
    {
        for (size_t i = 0; i < m_hiddenCount; i++) {
            if (m_hidden[i].contains(rect)) {
                return true;
            }
        }
        return false;
    }

private:
    Unit::Rect m_clip;
    bool m_hasClip = false;
    Unit::Rect m_hidden[2];
    size_t m_hiddenCount = 0;
};

// The two bands of a rounded box that no corner cuts into - one the full
// width of it, one the full height - which together hold as much of it as
// two rectangles can. On whole pixels, so that a piece cut against them
// stays on the pixel grid, where it is drawn without antialiasing.
static void hideBorderShape(BoxShadowVisibility& visibility,
                            const Unit::Rect& rect,
                            const BorderRadiusFixedData& radii)
{
    const float top =
        std::max(radii.m_topLeftVertical, radii.m_topRightVertical);
    const float bottom =
        std::max(radii.m_bottomLeftVertical, radii.m_bottomRightVertical);
    const float left =
        std::max(radii.m_topLeftHorizontal, radii.m_bottomLeftHorizontal);
    const float right =
        std::max(radii.m_topRightHorizontal, radii.m_bottomRightHorizontal);

    const float x = ceilf(rect.x());
    const float y = ceilf(rect.y());
    const float maxX = floorf(rect.maxX());
    const float maxY = floorf(rect.maxY());
    const float bandTop = ceilf(rect.y() + top);
    const float bandBottom = floorf(rect.maxY() - bottom);
    const float bandLeft = ceilf(rect.x() + left);
    const float bandRight = floorf(rect.maxX() - right);

    visibility.hide(Unit::Rect(x, bandTop, maxX - x, bandBottom - bandTop));
    if (left > 0 || right > 0 || top > 0 || bottom > 0) {
        visibility.hide(
            Unit::Rect(bandLeft, y, bandRight - bandLeft, maxY - y));
    }
}

// A blurred box shadow only varies around the corners of its shape: along a
// straight side it repeats one profile, and away from the sides it is flat.
// So it is painted as a nine patch. Each corner is a small piece blurred once
// for its radii - shared by every corner of every box that has those radii,
// whatever the size or the colour of the box - the sides are stretched from a
// line of a piece where the corner no longer reaches, and the rest is a plain
// fill.
//
// A piece holds the top-left corner of the shape. The other corners draw it
// turned by 90 degrees at a time, which swaps the two radii of an elliptical
// corner on every other turn.
class BoxShadowNinePatch {
public:
    // Straight side kept in a piece beyond what the corner and the blur
    // reach, which is where the sides are sampled.
    static const int straightPart = 2;

    // margin is the part of a piece outside the shape, which an outer shadow
    // fades into. For an inset shadow that is the solid side, as deep as the
    // blur reaches.
    BoxShadowNinePatch(WebView* webView, bool inset, const Unit::Color& color,
                       float blur, int margin = 0)
        : m_webView(webView)
        , m_inset(inset)
        , m_color(color)
        , m_blur(blur)
        , m_dpr(webView->screenInfo().devicePixelRatio)
        , m_margin(margin)
    {
        // How far ShadowBlur's three box passes carry a pixel.
        float kernelSize =
            ShadowBlur::computeKernelSizeAtStdDeviation(blur / 2 * m_dpr);
        m_reach = ceil(kernelSize * 1.5f / m_dpr) + 1;
        if (inset) {
            m_margin = m_reach;
        }
        for (size_t i = 0; i < 4; i++) {
            m_images[i] = nullptr;
            m_created[i] = false;
        }
    }

    // How far into the shape each corner reaches, for the given radii.
    void measure(const BorderRadiusFixedData& radii)
    {
        // As the top-left corner each of them is a turned copy of.
        const float r[4][2] = {
            { radii.m_topLeftHorizontal, radii.m_topLeftVertical },
            { radii.m_topRightVertical, radii.m_topRightHorizontal },
            { radii.m_bottomRightHorizontal, radii.m_bottomRightVertical },
            { radii.m_bottomLeftVertical, radii.m_bottomLeftHorizontal },
        };
        for (size_t i = 0; i < 4; i++) {
            m_horizontal[i] = r[i][0];
            m_vertical[i] = r[i][1];
            m_inner[i] =
                (int)ceil(std::max(r[i][0], r[i][1])) + m_reach + straightPart;
        }
    }

    // Whether the corners of a shape that wide, or that tall, stay clear of
    // each other along that axis.
    bool fitsAcross(int width) const
    {
        return m_inner[0] + m_inner[1] <= width &&
               m_inner[3] + m_inner[2] <= width;
    }
    bool fitsDown(int height) const
    {
        return m_inner[0] + m_inner[3] <= height &&
               m_inner[1] + m_inner[2] <= height;
    }

    // False when the corners of a width x height shape are too close for
    // their pieces to stay clear of each other.
    bool fit(int width, int height, const BorderRadiusFixedData& radii)
    {
        measure(radii);
        return fitsAcross(width) && fitsDown(height);
    }

    // How far the corners reach in from the start (left or top) and the end
    // of the shape along an axis: 0 across, 1 down.
    int capStart(int axis) const
    {
        return axis == 0 ? std::max(m_inner[0], m_inner[3])
                         : std::max(m_inner[0], m_inner[1]);
    }
    int capEnd(int axis) const
    {
        return axis == 0 ? std::max(m_inner[1], m_inner[2])
                         : std::max(m_inner[3], m_inner[2]);
    }

    // outerRect is the shape grown by the margin on every side. Of every
    // piece of it only the part visibility keeps is painted.
    void draw(Canvas* canvas, const Unit::Rect& outerRect,
              const BoxShadowVisibility& visibility)
    {
        const float x = outerRect.x();
        const float y = outerRect.y();
        const float maxX = outerRect.maxX();
        const float maxY = outerRect.maxY();
        const int band = m_margin + m_reach;
        int size[4];
        for (size_t i = 0; i < 4; i++) {
            size[i] = m_margin + m_inner[i];
        }

        // Clockwise from the top-left, a quarter turn more for each.
        const Unit::Rect cornerRects[4] = {
            Unit::Rect(x, y, size[0], size[0]),
            Unit::Rect(maxX - size[1], y, size[1], size[1]),
            Unit::Rect(maxX - size[2], maxY - size[2], size[2], size[2]),
            Unit::Rect(x, maxY - size[3], size[3], size[3]),
        };
        for (size_t i = 0; i < 4; i++) {
            Unit::Rect part;
            if (!visibility.visiblePart(cornerRects[i], part) ||
                isRejected(canvas, part)) {
                continue;
            }
            drawBoxShadowRotatePieceImage(
                canvas, Unit::Rect(0, 0, size[i], size[i]), cornerRects[i],
                part, 90 * i, piece(i), m_color, m_dpr);
        }

        // The sides, from the last column (top, bottom) or the last row of
        // the top-left piece, where it has settled into the side's profile.
        const Unit::Rect sideRects[4] = {
            Unit::Rect(x + size[0], y, maxX - size[1] - (x + size[0]), band),
            Unit::Rect(maxX - band, y + size[1], band,
                       maxY - size[2] - (y + size[1])),
            Unit::Rect(x + size[3], maxY - band, maxX - size[2] - (x + size[3]),
                       band),
            Unit::Rect(x, y + size[0], band, maxY - size[3] - (y + size[0])),
        };
        for (size_t i = 0; i < 4; i++) {
            Unit::Rect part;
            if (sideRects[i].width() <= 0 || sideRects[i].height() <= 0 ||
                !visibility.visiblePart(sideRects[i], part) ||
                isRejected(canvas, part)) {
                continue;
            }
            bool alongX = i == 0 || i == 2;
            drawBoxShadowRotatePieceImage(
                canvas,
                alongX ? Unit::Rect(size[0] - 1, 0, 1, band)
                       : Unit::Rect(0, size[0] - 1, band, 1),
                sideRects[i], part, (i == 1 || i == 2) ? 180 : 0, piece(0),
                m_color, m_dpr);
        }

        if (!m_inset) {
            // What is left inside the sides, around the inner parts of the
            // pieces, is the plain colour.
            const float l = x + band;
            const float t = y + band;
            const float r = maxX - band;
            const float b = maxY - band;
            // It stays inside the sides, so nothing of it shows when they
            // are hidden - the usual case, the fill being under the box.
            if (r <= l || b <= t ||
                visibility.isHidden(Unit::Rect(l, t, r - l, b - t))) {
                return;
            }
            int notch[4];
            for (size_t i = 0; i < 4; i++) {
                notch[i] = size[i] - band;
            }
            canvas->beginPath();
            canvas->moveTo(l + notch[0], t);
            canvas->lineTo(r - notch[1], t);
            canvas->lineTo(r - notch[1], t + notch[1]);
            canvas->lineTo(r, t + notch[1]);
            canvas->lineTo(r, b - notch[2]);
            canvas->lineTo(r - notch[2], b - notch[2]);
            canvas->lineTo(r - notch[2], b);
            canvas->lineTo(l + notch[3], b);
            canvas->lineTo(l + notch[3], b - notch[3]);
            canvas->lineTo(l, b - notch[3]);
            canvas->lineTo(l, t + notch[0]);
            canvas->lineTo(l + notch[0], t + notch[0]);
            canvas->closePath();
            canvas->setFillColor(m_color);
            canvas->fill();
        }
    }

    // Hands the pieces blurred for this paint over to the cache. Not before
    // the drawing is done: storing one may evict another.
    void finish()
    {
        for (size_t i = 0; i < 4; i++) {
            if (m_created[i]) {
                m_webView->storeBoxShadowImage(m_keys[i], m_images[i]);
                m_created[i] = false;
            }
        }
    }

    int margin() const
    {
        return m_margin;
    }

private:
    static bool isRejected(Canvas* canvas, const Unit::Rect& rect)
    {
        return canvas->canRejectPainting(
            LayoutRect(rect.x(), rect.y(), rect.width(), rect.height()));
    }

    // The piece of a corner, as the top-left corner it is drawn turned from.
    BufferedNativeImageData* piece(size_t i)
    {
        if (m_images[i]) {
            return m_images[i];
        }
        const int size = m_margin + m_inner[i];
        BoxShadowImageKey& key = m_keys[i];
        memset(key.words, 0, sizeof(key.words));
        const float floats[4] = { m_dpr, m_blur, m_horizontal[i],
                                  m_vertical[i] };
        key.words[0] = (int32_t)(m_inset ? BoxShadowImageKind::InsetCorner
                                         : BoxShadowImageKind::OuterCorner);
        memcpy(&key.words[1], floats, sizeof(floats));
        key.words[5] = m_margin;
        key.words[6] = size;

        // Corners with the same radii share it, also before it reaches the
        // cache.
        for (size_t j = 0; j < 4; j++) {
            if (j != i && m_images[j] && m_keys[j] == key) {
                m_images[i] = m_images[j];
                return m_images[i];
            }
        }
        m_images[i] = m_webView->lookupBoxShadowImage(key);
        if (m_images[i]) {
            return m_images[i];
        }

        // Only the top-left corner of the shape is inside the piece.
        const LayoutRect shape(m_margin, m_margin, size * 2, size * 2);
        const BorderRadiusFixedData radii(m_horizontal[i], m_vertical[i], 0, 0,
                                          0, 0, 0, 0);
        const size_t deviceSize = ceil(size * m_dpr);
        BufferedNativeImageData* image =
            blurredShadowMask(m_webView, deviceSize, deviceSize,
                              m_blur / 2 * m_dpr, [&](Canvas* cv) {
                                  if (m_inset) {
                                      cv->rect(Unit::Rect(0, 0, size, size));
                                      emitBorderRadiusPath(cv, shape, radii);
                                      cv->setFillRule(false);
                                      cv->fill();
                                  } else {
                                      emitBorderRadiusPath(cv, shape, radii);
                                      cv->clipPath();
                                      cv->drawRect(shape);
                                  }
                              });

        m_images[i] = image;
        m_created[i] = true;
        return image;
    }

    WebView* m_webView;
    bool m_inset;
    Unit::Color m_color;
    float m_blur;
    float m_dpr;
    int m_margin;
    int m_reach;
    float m_horizontal[4];
    float m_vertical[4];
    int m_inner[4];
    BufferedNativeImageData* m_images[4];
    BoxShadowImageKey m_keys[4];
    bool m_created[4];
};

// The pieces of a nine patch are placed on whole pixels.
static bool isWholePixels(float a, float b, float c)
{
    return a == floorf(a) && b == floorf(b) && c == floorf(c);
}

// A shape too short for corner pieces along one axis may still have a
// straight run along the other, where its shadow repeats one column (or
// row). The mask of a box just long enough for the two end caps and that
// run is kept, keyed without the box's length, and a longer box draws it as
// a strip: the caps at its ends and the run stretched between them.
//
// Which axis a shape gets a strip along, if any: 0 across, 1 down, -1 none.
static int shadowStripAxis(bool fitsAcross, bool fitsDown)
{
    if (fitsAcross == fitsDown) {
        return -1;
    }
    return fitsAcross ? 0 : 1;
}

// A strip's caps must not depend on the box's length; a percentage radius
// does.
static bool hasPercentBorderRadius(FrameBox* frame)
{
    if (!frame->hasFrameBorderRadius()) {
        return false;
    }
    BorderRadiusData br = frame->frameBorderRadius();
    const Length* lengths[8] = {
        &br.m_topLeftHorizontal,     &br.m_topLeftVertical,
        &br.m_topRightHorizontal,    &br.m_topRightVertical,
        &br.m_bottomRightHorizontal, &br.m_bottomRightVertical,
        &br.m_bottomLeftHorizontal,  &br.m_bottomLeftVertical,
    };
    for (size_t i = 0; i < 8; i++) {
        if (lengths[i]->isPercent() || lengths[i]->hasPercent()) {
            return true;
        }
    }
    return false;
}

// Draws mask, made for a box shorter along axis, onto dst: capStart and
// capEnd are the lengths of the caps from the start and the end of dst (and
// of the mask), scale is the size of a mask pixel in dst units, and the
// mask's first column (or row) past the start cap fills the run between
// the caps.
static void drawShadowMaskStrip(Canvas* canvas, BufferedNativeImageData* mask,
                                const Unit::Rect& dst, int axis, float capStart,
                                float capEnd, float scaleX, float scaleY,
                                const Unit::Color& color)
{
    const float maskWidth = mask->width();
    const float maskHeight = mask->height();
    if (axis == 0) {
        const float run = dst.width() - capStart - capEnd;
        const float column = capStart / scaleX;
        canvas->fillWithImageAlpha(
            mask, Unit::Rect(0, 0, column, maskHeight),
            Unit::Rect(dst.x(), dst.y(), capStart, dst.height()), color);
        if (run > 0) {
            canvas->fillWithImageAlpha(
                mask, Unit::Rect(column, 0, 1, maskHeight),
                Unit::Rect(dst.x() + capStart, dst.y(), run, dst.height()),
                color);
        }
        canvas->fillWithImageAlpha(
            mask,
            Unit::Rect(maskWidth - capEnd / scaleX, 0, capEnd / scaleX,
                       maskHeight),
            Unit::Rect(dst.maxX() - capEnd, dst.y(), capEnd, dst.height()),
            color);
    } else {
        const float run = dst.height() - capStart - capEnd;
        const float row = capStart / scaleY;
        canvas->fillWithImageAlpha(
            mask, Unit::Rect(0, 0, maskWidth, row),
            Unit::Rect(dst.x(), dst.y(), dst.width(), capStart), color);
        if (run > 0) {
            canvas->fillWithImageAlpha(
                mask, Unit::Rect(0, row, maskWidth, 1),
                Unit::Rect(dst.x(), dst.y() + capStart, dst.width(), run),
                color);
        }
        canvas->fillWithImageAlpha(
            mask,
            Unit::Rect(0, maskHeight - capEnd / scaleY, maskWidth,
                       capEnd / scaleY),
            Unit::Rect(dst.x(), dst.maxY() - capEnd, dst.width(), capEnd),
            color);
    }
}

void FrameBox::paintBoxShadows(Canvas* canvas)
{
    STARFISH_ASSERT(canvas != nullptr);

    ComputedStyle* s = style();
    STARFISH_ASSERT(s != nullptr);

    if (s->visibility() != VisibilityValue::VisibleVisibilityValue) {
        return;
    }

    bool hasShadow = s->boxShadow() ? true : false;
    if (hasShadow) {
        canvas->save();
        CanvasShadowDataList list =
            s->boxShadow()->toCanvasShadowDataList(this);

        for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
            float sd = shadow->spreadDistance();
            auto shadowColor =
                shadow->hasColor() ? shadow->color() : s->color();

            if (shadowColor.isTransparent()) {
                continue;
            }

            if (!shadow->inset()) {
                Unit::Rect borderRect = makeRect(BoxValue::BorderBoxBoxValue);

                int xx = 0, yy = 0, ww = 0, hh = 0;
                LayoutUnit rx = borderRect.x();
                LayoutUnit ry = borderRect.y();
                xx = rx.floor();
                yy = ry.floor();
                ww = snapSizeToPixel(borderRect.width() + sd * 2, rx);
                hh = snapSizeToPixel(borderRect.height() + sd * 2, ry);
                Unit::Rect shadowRect(xx, yy, ww, hh);

                if (shadow->radius() == 0) {
                    // fast path
                    // we can draw just rect only
                    canvas->save();
                    canvas->setFillColor(shadowColor);

                    xx = rx.floor();
                    yy = ry.floor();
                    ww = snapSizeToPixel(borderRect.width() + sd * 2, rx);
                    hh = snapSizeToPixel(borderRect.height() + sd * 2, ry);

                    Unit::Rect simpleShadowRect(
                        shadow->offsetX() - sd, shadow->offsetY() - sd,
                        shadowRect.width(), shadowRect.height());

                    const LayoutRect clipRect(
                        shadow->offsetX() - sd, shadow->offsetY() - sd,
                        shadowRect.width(), shadowRect.height());
                    applyBorderRadiusClippingIfNeeds(canvas, clipRect, sd);

                    Unit::Rect borderShapeClipRect(
                        shadow->offsetX() - sd, shadow->offsetY() - sd,
                        shadowRect.width(), shadowRect.height());
                    applyBorderShapeClippingUsedInPaintingBoxShadow(
                        shadowRect, borderRect, borderShapeClipRect, canvas);

                    canvas->drawRect(simpleShadowRect);

                    canvas->restore();
                    continue;
                }

                float radiusOffset = 0.0f;
                if (shadow->radius()) {
                    radiusOffset = shadow->radius();
                    radiusOffset =
                        std::min(ShadowBlur::RADIUS_LIMIT, radiusOffset);
                    radiusOffset *= 2;
                }

                WebView* wv = node()->webView();
                const int margin = ceil(radiusOffset / 2);
                BoxShadowNinePatch ninePatch(wv, false, shadowColor,
                                             shadow->radius(), margin);
                BorderRadiusFixedData radii(0, 0, 0, 0, 0, 0, 0, 0);
                if (hasFrameBorderRadius()) {
                    radii = computeFixedBorderRadius(
                        LayoutRect(0, 0, shadowRect.width(),
                                   shadowRect.height()),
                        sd, false);
                }
                ninePatch.measure(radii);
                const bool wholePixels =
                    isWholePixels(shadow->offsetX(), shadow->offsetY(), sd);
                const bool fitsAcross =
                    ninePatch.fitsAcross(shadowRect.width());
                const bool fitsDown = ninePatch.fitsDown(shadowRect.height());

                if (wholePixels && fitsAcross && fitsDown) {
                    canvas->save();
                    Unit::Rect outerRect(-margin + shadow->offsetX() - sd,
                                         -margin + shadow->offsetY() - sd,
                                         shadowRect.width() + margin * 2,
                                         shadowRect.height() + margin * 2);
                    applyBorderShapeClippingUsedInPaintingBoxShadow(
                        shadowRect, borderRect, outerRect, canvas);
                    canvas->setNeedsNoneAntialias();
                    // That clipping keeps the shadow out of the border box,
                    // so the patch has nothing to paint in there.
                    BoxShadowVisibility visibility;
                    if (borderRect.intersects(outerRect)) {
                        BorderRadiusFixedData shape(0, 0, 0, 0, 0, 0, 0, 0);
                        const LayoutRect box(0, 0, width(), height());
                        if (hasFrameBorderRadius()) {
                            shape = computeFixedBorderRadius(
                                box.snapSizeToPixel(), 0, false);
                        }
                        hideBorderShape(visibility, borderRect, shape);
                    }
                    ninePatch.draw(canvas, outerRect, visibility);
                    canvas->restore();
                    ninePatch.finish();
                } else {
                    canvas->save();
                    const float dpr = wv->screenInfo().devicePixelRatio;
                    const float offset = ceil(radiusOffset / 2);

                    // The shadow of a border box of the given size: the
                    // shape, and where its mask goes.
                    struct Geometry {
                        Unit::Rect borderRect;
                        Unit::Rect shadowRect;
                        Unit::Rect imageRect;
                    };
                    auto geometry = [&](float boxWidth, float boxHeight) {
                        Geometry g;
                        g.borderRect = Unit::Rect(0, 0, boxWidth, boxHeight);
                        g.shadowRect = Unit::Rect(
                            0, 0, snapSizeToPixel(boxWidth + sd * 2, 0),
                            snapSizeToPixel(boxHeight + sd * 2, 0));
                        g.imageRect = Unit::Rect(
                            -offset + shadow->offsetX() - sd,
                            -offset + shadow->offsetY() - sd,
                            ceil(g.shadowRect.width() + radiusOffset),
                            ceil(g.shadowRect.height() + radiusOffset));
                        return g;
                    };
                    const Geometry real = geometry(width(), height());

                    // A strip when only one axis is too short: the mask is
                    // that of a box just long enough for the caps along the
                    // other axis. The caps are measured from the mask's
                    // edges, so they take in the margin around the shape.
                    int axis = wholePixels && !hasPercentBorderRadius(this)
                                   ? shadowStripAxis(fitsAcross, fitsDown)
                                   : -1;
                    Geometry made = real;
                    float capStart = 0;
                    float capEnd = 0;
                    if (axis >= 0) {
                        const float endMargin = ceil(radiusOffset) - offset;
                        capStart = offset + ninePatch.capStart(axis);
                        capEnd = endMargin + ninePatch.capEnd(axis);
                        // With the fraction of the box's own length, so
                        // that the shape and the border box snap to pixels
                        // the way they do for the box.
                        const float length = axis == 0 ? width() : height();
                        const float shapeLength =
                            ninePatch.capStart(axis) + ninePatch.capEnd(axis) +
                            BoxShadowNinePatch::straightPart + length -
                            floor(length);
                        made = axis == 0
                                   ? geometry(shapeLength - sd * 2, height())
                                   : geometry(width(), shapeLength - sd * 2);
                        if (made.borderRect.width() <= 0 ||
                            made.borderRect.height() <= 0) {
                            axis = -1;
                            made = real;
                        }
                    }

                    Optional<BoxShadowImageKey> cacheKey = boxShadowImageKey(
                        this, *shadow, BoxShadowImageKind::Outer, dpr,
                        made.borderRect.width(), made.borderRect.height());
                    BufferedNativeImageData* nativeImage =
                        cacheKey ? wv->lookupBoxShadowImage(cacheKey.value())
                                 : nullptr;
                    bool cachedImage = nativeImage != nullptr;

                    if (!nativeImage) {
                        nativeImage = blurredShadowMask(
                            wv, ceil(made.imageRect.width() * dpr),
                            ceil(made.imageRect.height() * dpr),
                            shadow->radius() / 2 * dpr, [&](Canvas* cv) {
                                cv->translate(offset, offset);
                                const LayoutRect clipRect(
                                    0, 0, made.shadowRect.width(),
                                    made.shadowRect.height());
                                applyBorderRadiusClippingIfNeeds(cv, clipRect,
                                                                 sd);
                                cv->drawRect(made.shadowRect);
                            });
                        // The shadow does not show through the border box.
                        if (made.borderRect.intersects(made.imageRect)) {
                            cutShapeFromMask(wv, nativeImage, [&](Canvas* cv) {
                                cv->translate(-made.imageRect.x(),
                                              -made.imageRect.y());
                                if (hasFrameBorderRadius()) {
                                    applyBorderRadiusClippingIfNeeds(
                                        cv, LayoutRect(
                                                0, 0, made.borderRect.width(),
                                                made.borderRect.height()));
                                }
                                cv->drawRect(made.borderRect);
                            });
                        }
                    }

                    if (axis < 0) {
                        canvas->fillWithImageAlpha(
                            nativeImage,
                            Unit::Rect(0, 0, nativeImage->width(),
                                       nativeImage->height()),
                            real.imageRect, shadowColor);
                    } else {
                        canvas->setNeedsNoneAntialias();
                        drawShadowMaskStrip(
                            canvas, nativeImage, real.imageRect, axis, capStart,
                            capEnd,
                            made.imageRect.width() / nativeImage->width(),
                            made.imageRect.height() / nativeImage->height(),
                            shadowColor);
                    }
                    canvas->restore();

                    if (cachedImage) {
                    } else if (cacheKey) {
                        wv->storeBoxShadowImage(cacheKey.value(), nativeImage);
                    } else {
                        delete nativeImage;
                    }
                }
            }
        }
        list.clear();
        canvas->restore();
    }
}

void FrameBox::paintInsetBoxShadows(Canvas* canvas)
{
    ComputedStyle* s = style();

    if (s->visibility() != VisibilityValue::VisibleVisibilityValue) {
        return;
    }

    bool hasShadow = s->boxShadow() ? true : false;
    if (hasShadow) {
        canvas->save();
        CanvasShadowDataList list =
            s->boxShadow()->toCanvasShadowDataList(this);

        for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
            float sd = shadow->spreadDistance();
            auto shadowColor =
                shadow->hasColor() ? shadow->color() : s->color();

            if (shadowColor.isTransparent()) {
                continue;
            }

            if (shadow->inset()) {
                Unit::Rect borderRect = makeRect(BoxValue::BorderBoxBoxValue);
                Unit::Rect paddingRect = makeRect(BoxValue::PaddingBoxBoxValue);
                WebView* wv = node()->webView();

                if (shadow->radius() == 0) {
                    // fast path #1(no blur)
                    Unit::Rect shadowInnerRect(
                        paddingRect.x() + shadow->offsetX() + sd,
                        paddingRect.y() + shadow->offsetY() + sd,
                        paddingRect.width() - sd * 2,
                        paddingRect.height() - sd * 2);
                    canvas->save();
                    if (hasFrameBorderRadius()) {
                        applyBorderRadius(canvas,
                                          LayoutRect(paddingRect.x(),
                                                     paddingRect.y(),
                                                     paddingRect.width(),
                                                     paddingRect.height()),
                                          0, true);

                        applyBorderRadius(canvas,
                                          LayoutRect(shadowInnerRect.x(),
                                                     shadowInnerRect.y(),
                                                     shadowInnerRect.width(),
                                                     shadowInnerRect.height()),
                                          sd, true);
                    } else {
                        canvas->clip(paddingRect);
                        canvas->rect(paddingRect);
                        canvas->rect(shadowInnerRect);
                    }

                    canvas->setFillColor(shadowColor);
                    canvas->setFillRule(false);
                    canvas->fill();

                    canvas->restore();
                    continue;
                }

                const float dpr = wv->screenInfo().devicePixelRatio;
                // Create image buffer bigger than paddingbox+ shadowBox
                const float margin = 4.0f;
                const float half = 2.0f;
                const float dx =
                    (shadow->offsetX() < 0) ? -half + shadow->offsetX() : -half;
                const float dy =
                    (shadow->offsetY() < 0) ? -half + shadow->offsetY() : -half;

                // The shadow of a padding box of the given size: the hole,
                // the mask around it and the part of the mask that shows.
                struct Geometry {
                    Unit::Rect paddingRect;
                    Unit::Rect interiorRect;
                    Unit::Rect imageRect;
                    Unit::Rect rect;
                    Unit::Rect crop;
                    size_t imageWidth;
                    size_t imageHeight;
                    float scaleX;
                    float scaleY;
                };
                auto geometry = [&](float paddingWidth, float paddingHeight) {
                    Geometry g;
                    g.paddingRect = Unit::Rect(paddingRect.x(), paddingRect.y(),
                                               paddingWidth, paddingHeight);
                    Unit::Rect box(0, 0, paddingWidth + borderWidth(),
                                   paddingHeight + borderHeight());
                    Unit::Rect shadowRect(
                        g.paddingRect.x(), g.paddingRect.y(),
                        paddingWidth + std::abs(shadow->offsetX()),
                        paddingHeight + std::abs(shadow->offsetY()));
                    // The image is placed by a negative offset below, so
                    // only a positive one moves the hole inside it.
                    LayoutUnit x = g.paddingRect.x() +
                                   std::max(shadow->offsetX(), 0.f) + sd;
                    LayoutUnit y = g.paddingRect.y() +
                                   std::max(shadow->offsetY(), 0.f) + sd;
                    g.interiorRect =
                        Unit::Rect(x.floor(), y.floor(),
                                   snapSizeToPixel(paddingWidth - sd * 2, x),
                                   snapSizeToPixel(paddingHeight - sd * 2, y));

                    Unit::Rect exteriorRect;
                    exteriorRect.unite(box);
                    exteriorRect.unite(shadowRect);
                    exteriorRect.unite(g.interiorRect);
                    // Whole pixels, so that the mask draws one to one: a
                    // shape baked into it would blur under resampling.
                    g.imageRect =
                        Unit::Rect(0, 0, ceil(exteriorRect.width() + margin),
                                   ceil(exteriorRect.height() + margin));

                    LayoutUnit rx = g.paddingRect.x();
                    LayoutUnit ry = g.paddingRect.y();
                    g.rect = Unit::Rect(rx.floor(), ry.floor(),
                                        snapSizeToPixel(paddingWidth, rx),
                                        snapSizeToPixel(paddingHeight, ry));

                    // Only the padding box shows of the image, so that is
                    // the part that is kept, with a pixel around it for the
                    // sampling at its edges. The image is drawn from its
                    // device size onto imageRect; the crop is cut and drawn
                    // on that same scale.
                    g.imageWidth = ceil(g.imageRect.width() * dpr);
                    g.imageHeight = ceil(g.imageRect.height() * dpr);
                    g.scaleX = g.imageRect.width() / g.imageWidth;
                    g.scaleY = g.imageRect.height() / g.imageHeight;
                    g.crop = Unit::Rect(floor((g.rect.x() - dx) / g.scaleX) - 1,
                                        floor((g.rect.y() - dy) / g.scaleY) - 1,
                                        0, 0);
                    g.crop.setWidth(ceil((g.rect.maxX() - dx) / g.scaleX) + 1 -
                                    g.crop.x());
                    g.crop.setHeight(ceil((g.rect.maxY() - dy) / g.scaleY) + 1 -
                                     g.crop.y());
                    g.crop.intersect(
                        Unit::Rect(0, 0, g.imageWidth, g.imageHeight));
                    return g;
                };
                const Geometry real =
                    geometry(paddingRect.width(), paddingRect.height());
                const int iw = real.interiorRect.width();
                const int ih = real.interiorRect.height();

                BoxShadowNinePatch ninePatch(wv, true, shadowColor,
                                             shadow->radius());
                BorderRadiusFixedData radii(0, 0, 0, 0, 0, 0, 0, 0);
                if (hasFrameBorderRadius()) {
                    radii = computeFixedBorderRadius(
                        LayoutRect(real.interiorRect.x(), real.interiorRect.y(),
                                   iw, ih),
                        sd, true);
                }
                ninePatch.measure(radii);
                const bool wholePixels =
                    isWholePixels(shadow->offsetX(), shadow->offsetY(), sd);
                const bool fitsAcross = ninePatch.fitsAcross(iw);
                const bool fitsDown = ninePatch.fitsDown(ih);
                if (wholePixels && fitsAcross && fitsDown) {
                    LayoutUnit prx = paddingRect.x();
                    LayoutUnit pry = paddingRect.y();
                    Unit::Rect clipRect(
                        prx.floor(), pry.floor(),
                        snapSizeToPixel(paddingRect.width(), prx),
                        snapSizeToPixel(paddingRect.height(), pry));
                    canvas->save();
                    if (hasFrameBorderRadius()) {
                        applyBorderRadiusClippingIfNeeds(
                            canvas,
                            LayoutRect(clipRect.x(), clipRect.y(),
                                       clipRect.width(), clipRect.height()),
                            0, true);
                    } else {
                        canvas->clip(clipRect);
                    }
                    canvas->setNeedsNoneAntialias();

                    // The hole, where the full-size image below puts it.
                    const int reach = ninePatch.margin();
                    Unit::Rect outerRect(
                        real.interiorRect.x() +
                            std::min(shadow->offsetX(), 0.f) - reach,
                        real.interiorRect.y() +
                            std::min(shadow->offsetY(), 0.f) - reach,
                        iw + reach * 2, ih + reach * 2);
                    // Beyond the reach of the hole the shadow is solid: the
                    // padding box without the part of it the patch covers.
                    Unit::Rect covered = outerRect;
                    covered.intersect(clipRect);
                    if (!(covered == clipRect)) {
                        canvas->beginPath();
                        canvas->rect(clipRect);
                        if (covered.width() > 0 && covered.height() > 0) {
                            canvas->rect(covered);
                        }
                        canvas->setFillColor(shadowColor);
                        canvas->setFillRule(false);
                        canvas->fill();
                    }
                    // Only the padding box the patch is clipped to shows.
                    BoxShadowVisibility visibility;
                    visibility.clipTo(clipRect);
                    ninePatch.draw(canvas, outerRect, visibility);

                    canvas->restore();
                    ninePatch.finish();
                    continue;
                }

                // A strip when only one axis of the hole is too short: the
                // mask is that of a padding box whose hole is just long
                // enough for the caps along the other axis. The caps are
                // measured from the edges of the crop.
                int axis = wholePixels && !hasPercentBorderRadius(this)
                               ? shadowStripAxis(fitsAcross, fitsDown)
                               : -1;
                Geometry made = real;
                float capStart = 0;
                float capEnd = 0;
                if (axis >= 0) {
                    // With the fraction of the padding box's own length,
                    // so that the hole and the crop snap to pixels the way
                    // they do for the box.
                    const float length =
                        axis == 0 ? paddingRect.width() : paddingRect.height();
                    const float holeLength = ninePatch.capStart(axis) +
                                             ninePatch.capEnd(axis) +
                                             BoxShadowNinePatch::straightPart +
                                             length - floor(length);
                    made = axis == 0 ? geometry(holeLength + sd * 2,
                                                paddingRect.height())
                                     : geometry(paddingRect.width(),
                                                holeLength + sd * 2);
                    if (made.paddingRect.width() <= 0 ||
                        made.paddingRect.height() <= 0) {
                        axis = -1;
                        made = real;
                    }
                    // From the crop's start to the end of the start cap, in
                    // the same place in both masks; the end cap likewise
                    // from the crop's end.
                    if (axis == 0) {
                        capStart = made.interiorRect.x() +
                                   ninePatch.capStart(0) - dx -
                                   made.crop.x() * made.scaleX;
                        capEnd = made.crop.maxX() * made.scaleX -
                                 (made.interiorRect.maxX() -
                                  ninePatch.capEnd(0) - dx);
                    } else {
                        capStart = made.interiorRect.y() +
                                   ninePatch.capStart(1) - dy -
                                   made.crop.y() * made.scaleY;
                        capEnd = made.crop.maxY() * made.scaleY -
                                 (made.interiorRect.maxY() -
                                  ninePatch.capEnd(1) - dy);
                    }
                }

                Optional<BoxShadowImageKey> cacheKey = boxShadowImageKey(
                    this, *shadow, BoxShadowImageKind::Inset, dpr,
                    made.paddingRect.width() + borderWidth(),
                    made.paddingRect.height() + borderHeight());
                BufferedNativeImageData* nativeImage =
                    cacheKey ? wv->lookupBoxShadowImage(cacheKey.value())
                             : nullptr;
                bool cachedImage = nativeImage != nullptr;

                if (!nativeImage) {
                    nativeImage = blurredShadowMask(
                        wv, made.imageWidth, made.imageHeight,
                        shadow->radius() * dpr / 2, [&](Canvas* cv) {
                            // Draw an outline of Image
                            cv->beginPath();
                            cv->rect(made.imageRect);
                            cv->closePath();

                            cv->translate(half, half);

                            // Draw a shadow box that will not be filled.
                            const LayoutRect rect(made.interiorRect.x(),
                                                  made.interiorRect.y(),
                                                  made.interiorRect.width(),
                                                  made.interiorRect.height());
                            if (hasFrameBorderRadius()) {
                                // apply inner border radius line(anti-clock)
                                applyBorderRadius(cv, rect, sd, true);
                            } else {
                                cv->setFillRule(false);
                                cv->drawPixelSnappedRect(rect);
                            }
                            cv->fill();
                        });

                    BufferedNativeImageData* cropped =
                        BufferedNativeImageData::createAlphaMask(
                            made.crop.width(), made.crop.height());
                    const uint8_t* src =
                        nativeImage->data() +
                        (size_t)made.crop.y() * nativeImage->stride() +
                        (size_t)made.crop.x();
                    for (size_t row = 0; row < cropped->height(); row++) {
                        memcpy(cropped->data() + row * cropped->stride(),
                               src + row * nativeImage->stride(),
                               cropped->width());
                    }
                    delete nativeImage;
                    nativeImage = cropped;
                }

                canvas->save();
                if (hasFrameBorderRadius()) {
                    const LayoutRect r(real.rect.x(), real.rect.y(),
                                       real.rect.width(), real.rect.height());
                    applyBorderRadiusClippingIfNeeds(canvas, r, 0, true);
                } else {
                    canvas->clip(real.rect);
                }

                canvas->translate(dx, dy);
                const Unit::Rect dst(real.crop.x() * real.scaleX,
                                     real.crop.y() * real.scaleY,
                                     real.crop.width() * real.scaleX,
                                     real.crop.height() * real.scaleY);
                if (axis < 0) {
                    canvas->fillWithImageAlpha(
                        nativeImage,
                        Unit::Rect(0, 0, nativeImage->width(),
                                   nativeImage->height()),
                        dst, shadowColor);
                } else {
                    canvas->setNeedsNoneAntialias();
                    drawShadowMaskStrip(canvas, nativeImage, dst, axis,
                                        capStart, capEnd, made.scaleX,
                                        made.scaleY, shadowColor);
                }

                canvas->restore();

                if (cachedImage) {
                } else if (cacheKey) {
                    wv->storeBoxShadowImage(cacheKey.value(), nativeImage);
                } else {
                    delete nativeImage;
                }
            }
        }
        list.clear();
        canvas->restore();
    }
}

Unit::Rect FrameBox::makeRect(BoxValue box)
{
    float x, y, w, h;

    switch (box) {
    case BoxValue::BorderBoxBoxValue:
        x = 0;
        y = 0;
        w = width();
        h = height();
        break;
    case BoxValue::PaddingBoxBoxValue:
        x = borderLeft();
        y = borderTop();
        w = width() - borderWidth();
        h = height() - borderHeight();
        break;
    case BoxValue::ContentBoxBoxValue:
        x = paddingLeft() + borderLeft();
        y = paddingTop() + borderTop();
        w = contentWidth();
        h = contentHeight();
        break;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return Unit::Rect(x, y, w, h);
}

void FrameBox::paintBackground(Canvas* canvas, FrameBox* box,
                               HTMLElement* rootOrBodyelement)
{
#ifndef NDEBUG
    if (!box) {
        STARFISH_ASSERT(rootOrBodyelement &&
                        (rootOrBodyelement->isHTMLHtmlElement() ||
                         rootOrBodyelement->isHTMLBodyElement()));
    }
#endif
    ComputedStyle* style;
    if (box) {
        style = box->style();
    } else {
        style = rootOrBodyelement->style();
    }
    FrameBox rootBox(rootOrBodyelement, style);

    bool isRootOrBodyElementNeedsInCompositeState = false;
    if (rootOrBodyelement) {
        isRootOrBodyElementNeedsInCompositeState =
            rootOrBodyelement->document()
                ->browsingContext()
                ->rootStackingContextNeedsGraphicsBuffer();
        Document* domDocument = rootOrBodyelement->document();
        HTMLHtmlElement* root;
        if (rootOrBodyelement->isHTMLHtmlElement()) {
            root = rootOrBodyelement->asHTMLHtmlElement();
        } else {
            root = rootOrBodyelement->document()->rootElement();
        }

        rootBox.copyFrom(root->frame()->asFrameBox(), FrameBox::BorderBoxCopy);
        box = &rootBox;

        FrameDocument* document =
            rootOrBodyelement->document()->frame()->asFrameDocument();
        LayoutLocation loc =
            root->frame()->asFrameBox()->absolutePoint(document);
        box->setX(loc.x());
        box->setY(loc.y());

        if (isRootOrBodyElementNeedsInCompositeState) {
            box->setX(0);
            box->setY(0);
            box->setWidth(domDocument->window()->scrollWidth(false));
            box->setHeight(domDocument->window()->scrollHeight(false));
        }
    }

    if (!style->backgroundColor().isTransparent() &&
        style->visibility() == VisibilityValue::VisibleVisibilityValue) {
        if (rootOrBodyelement && isRootOrBodyElementNeedsInCompositeState) {
            // skip painting. compositor will draw color
        } else if (box->stackingContext() &&
                   box->stackingContext()->inScrollWithGraphicsBufferActive()) {
            // skip painting. compositor will draw color
        } else if (box->stackingContext() &&
                   box->stackingContext()->owner() == box &&
                   box->stackingContext()
                       ->isOwnerBackgroundDrawnByCompositor()) {
            // skip painting. compositor draws bg-color before tiles
        } else {
            canvas->save();
            Unit::Rect paintingRect;
            if (rootOrBodyelement) {
                Window* window = rootOrBodyelement->window();
                FrameDocument* doc =
                    window->document()->frame()->asFrameDocument();
                paintingRect =
                    Unit::Rect(doc->scrollLeft(), doc->scrollTop(),
                               window->innerWidth(), window->innerHeight());
            } else {
                unsigned int idx = style->backgroundLayerSize() - 1;
                paintingRect = box->makeRect(style->backgroundClip(idx));
            }
            canvas->setFillColor(style->backgroundColor());
            if (box->hasFrameBorderRadius()) {
                box->applyBorderRadius(
                    canvas,
                    LayoutRect(paintingRect.x(), paintingRect.y(),
                               paintingRect.width(), paintingRect.height()));
                canvas->fill();
            } else {
                canvas->drawPixelSnappedRect(
                    LayoutRect(paintingRect.x(), paintingRect.y(),
                               paintingRect.width(), paintingRect.height()));
            }
            canvas->restore();
        }
    }
    paintBackgroundLayers(canvas, box, rootOrBodyelement, style);
}

void FrameBox::paintGradient(Canvas* canvas, FrameBox* box,
                             ImageValue* imageValue, Unit::Rect dst,
                             const float& width, const float& height,
                             bool repeatX, bool repeatY,
                             ImageRenderingValue imageRenderingValue)
{
    if (width == 0 || height == 0) {
        return;
    }

    float startX = dst.x();
    float startY = dst.y();
    // dst is offset by the image position inside the painting area, which
    // starts at 0. Repeated tiles must reach the end of the painting area even
    // when the position is negative.
    float endX = dst.maxX();
    float endY = dst.maxY();

    if (repeatX) {
        startX = fmodf(startX, width);
        if (startX > 0) {
            startX -= width;
        }
        endX = std::max(endX, dst.width());
    }
    if (repeatY) {
        startY = fmodf(startY, height);
        if (startY > 0) {
            startY -= height;
        }
        endY = std::max(endY, dst.height());
    }
    auto value = imageValue->gradientValue();
    bool cacheable = value->isCacheable(width, height, true) &&
                     value->isCacheable(dst.width(), dst.height(), true);

    if (cacheable) {
        float imageWidth = width;
        float imageHeight = height;
        Unit::Rect rect = Unit::Rect(0, 0, width, height).snapSizeToPixel();
        GradientDrawingInfo* info = value->makeGradientDrawingInfo(rect, box);
        NativeGradient* gradient =
            box->document()->findInNativeGradientCache(info);
        std::unique_ptr<NativeGradient> newGradient;
        if (gradient == nullptr) {
            newGradient = NativeGradient::create(info);
            gradient = newGradient.get();
        }

        // if we can shrink result image, shrink!
        if (info->type == GradientType::LinearGradient) {
            if (fmodf(info->computedAngle, 180.0) == 0) {
                imageWidth = 1;
                repeatX = true;
            } else if (fmodf(info->computedAngle, 90.0) == 0) {
                imageHeight = 1;
                repeatY = true;
            }
        }

        WebView* wv = box->node()->webView();
        float imageScale = 1;
#if !defined(STARFISH_ENABLE_TEST)
#define STARFISH_NATIVEGRADIENT_MAX_SIZE 512
        while ((imageWidth / imageScale) > STARFISH_NATIVEGRADIENT_MAX_SIZE &&
               (imageHeight / imageScale) > STARFISH_NATIVEGRADIENT_MAX_SIZE) {
            imageRenderingValue = ImageRenderingPixelatedValue;
            imageScale += 0.25f;
        }
#endif

        if (gradient->gradientImageDataCached() == nullptr) {
            float w = floor(imageWidth / imageScale *
                            wv->screenInfo().devicePixelRatio);
            if (!w) {
                w = 1;
            }
            float h = floor(imageHeight / imageScale *
                            wv->screenInfo().devicePixelRatio);
            if (!h) {
                h = 1;
            }
            auto imageData = BufferedNativeImageData::create(w, h);
            Canvas* cv = Canvas::create(wv, imageData);
            cv->scale(1 / imageScale, 1 / imageScale);

            cv->clearColor(Unit::Color(0, 0, 0, 0));
            if (imageValue->gradientValue()->type() ==
                GradientType::LinearGradient) {
                cv->drawLinearGradient(rect, info, gradient);
            } else if (imageValue->gradientValue()->type() ==
                       GradientType::RadialGradient) {
                cv->drawRadialGradient(rect, info, gradient);
            }
            cv->fill();
            delete cv;

            gradient->setGradientImageDataCached(imageData);
            box->document()->cacheNativeGradient(info, newGradient);
        }

        Unit::Rect drawRect =
            Unit::Rect(startX, startY, width, height).snapSizeToPixel();
        canvas->save();
        for (float y = startY; y < endY;
             y += height, canvas->translate(0, drawRect.height())) {
            canvas->save();
            for (float x = startX; x < endX;
                 x += width, canvas->translate(drawRect.width(), 0)) {
                canvas->drawImage(gradient->gradientImageDataCached(), drawRect,
                                  imageRenderingValue);
            }
            canvas->restore();
        }
        canvas->restore();
    } else {
        Unit::Rect rect =
            Unit::Rect(startX, startY, width, height).snapSizeToPixel();
        GradientDrawingInfo* info = value->makeGradientDrawingInfo(rect, box);
        std::unique_ptr<NativeGradient> gradient = NativeGradient::create(info);
        canvas->save();
        for (float y = startY; y < endY;
             y += height, canvas->translate(0, rect.height())) {
            canvas->save();
            for (float x = startX; x < endX;
                 x += width, canvas->translate(rect.width(), 0)) {
                if (imageValue->gradientValue()->type() ==
                    GradientType::LinearGradient) {
                    canvas->drawLinearGradient(rect, info, gradient.get());
                } else if (imageValue->gradientValue()->type() ==
                           GradientType::RadialGradient) {
                    canvas->drawRadialGradient(rect, info, gradient.get());
                }
            }
            canvas->restore();
        }
        canvas->restore();
    }
}

void FrameBox::paintBackgroundLayers(Canvas* canvas, FrameBox* box,
                                     NULLABLE HTMLElement* rootOrBodyelement,
                                     ComputedStyle* style)
{
    STARFISH_ASSERT(canvas != nullptr);
    STARFISH_ASSERT(box != nullptr);
    STARFISH_ASSERT(style != nullptr);

    ImageRenderingValue imageRenderingValue = style->imageRendering();

    auto backgroundLayerSize = style->backgroundLayerSize();
    for (unsigned int i = 0; i < backgroundLayerSize; i++) {
        unsigned int idx = backgroundLayerSize - i - 1;

        if (style->backgroundImage(idx) == nullptr) {
            continue;
        }
        auto type = style->backgroundImage(idx)->type();

        NativeImageData* id = nullptr;
        float width = 0;
        float height = 0;
        if (type.isURL()) {
            STARFISH_ASSERT(style->background() != nullptr);
            ImageResource* ir = style->background()->imageResource(idx);
            if (box->node() != nullptr && ir != nullptr) {
                box->node()->webView()->putURLIntoActiveImageURLsInRenderingSet(
                    ir->url()->urlString()->toUTF8NonGCString());
            }

            id = style->backgroundImageData(idx);
            if (id == nullptr || id->width() == 0 || id->height() == 0) {
                return;
            }

            if (id->isSVGNativeImageData() &&
                !id->asSVGNativeImageData()->hasViewport()) {
                id->asSVGNativeImageData()->updateContentSize(box);
            }

            width = id->width();
            height = id->height();
        } else if (type.isGradient()) {
            ImageValue* imageValue = style->backgroundImage(idx);
            if (!imageValue->gradientValue()->isEffective()) {
                continue;
            }

            Unit::Rect rect;
            if (rootOrBodyelement != nullptr) {
                Window* window = rootOrBodyelement->window();
                FrameDocument* doc =
                    window->document()->frame()->asFrameDocument();
                rect = Unit::Rect(doc->scrollLeft(), doc->scrollTop(),
                                  window->innerWidth(), window->innerHeight());
                if (rootOrBodyelement->webView()
                        ->rootStackingContext()
                        ->needsGraphicsBuffer()) {
                    rect = Unit::Rect(0, 0, doc->scrollWidth(),
                                      doc->scrollHeight());
                }
            } else {
                unsigned int idx = style->backgroundLayerSize() - 1;
                rect = box->makeRect(BoxValue::PaddingBoxBoxValue);
            }
            width = rect.width();
            height = rect.height();
        } else {
            continue;
        }

        Unit::Rect paintingRect;
        Unit::Rect positioningRect;
        BackgroundAttachmentValue attachment = style->backgroundAttachment(idx);
        if (rootOrBodyelement != nullptr) {
            Window* window = rootOrBodyelement->window();
            FrameDocument* doc = window->document()->frame()->asFrameDocument();
            paintingRect =
                Unit::Rect(doc->scrollLeft(), doc->scrollTop(),
                           window->innerWidth(), window->innerHeight());
            if (rootOrBodyelement->webView()
                    ->rootStackingContext()
                    ->needsGraphicsBuffer()) {
                paintingRect =
                    Unit::Rect(0, 0, doc->scrollWidth(), doc->scrollHeight());
            }
        }

        if (attachment == FixedBackgroundAttachmentValue) {
            FrameDocument* doc = box->document()->frame()->asFrameDocument();
            LayoutLocation loc;
            if (rootOrBodyelement == nullptr) {
                loc = box->absolutePoint(doc);
            }
            positioningRect = doc->makeRect(style->backgroundOrigin(idx));
            positioningRect.setX(-loc.x().toFloat() + doc->scrollLeft());
            positioningRect.setY(-loc.y().toFloat() + doc->scrollTop());
            if (rootOrBodyelement == nullptr) {
                paintingRect = box->makeRect(style->backgroundClip(idx));
            }
        } else if (attachment == LocalBackgroundAttachmentValue &&
                   box->isFrameBlockBox()) {
            FrameBox scrollBox(box->node(), style);
            scrollBox.copyFrom(box,
                               FrameBox::BorderCopy | FrameBox::PaddingCopy);
            scrollBox.setWidth(box->asFrameBlockBox()->scrollWidth());
            scrollBox.setHeight(box->asFrameBlockBox()->scrollHeight());
            positioningRect = scrollBox.makeRect(style->backgroundOrigin(idx));
            positioningRect.setX(positioningRect.x() -
                                 box->asFrameBlockBox()->scrollLeft());
            positioningRect.setY(positioningRect.x() -
                                 box->asFrameBlockBox()->scrollTop());
            if (rootOrBodyelement != nullptr) {
                positioningRect.setX(positioningRect.x() + box->x());
                positioningRect.setY(positioningRect.y() + box->y());
            } else {
                paintingRect = scrollBox.makeRect(style->backgroundClip(idx));
            }
        } else {
            positioningRect = box->makeRect(style->backgroundOrigin(idx));

            if (rootOrBodyelement != nullptr) {
                positioningRect.setX(positioningRect.x() + box->x());
                positioningRect.setY(positioningRect.y() + box->y());
            } else {
                paintingRect = box->makeRect(style->backgroundClip(idx));
            }
        }
        canvas->save();
        canvas->translate(paintingRect.x(), paintingRect.y());
        canvas->clip(
            Unit::Rect(0, 0, paintingRect.width(), paintingRect.height()));

        float positionW = positioningRect.width();
        float positionH = positioningRect.height();
        float paintingW = paintingRect.width();
        float paintingH = paintingRect.height();
        float imgW = positionW;
        float imgH = positionH;

        float boxR = positionW / positionH;
        float imgR = width / height;
        float hasSpecifiedSize = false;

        if (style->backgroundSizeIsLength(idx)) {
            LengthSize bgSize = style->backgroundSizeLengthValue(idx);
            if (bgSize.width().isAuto() && bgSize.height().isAuto()) {
                imgW = width;
                imgH = height;
            } else if (bgSize.width().isAuto() && !bgSize.height().isAuto()) {
                hasSpecifiedSize = true;
                imgH = bgSize.height().specifiedValue(positionH, box);
                imgW = imgH * width / height;
            } else if (!bgSize.width().isAuto() && bgSize.height().isAuto()) {
                hasSpecifiedSize = true;
                imgW = bgSize.width().specifiedValue(positionW, box);
                imgH = imgW * height / width;
            } else {
                hasSpecifiedSize = true;
                imgW = bgSize.width().specifiedValue(positionW, box);
                imgH = bgSize.height().specifiedValue(positionH, box);
            }
        } else {
            BackgroundSizeValue bgSize = style->backgroundSizeTypeValue(idx);
            if (bgSize == BackgroundSizeValue::CoverBackgroundSizeValue) {
                if (boxR < imgR) {
                    imgW = positionH * imgR;
                } else {
                    imgH = positionW / imgR;
                }
            } else {
                STARFISH_ASSERT(
                    bgSize == BackgroundSizeValue::ContainBackgroundSizeValue);
                if (boxR > imgR) {
                    imgW = positionH * imgR;
                } else {
                    imgH = positionW / imgR;
                }
            }
        }

        Length positionX = style->backgroundPositionX(idx);
        Length positionY = style->backgroundPositionY(idx);
        LayoutUnit x = positionX.specifiedValue(positionW - imgW, box) +
                       positioningRect.x() - paintingRect.x();
        LayoutUnit y = positionY.specifiedValue(positionH - imgH, box) +
                       positioningRect.y() - paintingRect.y();

        auto repeatX = style->backgroundRepeatX(idx);
        auto repeatY = style->backgroundRepeatY(idx);

        bool shouldApplyRepeat = type.isGradient() ? hasSpecifiedSize : true;

        if (shouldApplyRepeat &&
            (repeatX == RepeatStyleValue::RepeatRepeatValue &&
             repeatY == RepeatStyleValue::RepeatRepeatValue)) {
            if (type.isURL()) {
                if (positioningRect.x() == paintingRect.x() &&
                    positioningRect.y() == paintingRect.y() &&
                    paintingW == imgW && paintingH == imgH) {
                    canvas->drawImage(id, Unit::Rect(x, y, imgW, imgH),
                                      imageRenderingValue);
                } else {
                    canvas->drawRepeatImage(
                        id, Unit::Rect(x, y, paintingW, paintingH), imgW, imgH,
                        true, true, imageRenderingValue);
                }
            } else if (type.isGradient()) {
                paintGradient(canvas, box, style->backgroundImage(idx),
                              Unit::Rect(x, y, paintingW, paintingH), imgW,
                              imgH, true, true, imageRenderingValue);
            }
        } else if (shouldApplyRepeat &&
                   repeatX == RepeatStyleValue::NoRepeatRepeatValue &&
                   repeatY == RepeatStyleValue::RepeatRepeatValue) {
            if (type.isURL()) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, imgW, paintingH),
                                        imgW, imgH, false, true,
                                        imageRenderingValue);
            } else if (type.isGradient()) {
                paintGradient(canvas, box, style->backgroundImage(idx),
                              Unit::Rect(x, y, imgW, paintingH), imgW, imgH,
                              false, true, imageRenderingValue);
            }

        } else if (shouldApplyRepeat &&
                   repeatX == RepeatStyleValue::RepeatRepeatValue &&
                   repeatY == RepeatStyleValue::NoRepeatRepeatValue) {
            if (type.isURL()) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, paintingW, imgH),
                                        imgW, imgH, true, false,
                                        imageRenderingValue);
            } else if (type.isGradient()) {
                paintGradient(canvas, box, style->backgroundImage(idx),
                              Unit::Rect(x, y, paintingW, imgH), imgW, imgH,
                              true, false, imageRenderingValue);
            }
        } else {
            if (type.isURL()) {
                canvas->drawImage(id, Unit::Rect(x, y, imgW, imgH),
                                  imageRenderingValue);
            } else if (type.isGradient()) {
                paintGradient(canvas, box, style->backgroundImage(idx),
                              Unit::Rect(x, y, imgW, imgH), imgW, imgH, false,
                              false, imageRenderingValue);
            }
        }
        canvas->restore();
    }
}

void FrameBox::paintDottedLine(Canvas* canvas, const LayoutLocation& p1,
                               const LayoutLocation& p2,
                               const LayoutLocation& p3,
                               const LayoutLocation& p4,
                               const LayoutLocation& p5,
                               const LayoutLocation& p6, BoxSide side)
{
    LayoutUnit x1, x2, y1, y2;
    LayoutUnit width;
    if (side == TopSide) {
        width = borderTop();
        x1 = p1.x() + width / 2;
        y1 = p1.y() + width / 2;
        x2 = p5.x() - width / 2;
        y2 = p5.y() + width / 2;
    } else if (side == RightSide) {
        width = borderRight();
        x1 = p1.x() - width / 2;
        y1 = p1.y() + width / 2;
        x2 = p5.x() - width / 2;
        y2 = p5.y() - width / 2;
    } else if (side == BottomSide) {
        width = borderBottom();
        x1 = p1.x() + width / 2;
        y1 = p1.y() - width / 2;
        x2 = p5.x() - width / 2;
        y2 = p5.y() - width / 2;
    } else {
        width = borderLeft();
        x1 = p1.x() + width / 2;
        y1 = p1.y() + width / 2;
        x2 = p5.x() + width / 2;
        y2 = p5.y() - width / 2;
    }

    LayoutUnit dist =
        sqrt(float((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))) -
        (float(width * 2.0f));
    LayoutUnit numberOf = dist / (width * 2);

    size_t interval = numberOf.toDouble();
    if (dist < width) {
        interval = 0;
    }

    if (dist + (width * 2.0f) < width) {
        return;
    }
    interval += 1;

    canvas->save();

    LayoutUnit x, y;

    double ratio = 0.0f;
    for (size_t i = 0; i <= interval; i++) {
        x = (1.0f - ratio) * x1 + ratio * x2;
        y = (1.0f - ratio) * y1 + ratio * y2;
        canvas->arc(x, y, width / 2, 0.0, 2 * M_PI);
        canvas->fill();
        ratio += (1.0f / interval);
    }

    canvas->restore();
}

void FrameBox::paintDashedLine(Canvas* canvas, const LayoutLocation& p1,
                               const LayoutLocation& p2,
                               const LayoutLocation& p3,
                               const LayoutLocation& p4,
                               const LayoutLocation& p5,
                               const LayoutLocation& p6, BoxSide side)
{
    GCAtomicVector<double> dashes;
    dashes.emplace_back(2.0);
    dashes.emplace_back(1.0);

    int ndash = sizeof(dashes) / sizeof(dashes[0]);
    double offset = 0.0;

    LayoutUnit x1, x2, y1, y2;
    LayoutUnit width;
    if (side == TopSide) {
        width = borderTop();
        x1 = p2.x();
        y1 = p2.y() + width / 2;
        x2 = p4.x();
        y2 = p4.y() + width / 2;
    } else if (side == RightSide) {
        width = borderRight();
        x1 = p2.x() - width / 2;
        y1 = p2.y();
        x2 = p4.x() - width / 2;
        y2 = p4.y();
    } else if (side == BottomSide) {
        width = borderBottom();
        x1 = p2.x();
        y1 = p2.y() - width / 2;
        x2 = p4.x();
        y2 = p4.y() - width / 2;
    } else {
        width = borderLeft();
        x1 = p2.x() + width / 2;
        y1 = p2.y();
        x2 = p4.x() + width / 2;
        y2 = p4.y();
    }

    dashes[0] *= width.toDouble();
    dashes[1] *= width.toDouble();

    canvas->drawRect(p1, p2, p3, p3);
    canvas->save();
    canvas->setDash(dashes);
    canvas->setDashOffset(offset);
    canvas->setLineWidth(width.toFloat());
    canvas->moveTo(x1.toDouble(), y1.toDouble());
    canvas->lineTo(x2.toDouble(), y2.toDouble());
    canvas->stroke();
    canvas->restore();
    canvas->drawRect(p4, p5, p6, p6);
}

// Draws the border around the area defined by "rect"
void FrameBox::paintBorders(Canvas* canvas, const LayoutRect& rect)
{
    canvas->save();

    // draw border-image
    BorderData border = style()->border();
    if (border.hasBorderImageData()) {
        // Draw image borders at the four corners as shown below.
        //   ______________
        //  |_|          |_|
        //  |              |
        //  |              |
        //  |_            _|
        //  |_|__________|_|
        //
        double bLWidth = border.left().width().specifiedValue(width(), this);
        double bTWidth = border.top().width().specifiedValue(height(), this);
        double bRWidth = border.right().width().specifiedValue(width(), this);
        double bBWidth = border.bottom().width().specifiedValue(height(), this);

        double bLOutset =
            border.image().outsets().left().computedBorderImageOutset(bLWidth,
                                                                      this);
        double bTOutset =
            border.image().outsets().top().computedBorderImageOutset(bTWidth,
                                                                     this);
        double bROutset =
            border.image().outsets().right().computedBorderImageOutset(bRWidth,
                                                                       this);
        double bBOutset =
            border.image().outsets().bottom().computedBorderImageOutset(bBWidth,
                                                                        this);

        LayoutRect outsetRect = rect;
        outsetRect.setX(outsetRect.x() - bLOutset);
        outsetRect.setY(outsetRect.y() - bTOutset);
        outsetRect.setWidth(outsetRect.width() + bLOutset + bROutset);
        outsetRect.setHeight(outsetRect.height() + bTOutset + bBOutset);

        size_t imgWidth = border.image().imageData()->width();
        size_t imgHeight = border.image().imageData()->height();

        size_t lSlice = std::min(
            (size_t)border.image().slices().left().computedBorderImageSlice(
                imgWidth, this),
            imgWidth);
        size_t tSlice = std::min(
            (size_t)border.image().slices().top().computedBorderImageSlice(
                imgHeight, this),
            imgHeight);
        size_t rSlice = std::min(
            (size_t)border.image().slices().right().computedBorderImageSlice(
                imgWidth, this),
            imgWidth);
        size_t bSlice = std::min(
            (size_t)border.image().slices().bottom().computedBorderImageSlice(
                imgHeight, this),
            imgHeight);

        double bImgLWidth =
            border.image().widths().left().computeBorderImageWidth(
                width(), bLWidth, lSlice, this);
        double bImgTWidth =
            border.image().widths().top().computeBorderImageWidth(
                height(), bTWidth, tSlice, this);
        double bImgRWidth =
            border.image().widths().right().computeBorderImageWidth(
                width(), bRWidth, rSlice, this);
        double bImgBWidth =
            border.image().widths().bottom().computeBorderImageWidth(
                height(), bBWidth, bSlice, this);

        double value =
            std::min((float)outsetRect.width() / (bImgLWidth + bImgRWidth),
                     (float)outsetRect.height() / (bImgTWidth + bImgBWidth));
        if (value < 1) {
            bImgLWidth *= value;
            bImgRWidth *= value;
            bImgTWidth *= value;
            bImgBWidth *= value;
        }

        NativeImageData* imgData = border.image().imageData();
        canvas->setNeedsGoodQualityAntialias();

        ImageRenderingValue imageRenderingValue = style()->imageRendering();

        DrawImageInfo borderinfo = { 1.0, 1.0,
                                     BorderImageRepeatValue::StretchValue,
                                     BorderImageRepeatValue::StretchValue };
        // Four Corners
        // left-top
        if ((lSlice > 0 && bImgLWidth > 0) && (tSlice > 0 && bImgTWidth > 0)) {
            canvas->drawImage(imgData, Unit::Rect(0, 0, lSlice, tSlice),
                              Unit::Rect(outsetRect.x(), outsetRect.y(),
                                         bImgLWidth, bImgTWidth),
                              borderinfo, imageRenderingValue);
        }
        // right-top
        if ((rSlice > 0 && bImgRWidth > 0) && (tSlice > 0 && bImgTWidth > 0)) {
            canvas->drawImage(
                imgData, Unit::Rect(imgWidth - rSlice, 0, rSlice, tSlice),
                Unit::Rect(outsetRect.x() + outsetRect.width() - bImgRWidth,
                           outsetRect.y(), bImgRWidth, bImgTWidth),
                borderinfo, imageRenderingValue);
        }
        // left-bottom
        if ((lSlice > 0 && bImgLWidth > 0) && (bSlice > 0 && bImgBWidth > 0)) {
            canvas->drawImage(
                imgData, Unit::Rect(0, imgHeight - bSlice, lSlice, bSlice),
                Unit::Rect(outsetRect.x(),
                           outsetRect.y() + outsetRect.height() - bImgBWidth,
                           bImgLWidth, bImgBWidth),
                borderinfo, imageRenderingValue);
        }

        // right-bottom
        if ((rSlice > 0 && bImgRWidth > 0) && (bSlice > 0 && bImgBWidth > 0)) {
            canvas->drawImage(
                imgData,
                Unit::Rect(imgWidth - rSlice, imgHeight - bSlice, rSlice,
                           bSlice),
                Unit::Rect(outsetRect.x() + outsetRect.width() - bImgRWidth,
                           outsetRect.y() + outsetRect.height() - bImgBWidth,
                           bImgRWidth, bImgBWidth),
                borderinfo, imageRenderingValue);
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Four Edges
        if (lSlice + rSlice < imgWidth) {
            // middle-top
            borderinfo.hScale = tSlice / bImgTWidth;
            borderinfo.vScale = tSlice / bImgTWidth;
            borderinfo.hRepeat = border.image().repeatX();
            borderinfo.vRepeat = BorderImageRepeatValue::StretchValue;
            if ((tSlice > 0 && bImgTWidth > 0) &&
                (imgWidth - (lSlice + rSlice) > 0)) {
                canvas->drawImage(
                    imgData,
                    Unit::Rect(lSlice, 0, imgWidth - (lSlice + rSlice), tSlice),
                    Unit::Rect(outsetRect.x() + bImgLWidth, outsetRect.y(),
                               outsetRect.width() - (bImgLWidth + bImgRWidth),
                               bImgTWidth),
                    borderinfo, imageRenderingValue);
            }
            // middle-bottom
            borderinfo.hScale = bSlice / bImgBWidth;
            borderinfo.vScale = bSlice / bImgBWidth;
            if ((bSlice > 0 && bImgBWidth > 0) &&
                (imgWidth - (lSlice + rSlice) > 0)) {
                canvas->drawImage(
                    imgData,
                    Unit::Rect(lSlice, imgHeight - bSlice,
                               imgWidth - (lSlice + rSlice), bSlice),
                    Unit::Rect(outsetRect.x() + bImgLWidth,
                               outsetRect.y() + outsetRect.height() -
                                   bImgBWidth,
                               outsetRect.width() - (bImgLWidth + bImgRWidth),
                               bImgBWidth),
                    borderinfo, imageRenderingValue);
            }
        }

        if (tSlice + bSlice < imgHeight) {
            // left-middle
            borderinfo.hScale = lSlice / bImgLWidth;
            borderinfo.vScale = lSlice / bImgLWidth;
            borderinfo.hRepeat = BorderImageRepeatValue::StretchValue;
            borderinfo.vRepeat = border.image().repeatY();
            if ((lSlice > 0 && bImgLWidth > 0) &&
                (imgHeight - (tSlice + bSlice) > 0)) {
                canvas->drawImage(
                    imgData,
                    Unit::Rect(0, tSlice, lSlice,
                               imgHeight - (tSlice + bSlice)),
                    Unit::Rect(outsetRect.x(), outsetRect.y() + bImgTWidth,
                               bImgLWidth,
                               outsetRect.height() - (bImgTWidth + bImgBWidth)),
                    borderinfo, imageRenderingValue);
            }

            // right-middle
            borderinfo.hScale = rSlice / bImgRWidth;
            borderinfo.vScale = rSlice / bImgRWidth;
            if ((rSlice > 0 && bImgRWidth > 0) &&
                (imgHeight - (tSlice + bSlice) > 0)) {
                canvas->drawImage(
                    imgData,
                    Unit::Rect(imgWidth - rSlice, tSlice, rSlice,
                               imgHeight - (tSlice + bSlice)),
                    Unit::Rect(outsetRect.x() + outsetRect.width() - bImgRWidth,
                               outsetRect.y() + bImgTWidth, bImgRWidth,
                               outsetRect.height() - (bImgTWidth + bImgBWidth)),
                    borderinfo, imageRenderingValue);
            }
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // One Middle
        if (border.image().sliceFill() && lSlice + rSlice < imgWidth &&
            tSlice + bSlice < imgHeight) {
            // middle-middle
            borderinfo.hScale = 1.0;
            borderinfo.vScale = 1.0;
            borderinfo.hRepeat = border.image().repeatX();
            borderinfo.vRepeat = border.image().repeatY();
            if (tSlice > 0 && bImgTWidth > 0) {
                borderinfo.hScale = tSlice / bImgTWidth;
            } else if (bSlice > 0 && bImgBWidth > 0) {
                borderinfo.hScale = bSlice / bImgBWidth;
            }
            if (lSlice > 0 && bImgLWidth > 0) {
                borderinfo.vScale = lSlice / bImgLWidth;
            } else if (rSlice > 0 && bImgRWidth > 0) {
                borderinfo.vScale = rSlice / bImgRWidth;
            }
            canvas->drawImage(
                imgData,
                Unit::Rect(lSlice, tSlice, imgWidth - (lSlice + rSlice),
                           imgHeight - (tSlice + bSlice)),
                Unit::Rect(outsetRect.x() + bImgLWidth,
                           outsetRect.y() + bImgTWidth,
                           outsetRect.width() - (bImgLWidth + bImgRWidth),
                           outsetRect.height() - (bImgTWidth + bImgBWidth)),
                borderinfo, imageRenderingValue);
        }

        canvas->setNeedsFastAntialias();
    } else if (border.hasBorderStyle()) {
        if (hasFrameBorderRadius()) {
            LayoutRect borderRect = rect;
            borderRect = rect.snapSizeToPixel();

            BorderRadiusData br = frameBorderRadius();
            BorderRadiusFixedData fixed(br, width(), height(), this);
            reduceBorderRadiusToFit(borderRect, fixed);

            const float topLeftVertical = fixed.m_topLeftVertical;
            const float topLeftHorizontal = fixed.m_topLeftHorizontal;
            const float topRightHorizontal = fixed.m_topRightHorizontal;
            const float topRightVertical = fixed.m_topRightVertical;
            const float bottomLeftVertical = fixed.m_bottomLeftVertical;
            const float bottomLeftHorizontal = fixed.m_bottomLeftHorizontal;
            const float bottomRightHorizontal = fixed.m_bottomRightHorizontal;
            const float bottomRightVertical = fixed.m_bottomRightVertical;

            float x, y, arcR;

            {
                canvas->setFillColor(border.left().color());

                if (topLeftHorizontal && topLeftVertical) {
                    canvas->save();
                    canvas->translate(
                        topLeftHorizontal + borderRect.x().toFloat(),
                        topLeftVertical + borderRect.y().toFloat());
                    if (topLeftVertical > topLeftHorizontal) {
                        canvas->scale(1 * (topLeftHorizontal / topLeftVertical),
                                      1);
                        arcR = topLeftVertical;
                    } else {
                        canvas->scale(
                            1, 1 * (topLeftVertical / topLeftHorizontal));
                        arcR = topLeftHorizontal;
                    }
                    canvas->arc(0, 0, arcR, M_PI, M_PI + M_PI / 4);
                    canvas->restore();

                    if (topLeftHorizontal > borderLeft() &&
                        topLeftVertical > borderTop()) {
                        canvas->save();
                        canvas->translate(
                            topLeftHorizontal + borderRect.x().toFloat(),
                            topLeftVertical + borderRect.y().toFloat());
                        float newHorizontal = topLeftHorizontal - borderLeft();
                        float newVertical = topLeftVertical - borderTop();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }
                        canvas->arcNegative(0, 0, arcR, M_PI + M_PI / 4, M_PI);
                        canvas->restore();
                    } else {
                        canvas->lineTo(borderRect.x() + borderLeft(),
                                       borderRect.y() + borderTop());
                    }
                } else {
                    canvas->moveTo(borderRect.x(), borderRect.y());
                    canvas->lineTo(borderRect.x() + borderLeft(),
                                   borderRect.y() + borderTop());
                }

                if (bottomLeftHorizontal && bottomLeftVertical) {
                    if (bottomLeftHorizontal > borderLeft() &&
                        bottomLeftVertical > borderBottom()) {
                        canvas->save();
                        canvas->translate(borderRect.x() + bottomLeftHorizontal,
                                          borderRect.maxY() -
                                              bottomLeftVertical);

                        float newHorizontal =
                            bottomLeftHorizontal - borderLeft();
                        float newVertical = bottomLeftVertical - borderBottom();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }

                        canvas->arcNegative(0, 0, arcR, M_PI,
                                            M_PI - M_PI / 2 + M_PI / 4);
                        canvas->restore();
                    } else {
                        canvas->lineTo(borderRect.x() + borderLeft(),
                                       borderRect.maxY() - borderBottom());
                    }

                    canvas->save();
                    canvas->translate(borderRect.x() + bottomLeftHorizontal,
                                      borderRect.maxY() - bottomLeftVertical);

                    if (bottomLeftVertical > bottomLeftHorizontal) {
                        canvas->scale(
                            1 * (bottomLeftHorizontal / bottomLeftVertical), 1);
                        arcR = bottomLeftVertical;
                    } else {
                        canvas->scale(
                            1, 1 * (bottomLeftVertical / bottomLeftHorizontal));
                        arcR = bottomLeftHorizontal;
                    }

                    canvas->arc(0, 0, arcR, M_PI - M_PI / 2 + M_PI / 4, M_PI);
                    canvas->restore();

                    x = borderRect.x();
                    y = borderRect.y() + bottomLeftVertical;
                    canvas->lineTo(x, y);
                } else {
                    canvas->lineTo(borderRect.x() + borderLeft(),
                                   borderRect.maxY() - borderTop());
                    canvas->lineTo(borderRect.x(), borderRect.maxY());
                }

                canvas->fill();
            }

            // draw border-top
            {
                canvas->setFillColor(border.top().color());

                if (topLeftHorizontal && topLeftVertical) {
                    if (topLeftHorizontal > borderLeft() &&
                        topLeftVertical > borderTop()) {
                        canvas->save();
                        canvas->translate(
                            topLeftHorizontal + borderRect.x().toFloat(),
                            topLeftVertical + borderRect.y().toFloat());

                        float newHorizontal = topLeftHorizontal - borderLeft();
                        float newVertical = topLeftVertical - borderTop();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }

                        canvas->arcNegative(0, 0, arcR, M_PI + M_PI / 2,
                                            M_PI + M_PI / 4);
                        canvas->restore();
                    } else {
                        x = borderRect.x() + borderLeft();
                        y = borderRect.y() + borderTop();
                        canvas->moveTo(x, y);
                    }

                    canvas->save();
                    canvas->translate(
                        topLeftHorizontal + borderRect.x().toFloat(),
                        topLeftVertical + borderRect.y().toFloat());
                    if (topLeftVertical > topLeftHorizontal) {
                        canvas->scale(1 * (topLeftHorizontal / topLeftVertical),
                                      1);
                        arcR = topLeftVertical;
                    } else {
                        canvas->scale(
                            1, 1 * (topLeftVertical / topLeftHorizontal));
                        arcR = topLeftHorizontal;
                    }
                    canvas->arc(0, 0, arcR, M_PI + M_PI / 4, M_PI + M_PI / 2);
                    canvas->restore();
                } else {
                    canvas->moveTo(borderRect.x() + borderLeft(),
                                   borderRect.y() + borderTop());
                    canvas->lineTo(borderRect.x(), borderRect.y());
                }

                if (topRightHorizontal && topRightVertical) {
                    canvas->save();
                    canvas->translate(
                        -topRightHorizontal + borderRect.maxX().toFloat(),
                        topRightVertical + borderRect.y().toFloat());

                    if (topRightVertical > topRightHorizontal) {
                        canvas->scale(
                            1 * (topRightHorizontal / topRightVertical), 1);
                        arcR = topRightVertical;
                    } else {
                        canvas->scale(
                            1, 1 * (topRightVertical / topRightHorizontal));
                        arcR = topRightHorizontal;
                    }

                    canvas->arc(0, 0, arcR, -M_PI / 2, M_PI / 4 - M_PI / 2);
                    canvas->restore();

                    if (topRightHorizontal > borderRight() &&
                        topRightVertical > borderTop()) {
                        canvas->save();
                        canvas->translate(
                            -topRightHorizontal + borderRect.maxX().toFloat(),
                            topRightVertical + borderRect.y().toFloat());
                        float newHorizontal =
                            topRightHorizontal - borderRight();
                        float newVertical = topRightVertical - borderTop();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }
                        canvas->arcNegative(0, 0, arcR, M_PI / 4 - M_PI / 2,
                                            -M_PI / 2);
                        canvas->restore();
                    } else {
                        x = borderRect.maxX() - borderRight();
                        y = borderRect.y() + borderTop();
                        canvas->lineTo(x, y);
                    }
                } else {
                    canvas->lineTo(borderRect.maxX(), borderRect.y());
                    canvas->lineTo(borderRect.maxX() - borderRight(),
                                   borderRect.y() + borderTop());
                }
                canvas->fill();
            }

            // draw border-right
            {
                canvas->setFillColor(border.right().color());

                if (topRightHorizontal && topRightVertical) {
                    if (topRightHorizontal > borderRight() &&
                        topRightVertical > borderTop()) {
                        canvas->save();
                        canvas->translate(
                            borderRect.maxX().toFloat() - topRightHorizontal,
                            borderRect.y().toFloat() + topRightVertical);

                        float newHorizontal =
                            topRightHorizontal - borderRight();
                        float newVertical = topRightVertical - borderTop();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }

                        canvas->arcNegative(0, 0, arcR, M_PI / 2 - M_PI / 2,
                                            M_PI / 4 - M_PI / 2);
                        canvas->restore();
                    } else {
                        x = borderRect.maxX() - borderRight();
                        y = borderRect.y() + borderTop();
                        canvas->moveTo(x, y);
                    }

                    canvas->save();

                    canvas->translate(
                        borderRect.maxX().toFloat() - topRightHorizontal,
                        borderRect.y().toFloat() + topRightVertical);
                    if (topRightVertical > topRightHorizontal) {
                        canvas->scale(
                            1 * (topRightHorizontal / topRightVertical), 1);
                        arcR = topRightVertical;
                    } else {
                        canvas->scale(
                            1, 1 * (topRightVertical / topRightHorizontal));
                        arcR = topRightHorizontal;
                    }
                    canvas->arc(0, 0, arcR, M_PI / 4 - M_PI / 2,
                                M_PI / 2 - M_PI / 2);
                    canvas->restore();
                } else {
                    canvas->moveTo(borderRect.maxX() - borderRight(),
                                   borderRect.y() + borderTop());
                    canvas->lineTo(borderRect.maxX(), borderRect.y());
                }

                if (bottomRightHorizontal && bottomRightVertical) {
                    x = borderRect.maxX();
                    y = borderRect.maxY() - bottomRightVertical;
                    canvas->lineTo(x, y);

                    canvas->save();
                    canvas->translate(borderRect.maxX() - bottomRightHorizontal,
                                      borderRect.maxY() - bottomRightVertical);
                    if (bottomRightVertical > bottomRightHorizontal) {
                        canvas->scale(
                            1 * (bottomRightHorizontal / bottomRightVertical),
                            1);
                        arcR = bottomRightVertical;
                    } else {
                        canvas->scale(1, 1 * (bottomRightVertical /
                                              bottomRightHorizontal));
                        arcR = bottomRightHorizontal;
                    }
                    canvas->arc(0, 0, arcR, 0, M_PI / 4);
                    canvas->restore();

                    if (bottomRightHorizontal > borderRight() &&
                        bottomRightVertical > borderBottom()) {
                        canvas->save();
                        canvas->translate(
                            borderRect.maxX() - bottomRightHorizontal,
                            borderRect.maxY() - bottomRightVertical);

                        float newHorizontal =
                            bottomRightHorizontal - borderRight();
                        float newVertical =
                            bottomRightVertical - borderBottom();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }

                        canvas->arcNegative(0, 0, arcR, M_PI / 4, 0);
                        canvas->restore();
                    } else {
                        x = borderRect.maxX() - borderRight();
                        y = borderRect.maxY() - borderBottom();
                        canvas->lineTo(x, y);
                    }
                } else {
                    canvas->lineTo(borderRect.maxX(), borderRect.maxY());
                    canvas->lineTo(borderRect.maxX() - borderRight(),
                                   borderRect.maxY() - borderBottom());
                }

                canvas->fill();
            }

            // border-bottom
            {
                canvas->setFillColor(border.bottom().color());
                if (bottomRightHorizontal && bottomRightVertical) {
                    if (bottomRightHorizontal > borderRight() &&
                        bottomRightVertical > borderBottom()) {
                        canvas->save();
                        canvas->translate(
                            borderRect.maxX() - bottomRightHorizontal,
                            borderRect.maxY() - bottomRightVertical);

                        float newHorizontal =
                            bottomRightHorizontal - borderRight();
                        float newVertical =
                            bottomRightVertical - borderBottom();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }

                        canvas->arcNegative(0, 0, arcR, M_PI / 2, M_PI / 4);
                        canvas->restore();
                    } else {
                        x = borderRect.maxX() - borderRight();
                        y = borderRect.maxY() - borderBottom();
                        canvas->moveTo(x, y);
                    }

                    canvas->save();
                    canvas->translate(borderRect.maxX() - bottomRightHorizontal,
                                      borderRect.maxY() - bottomRightVertical);
                    if (bottomRightVertical > bottomRightHorizontal) {
                        canvas->scale(
                            1 * (bottomRightHorizontal / bottomRightVertical),
                            1);
                        arcR = bottomRightVertical;
                    } else {
                        canvas->scale(1, 1 * (bottomRightVertical /
                                              bottomRightHorizontal));
                        arcR = bottomRightHorizontal;
                    }
                    canvas->arc(0, 0, arcR, M_PI / 4, M_PI / 2);
                    canvas->restore();
                } else {
                    canvas->moveTo(borderRect.maxX() - borderRight(),
                                   borderRect.maxY() - borderBottom());
                    canvas->lineTo(borderRect.maxX(), borderRect.maxY());
                }

                if (bottomLeftHorizontal && bottomLeftVertical) {
                    canvas->save();
                    canvas->translate(borderRect.x() + bottomLeftHorizontal,
                                      borderRect.maxY() - bottomLeftVertical);
                    if (bottomLeftVertical > bottomLeftHorizontal) {
                        canvas->scale(
                            1 * (bottomLeftHorizontal / bottomLeftVertical), 1);
                        arcR = bottomLeftVertical;
                    } else {
                        canvas->scale(
                            1, 1 * (bottomLeftVertical / bottomLeftHorizontal));
                        arcR = bottomLeftHorizontal;
                    }
                    canvas->arc(0, 0, arcR, M_PI - M_PI / 2,
                                M_PI + M_PI / 4 - M_PI / 2);
                    canvas->restore();

                    if (bottomLeftHorizontal > borderLeft() &&
                        bottomLeftVertical > borderBottom()) {
                        canvas->save();
                        canvas->translate(borderRect.x() + bottomLeftHorizontal,
                                          borderRect.maxY() -
                                              bottomLeftVertical);

                        float newHorizontal =
                            bottomLeftHorizontal - borderLeft();
                        float newVertical = bottomLeftVertical - borderBottom();
                        if (newVertical > newHorizontal) {
                            canvas->scale(1 * (newHorizontal / newVertical), 1);
                            arcR = newVertical;
                        } else {
                            canvas->scale(1, 1 * (newVertical / newHorizontal));
                            arcR = newHorizontal;
                        }

                        canvas->arcNegative(0, 0, arcR,
                                            M_PI + M_PI / 4 - M_PI / 2,
                                            M_PI - M_PI / 2);
                        canvas->restore();
                    } else {
                        x = borderRect.x() + borderLeft();
                        y = borderRect.maxY() - borderBottom();
                        canvas->lineTo(x, y);
                    }
                } else {
                    canvas->lineTo(borderRect.x(), borderRect.maxY());
                    canvas->lineTo(borderRect.x() + borderLeft(),
                                   borderRect.maxY() - borderBottom());
                }
                canvas->fill();
            }

        } else {
            if (style()->isFourSideBorderStyleValueSolid() &&
                (border.top().color() == border.right().color()) &&
                (border.right().color() == border.bottom().color()) &&
                (border.bottom().color() == border.left().color())) {
                canvas->setFillColor(border.top().color());
                canvas->beginPath();

                canvas->moveTo(rect.x().floor(), rect.y().floor());
                canvas->lineTo(rect.maxX().floor(), rect.y().floor());
                canvas->lineTo(rect.maxX().floor(), rect.maxY().floor());
                canvas->lineTo(rect.x().floor(), rect.maxY().floor());
                canvas->lineTo(rect.x().floor(), rect.y().floor());

                canvas->lineTo((rect.x() + borderLeft()).floor(),
                               (rect.y() + borderTop()).floor());
                canvas->lineTo((rect.x() + borderLeft()).floor(),
                               (rect.maxY() - borderBottom()).floor());
                canvas->lineTo((rect.maxX() - borderRight()).floor(),
                               (rect.maxY() - borderBottom()).floor());
                canvas->lineTo((rect.maxX() - borderRight()).floor(),
                               (rect.y() + borderTop()).floor());
                canvas->lineTo((rect.x() + borderLeft()).floor(),
                               (rect.y() + borderTop()).floor());
                canvas->lineTo(rect.x().floor(), rect.y().floor());
                canvas->fill();
            } else {
                // Draw trapezium-like borders around the given rect
                //    _______________
                //   |\_____________/|
                //   ||             ||
                //   ||             ||
                //   ||             ||
                //   ||_____________||
                //   |/_____________\|
                //

                Unit::Color black =
                    NamedColor::namedColorToColor(NamedColor::blackNamedColor);
                // top
                if (border.top().style() ==
                    BorderStyleValue::InsetBorderStyleValue) {
                    canvas->setFillColor(border.top().color().getDarkerColor());
                    canvas->setStrokeColor(
                        border.top().color().getDarkerColor());
                } else if ((border.top().style() ==
                            BorderStyleValue::OutsetBorderStyleValue) &&
                           (border.top().color() == black)) {
                    canvas->setFillColor(
                        Unit::Color(238, 238, 238, border.top().color().a()));
                    canvas->setStrokeColor(
                        Unit::Color(238, 238, 238, border.top().color().a()));
                } else {
                    canvas->setFillColor(border.top().color());
                    canvas->setStrokeColor(border.top().color());
                }

                if ((border.top().style() ==
                     BorderStyleValue::DashedBorderStyleValue)) {
                    paintDashedLine(
                        canvas, LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + borderLeft(), rect.y()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y()),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        TopSide);
                } else if ((border.top().style() ==
                            BorderStyleValue::DottedBorderStyleValue)) {
                    paintDottedLine(
                        canvas, LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + borderLeft(), rect.y()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y()),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        TopSide);
                } else if ((border.top().style() ==
                            BorderStyleValue::DoubleBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width() -
                                           (borderRight() / 3),
                                       rect.y() + (borderTop() / 3)),
                        LayoutLocation(rect.x() + (borderLeft() / 3),
                                       rect.y() + (borderTop() / 3)));

                    canvas->drawRect(
                        LayoutLocation(rect.x() + (borderLeft() * 2 / 3),
                                       rect.y() + (borderTop() * 2 / 3)),
                        LayoutLocation(rect.x() + rect.width() -
                                           (borderRight() * 2 / 3),
                                       rect.y() + (borderTop() * 2 / 3)),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()));
                } else if ((border.top().style() ==
                            BorderStyleValue::GrooveBorderStyleValue) ||
                           (border.top().style() ==
                            BorderStyleValue::RidgeBorderStyleValue)) {
                    if ((border.top().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    } else {
                        canvas->setFillColor(border.top().color());
                    }
                    canvas->drawRect(
                        LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width() -
                                           (borderRight() / 2),
                                       rect.y() + (borderTop() / 2)),
                        LayoutLocation(rect.x() + (borderLeft() / 2),
                                       rect.y() + (borderTop() / 2)));

                    if ((border.top().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(border.top().color());
                    } else {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    }
                    canvas->drawRect(
                        LayoutLocation(rect.x() + (borderLeft() / 2),
                                       rect.y() + (borderTop() / 2)),
                        LayoutLocation(rect.x() + rect.width() -
                                           (borderRight() / 2),
                                       rect.y() + (borderTop() / 2)),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()));
                } else if ((border.top().style() !=
                            BorderStyleValue::HiddenBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()));
                }

                // right
                if ((border.right().style() ==
                     BorderStyleValue::InsetBorderStyleValue) &&
                    (border.right().color() == black)) {
                    canvas->setFillColor(
                        Unit::Color(238, 238, 238, border.right().color().a()));
                } else if (border.right().style() ==
                           BorderStyleValue::OutsetBorderStyleValue) {
                    canvas->setFillColor(
                        border.right().color().getDarkerColor());
                } else {
                    canvas->setFillColor(border.right().color());
                }

                if ((border.right().style() ==
                     BorderStyleValue::DashedBorderStyleValue)) {
                    paintDashedLine(
                        canvas,
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        RightSide);
                } else if ((border.right().style() ==
                            BorderStyleValue::DottedBorderStyleValue)) {
                    paintDottedLine(
                        canvas,
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        RightSide);
                } else if ((border.right().style() ==
                            BorderStyleValue::DoubleBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x() + rect.width() -
                                           borderRight() / 3,
                                       rect.y() + borderTop() / 3),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() / 3,
                            rect.y() + rect.height() - borderBottom() / 3));

                    canvas->drawRect(
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width() -
                                           borderRight() * 2 / 3,
                                       rect.y() + borderTop() * 2 / 3),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() * 2 / 3,
                            rect.y() + rect.height() - borderBottom() * 2 / 3),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()));
                } else if ((border.right().style() ==
                            BorderStyleValue::GrooveBorderStyleValue) ||
                           (border.right().style() ==
                            BorderStyleValue::RidgeBorderStyleValue)) {
                    if ((border.right().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(border.top().color());
                    } else {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    }
                    canvas->drawRect(
                        LayoutLocation(rect.x() + rect.width() -
                                           borderRight() / 2,
                                       rect.y() + borderTop() / 2),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() / 2,
                            rect.y() + rect.height() - borderBottom() / 2));

                    if ((border.right().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    } else {
                        canvas->setFillColor(border.top().color());
                    }
                    canvas->drawRect(
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width() -
                                           borderRight() / 2,
                                       rect.y() + borderTop() / 2),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() / 2,
                            rect.y() + rect.height() - borderBottom() / 2),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()));
                } else if ((border.right().style() !=
                            BorderStyleValue::HiddenBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + rect.width(), rect.y()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()));
                }

                // bottom
                if ((border.bottom().style() ==
                     BorderStyleValue::InsetBorderStyleValue) &&
                    (border.bottom().color() == black)) {
                    canvas->setFillColor(Unit::Color(
                        238, 238, 238, border.bottom().color().a()));
                } else if (border.bottom().style() ==
                           BorderStyleValue::OutsetBorderStyleValue) {
                    canvas->setFillColor(
                        border.bottom().color().getDarkerColor());
                } else {
                    canvas->setFillColor(border.bottom().color());
                }
                if ((border.bottom().style() ==
                     BorderStyleValue::DashedBorderStyleValue)) {
                    paintDashedLine(
                        canvas,
                        LayoutLocation(rect.x(), rect.y() + rect.height()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        BottomSide);
                } else if (border.bottom().style() ==
                           BorderStyleValue::DottedBorderStyleValue) {
                    paintDottedLine(
                        canvas,
                        LayoutLocation(rect.x(), rect.y() + rect.height()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        BottomSide);
                } else if ((border.bottom().style() ==
                            BorderStyleValue::DoubleBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x() + borderLeft() / 3,
                                       rect.y() + rect.height() -
                                           borderBottom() / 3),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() / 3,
                            rect.y() + rect.height() - borderBottom() / 3),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x(), rect.y() + rect.height()));

                    canvas->drawRect(
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() * 2 / 3,
                            rect.y() + rect.height() - borderBottom() * 2 / 3),
                        LayoutLocation(rect.x() + borderLeft() * 2 / 3,
                                       rect.y() + rect.height() -
                                           borderBottom() * 2 / 3));
                } else if ((border.bottom().style() ==
                            BorderStyleValue::GrooveBorderStyleValue) ||
                           (border.bottom().style() ==
                            BorderStyleValue::RidgeBorderStyleValue)) {
                    if ((border.bottom().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(border.top().color());
                    } else {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    }
                    canvas->drawRect(
                        LayoutLocation(rect.x() + borderLeft() / 2,
                                       rect.y() + rect.height() -
                                           borderBottom() / 2),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() / 2,
                            rect.y() + rect.height() - borderBottom() / 2),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x(), rect.y() + rect.height()));

                    if ((border.bottom().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    } else {
                        canvas->setFillColor(border.top().color());
                    }
                    canvas->drawRect(
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(
                            rect.x() + rect.width() - borderRight() / 2,
                            rect.y() + rect.height() - borderBottom() / 2),
                        LayoutLocation(rect.x() + borderLeft() / 2,
                                       rect.y() + rect.height() -
                                           borderBottom() / 2));
                } else if ((border.bottom().style() !=
                            BorderStyleValue::HiddenBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width() - borderRight(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + rect.width(),
                                       rect.y() + rect.height()),
                        LayoutLocation(rect.x(), rect.y() + rect.height()));
                }

                // left
                if (border.left().style() ==
                    BorderStyleValue::InsetBorderStyleValue) {
                    canvas->setFillColor(
                        border.left().color().getDarkerColor());
                } else if ((border.left().style() ==
                            BorderStyleValue::OutsetBorderStyleValue) &&
                           (border.left().color() == black)) {
                    canvas->setFillColor(
                        Unit::Color(238, 238, 238, border.left().color().a()));
                } else {
                    canvas->setFillColor(border.left().color());
                }

                if (border.left().style() ==
                    BorderStyleValue::DashedBorderStyleValue) {
                    paintDashedLine(
                        canvas, LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x(), rect.y() + borderTop()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x(), rect.y() + rect.height() -
                                                     borderBottom()),
                        LayoutLocation(rect.x(), rect.y() + rect.height()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LeftSide);
                } else if (border.left().style() ==
                           BorderStyleValue::DottedBorderStyleValue) {
                    paintDottedLine(
                        canvas, LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x(), rect.y() + borderTop()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x(), rect.y() + rect.height() -
                                                     borderBottom()),
                        LayoutLocation(rect.x(), rect.y() + rect.height()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LeftSide);
                } else if ((border.left().style() ==
                            BorderStyleValue::DoubleBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + borderLeft() / 3,
                                       rect.y() + borderTop() / 3),
                        LayoutLocation(rect.x() + borderLeft() / 3,
                                       rect.y() + rect.height() -
                                           borderBottom() / 3),
                        LayoutLocation(rect.x(), rect.y() + rect.height()));

                    canvas->drawRect(
                        LayoutLocation(rect.x() + borderLeft() * 2 / 3,
                                       rect.y() + borderTop() * 2 / 3),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x() + borderLeft() * 2 / 3,
                                       rect.y() + rect.height() -
                                           borderBottom() * 2 / 3));
                } else if ((border.left().style() ==
                            BorderStyleValue::GrooveBorderStyleValue) ||
                           (border.left().style() ==
                            BorderStyleValue::RidgeBorderStyleValue)) {
                    if ((border.left().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    } else {
                        canvas->setFillColor(border.top().color());
                    }
                    canvas->drawRect(
                        LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + borderLeft() / 2,
                                       rect.y() + borderTop() / 2),
                        LayoutLocation(rect.x() + borderLeft() / 2,
                                       rect.y() + rect.height() -
                                           borderBottom() / 2),
                        LayoutLocation(rect.x(), rect.y() + rect.height()));

                    if ((border.left().style() ==
                         BorderStyleValue::GrooveBorderStyleValue)) {
                        canvas->setFillColor(border.top().color());
                    } else {
                        canvas->setFillColor(
                            border.top().color().getDarkerColor());
                    }
                    canvas->drawRect(LayoutLocation(rect.x() + borderLeft() / 2,
                                                    rect.y() + borderTop() / 2),
                                     LayoutLocation(rect.x() + borderLeft(),
                                                    rect.y() + borderTop()),
                                     LayoutLocation(rect.x() + borderLeft(),
                                                    rect.y() + rect.height() -
                                                        borderBottom()),
                                     LayoutLocation(rect.x() + borderLeft() / 2,
                                                    rect.y() + rect.height() -
                                                        borderBottom() / 2));
                } else if ((border.left().style() !=
                            BorderStyleValue::HiddenBorderStyleValue)) {
                    canvas->drawRect(
                        LayoutLocation(rect.x(), rect.y()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + borderTop()),
                        LayoutLocation(rect.x() + borderLeft(),
                                       rect.y() + rect.height() -
                                           borderBottom()),
                        LayoutLocation(rect.x(), rect.y() + rect.height()));
                }
            }
        }
    }

    canvas->restore();
}

bool FrameBox::needsToEstablishStackingContextForScrolling()
{
    if (isFrameBlockBox()) {
        if (appliedOverflowX() == OverflowValue::AutoOverflow ||
            appliedOverflowX() == OverflowValue::ScrollOverflow) {
            if (asFrameBlockBox()->hasBiggerContentThanFrameWidth()) {
                return true;
            }
        }
        if (appliedOverflowY() == OverflowValue::AutoOverflow ||
            appliedOverflowY() == OverflowValue::ScrollOverflow) {
            if (asFrameBlockBox()->hasBiggerContentThanFrameHeight()) {
                return true;
            }
        }
    }
    return false;
}

bool FrameBox::canOwnsStackingContext()
{
    if (isRootElement()) {
        return true;
    } else if (needsGraphicsBuffer()) {
        return true;
    } else if (style()->hasTransforms(this)) {
        return true;
    } else if (style()->position() == FixedPositionValue) {
        return true;
    } else if ((isPositioned() || isFlexItem()) &&
               style()->isSpecifiedZIndex()) {
        return true;
    } else if (style()->opacity() != 1 || isRunningOpacityAnimation()) {
        return true;
    } else if (style()->hasAvailableFilter()) {
        return true;
    } else if (style()->maskLayerSize()) {
        return true;
    } else if (needsToEstablishStackingContextForScrolling()) {
        return true;
    } else if (!isAnonymous() && node()) {
        // A context drawn into its own graphics buffer last frame keeps that
        // buffer even after the style reason for it (a transform transition,
        // will-change) is gone; see the compositedBefore carry-over in
        // StackingContext::applyStackingContextProperties. Its descendant
        // contexts must then stay parented here so they keep painting into
        // that buffer: re-parenting them to an ancestor context would draw
        // them into the ancestor's tiles, underneath this box's own layer.
        auto& info = node()->webView()->prevDrawnStackingContextInfo();
        auto iter = info.find(node());
        if (iter != info.end() && iter->second.needsGraphicsBuffer) {
            return true;
        }
    }

    return false;
}

bool FrameBox::needsToPaintBackgroundOrBorderOrBoxShadow()
{
    auto s = style();
    if (s->visibility() != VisibilityValue::VisibleVisibilityValue) {
        return false;
    }
    auto border = s->border();
    return s->boxShadow() || !s->backgroundColor().isTransparent() ||
           s->backgroundLayerSize() || border.hasBorderImageData() ||
           border.hasBorderStyle();
}

void FrameBox::paintContent(PaintingContext& ctx)
{
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

LayoutRect FrameBox::paintExtent(PaintPassMemos* memos)
{
    if (memos) {
        if (LayoutRect* memo = memos->m_paintExtent.find(this)) {
            return *memo;
        }
    }
    LayoutRect r = frameVisibleRect();
    // When overflow clipping applies, descendants are clipped to the padding
    // box, which frameVisibleRect() already covers.
    bool childrenClipped = isFrameBlockBox() && shouldApplyOverflow();
    if (!childrenClipped) {
        auto iter = childFrameBoxIterator(alloca(maxChildFrameBoxIteratorSize));
        while (iter->hasNext()) {
            FrameBox* child = iter->next();
            if (child->needToEstablishStackingContext()) {
                // Painted through the stacking-context tree, not this walk.
                continue;
            }
            LayoutRect cr = child->paintExtent(memos);
            cr.setX(cr.x() + child->x());
            cr.setY(cr.y() + child->y());
            r.unite(cr);
        }
    }
    if (memos) {
        memos->m_paintExtent.put(this, r);
    }
    return r;
}

void FrameBox::paintChildrenWith(PaintingContext& ctx)
{
    LayoutRect clipRect;
    bool hasClipRect = ctx.m_canvas->clipBoundingRect(clipRect);
    if (hasClipRect) {
        // Guard against fixed-point/device rounding at the clip edges.
        clipRect = LayoutRect(clipRect.x() - 1, clipRect.y() - 1,
                              clipRect.width() + 2, clipRect.height() + 2);
    }
    Frame* child = firstChild();
    while (child) {
        FrameBox* box = child->asFrameBox();
        if (hasClipRect) {
            LayoutRect e = box->paintExtent(ctx.m_memos);
            e.setX(e.x() + box->x());
            e.setY(e.y() + box->y());
            if (!clipRect.intersects(e)) {
                child = child->next();
                continue;
            }
        }
        ctx.m_canvas->translate(box->x(), box->y());
        box->paintContent(ctx);
        ctx.m_canvas->translate(-box->x(), -box->y());
        child = child->next();
    }
}

void FrameBox::paintStackingContextContent(Canvas* canvas,
                                           PaintPassMemos* memos)
{
    PaintingContext ctx(canvas, memos);

    // the in-flow, non-inline-level, non-positioned descendants.
    ctx.m_paintingStage = PaintingNormalFlowBlock;
    paintChildrenWith(ctx);

    // the non-positioned float
    ctx.m_paintingStage = PaintingNonPositionedFloats;
    paintChildrenWith(ctx);

    // block-level replaced element
    ctx.m_paintingStage = PaintingReplacedBlock;
    paintChildrenWith(ctx);

    // the in-flow, inline-level, non-positioned descendants, including inline
    // tables and inline blocks.
    ctx.m_paintingStage = PaintingNormalFlowInline;
    paintChildrenWith(ctx);
}

void FrameBox::establishesStackingContextIfNeedsAndComputingPaintingFlags()
{
    computePaintingFlags();

    if (!isAnonymous()) {
        Node* nd = node();
        auto& info = nd->webView()->prevDrawnStackingContextInfo();
        auto iter = info.find(nd);
        if (iter != info.end() && !needToEstablishStackingContext()) {
            // stacking context is disappear
            // trigger repaint tracker
            nd->webView()->markNeedsPaintingConsiderInRendering();
            if (iter->second.needsGraphicsBuffer) {
                // The content was composited into its own buffer, so the
                // enclosing contexts' composed visibleRects excluded it. It
                // now paints into them; keep their pre-rebuild rects from
                // being restored or the buffer they size misses it.
                markAncestorStackingContextVisibleRectDirty();
            }
        }
    }

    if (needToEstablishStackingContext()) {
        STARFISH_ASSERT(isRootElement() || stackingContext() == nullptr);
        if (isRootElement()) {
            if (node()
                    ->document()
                    ->browsingContext()
                    ->isTopLevelBrowsingContext()) {
                ensureFrameBoxRareData()->m_stackingContext =
                    new StackingContext(this, nullptr);
            } else {
                STARFISH_ASSERT(node()
                                    ->document()
                                    ->browsingContext()
                                    ->sourceElement()
                                    ->frame()
                                    ->asFrameBox()
                                    ->stackingContext());
                ensureFrameBoxRareData()->m_stackingContext =
                    new StackingContext(this, node()
                                                  ->document()
                                                  ->browsingContext()
                                                  ->sourceElement()
                                                  ->frame()
                                                  ->asFrameBox()
                                                  ->stackingContext());
            }
        } else if (layoutParent() && !isFrameDocument()) {
            // Fullscreen top-layer emulation, part 2 (part 1 is the forced
            // position:fixed + z-index INT_MAX in StyleResolver). The UA
            // z-index alone is confined to the nearest ancestor stacking
            // context, so a sibling context with a higher z-index (e.g. an
            // overlay next to the container that holds a fullscreen iframe)
            // would still paint above the fullscreen element. A real top
            // layer escapes ancestor contexts entirely; emulate that by
            // parenting the fullscreen element's stacking context to its
            // document's root context, where the INT_MAX z-index wins. Both
            // painting and hit testing follow the stacking context tree, so
            // this also routes input to the fullscreen content.
            if (!isAnonymous() && node()->isElement() &&
                node()->document()->fullscreenElement() == node()) {
                Frame* rootFrame =
                    node()->document()->rootElement()
                        ? node()->document()->rootElement()->frame()
                        : nullptr;
                if (rootFrame && rootFrame->asFrameBox()->stackingContext()) {
                    ensureFrameBoxRareData()->m_stackingContext =
                        new StackingContext(
                            this, rootFrame->asFrameBox()->stackingContext());
                    return;
                }
            }
            FrameBox* p = layoutParent()->asFrameBox();
            while (true) {
                if (p->needToEstablishStackingContext() &&
                    p->stackingContext()) {
                    if (p->canOwnsStackingContext() ||
                        p->shouldApplyOverflow()) {
                        ensureFrameBoxRareData()->m_stackingContext =
                            new StackingContext(this, p->stackingContext());
                        break;
                    }
                }
                if (!p->layoutParent()) {
                    break;
                }
                p = p->layoutParent()->asFrameBox();
            }
        }
    }
}

LayoutRect FrameBox::frameVisibleRect()
{
    LayoutRect out(0, 0, width(), height());
    ComputedStyle* cs = style();
    OutlineData* outline = nullptr;
    ShadowDataList* boxShadow = nullptr;
    BorderData* border = nullptr;
    FilterFunctions* filter = nullptr;

    if (cs) {
        size_t len = cs->rareComputedStyleData()->m_styles.size();
        for (size_t i = 0; i < len; i++) {
            switch (cs->rareComputedStyleData()->m_styles[i].keyKind()) {
            case RareComputedStyleData::KeyKind::Outline:
                outline =
                    cs->rareComputedStyleData()->m_styles[i].m_value.m_outline;
                break;
            case RareComputedStyleData::KeyKind::BoxShadow:
                boxShadow = cs->rareComputedStyleData()
                                ->m_styles[i]
                                .m_value.m_boxShadowDataList;
                break;
            case RareComputedStyleData::KeyKind::Border:
                border = cs->rareComputedStyleData()
                             ->m_styles[i]
                             .m_value.m_borderData;
                break;
            case RareComputedStyleData::KeyKind::Filter:
                filter =
                    cs->rareComputedStyleData()->m_styles[i].m_value.m_filter;
                break;
            default:
                break;
            }
        }
    }

    if (outline) {
        LayoutRect outlineRect = frameVisibleOutlineRect(outline);
        out.unite(outlineRect);
    }

    if (boxShadow) {
        LayoutRect shadowRect = frameVisibleShadowsRect(boxShadow);
        out.unite(shadowRect);
    }

    if (filter) {
        LayoutRect filterRect = frameVisibleFilterRect(filter);
        out.unite(filterRect);
    }

    if (isInlineTextBox()) {
        // every text node must have SomputedStyle
        auto textShadow = cs->textShadow();
        if (textShadow) {
            LayoutRect owner = frameRect();
            owner.setX(0);
            owner.setY(0);
            for (size_t i = 0; i < textShadow->size(); i++) {
                LayoutRect rect = computeVisibleShadowRect(
                    owner, textShadow->at(i).toCanvasShadowData(this));
                out.unite(rect);
            }
        }
    }

    if (border) {
        const BorderImageData& bi = border->image();
        if (!bi.isNull()) {
            auto outsets = bi.outsets();
            double bLWidth =
                border->left().width().specifiedValue(width(), this);
            double bTWidth =
                border->top().width().specifiedValue(height(), this);
            double bRWidth =
                border->right().width().specifiedValue(width(), this);
            double bBWidth =
                border->bottom().width().specifiedValue(height(), this);

            double bLOutset =
                outsets.left().computedBorderImageOutset(bLWidth, this);
            double bTOutset =
                outsets.top().computedBorderImageOutset(bTWidth, this);
            double bROutset =
                outsets.right().computedBorderImageOutset(bRWidth, this);
            double bBOutset =
                outsets.bottom().computedBorderImageOutset(bBWidth, this);
            LayoutRect r = frameRect();
            if (bLOutset > 0) {
                r.setX(r.x() - bLOutset);
                r.setWidth(r.width() + bLOutset);
            }
            if (bTOutset > 0) {
                r.setY(r.x() - bTOutset);
                r.setHeight(r.height() + bTOutset);
            }
            if (bROutset > 0) {
                r.setWidth(r.width() + bROutset);
            }
            if (bBOutset > 0) {
                r.setHeight(r.height() + bBOutset);
            }

            out.unite(r);
        }
    }

    return out;
}

LayoutRect FrameBox::frameVisibleOutlineRect(OutlineData* outline)
{
    STARFISH_ASSERT(outline != nullptr);

    LayoutRect r = frameRect();
    r.setX(0);
    r.setY(0);

    if (outline->border().style() != BorderStyleValue::NoneBorderStyleValue) {
        LayoutUnit t = outlineThickness();
        r.setX(r.x() - t);
        r.setY(r.y() - t);
        r.setWidth(r.width() + t * 2);
        r.setHeight(r.height() + t * 2);
    }

    return r;
}

LayoutRect FrameBox::frameVisibleShadowsRect(ShadowDataList* boxShadow)
{
    STARFISH_ASSERT(boxShadow != nullptr);

    LayoutRect owner = frameRect();
    owner.setX(0);
    owner.setY(0);
    LayoutRect ret = owner;

    for (size_t i = 0; i < boxShadow->size(); i++) {
        LayoutRect rect = computeVisibleShadowRect(
            owner, boxShadow->at(i).toCanvasShadowData(this));
        ret.unite(rect);
    }

    return ret;
}

LayoutRect FrameBox::frameVisibleFilterRect(FilterFunctions* filter)
{
    STARFISH_ASSERT(filter != nullptr);

    LayoutRect owner = frameRect();
    owner.setX(0);
    owner.setY(0);
    LayoutRect ret = owner;

    Length standardDeviation;
    if (filter->getStandardDeviationOfBlurFilter(standardDeviation)) {
        float sd = standardDeviation.numberData();
        if (sd > 0) {
            float radiusOffset =
                ShadowBlur::computeKernelSizeAtStdDeviation(sd);
            LayoutRect rect(owner.x() - radiusOffset, owner.y() - radiusOffset,
                            ceil(owner.width() + radiusOffset * 2),
                            ceil(owner.height() + radiusOffset * 2));
            ret.unite(rect);
        }
    }

    return ret;
}

LayoutRect FrameBox::frameScrollingRect()
{
    LayoutRect out(0, 0, width(), height());
    ComputedStyle* cs = style();
    BorderData* border = nullptr;
    FilterFunctions* filter = nullptr;

    if (cs) {
        size_t len = cs->rareComputedStyleData()->m_styles.size();
        for (size_t i = 0; i < len; i++) {
            switch (cs->rareComputedStyleData()->m_styles[i].keyKind()) {
            case RareComputedStyleData::KeyKind::Border:
                border = cs->rareComputedStyleData()
                             ->m_styles[i]
                             .m_value.m_borderData;
                break;
            default:
                break;
            }
        }
    }

    if (border) {
        const BorderImageData& bi = border->image();
        if (!bi.isNull()) {
            auto outsets = bi.outsets();
            double bLWidth =
                border->left().width().specifiedValue(width(), this);
            double bTWidth =
                border->top().width().specifiedValue(height(), this);
            double bRWidth =
                border->right().width().specifiedValue(width(), this);
            double bBWidth =
                border->bottom().width().specifiedValue(height(), this);

            double bLOutset =
                outsets.left().computedBorderImageOutset(bLWidth, this);
            double bTOutset =
                outsets.top().computedBorderImageOutset(bTWidth, this);
            double bROutset =
                outsets.right().computedBorderImageOutset(bRWidth, this);
            double bBOutset =
                outsets.bottom().computedBorderImageOutset(bBWidth, this);
            LayoutRect r = frameRect();
            if (bLOutset > 0) {
                r.setX(r.x() - bLOutset);
                r.setWidth(r.width() + bLOutset);
            }
            if (bTOutset > 0) {
                r.setY(r.x() - bTOutset);
                r.setHeight(r.height() + bTOutset);
            }
            if (bROutset > 0) {
                r.setWidth(r.width() + bROutset);
            }
            if (bBOutset > 0) {
                r.setHeight(r.height() + bBOutset);
            }

            out.unite(r);
        }
    }

    return out;
}

static bool styleHasDrawableContents(ComputedStyle* cs, FrameBox* b,
                                     bool checkBackgroundColor = true)
{
    StyleBackgroundData* background = nullptr;
    OutlineData* outline = nullptr;
    ShadowDataList* boxShadow = nullptr;
    BorderData* border = nullptr;
    FilterFunctions* filter = nullptr;

    size_t len = cs->rareComputedStyleData()->m_styles.size();
    for (size_t i = 0; i < len; i++) {
        switch (cs->rareComputedStyleData()->m_styles[i].keyKind()) {
        case RareComputedStyleData::KeyKind::Background:
            background =
                cs->rareComputedStyleData()->m_styles[i].m_value.m_background;
            break;
        case RareComputedStyleData::KeyKind::Outline:
            outline =
                cs->rareComputedStyleData()->m_styles[i].m_value.m_outline;
            break;
        case RareComputedStyleData::KeyKind::BoxShadow:
            boxShadow = cs->rareComputedStyleData()
                            ->m_styles[i]
                            .m_value.m_boxShadowDataList;
            break;
        case RareComputedStyleData::KeyKind::Border:
            border =
                cs->rareComputedStyleData()->m_styles[i].m_value.m_borderData;
            break;
        case RareComputedStyleData::KeyKind::Filter:
            filter = cs->rareComputedStyleData()->m_styles[i].m_value.m_filter;
            break;
        default:
            break;
        }
    }

    if (checkBackgroundColor && background &&
        !background->color().isTransparent()) {
        return true;
    }

    if (background && background->sizeOfLayers()) {
        size_t backgroundLayerSize = background->sizeOfLayers();
        for (size_t i = 0; i < backgroundLayerSize; i++) {
            if (background->image(i) == nullptr) {
                continue;
            }
            ImageValue* imageValue = background->image(i);
            auto type = imageValue->type();
            if (type.isGradient()) {
                if (imageValue->gradientValue()->isEffective()) {
                    return true;
                }
            } else {
                return true;
            }
        }
    }

    if (outline &&
        outline->border().style() != BorderStyleValue::NoneBorderStyleValue &&
        outline->border().width().specifiedValue(1, b->node())) {
        return true;
    }

    if (boxShadow && boxShadow->size()) {
        size_t s = boxShadow->size();
        for (size_t i = 0; i < s; i++) {
            const auto& bs = boxShadow->at(i);
            auto shadowColor = bs.hasColor() ? bs.color() : cs->color();

            if (shadowColor.isTransparent()) {
                continue;
            }
            if (!bs.offsetX().isZero() || !bs.offsetY().isZero() ||
                !bs.radius().isZero() || !bs.spreadDistance().isZero()) {
                return true;
            }
        }
    }

    if (filter && filter->size()) {
        return true;
    }

    if (border && (border->hasBorderStyle() || !border->image().isNull())) {
        if (!border->top().width().isZero() ||
            !border->right().width().isZero() ||
            !border->bottom().width().isZero() ||
            !border->left().width().isZero()) {
            return true;
        }
    }

    return false;
}

bool FrameBox::isBoxesInvisibleFromHere()
{
    ComputedStyle* cs = style();

    if (cs && cs->isAbsolutePositioned() && cs->hasZeroClipRect()) {
        return true;
    }

    if (cs && cs->opacity() == 0) {
        return true;
    }

    if (UNLIKELY(isFrameSVGInvisibleBox())) {
        return true;
    }

    return false;
}

bool FrameBox::tryUniteVisibleRect(Frame::ComputeVisibleRectContext& ctx)
{
    if (ctx.sourceStackingContext &&
        (this != ctx.sourceStackingContext->owner() && stackingContext() &&
         (stackingContext()->needsGraphicsBuffer() ||
          stackingContext()->needsGraphicsBufferReason()))) {
        return false;
    }

    if (ctx.purpose != Frame::ComputeVisibleRectContext::Scrolling &&
        stackingContext() && ctx.sourceStackingContext &&
        ctx.sourceStackingContext != stackingContext()) {
        StackingContext* sc = stackingContext();
        bool isAncestor = ctx.sourceStackingContext->isAncestorOf(sc);
        bool isBrother = false;
        bool isSonOfBrother = false;

        // younger brother of source also needs computing
        if (!isAncestor && ctx.sourceStackingContext->parent()) {
            bool meetSource = false;
            auto iter =
                ctx.sourceStackingContext->parent()->childContexts().begin();
            while (iter !=
                   ctx.sourceStackingContext->parent()->childContexts().end()) {
                StackingContextChild* child = *iter;
                auto iter2 = child->begin();
                while (iter2 != child->end()) {
                    if (*iter2 == ctx.sourceStackingContext) {
                        meetSource = true;
                    } else if (meetSource && sc == *iter2) {
                        isBrother = true;
                    }
                    iter2++;
                }
                iter++;
            }
        }

        // son of brother of source also needs computing
        if (!isAncestor && !isBrother && ctx.sourceStackingContext->parent()) {
            bool meetSource = false;
            auto iter =
                ctx.sourceStackingContext->parent()->childContexts().begin();
            while (iter !=
                   ctx.sourceStackingContext->parent()->childContexts().end()) {
                StackingContextChild* child = *iter;
                auto iter2 = child->begin();
                while (iter2 != child->end()) {
                    if (*iter2 == ctx.sourceStackingContext) {
                        meetSource = true;
                    } else if (meetSource && (*iter2)->isAncestorOf(sc)) {
                        isSonOfBrother = true;
                    }
                    iter2++;
                }
                iter++;
            }
        }

        if (!isAncestor && !isBrother && !isSonOfBrother) {
            return false;
        }
    }

    ComputedStyle* cs = style();
    ComputedStyle* parentStyle = nullptr;
    bool isScrollingPurpose =
        ctx.purpose == Frame::ComputeVisibleRectContext::Scrolling;
    if (!cs) {
        if (isLineBox()) {
            parentStyle = layoutParent()->style();
        }
    } else if (isScrollingPurpose &&
               cs->position() == PositionValue::FixedPositionValue) {
        return false;
    }

    if (!isScrollingPurpose &&
        ((cs && cs->visibility() == HiddenVisibilityValue) ||
         (parentStyle && parentStyle->visibility() == HiddenVisibilityValue))) {
        return true;
    }

    if (!isScrollingPurpose && ctx.isVisibleRectCollapsible &&
        this != ctx.sourceStackingContext->owner() && cs &&
        cs->opacity() == 0) {
        return false;
    }

    if (ctx.isVisibleRectCollapsible && isBoxesInvisibleFromHere()) {
        return false;
    }

    bool ret = !shouldApplyOverflow();
    // we need to return true when `true ==
    // StackingContext->inScrollWithGraphicsBufferActive()` since we need to
    // test children boxes for scrolling
    if (ctx.sourceStackingContext &&
        ctx.sourceStackingContext->inScrollWithGraphicsBufferActive() &&
        ctx.sourceStackingContext == stackingContext()) {
        ret = true;
    }
    bool boxHasDrawableContents = true;
    bool drawableContentsInStyle = true;
    if (ctx.isVisibleRectCollapsible) {
        bool shouldCareBackgroundColor = true;
        if (node() &&
            (node()->isHTMLHtmlElement() || node()->isHTMLBodyElement())) {
            auto bgColor = node()
                               ->document()
                               ->browsingContext()
                               ->hasWindowBackgroundColor();
            if (bgColor.first.hasValue() && bgColor.first.value() == node()) {
                shouldCareBackgroundColor = false;
            }
        } else if (!isScrollingPurpose && contentSurface()) {
            // there is only background-color on video or canvas element, we
            // should not make graphics buffer since we can draw background
            // color property with compositor
            shouldCareBackgroundColor = false;
        } else if (!isScrollingPurpose && ctx.sourceStackingContext &&
                   ctx.sourceStackingContext->owner() == this &&
                   ctx.sourceStackingContext
                       ->isOwnerBackgroundDrawnByCompositor()) {
            // stacking context owner: compositor draws bg-color directly
            // (before tiles or via drawRect when buffer is empty), so exclude
            // it from visibleRect — buffer is sized to content only
            shouldCareBackgroundColor = false;
        }

        drawableContentsInStyle =
            cs && styleHasDrawableContents(cs, this, shouldCareBackgroundColor);
    }
    if (isFrameBlockBox() && !drawableContentsInStyle) {
        boxHasDrawableContents = false;
    } else if (isFrameReplaced() &&
               asFrameReplaced()->isFrameReplacedIFrame() &&
               !drawableContentsInStyle) {
        if (ctx.sourceStackingContext &&
            this == ctx.sourceStackingContext->owner() &&
            !ctx.contentOnlyExtent) {
            // We are computing this iframe's OWN graphics-buffer extent. The
            // buffer must span the iframe box so the child document (whose root
            // stacking context may itself be composited) has a backing region
            // to be drawn into/under. Treating the iframe as having no drawable
            // contents here lets its visible rect collapse to 0, yielding a
            // 0x0 graphics buffer and a blank iframe -- observed as the answer
            // card being cut off / blank while a streaming/entry transform or
            // opacity animation forces the iframe to be composited.
            boxHasDrawableContents = true;
        } else if (node()->asHTMLIFrameElement()->browsingContext()) {
            if (node()
                    ->asHTMLIFrameElement()
                    ->browsingContext()
                    ->rootStackingContextNeedsGraphicsBuffer()) {
                boxHasDrawableContents = false;
            } else {
                boxHasDrawableContents = drawableContentsInStyle;
            }
        }
    } else if (isFrameReplaced()) {
        if (asFrameReplaced()->isFrameReplacedImage()) {
            NativeImageData* id = node()->asHTMLImageElement()->imageData();
            if (id) {
                if (id->width() < ExtraSmallNativeImageSize &&
                    id->height() < ExtraSmallNativeImageSize &&
                    id->isEmptyImage()) {
                    boxHasDrawableContents = drawableContentsInStyle;
                } else {
                    boxHasDrawableContents = true;
                }
            } else {
                boxHasDrawableContents = drawableContentsInStyle;
            }
        } else if (isFrameSVGSVGBox()) {
            boxHasDrawableContents = true;
        } else {
            boxHasDrawableContents = drawableContentsInStyle;
        }
    }

    if (!ctx.isVisibleRectCollapsible || boxHasDrawableContents) {
        if (isScrollingPurpose) {
            ctx.uniteRect(frameScrollingRect());
        } else {
            ctx.uniteRect(frameVisibleRect());
        }
    }

    if (ctx.isVisibleRectCollapsible && isFrameBlockBox() &&
        !boxHasDrawableContents) {
        ret = true;
    }

    if (ctx.isForSpecialValueForTableCell && isScrollingPurpose &&
        isFrameFlexibleBox()) {
        if (cs->height().isDefinite(true)) {
            ret = false;
        }
    }

    if (boxHasDrawableContents) {
        size_t len = isInlineTextBox() ? 2 : 1;
        for (size_t i = 0; cs && i < len; ++i) {
            const ShadowDataList* list =
                i == 0 ? cs->boxShadow() : cs->textShadow();
            LayoutRect owner = frameRect();
            owner.setX(0);
            owner.setY(0);
            LayoutRect shadowsRect = owner;
            bool hasShadow = list != nullptr;
            // The rects are only united, so the shadows are taken one at a
            // time in list order rather than through a list built per call.
            for (size_t j = 0; list && j < list->size(); j++) {
                CanvasShadowData shadow = list->at(j).toCanvasShadowData(this);
                if (ctx.isVisibleRectCollapsible && isFrameBlockBox()) {
                    if (shadow.hasColor() && !shadow.color().isTransparent()) {
                        LayoutRect rect =
                            computeVisibleShadowRect(owner, shadow);
                        shadowsRect.unite(rect);
                    } else if (!shadow.hasColor() &&
                               !cs->color().isTransparent()) {
                        LayoutRect rect =
                            computeVisibleShadowRect(owner, shadow);
                        shadowsRect.unite(rect);
                    }
                } else if (ctx.isForSpecialValueForTableCell &&
                           isScrollingPurpose && isFrameFlexibleBox()) {
                    LayoutRect rect = computeVisibleShadowRect(owner, shadow);
                    shadowsRect.unite(rect);
                    ret = false;
                }
            }
            if (hasShadow) {
                ctx.uniteRect(shadowsRect);
            }
        }
    }

    // Composing a context's rect (see computeVisibleRect in
    // StackingContext.cpp): a child context's subtree arrives as that
    // context's own composed rect, so only its owner box is united here.
    // Only the contexts below the source in the context tree are composed
    // that way. A box whose ancestors cannot own a stacking context has its
    // context parented above the source (establishesStackingContextIfNeeds
    // skips them), so its subtree reaches the source's rect through this
    // walk alone and must still be descended into.
    if (ctx.subtreeRectsFromChildContexts && stackingContext() &&
        ctx.sourceStackingContext &&
        this != ctx.sourceStackingContext->owner()) {
        StackingContext* c = stackingContext()->parent();
        while (c && c != ctx.sourceStackingContext) {
            c = c->parent();
        }
        if (c) {
            return false;
        }
    }

    return ret;
}

void FrameBox::computeVisibleRect(Frame::ComputeVisibleRectContext& ctx)
{
    Frame::ComputeVisibleRectContextFragment f(ctx, this);
    tryUniteVisibleRect(ctx);
}

bool FrameBox::isVisible()
{
    ComputedStyle* cs = style();

    if ((cs && cs->visibility() == HiddenVisibilityValue)) {
        return false;
    }

    return cs && styleHasDrawableContents(cs, this);
}

void FrameBox::clearStackingContextIfNeeds()
{
    if (stackingContext()) {
        frameBoxRareData()->m_stackingContext = nullptr;
    }
}

LayoutUnit FrameBox::widthAfterApplyingMinMaxWidths(
    LayoutContext& ctx, LayoutUnit width, LayoutUnit parentWidth,
    bool underComputingPreferredWidth)
{
    ComputedStyle* style = Frame::style();
    if (style->minWidth().isSpecified()) {
        if (style->minWidth().isDefinite(!underComputingPreferredWidth)) {
            LayoutUnit minWidth =
                style->minWidth().specifiedValue(parentWidth, this);
            minWidth = contentWidthAfterApplyingBoxSizing(minWidth);

            if (minWidth > width) {
                return minWidth;
            }
        }
    } else if (style->minWidth().isIntrinsic()) {
        LayoutUnit minWidth = intMaxForLayoutUnit;
        if (!underComputingPreferredWidth) {
            if (!isFrameReplaced() && style->width().isSpecified()) {
                LayoutUnit width =
                    style->width().specifiedValue(parentWidth, this);
                width = contentWidthAfterApplyingBoxSizing(width);
                minWidth = width;
            }

            PreferredWidthContext p(ctx, nullptr, this, this, parentWidth);
            p.setIntrinsicMode(true);
            p.computePreferredWidth();
            minWidth = std::max(p.preferredWidth(), p.preferredMinWidth());
            if (minWidth != intMaxForLayoutUnit && minWidth > width) {
                return minWidth;
            }
        }

    } else if (isFlexItem()) {
        LayoutUnit minWidth = intMaxForLayoutUnit;

        if (!underComputingPreferredWidth &&
            layoutParent()->asFrameFlexibleBox()->isMainAxisInInlineAxis() &&
            appliedOverflowX() == VisibleOverflow) {
            if (!isFrameReplaced() && style->width().isSpecified()) {
                LayoutUnit width =
                    style->width().specifiedValue(parentWidth, this);
                width = contentWidthAfterApplyingBoxSizing(width);

                minWidth = width;
            }

            PreferredWidthContext p(ctx, nullptr, this, this,
                                    parentWidth - mbpWidth());
            p.computePreferredWidth();
            minWidth = std::min(minWidth, p.preferredMinWidth());
        }

        if (minWidth != intMaxForLayoutUnit && minWidth > width) {
            return minWidth;
        }
    }

    if (style->maxWidth().isSpecified()) {
        if (style->maxWidth().isDefinite(!underComputingPreferredWidth)) {
            LayoutUnit maxWidth =
                style->maxWidth().specifiedValue(parentWidth, this);

            maxWidth = contentWidthAfterApplyingBoxSizing(maxWidth);

            if (maxWidth >= 0 && maxWidth < width) {
                return maxWidth;
            }
        }
    }
    return width;
}

LayoutUnit FrameBox::heightAfterApplyingMinMaxHeights(LayoutContext& ctx,
                                                      LayoutUnit height,
                                                      LayoutUnit parentHeight,
                                                      bool parentHasFixedValue)
{
    ComputedStyle* style = Frame::style();
    if (style->minHeight().isSpecified()) {
        if (!style->minHeight().isDefinite(parentHasFixedValue)) {
            return height;
        }

        LayoutUnit minHeight =
            style->minHeight().specifiedValue(parentHeight, this);

        minHeight = contentHeightAfterApplyingBoxSizing(minHeight);

        if (minHeight > height) {
            return minHeight;
        }
    } else if (isFlexItem()) {
        LayoutUnit minHeight = intMaxForLayoutUnit;
        auto flexibleBox = layoutParent()->asFrameFlexibleBox();

        if (!flexibleBox->shouldApplyLineClamp(this) &&
            !flexibleBox->isMainAxisInInlineAxis() &&
            appliedOverflowY() == VisibleOverflow) {
            // lookupFirstLineOrDefiniteHeight() scans the item's whole
            // subtree; reuse the previous scan while the subtree is clean and
            // the item's width is unchanged. A full layoutFlexItem() run
            // invalidates the entry, real damage clears the whole memo.
            Optional<LayoutUnit> result;
            FlexItemMeasureMemo* memo = flexItemMeasureMemo();
            if (!needsLayout() && memo && memo->m_firstLine.m_valid &&
                memo->m_firstLine.m_width == width()) {
                if (memo->m_firstLine.m_hasValue) {
                    result = memo->m_firstLine.m_value;
                }
            } else {
                result = ctx.lookupFirstLineOrDefiniteHeight(this);
                if (!needsLayout()) {
                    FlexItemMeasureMemo::FirstLineEntry& e =
                        ensureFlexItemMeasureMemo()->m_firstLine;
                    e.m_width = width();
                    e.m_value = result ? result.value() : LayoutUnit();
                    e.m_hasValue = result.hasValue();
                    e.m_valid = true;
                }
            }
            if (result) {
                minHeight = std::min(contentHeight(), result.value());
                // CSS Flexbox §4.5: the content-based minimum is capped by the
                // specified size suggestion -- the item's definite preferred
                // main size. (widthAfterApplyingMinMaxWidths() does the same
                // for a row flex item via its specified width.) Without this an
                // item with a definite height smaller than its content
                // over-expands to the content height, overriding the height.
                if (!style->height().isAuto() &&
                    style->height().isDefinite(parentHasFixedValue)) {
                    LayoutUnit specified = contentHeightAfterApplyingBoxSizing(
                        style->height().specifiedValue(parentHeight, this));
                    minHeight = std::min(minHeight, specified);
                }
            }
        }

        if (minHeight != intMaxForLayoutUnit && minHeight > height) {
            return minHeight;
        }
    }

    if (style->maxHeight().isSpecified()) {
        if (!style->maxHeight().isDefinite(parentHasFixedValue)) {
            return height;
        }

        LayoutUnit maxHeight =
            style->maxHeight().specifiedValue(parentHeight, this);

        maxHeight = contentHeightAfterApplyingBoxSizing(maxHeight);

        if (maxHeight >= 0 && maxHeight < height) {
            return maxHeight;
        }
    }
    return height;
}

LayoutUnit FrameBox::outlineThickness()
{
    LayoutUnit cbContentWidth = containingBlock(this)->contentWidth();
    LayoutUnit outlineWidth =
        style()->outlineWidth().specifiedValue(cbContentWidth, this);
    LayoutUnit outlineOffset =
        style()->outlineOffset().specifiedValue(cbContentWidth, this);
    return outlineWidth + outlineOffset;
}

LayoutRect computeBoxExtent(LayoutRect rt, const SkMatrix& m)
{
    auto tp = m.getType();
    if (!(tp & SkMatrix::TypeMask::kScale_Mask) &&
        !(tp & SkMatrix::TypeMask::kAffine_Mask) &&
        !(tp & SkMatrix::TypeMask::kPerspective_Mask)) {
        SkRect skRect = SkRect::MakeXYWH((float)rt.x(), (float)rt.y(),
                                         (float)rt.width(), (float)rt.height());
        m.mapRect(&skRect);
        skRect.sort();

        return LayoutRect(skRect.x(), skRect.y(), skRect.width(),
                          skRect.height());
    } else {
        SkPoint pt[4];

        pt[0].fX = rt.x();
        pt[0].fY = rt.y();

        pt[1].fX = rt.maxX();
        pt[1].fY = rt.y();

        pt[2].fX = rt.x();
        pt[2].fY = rt.maxY();

        pt[3].fX = rt.maxX();
        pt[3].fY = rt.maxY();

        m.mapPoints(pt, 4);

        LayoutUnit minX = pt[0].x();
        LayoutUnit minY = pt[0].y();
        LayoutUnit maxX = pt[0].x();
        LayoutUnit maxY = pt[0].y();
        for (size_t i = 1; i < 4; i++) {
            minX = std::min((float)pt[i].x(), (float)minX);
            minY = std::min((float)pt[i].y(), (float)minY);

            maxX = std::max((float)pt[i].x(), (float)maxX);
            maxY = std::max((float)pt[i].y(), (float)maxY);
        }

        return LayoutRect(minX, minY, (maxX - minX).abs(), (maxY - minY).abs());
    }
}

enum ComputeMatrixFor {
    Screen,
    GraphicsLayer,
    Window,
    GraphicsLayerOnGraphicsLayer
};

ALWAYS_INLINE void applyTransformIfNeeded(FrameBox* fBox, SkMatrix& m,
                                          bool inRendering)
{
    if (inRendering) {
        // The stacking-context tree is what painting and hit testing follow,
        // and a context exists only for a box that established one, so its
        // pointer answers this - unlike needToEstablishStackingContext(),
        // which re-derives the answer from style and from the box's overflow
        // status, once per box on every ancestor walk.
        StackingContext* sc = fBox->stackingContext();
        if (!sc) {
            return;
        }
        SkMatrix m2 = sc->transformMatrix();
        if (!m2.isIdentity()) {
            LayoutLocation to = sc->transformOrigin();
            m.preTranslate((float)to.x(), (float)to.y());
            m.preConcat(m2);
            m.preTranslate(-(float)to.x(), -(float)to.y());
        }
        return;
    }

    if (!fBox->needToEstablishStackingContext()) {
        return;
    }

    // fBox->style() maps to node()->style(), which can be cleared to null
    // while the layout tree is being torn down (e.g. a blur/focus change
    // triggering getBoundingClientRect() during app shutdown while media is
    // playing). The rendering branch above already null-checks
    // stackingContext(); mirror that here so a box whose style is gone
    // contributes no transform instead of crashing.
    ComputedStyle* cs = fBox->style();
    StyleTransformDataGroup* transforms = cs ? cs->transforms(fBox) : nullptr;
    if (!transforms) {
        return;
    }

    SkMatrix m2 =
        cs->transformsToMatrix(fBox->width(), fBox->height(), fBox, true);
    if (m2.isIdentity()) {
        return;
    }

    LayoutUnit ox = fBox->width() / 2;
    LayoutUnit oy = fBox->height() / 2;
    if (cs->hasTransformOrigin()) {
        StyleTransformOrigin* origin = cs->transformOrigin();
        auto od = origin->originValue();
        ox = od->getXAxis().specifiedValue(fBox->width(), fBox);
        oy = od->getYAxis().specifiedValue(fBox->height(), fBox);
    }
    m.preTranslate((float)ox, (float)oy);
    m.preConcat(m2);
    m.preTranslate(-(float)ox, -(float)oy);
}

// The ancestor walk in computeBoxMatrix() carries a small amount of state
// upward (the overflow status and whether scroll offsets still apply). Two
// walks that reach the same box with the same state produce the same matrix
// for everything above it, which is what the cache below keys on.
struct ScreenMatrixCacheState {
    Frame* child;
    FrameBox* absChild;
    ComputeMatrixFor forWhat;
    bool seenContainingBlockForAbsBlock;
    bool seenAbsBlock;
    bool seenFixedBlock;
    bool canScroll;

    ScreenMatrixCacheState()
        : child(nullptr)
        , absChild(nullptr)
        , forWhat(ComputeMatrixFor::Screen)
        , seenContainingBlockForAbsBlock(false)
        , seenAbsBlock(false)
        , seenFixedBlock(false)
        , canScroll(false)
    {
    }

    ScreenMatrixCacheState(const OverflowStatus& status,
                           ComputeMatrixFor forWhat, bool canScroll)
        : child(status.m_child)
        , absChild(status.m_absChild)
        , forWhat(forWhat)
        , seenContainingBlockForAbsBlock(
              status.m_seenContainingBlockForAbsBlock)
        , seenAbsBlock(status.m_seenAbsBlock)
        , seenFixedBlock(status.m_seenFixedBlock)
        , canScroll(canScroll)
    {
    }

    bool operator==(const ScreenMatrixCacheState& o) const
    {
        return child == o.child && absChild == o.absChild &&
               forWhat == o.forWhat &&
               seenContainingBlockForAbsBlock ==
                   o.seenContainingBlockForAbsBlock &&
               seenAbsBlock == o.seenAbsBlock &&
               seenFixedBlock == o.seenFixedBlock && canScroll == o.canScroll;
    }
};

struct ScreenMatrixCache {
    struct Entry {
        ScreenMatrixCacheState state;
        SkMatrix matrix;
        // GraphicsLayer walks stop at the owning buffer; a hit has to
        // report the holder the cached walk found.
        FrameBox* graphicsLayerHolder;
    };
    // Matrix from the root down to and including the keyed box, per state
    // the walk arrived at the box with. Almost always a single entry.
    std::unordered_map<FrameBox*, std::vector<Entry>> entries;

    bool lookup(FrameBox* box, const ScreenMatrixCacheState& state,
                SkMatrix& out, FrameBox*& graphicsLayerHolder)
    {
        auto iter = entries.find(box);
        if (iter == entries.end()) {
            return false;
        }
        for (const Entry& e : iter->second) {
            if (e.state == state) {
                out = e.matrix;
                graphicsLayerHolder = e.graphicsLayerHolder;
                return true;
            }
        }
        return false;
    }

    void insert(FrameBox* box, const ScreenMatrixCacheState& state,
                const SkMatrix& matrix, FrameBox* graphicsLayerHolder)
    {
        entries[box].push_back(Entry{ state, matrix, graphicsLayerHolder });
    }
};

ScreenMatrixCacheScope::ScreenMatrixCacheScope(WebView* webView)
    : m_webView(webView)
    , m_previous(webView->screenMatrixCache())
{
    webView->setScreenMatrixCache(new ScreenMatrixCache());
}

ScreenMatrixCacheScope::~ScreenMatrixCacheScope()
{
    end();
}

void ScreenMatrixCacheScope::end()
{
    if (!m_webView) {
        return;
    }
    delete m_webView->screenMatrixCache();
    m_webView->setScreenMatrixCache(m_previous);
    m_webView = nullptr;
}

static SkMatrix computeBoxMatrix(
    FrameBox* self, ComputeMatrixFor forWhat,
    bool includesScrollOnTopForGraphicsLayerMode = false)
{
    bool seenFixedPositionedLayer = false;
    bool inRendering = false;

    if (self->node() && self->node()->webView()->inRendering()) {
        inRendering = true;
    }

    FrameBox* graphicsLayerHolder = nullptr;

    VectorWithInlineStorage<32, std::pair<FrameBox*, bool>,
                            std::allocator<std::pair<FrameBox*, bool>>>
        frameList;

    Frame* f = self;
    OverflowStatus status(f);
    bool canScroll = OverflowStatus::isScrollableFrame(f);

    if (forWhat == ComputeMatrixFor::GraphicsLayerOnGraphicsLayer) {
        forWhat = GraphicsLayer;
        frameList.push_back(std::make_pair(f->asFrameBox(), canScroll));
        f = f->layoutParent();
    }

    // A walk can stop at the first stacking-context owner the cache already
    // has for the state the walk arrives with; every owner passed on a miss
    // is recorded on the way back down.
    // The cache lives on the WebView driving the pass; a box with no node
    // (an anonymous box) picks it up from the first ancestor that has one.
    ScreenMatrixCache* cache =
        self->node() ? self->node()->webView()->screenMatrixCache() : nullptr;
    FrameBox* cachedFrom = nullptr;
    SkMatrix cachedMatrix;
    VectorWithInlineStorage<
        8, std::pair<size_t, ScreenMatrixCacheState>,
        std::allocator<std::pair<size_t, ScreenMatrixCacheState>>>
        cacheProbes;

    while (f) {
        if (forWhat == ComputeMatrixFor::GraphicsLayer &&
            f->asFrameBox()->stackingContext() &&
            f->asFrameBox()->stackingContext()->needsGraphicsBuffer()) {
            graphicsLayerHolder = f->asFrameBox();
            break;
        }

        if (!cache && f != self && f->node()) {
            cache = f->node()->webView()->screenMatrixCache();
        }

        if (cache && f != self && f->asFrameBox()->stackingContext()) {
            ScreenMatrixCacheState state(status, forWhat, canScroll);
            if (cache->lookup(f->asFrameBox(), state, cachedMatrix,
                              graphicsLayerHolder)) {
                cachedFrom = f->asFrameBox();
                break;
            }
            cacheProbes.push_back(std::make_pair(frameList.size(), state));
        }

        bool applyOverflow = status.canApplyOverflow(f);
        bool canScrollNow = false;
        if (applyOverflow) {
            status.reset(f);
            canScroll =
                status.m_child->style()->position() != FixedPositionValue;
            canScrollNow = canScroll && f && f->isFrameBlockBox();
        } else {
            if (status.m_seenAbsBlock &&
                !status.m_seenContainingBlockForAbsBlock) {
                canScroll = false;
            }
            canScrollNow = canScroll && f && f->isFrameBlockBox();
        }

        if (canScroll) {
            if (f && f->style() &&
                f->style()->position() == FixedPositionValue) {
                canScroll = false;
            }
        }

        if (forWhat == ComputeMatrixFor::Window && f->isFrameDocument()) {
            break;
        }

        frameList.push_back(std::make_pair(f->asFrameBox(), canScrollNow));

        f = f->layoutParent();
    }

    SkMatrix m = SkMatrix::I();

    if (graphicsLayerHolder == self) {
        return m;
    }

    auto iter = frameList.rbegin();
    FrameBox* lastParentBox = nullptr;

    if (forWhat == ComputeMatrixFor::GraphicsLayer) {
        lastParentBox = graphicsLayerHolder;
    }

    if (cachedFrom) {
        m = cachedMatrix;
        lastParentBox = cachedFrom;
    }

    size_t frameIndex = frameList.size();
    size_t probeIndex = cacheProbes.size();

    while (iter != frameList.rend()) {
        frameIndex--;
        FrameBox* fBox = iter->first;
        LayoutLocation pos;
        if (fBox == self) {
            pos = fBox->absolutePoint(lastParentBox);
        } else if (fBox->isFrameDocument()) {
            pos = fBox->absolutePoint(lastParentBox);
            pos.setX(pos.x() - fBox->node()->window()->scrollX(false));
            pos.setY(pos.y() - fBox->node()->window()->scrollY(false));
        } else {
            pos = fBox->absolutePoint(lastParentBox);
            if (fBox->isFrameBlockBox() && iter->second) {
                Node* nd = fBox->node();
                if (nd) {
                    if (nd->asElement()->hasRareMembers()) {
                        pos.setX(pos.x() -
                                 nd->asElement()->rareMembers()->m_scrollLeft);
                        pos.setY(pos.y() -
                                 nd->asElement()->rareMembers()->m_scrollTop);
                    }
                }
            }
        }
        m.preTranslate((float)pos.x(), (float)pos.y());

        applyTransformIfNeeded(fBox, m, inRendering);

        lastParentBox = fBox;
        if (probeIndex && cacheProbes[probeIndex - 1].first == frameIndex) {
            probeIndex--;
            cache->insert(fBox, cacheProbes[probeIndex].second, m,
                          graphicsLayerHolder);
        }
        iter++;
    }

    if (forWhat == GraphicsLayer && graphicsLayerHolder &&
        includesScrollOnTopForGraphicsLayerMode) {
        if (graphicsLayerHolder->isFrameBlockBox()) {
            m.postTranslate(
                -graphicsLayerHolder->asFrameBlockBox()->scrollLeft(),
                -graphicsLayerHolder->asFrameBlockBox()->scrollTop());
        }
    }

    return m;
}

SkMatrix FrameBox::computeScreenMatrix(bool includesScrollOnTop)
{
    auto m = computeBoxMatrix(this, ComputeMatrixFor::Screen);
    if (includesScrollOnTop && shouldApplyOverflow() && isFrameBlockBox()) {
        m.postTranslate(-asFrameBlockBox()->scrollLeft(),
                        -asFrameBlockBox()->scrollTop());
    }
    return m;
}

SkMatrix FrameBox::computeMatrixOnGraphicsBuffer(bool includesScrollOnTop)
{
    return computeBoxMatrix(this, ComputeMatrixFor::GraphicsLayer,
                            includesScrollOnTop);
}

SkMatrix FrameBox::computeMatrixOnGraphicsBufferOnGraphicsBuffer()
{
    STARFISH_ASSERT(stackingContext()->needsGraphicsBuffer());
    return computeBoxMatrix(this,
                            ComputeMatrixFor::GraphicsLayerOnGraphicsLayer);
}

SkMatrix FrameBox::computeMatrixOnWindow()
{
    return computeBoxMatrix(this, ComputeMatrixFor::Window);
}

LayoutRect FrameBox::computeScreenExtent()
{
    SkMatrix screenMatrix = computeScreenMatrix();

    StackingContext* sc = stackingContext();
    LayoutRect vr;
    if (sc && sc->isRootContext()) {
        vr = LayoutRect(0, 0, asFrameBlockBox()->scrollWidth(),
                        asFrameBlockBox()->scrollHeight());
    } else {
        vr = frameVisibleRect();
    }

    return computeBoxExtent(vr, screenMatrix);
}
} // namespace Starfish
