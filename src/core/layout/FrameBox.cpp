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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/style/FilterFunctions.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/CanvasShadowData.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/modules/canvas/Compositor.h"
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

    // padding
    LengthData padding = style()->padding();
    if (padding.left().isSpecified() && !m_flags.m_isLeftMBPCleared) {
        setPaddingLeft(padding.left().specifiedValue(parentContentWidth, this));
    } else {
        setPaddingLeft(0);
    }
    if (padding.top().isSpecified()) {
        setPaddingTop(padding.top().specifiedValue(parentContentWidth, this));
    } else {
        setPaddingTop(0);
    }
    if (padding.right().isSpecified() && !m_flags.m_isRightMBPCleared) {
        setPaddingRight(
            padding.right().specifiedValue(parentContentWidth, this));
    } else {
        setPaddingRight(0);
    }
    if (padding.bottom().isSpecified()) {
        setPaddingBottom(
            padding.bottom().specifiedValue(parentContentWidth, this));
    } else {
        setPaddingBottom(0);
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
    BorderData border = style()->border();
    if (border.hasBorderStyle()) {
        if (border.left().width().isSpecified() &&
            !m_flags.m_isLeftMBPCleared) {
            setBorderLeft(
                border.left().width().specifiedValue(parentContentWidth, this));
        } else {
            setBorderLeft(0);
        }
        if (border.top().width().isSpecified()) {
            setBorderTop(
                border.top().width().specifiedValue(parentContentWidth, this));
        } else {
            setBorderTop(0);
        }
        if (border.right().width().isSpecified() &&
            !m_flags.m_isRightMBPCleared) {
            setBorderRight(border.right().width().specifiedValue(
                parentContentWidth, this));
        } else {
            setBorderRight(0);
        }
        if (border.bottom().width().isSpecified()) {
            setBorderBottom(border.bottom().width().specifiedValue(
                parentContentWidth, this));
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
    LengthData margin = style()->margin();
    if (margin.left().isSpecified() && !m_flags.m_isLeftMBPCleared) {
        setMarginLeft(margin.left().specifiedValue(parentContentWidth, this));
    } else {
        setMarginLeft(0);
    }
    if (margin.top().isSpecified()) {
        setMarginTop(margin.top().specifiedValue(parentContentWidth, this));
    } else {
        setMarginTop(0);
    }
    if (margin.right().isSpecified() && !m_flags.m_isRightMBPCleared) {
        setMarginRight(margin.right().specifiedValue(parentContentWidth, this));
    } else {
        setMarginRight(0);
    }
    if (margin.bottom().isSpecified()) {
        setMarginBottom(
            margin.bottom().specifiedValue(parentContentWidth, this));
    } else {
        setMarginBottom(0);
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
        case JustifyContentValue::FlexStartJustifyContentValue:
        case JustifyContentValue::SpaceBetweenJustifyContentValue:
            break;
        case JustifyContentValue::CenterJustifyContentValue:
        case JustifyContentValue::SpaceAroundJustifyContentValue:
            offset = (flexibleBox->contentWidth() - outerWidth()) / 2;
            break;
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
        }
    } else {
        JustifyContentValue justifyContent =
            flexibleBox->style()->justifyContent();
        switch (justifyContent) {
        case JustifyContentValue::FlexStartJustifyContentValue:
        case JustifyContentValue::SpaceBetweenJustifyContentValue:
            break;
        case JustifyContentValue::CenterJustifyContentValue:
        case JustifyContentValue::SpaceAroundJustifyContentValue:
            offset = (flexibleBox->contentHeight() - outerHeight()) / 2;
            break;
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
        style()->orignalTextAlign() ==
            TextAlignValue::StarfishCenterTextAlignValue) {
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
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }

        canvas->setFillColor(style()->outlineColor());

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

template <typename T>
void FrameBox::applyBorderRadius(T canvas, const LayoutRect& rect,
                                 float spreadDistance, bool inset)
{
    // apply clip if border-radius exists
    if (hasFrameBorderRadius()) {
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
                    canvas->scale(
                        1 * (bottomLeftHorizontal / bottomLeftVertical), 1);
                    arcR = bottomLeftVertical;
                } else {
                    canvas->scale(
                        1, 1 * (bottomLeftVertical / bottomLeftHorizontal));
                    arcR = bottomLeftHorizontal;
                }

                canvas->arcNegative(0, 0, arcR, M_PI,
                                    M_PI - M_PI / 2 + M_PI / 4);
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
                    canvas->scale(
                        1 * (bottomLeftHorizontal / bottomLeftVertical), 1);
                    arcR = bottomLeftVertical;
                } else {
                    canvas->scale(
                        1, 1 * (bottomLeftVertical / bottomLeftHorizontal));
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
                    canvas->scale(
                        1 * (bottomRightHorizontal / bottomRightVertical), 1);
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
                    canvas->scale(
                        1 * (bottomRightHorizontal / bottomRightVertical), 1);
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
                    canvas->scale(1 * (topRightHorizontal / topRightVertical),
                                  1);
                    arcR = topRightVertical;
                } else {
                    canvas->scale(1,
                                  1 * (topRightVertical / topRightHorizontal));
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
                    canvas->scale(1 * (topRightHorizontal / topRightVertical),
                                  1);
                    arcR = topRightVertical;
                } else {
                    canvas->scale(1,
                                  1 * (topRightVertical / topRightHorizontal));
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
                canvas->arcNegative(0, 0, arcR, M_PI + M_PI / 2,
                                    M_PI + M_PI / 4);
                canvas->restore();
            } else {
                canvas->lineTo(rect.x(), rect.y());
            }
        }
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

template <typename T>
void FrameBox::applyBorderRadiusClippingIfNeeds(T canvas,
                                                const LayoutRect& rect,
                                                float spreadDistance,
                                                bool inset)
{
    if (hasFrameBorderRadius()) {
        applyBorderRadius(canvas, rect, spreadDistance, inset);
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

    paintBorders(canvas, LayoutRect(0, 0, width(), height()));

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

                    auto shadowColor =
                        shadow->hasColor() ? shadow->color() : s->color();
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

                float shortSide =
                    std::min(shadowRect.width(), shadowRect.height());
                bool canUseFastPath =
                    (shadow->radius() < shortSide / 2) && shadow->radius() > 0;
                float topLeftHorizontal = 0;
                float topRightHorizontal = 0;
                float topLeftVertical = 0;
                float bottomLeftVertical = 0;
                float topRightVertical = 0;
                float bottomRightVertical = 0;
                float bottomLeftHorizontal = 0;
                float bottomRightHorizontal = 0;
                if (canUseFastPath && hasFrameBorderRadius()) {
                    auto br = frameBorderRadius();
                    computeBorderRadiusProperties(
                        br, frameRect(), topLeftHorizontal, topRightHorizontal,
                        topLeftVertical, bottomLeftVertical, topRightVertical,
                        bottomRightVertical, bottomLeftHorizontal,
                        bottomRightHorizontal);
                    if (topLeftHorizontal == topRightHorizontal &&
                        topRightHorizontal == topLeftVertical &&
                        topLeftVertical == bottomLeftVertical &&
                        bottomLeftVertical == topRightVertical &&
                        topRightVertical == bottomRightVertical &&
                        bottomRightVertical == bottomLeftHorizontal &&
                        bottomLeftHorizontal == bottomRightHorizontal) {
                        auto harfWidth = shadowRect.width() / 2;
                        auto harfHeight = shadowRect.height() / 2;
                        if (topLeftHorizontal > harfWidth) {
                            topLeftHorizontal = harfWidth;
                            topRightHorizontal = harfWidth;
                            bottomLeftHorizontal = harfWidth;
                            bottomRightHorizontal = harfWidth;
                        }

                        if (topLeftVertical > harfHeight) {
                            topLeftVertical = harfHeight;
                            bottomLeftVertical = harfHeight;
                            topRightVertical = harfHeight;
                            bottomRightVertical = harfHeight;
                        }

                    } else {
                        canUseFastPath = false;
                    }
                }
                if (canUseFastPath) {
                    canvas->save();
                    canvas->setNeedsNoneAntialias();
                    size_t bufImageSize = std::max((double)ceil(radiusOffset),
                                                   (double)topLeftHorizontal);
                    NativeImageData* nativeImage =
                        NativeImageData::create(bufImageSize, bufImageSize);
                    Canvas* cv = Canvas::create(node()->webView(), nativeImage);
                    cv->unsetDevicePixelRatio();
                    auto shadowColor =
                        shadow->hasColor() ? shadow->color() : s->color();
                    cv->setFillColor(shadowColor);
                    cv->clearColor(Unit::Color(0, 0, 0, 0));

                    cv->translate(ceil(radiusOffset / 2),
                                  ceil(radiusOffset / 2));
                    const LayoutRect clipRect(0, 0, shadowRect.width(),
                                              shadowRect.height());
                    applyBorderRadiusClippingIfNeeds(cv, clipRect, sd);
                    cv->drawRect(shadowRect);
                    ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                                  nativeImage->height(), nativeImage->stride());
                    sb.process(shadow->radius() / 2);
                    delete cv;

                    float offset = ceil(radiusOffset / 2);
                    Unit::Rect imageRect(
                        -offset + shadow->offsetX() - sd,
                        -offset + shadow->offsetY() - sd,
                        ceil(shadowRect.width() + radiusOffset),
                        ceil(shadowRect.height() + radiusOffset));

                    applyBorderShapeClippingUsedInPaintingBoxShadow(
                        shadowRect, borderRect, imageRect, canvas);

                    float pieceSize = bufImageSize;
                    // center

                    // pick color from blurred buffer
                    uint8_t* buf = nativeImage->data();
                    size_t edgeHeight =
                        (nativeImage->height() > 0 ? nativeImage->height() - 1
                                                   : 0);
                    size_t edgeWidth =
                        (nativeImage->width() > 0 ? nativeImage->width() - 1
                                                  : 0);
                    size_t base =
                        edgeHeight * nativeImage->stride() + edgeWidth * 4;
#ifdef PORT_PIXEL_ORDER_RGBA
                    unsigned char r = buf[base];
                    unsigned char g = buf[base + 1];
                    unsigned char b = buf[base + 2];
                    unsigned char a = buf[base + 3];
#else
                    unsigned char b = buf[base];
                    unsigned char g = buf[base + 1];
                    unsigned char r = buf[base + 2];
                    unsigned char a = buf[base + 3];
#endif

                    canvas->setFillColor(Unit::Color(r, g, b, a));
                    canvas->drawRect(Unit::Rect(
                        imageRect.x() + pieceSize, imageRect.y() + pieceSize,
                        imageRect.width() - pieceSize * 2,
                        imageRect.height() - pieceSize * 2));

                    // top-left
                    Unit::Rect src;
                    Unit::Rect dst;
                    src = Unit::Rect(0, 0, pieceSize, pieceSize);
                    dst = Unit::Rect(imageRect.x(), imageRect.y(), pieceSize,
                                     pieceSize);
                    DrawImageInfo drawImageInfo = {
                        1.0, 1.0, BorderImageRepeatValue::StretchValue,
                        BorderImageRepeatValue::StretchValue
                    };
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);

                    // top-left -> top-right
                    src = Unit::Rect(pieceSize - 1, 0, 1, pieceSize);
                    dst = Unit::Rect(imageRect.x() + pieceSize, imageRect.y(),
                                     imageRect.width() - pieceSize * 2,
                                     pieceSize);
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);

                    // top-right
                    src = Unit::Rect(0, 0, pieceSize, pieceSize);
                    dst = Unit::Rect(imageRect.maxX() - pieceSize,
                                     imageRect.y(), pieceSize, pieceSize);
                    canvas->save();
                    canvas->translate(dst.x(), dst.y());
                    dst.setX(0);
                    dst.setY(0);
                    canvas->translate(dst.width() / 2.f, dst.height() / 2.f);
                    canvas->rotate(UnitHelper::convertFromDegToRad(90));
                    canvas->translate(-dst.width() / 2.f, -dst.height() / 2.f);
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);
                    canvas->restore();

                    // top-right -> bottom-right
                    src = Unit::Rect(0, pieceSize - 1, pieceSize, 1);
                    dst = Unit::Rect(imageRect.maxX() - pieceSize,
                                     imageRect.y() + pieceSize, pieceSize,
                                     imageRect.height() - pieceSize * 2);
                    canvas->save();
                    canvas->translate(dst.x(), dst.y());
                    dst.setX(0);
                    dst.setY(0);
                    canvas->translate(dst.width() / 2.f, dst.height() / 2.f);
                    canvas->rotate(UnitHelper::convertFromDegToRad(180));
                    canvas->translate(-dst.width() / 2.f, -dst.height() / 2.f);
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);
                    canvas->restore();

                    // bottom-right
                    src = Unit::Rect(0, 0, pieceSize, pieceSize);
                    dst = Unit::Rect(imageRect.maxX() - pieceSize,
                                     imageRect.maxY() - pieceSize, pieceSize,
                                     pieceSize);
                    canvas->save();
                    canvas->translate(dst.x(), dst.y());
                    dst.setX(0);
                    dst.setY(0);
                    canvas->translate(dst.width() / 2.f, dst.height() / 2.f);
                    canvas->rotate(UnitHelper::convertFromDegToRad(180));
                    canvas->translate(-dst.width() / 2.f, -dst.height() / 2.f);
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);
                    canvas->restore();

                    // bottom-right -> bottom-left
                    src = Unit::Rect(pieceSize - 1, 0, 1, pieceSize);
                    dst = Unit::Rect(
                        imageRect.x() + pieceSize, imageRect.maxY() - pieceSize,
                        imageRect.width() - pieceSize * 2, pieceSize);
                    canvas->save();
                    canvas->translate(dst.x(), dst.y());
                    dst.setX(0);
                    dst.setY(0);
                    canvas->translate(dst.width() / 2.f, dst.height() / 2.f);
                    canvas->rotate(UnitHelper::convertFromDegToRad(180));
                    canvas->translate(-dst.width() / 2.f, -dst.height() / 2.f);
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);
                    canvas->restore();

                    // bottom-left
                    src = Unit::Rect(0, 0, pieceSize, pieceSize);
                    dst =
                        Unit::Rect(imageRect.x(), imageRect.maxY() - pieceSize,
                                   pieceSize, pieceSize);
                    canvas->save();
                    canvas->translate(dst.x(), dst.y());
                    dst.setX(0);
                    dst.setY(0);
                    canvas->translate(dst.width() / 2.f, dst.height() / 2.f);
                    canvas->rotate(UnitHelper::convertFromDegToRad(270));
                    canvas->translate(-dst.width() / 2.f, -dst.height() / 2.f);
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);
                    canvas->restore();

                    // bottom-left -> top-left
                    src = Unit::Rect(0, pieceSize - 1, pieceSize, 1);
                    dst = Unit::Rect(imageRect.x(), imageRect.y() + pieceSize,
                                     pieceSize,
                                     imageRect.height() - pieceSize * 2);
                    canvas->drawImage(nativeImage, src, dst, drawImageInfo);

                    canvas->restore();

                    delete nativeImage;
                } else {
                    canvas->save();
                    NativeImageData* nativeImage = NativeImageData::create(
                        ceil(shadowRect.width() + radiusOffset),
                        ceil(shadowRect.height() + radiusOffset));
                    Canvas* cv = Canvas::create(node()->webView(), nativeImage);
                    cv->unsetDevicePixelRatio();
                    cv->clearColor(Unit::Color(0, 0, 0, 0));

                    if (shadow->hasColor()) {
                        cv->setFillColor(shadow->color());
                    } else {
                        cv->setFillColor(s->color());
                    }

                    cv->translate(ceil(radiusOffset / 2),
                                  ceil(radiusOffset / 2));
                    const LayoutRect clipRect(0, 0, shadowRect.width(),
                                              shadowRect.height());
                    applyBorderRadiusClippingIfNeeds(cv, clipRect, sd);
                    cv->drawRect(shadowRect);

                    ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                                  nativeImage->height(), nativeImage->stride());
                    sb.process(shadow->radius() / 2);
                    delete cv;

                    float offset = ceil(radiusOffset / 2);
                    Unit::Rect imageRect(
                        -offset + shadow->offsetX() - sd,
                        -offset + shadow->offsetY() - sd,
                        ceil(shadowRect.width() + radiusOffset),
                        ceil(shadowRect.height() + radiusOffset));

                    applyBorderShapeClippingUsedInPaintingBoxShadow(
                        shadowRect, borderRect, imageRect, canvas);
                    canvas->drawImage(nativeImage, imageRect);
                    canvas->restore();

                    delete nativeImage;
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

            if (shadow->inset()) {
                Unit::Rect borderRect = makeRect(BoxValue::BorderBoxBoxValue);
                Unit::Rect paddingRect = makeRect(BoxValue::PaddingBoxBoxValue);
                Unit::Rect shadowRect(
                    paddingRect.x(), paddingRect.y(),
                    paddingRect.width() + std::abs(shadow->offsetX()),
                    paddingRect.height() + std::abs(shadow->offsetY()));

                int ix = 0, iy = 0, iw = 0, ih = 0;
                LayoutUnit x = ((shadow->offsetX() < 0)
                                    ? 0.0f
                                    : paddingRect.x() + shadow->offsetX()) +
                               sd;
                LayoutUnit y = ((shadow->offsetY() < 0)
                                    ? 0.0f
                                    : paddingRect.y() + shadow->offsetY()) +
                               sd;
                ix = x.floor();
                iy = y.floor();
                iw = snapSizeToPixel(paddingRect.width() - sd * 2, x);
                ih = snapSizeToPixel(paddingRect.height() - sd * 2, y);
                Unit::Rect interiorRect(ix, iy, iw, ih);

                Unit::Rect exteriorRect;
                exteriorRect.unite(borderRect);
                exteriorRect.unite(shadowRect);
                exteriorRect.unite(interiorRect);

                // Create image buffer bigger than paddingbox+ shadowBox
                const float margin = 4.0f;
                const float half = 2.0f;

                Unit::Rect ImageRect(0, 0, exteriorRect.width() + margin,
                                     exteriorRect.height() + margin);

                NativeImageData* nativeImage = NativeImageData::create(
                    ceil(ImageRect.width()), ceil(ImageRect.height()));
                Canvas* cv = Canvas::create(node()->webView(), nativeImage);
                cv->unsetDevicePixelRatio();
                cv->clearColor(Unit::Color(0, 0, 0, 0));

                if (shadow->hasColor()) {
                    cv->setFillColor(shadow->color());
                } else {
                    cv->setFillColor(s->color());
                }
                // Draw an outline of Image
                cv->beginPath();
                cv->moveTo(ImageRect.x(), ImageRect.y());
                cv->lineTo(ImageRect.x() + ImageRect.width(), ImageRect.y());
                cv->lineTo(ImageRect.x() + ImageRect.width(),
                           ImageRect.y() + ImageRect.height());
                cv->lineTo(ImageRect.x(), ImageRect.y() + ImageRect.height());
                cv->lineTo(ImageRect.x(), ImageRect.y());
                cv->closePath();

                cv->translate(half, half);

                // Draw a shadow box that will not be filled.
                if (hasFrameBorderRadius()) {
                    const LayoutRect rect(interiorRect.x(), interiorRect.y(),
                                          interiorRect.width(),
                                          interiorRect.height());
                    // apply inner border radius line(anti-clock)
                    applyBorderRadius(cv, rect, sd, true);
                } else {
                    // draw interiorRect
                    const LayoutRect rect(interiorRect.x(), interiorRect.y(),
                                          interiorRect.width(),
                                          interiorRect.height());
                    cv->setFillRule(false);
                    cv->drawPixelSnappedRect(rect);
                }
                cv->fill();

                ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                              nativeImage->height(), nativeImage->stride());
                sb.process(shadow->radius() / 2);
                delete cv;

                int xx = 0, yy = 0, ww = 0, hh = 0;
                LayoutUnit rx = paddingRect.x();
                LayoutUnit ry = paddingRect.y();
                xx = rx.floor();
                yy = ry.floor();
                ww = snapSizeToPixel(paddingRect.width(), rx);
                hh = snapSizeToPixel(paddingRect.height(), ry);
                Unit::Rect rect(xx, yy, ww, hh);
                canvas->save();
                if (hasFrameBorderRadius()) {
                    const LayoutRect r(rect.x(), rect.y(), rect.width(),
                                       rect.height());
                    applyBorderRadiusClippingIfNeeds(canvas, r, 0, true);
                } else {
                    canvas->clip(rect);
                }

                float dx =
                    (shadow->offsetX() < 0) ? -half + shadow->offsetX() : -half;
                float dy =
                    (shadow->offsetY() < 0) ? -half + shadow->offsetY() : -half;

                canvas->translate(dx, dy);
                canvas->drawImage(nativeImage, ImageRect);
                canvas->restore();

                delete nativeImage;
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

static inline void paintRepeatGradient(
    Canvas* canvas, FrameBox* box, ComputedStyle* style,
    const unsigned int& idx, Unit::Rect dst, const float& width,
    const float& height, bool repeatX, bool repeatY,
    const ImageRenderingValue& imageRenderingValue)
{
    float startX = dst.x();
    float startY = dst.y();

    if (repeatX) {
        startX = fmodf(startX, width);
        if (startX > 0) {
            startX -= width;
        }
    }
    if (repeatY) {
        startY = fmodf(startY, height);
        if (startY > 0) {
            startY -= height;
        }
    }
    ImageValue* imageValue = style->backgroundImage(idx);
    auto value = imageValue->gradientValue();
    bool cacheable = (value->isCacheable() &&
                      ((width * height) >= CACHEABLE_GRADIENT_SIZE) &&
                      ((width * height) <= STARFISH_NATIVEGRADIENT_CACHE_SIZE));

    if (cacheable) {
        Unit::Rect rect = Unit::Rect(0, 0, width, height).snapSizeToPixel();
        GradientDrawingInfo* info = value->makeGradientDrawingInfo(rect, box);
        std::shared_ptr<NativeGradient> gradient =
            box->document()->findInNativeGradientCache(info);
        if (gradient.get() == nullptr) {
            gradient = NativeGradient::create(info);
        }
        canvas->save();
        if (gradient->gradientImageDataCached() == nullptr) {
            auto imageData = NativeImageData::create(ceil(width), ceil(height));
            Canvas* cv = Canvas::create(box->node()->webView(), imageData);

            cv->clearColor(Unit::Color(0, 0, 0, 0));
            if (imageValue->gradientValue()->type() ==
                GradientType::LinearGradient) {
                cv->drawLinearGradient(rect, info, gradient.get());
            } else if (imageValue->gradientValue()->type() ==
                       GradientType::RadialGradient) {
                cv->drawRadialGradient(rect, info, gradient.get());
            }
            cv->fill();
            delete cv;

            gradient->setGradientImageDataCached(imageData);
            box->document()->cacheNativeGradient(info, gradient);
        }
        canvas->drawRepeatImage(gradient->gradientImageDataCached(), dst, width,
                                height, repeatX, repeatY, imageRenderingValue);
        canvas->restore();
    } else {
        Unit::Rect rect =
            Unit::Rect(startX, startY, width, height).snapSizeToPixel();
        GradientDrawingInfo* info = value->makeGradientDrawingInfo(rect, box);
        std::shared_ptr<NativeGradient> gradient = NativeGradient::create(info);
        canvas->save();
        for (float y = startY; y < dst.maxY();
             y += height, canvas->translate(0, rect.height())) {
            canvas->save();
            for (float x = startX; x < dst.maxX();
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
            width = id->width();
            height = id->height();
        } else if (type.isGradient()) {
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
            (repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
             repeatY == BackgroundRepeatValue::RepeatRepeatValue)) {
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
                paintRepeatGradient(canvas, box, style, idx,
                                    Unit::Rect(x, y, paintingW, paintingH),
                                    imgW, imgH, true, true,
                                    imageRenderingValue);
            }
        } else if (shouldApplyRepeat &&
                   repeatX == BackgroundRepeatValue::NoRepeatRepeatValue &&
                   repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
            if (type.isURL()) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, imgW, paintingH),
                                        imgW, imgH, false, true,
                                        imageRenderingValue);
            } else if (type.isGradient()) {
                paintRepeatGradient(canvas, box, style, idx,
                                    Unit::Rect(x, y, imgW, paintingH), imgW,
                                    imgH, false, true, imageRenderingValue);
            }

        } else if (shouldApplyRepeat &&
                   repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
                   repeatY == BackgroundRepeatValue::NoRepeatRepeatValue) {
            if (type.isURL()) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, paintingW, imgH),
                                        imgW, imgH, true, false,
                                        imageRenderingValue);
            } else if (type.isGradient()) {
                paintRepeatGradient(canvas, box, style, idx,
                                    Unit::Rect(x, y, paintingW, imgH), imgW,
                                    imgH, true, false, imageRenderingValue);
            }
        } else {
            if (type.isURL()) {
                canvas->drawImage(id, Unit::Rect(x, y, imgW, imgH),
                                  imageRenderingValue);
            } else if (type.isGradient()) {
                ImageValue* imageValue = style->backgroundImage(idx);
                Unit::Rect rect =
                    Unit::Rect(x, y, imgW, imgH).snapSizeToPixel();
                auto info =
                    imageValue->gradientValue()->makeGradientDrawingInfo(rect,
                                                                         box);
                auto gradient = NativeGradient::create(info);

                if (imageValue->gradientValue()->type() ==
                    GradientType::LinearGradient) {
                    canvas->drawLinearGradient(rect, info, gradient.get());
                } else if (imageValue->gradientValue()->type() ==
                           GradientType::RadialGradient) {
                    canvas->drawRadialGradient(rect, info, gradient.get());
                }
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
    std::vector<double> dashes{ 2.0, 1.0 };
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
                imgData, Unit::Rect(imgWidth - rSlice, imgHeight - bSlice,
                                    rSlice, bSlice),
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
                    imgData, Unit::Rect(lSlice, imgHeight - bSlice,
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
                    imgData, Unit::Rect(0, tSlice, lSlice,
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
                    imgData, Unit::Rect(imgWidth - rSlice, tSlice, rSlice,
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
                } else if ((border.bottom().style() ==
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
                } else if ((border.bottom().style() ==
                            BorderStyleValue::GrooveBorderStyleValue) ||
                           (border.bottom().style() ==
                            BorderStyleValue::RidgeBorderStyleValue)) {
                    if ((border.bottom().style() ==
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

                    if ((border.bottom().style() ==
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

void FrameBox::paintChildrenWith(PaintingContext& ctx)
{
    Frame* child = firstChild();
    while (child) {
        ctx.m_canvas->translate(child->asFrameBox()->x(),
                                child->asFrameBox()->y());
        child->asFrameBox()->paintContent(ctx);
        ctx.m_canvas->translate(-child->asFrameBox()->x(),
                                -child->asFrameBox()->y());
        child = child->next();
    }
}

void FrameBox::paintStackingContextContent(Canvas* canvas)
{
    PaintingContext ctx(canvas);

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
        } else {
            FrameBox* p;
            p = layoutParent()->asFrameBox();
            while (true) {
                if (p->needToEstablishStackingContext()) {
                    if (p->canOwnsStackingContext() ||
                        p->shouldApplyOverflow()) {
                        break;
                    }
                }
                p = p->layoutParent()->asFrameBox();
            }
            ensureFrameBoxRareData()->m_stackingContext =
                new StackingContext(this, p->stackingContext());
        }
    }
}

ALWAYS_INLINE LayoutRect computeVisibleShadowRect(
    const LayoutRect& owner, const CanvasShadowData& shadow)
{
    LayoutRect ret = owner;
    if (!shadow.inset()) {
        float radiusOffset = 0.0f;
        if (shadow.radius()) {
            radiusOffset = shadow.radius();
            radiusOffset =
                ShadowBlur::computeKernelSizeAtStdDeviation(radiusOffset / 2);
        }
        float sd = shadow.spreadDistance();
        LayoutRect rect(owner.x() + shadow.offsetX() - radiusOffset - sd,
                        owner.y() + shadow.offsetY() - radiusOffset - sd,
                        ceil(owner.width() + sd * 2 + radiusOffset * 2),
                        ceil(owner.height() + sd * 2 + radiusOffset * 2));

        ret.unite(rect);
    }
    return ret;
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
            CanvasShadowDataList list =
                textShadow->toCanvasShadowDataList(this);
            LayoutRect owner = frameRect();
            owner.setX(0);
            owner.setY(0);
            for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
                LayoutRect rect = computeVisibleShadowRect(owner, *shadow);
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

    CanvasShadowDataList list = boxShadow->toCanvasShadowDataList(this);
    for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
        LayoutRect rect = computeVisibleShadowRect(owner, *shadow);
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

static bool styleHasDrawableContents(ComputedStyle* cs, FrameBox* b)
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

    if (background && !background->color().isTransparent()) {
        return true;
    }

    if (background && background->sizeOfLayers()) {
        return true;
    }

    if (outline &&
        outline->border().style() != BorderStyleValue::NoneBorderStyleValue &&
        outline->border().width().specifiedValue(1, b->node())) {
        return true;
    }

    if (boxShadow && boxShadow->size()) {
        return true;
    }

    if (filter && filter->size()) {
        return true;
    }

    if (border && (border->hasBorderStyle() || !border->image().isNull())) {
        return true;
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

    return false;
}

bool FrameBox::tryUniteVisibleRect(Frame::ComputeVisibleRectContext& ctx)
{
    if (ctx.sourceStackingContext &&
        (this != ctx.sourceStackingContext->owner() && stackingContext() &&
         stackingContext()->needsGraphicsBuffer())) {
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
        if (!isAncestor && !isBrother) {
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
    if (!cs) {
        if (isLineBox()) {
            parentStyle = layoutParent()->style();
        }
    }

    if ((cs && cs->visibility() == HiddenVisibilityValue) ||
        (parentStyle && parentStyle->visibility() == HiddenVisibilityValue)) {
        return true;
    }

    if (ctx.isVisibleRectCollapsible &&
        this != ctx.sourceStackingContext->owner() &&
        ctx.purpose != Frame::ComputeVisibleRectContext::Scrolling && cs &&
        cs->opacity() == 0) {
        return false;
    }

    if (ctx.isVisibleRectCollapsible && isBoxesInvisibleFromHere()) {
        return false;
    }

    bool ret = !shouldApplyOverflow();
    bool boxHasDrawableContents = true;
    bool drawableContentsInStyle = true;
    if (ctx.isVisibleRectCollapsible) {
        drawableContentsInStyle = cs && styleHasDrawableContents(cs, this);
    }
    if (isFrameBlockBox() && !drawableContentsInStyle) {
        boxHasDrawableContents = false;
    } else if (isFrameReplaced() &&
               asFrameReplaced()->isFrameReplacedIFrame() &&
               !drawableContentsInStyle) {
        if (node()->asHTMLIFrameElement()->browsingContext()) {
            if (node()
                    ->asHTMLIFrameElement()
                    ->browsingContext()
                    ->rootStackingContextNeedsGraphicsBuffer()) {
                boxHasDrawableContents = false;
            }
        }
    }

    if (!ctx.isVisibleRectCollapsible || boxHasDrawableContents) {
        if (ctx.purpose == Frame::ComputeVisibleRectContext::Scrolling) {
            ctx.uniteRect(frameScrollingRect());
        } else {
            ctx.uniteRect(frameVisibleRect());
        }
    }

    if (ctx.isVisibleRectCollapsible && isFrameBlockBox() &&
        !boxHasDrawableContents) {
        ret = true;
    }

    if (ctx.isForSpecialValueForTableCell &&
        ctx.purpose == Frame::ComputeVisibleRectContext::Scrolling &&
        isFrameFlexibleBox()) {
        if (cs->height().isDefinite(true)) {
            ret = false;
        }
    }

    if (boxHasDrawableContents) {
        size_t len = isInlineTextBox() ? 2 : 1;
        for (size_t i = 0; cs && i < len; ++i) {
            CanvasShadowDataList list;
            LayoutRect owner = frameRect();
            owner.setX(0);
            owner.setY(0);
            LayoutRect shadowsRect = owner;
            bool hasShadow = false;
            if (i == 0) {
                if (cs->boxShadow()) {
                    list = cs->boxShadow()->toCanvasShadowDataList(this);
                    hasShadow = true;
                }
            } else {
                if (cs->textShadow()) {
                    list = cs->textShadow()->toCanvasShadowDataList(this);
                    hasShadow = true;
                }
            }
            for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
                if (ctx.isVisibleRectCollapsible && isFrameBlockBox()) {
                    if (shadow->hasColor() &&
                        !shadow->color().isTransparent()) {
                        LayoutRect rect =
                            computeVisibleShadowRect(owner, *shadow);
                        shadowsRect.unite(rect);
                    } else if (!shadow->hasColor() &&
                               !cs->color().isTransparent()) {
                        LayoutRect rect =
                            computeVisibleShadowRect(owner, *shadow);
                        shadowsRect.unite(rect);
                    }
                } else if (ctx.isForSpecialValueForTableCell &&
                           ctx.purpose ==
                               Frame::ComputeVisibleRectContext::Scrolling &&
                           isFrameFlexibleBox()) {
                    LayoutRect rect = computeVisibleShadowRect(owner, *shadow);
                    shadowsRect.unite(rect);
                    ret = false;
                }
            }
            if (hasShadow) {
                ctx.uniteRect(shadowsRect);
            }
        }
    }

    return ret;
}

void FrameBox::computeVisibleRect(Frame::ComputeVisibleRectContext& ctx)
{
    Frame::ComputeVisibleRectContextFragment f(ctx, this);
    tryUniteVisibleRect(ctx);
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
    } else if (isFlexItem()) {
        LayoutUnit minWidth = intMaxForLayoutUnit;

        if (!underComputingPreferredWidth &&
            layoutParent()->asFrameFlexibleBox()->isMainAxisInInlineAxis() &&
            appliedOverflowX() == VisibleOverflow) {
            if (isFrameReplaced()) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            } else {
                if (style->width().isSpecified()) {
                    LayoutUnit width =
                        style->width().specifiedValue(parentWidth, this);
                    width = contentWidthAfterApplyingBoxSizing(width);

                    minWidth = width;
                }
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

        if (!layoutParent()->asFrameFlexibleBox()->isMainAxisInInlineAxis() &&
            appliedOverflowY() == VisibleOverflow) {
            if (isFrameReplaced()) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            } else {
                if (style->height().isDefinite(parentHasFixedValue)) {
                    LayoutUnit height = LayoutUnit(
                        style->height().specifiedValue(parentHeight, this));
                    height = contentHeightAfterApplyingBoxSizing(height);

                    minHeight = height;
                }
            }
            minHeight = std::min(minHeight, ctx.contentHeight(this));
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
    if (fBox->needToEstablishStackingContext()) {
        if (inRendering) {
            StackingContext* sc = fBox->stackingContext();
            if (sc) {
                SkMatrix m2 = sc->transformMatrix();
                if (!m2.isIdentity()) {
                    LayoutLocation to =
                        fBox->stackingContext()->transformOrigin();
                    m.preTranslate((float)to.x(), (float)to.y());
                    m.preConcat(m2);
                    m.preTranslate(-(float)to.x(), -(float)to.y());
                }
            }
        } else {
            ComputedStyle* cs = fBox->style();
            StyleTransformDataGroup* transforms = cs->transforms(fBox);
            if (transforms) {
                SkMatrix m2 = cs->transformsToMatrix(
                    fBox->width(), fBox->height(), fBox, true);
                if (!m2.isIdentity()) {
                    LayoutUnit ox = fBox->width() / 2;
                    LayoutUnit oy = fBox->height() / 2;
                    if (cs->hasTransformOrigin()) {
                        StyleTransformOrigin* origin = cs->transformOrigin();
                        auto od = origin->originValue();
                        ox = od->getXAxis().specifiedValue(fBox->width(), fBox);
                        oy =
                            od->getYAxis().specifiedValue(fBox->height(), fBox);
                    }
                    m.preTranslate((float)ox, (float)oy);
                    m.preConcat(m2);
                    m.preTranslate(-(float)ox, -(float)oy);
                }
            }
        }
    }
}

static SkMatrix computeBoxMatrix(FrameBox* self, ComputeMatrixFor forWhat)
{
    bool seenFixedPositionedLayer = false;
    bool inRendering = false;

    if (self->node() && self->node()->webView()->inRendering()) {
        inRendering = true;
    }

    FrameBox* turnOffScrollUntilMeet = nullptr;
    FrameBox* graphicsLayerHolder = nullptr;

    std::vector<FrameBox*> frameList;
    frameList.reserve(32);

    Frame* f = self;

    if (forWhat == ComputeMatrixFor::GraphicsLayerOnGraphicsLayer) {
        forWhat = GraphicsLayer;
        frameList.push_back(f->asFrameBox());
        f = f->layoutParent();
    }

    while (f) {
        if (forWhat == ComputeMatrixFor::GraphicsLayer &&
            f->asFrameBox()->stackingContext() &&
            f->asFrameBox()->stackingContext()->needsGraphicsBuffer()) {
            graphicsLayerHolder = f->asFrameBox();
            break;
        }

        if (f->style() && f->style()->position() == FixedPositionValue) {
            if (!seenFixedPositionedLayer) {
                seenFixedPositionedLayer = true;
                turnOffScrollUntilMeet = f->asFrameBox();
            }
        }

        if (forWhat == ComputeMatrixFor::Window && f->isFrameDocument()) {
            break;
        }

        frameList.push_back(f->asFrameBox());

        f = f->layoutParent();
    }

    SkMatrix m = SkMatrix::I();
    if (seenFixedPositionedLayer) {
        FrameBox* top = *frameList.rbegin();
        if (top->isFrameBlockBox()) {
            m.preTranslate(top->asFrameBlockBox()->scrollLeft(),
                           top->asFrameBlockBox()->scrollTop());
        }

        auto iter = frameList.rbegin();
        FrameBox* lastParentBox = nullptr;

        if (forWhat == ComputeMatrixFor::GraphicsLayer) {
            lastParentBox = graphicsLayerHolder;
        }

        bool canScroll = false;
        while (iter != frameList.rend()) {
            FrameBox* fBox = *iter;
            LayoutLocation pos;
            if (canScroll) {
                pos = fBox->absolutePointIncludingScroll(lastParentBox);
            } else {
                pos = fBox->absolutePoint(lastParentBox);
                if (fBox == turnOffScrollUntilMeet) {
                    canScroll = true;
                }
            }
            m.preTranslate((float)pos.x(), (float)pos.y());

            applyTransformIfNeeded(fBox, m, inRendering);

            lastParentBox = fBox;
            iter++;
        }
    } else {
        auto iter = frameList.rbegin();
        FrameBox* lastParentBox = nullptr;

        if (forWhat == ComputeMatrixFor::GraphicsLayer) {
            lastParentBox = graphicsLayerHolder;
        }

        while (iter != frameList.rend()) {
            FrameBox* fBox = *iter;
            LayoutLocation pos;
            if (fBox == self) {
                pos = fBox->absolutePoint(lastParentBox);
            } else if (fBox->isFrameDocument()) {
                pos = fBox->absolutePoint(lastParentBox);
                pos.setX(pos.x() - fBox->node()->window()->scrollX(false));
                pos.setY(pos.y() - fBox->node()->window()->scrollY(false));
            } else {
                pos = fBox->absolutePoint(lastParentBox);
                if (fBox->isFrameBlockBox()) {
                    Node* nd = fBox->node();
                    if (nd) {
                        if (nd->asElement()->hasRareMembers()) {
                            pos.setX(
                                pos.x() -
                                nd->asElement()->rareMembers()->m_scrollLeft);
                            pos.setY(
                                pos.y() -
                                nd->asElement()->rareMembers()->m_scrollTop);
                        }
                    }
                }
            }
            m.preTranslate((float)pos.x(), (float)pos.y());

            applyTransformIfNeeded(fBox, m, inRendering);

            lastParentBox = fBox;
            iter++;
        }
    }

    if (forWhat == GraphicsLayer && graphicsLayerHolder) {
        if (graphicsLayerHolder->isFrameBlockBox()) {
            m.postTranslate(
                -graphicsLayerHolder->asFrameBlockBox()->scrollLeft(),
                -graphicsLayerHolder->asFrameBlockBox()->scrollTop());
        }
    }

    return m;
}

SkMatrix FrameBox::computeScreenMatrix()
{
    return computeBoxMatrix(this, ComputeMatrixFor::Screen);
}

SkMatrix FrameBox::computeMatrixOnGraphicsBuffer()
{
    return computeBoxMatrix(this, ComputeMatrixFor::GraphicsLayer);
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
}
