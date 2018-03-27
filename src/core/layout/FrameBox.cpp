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
#include "core/dom/Document.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/modules/canvas/ShadowBlur.h"

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
#if defined(PORT_GRAPHIC_BACKEND_EFL)
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBoxRareData, m_bufferForBorderRadius));
#endif
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

    return VerticalDataLocToContainingBlock(containgBlockContentHeight, absY, t,
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
            TextAlignValue::StarFishCenterTextAlignValue) {
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
            node()->document()->frame()->asFrameBox());
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

        LayoutRect rt = frameRect();

        rt.setX(-offset);
        rt.setY(-offset);

        rt.setWidth(rt.width() + offset * 2);
        rt.setHeight(rt.height() + offset * 2);

        if (s != BorderStyleValue::SolidBorderStyleValue) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }

        canvas->setColor(style()->outlineColor());

        // top
        canvas->drawRect(LayoutRect(rt.x(), rt.y(), rt.width() - outlineWidth,
                                    outlineWidth));
        // right
        canvas->drawRect(LayoutRect(rt.x() + rt.width() - outlineWidth, rt.y(),
                                    outlineWidth, rt.height() - outlineWidth));
        // bottom
        canvas->drawRect(LayoutRect(rt.x() + outlineWidth,
                                    rt.y() + rt.height() - outlineWidth,
                                    rt.width() - outlineWidth, outlineWidth));
        // left
        canvas->drawRect(LayoutRect(rt.x(), rt.y() + outlineWidth, outlineWidth,
                                    rt.height() - outlineWidth));

        canvas->restore();
    }
}

void FrameBox::applyBorderRadius(Canvas* canvas, const LayoutRect& rect,
                                 float spreadDistance, bool inset)
{
    // apply clip if border-radius exists
    if (style()->hasBorderRadius()) {
        // const LayoutRect rect(0, 0, width(), height());
        auto br = style()->borderRadius();

        float maxWidthValue = rect.width();
        float maxHeightValue = rect.height();

        if (!br.m_topLeftVertical.isPercent() ||
            !br.m_bottomLeftVertical.isPercent() ||
            !br.m_bottomLeftVertical.isPercent() ||
            !br.m_bottomRightVertical.isPercent() ||
            !br.m_bottomLeftHorizontal.isPercent() ||
            !br.m_bottomRightHorizontal.isPercent() ||
            !br.m_topLeftHorizontal.isPercent() ||
            !br.m_topRightHorizontal.isPercent()) {
            maxHeightValue = maxWidthValue =
                std::min(maxHeightValue, maxWidthValue);
        }

        float topLeftHorizontal =
            br.m_topLeftHorizontal.specifiedValue(width(), this);
        float topRightHorizontal =
            br.m_topRightHorizontal.specifiedValue(width(), this);

        float topLeftVertical =
            br.m_topLeftVertical.specifiedValue(height(), this);
        float bottomLeftVertical =
            br.m_bottomLeftVertical.specifiedValue(height(), this);

        float topRightVertical =
            br.m_topRightVertical.specifiedValue(height(), this);
        float bottomRightVertical =
            br.m_bottomRightVertical.specifiedValue(height(), this);

        float bottomLeftHorizontal =
            br.m_bottomLeftHorizontal.specifiedValue(width(), this);
        float bottomRightHorizontal =
            br.m_bottomRightHorizontal.specifiedValue(width(), this);

        if (inset) {
#define APPLY_BORDER_WIDTH(POS, BORDER_SIDE) \
    if (POS) {                               \
        POS -= BORDER_SIDE();                \
    }
            APPLY_BORDER_WIDTH(topLeftHorizontal, borderLeft);
            APPLY_BORDER_WIDTH(topRightHorizontal, borderRight);
            APPLY_BORDER_WIDTH(topLeftVertical, borderTop);
            APPLY_BORDER_WIDTH(bottomLeftVertical, borderBottom);
            APPLY_BORDER_WIDTH(topRightVertical, borderTop);
            APPLY_BORDER_WIDTH(bottomRightVertical, borderBottom);
            APPLY_BORDER_WIDTH(bottomLeftHorizontal, borderLeft);
            APPLY_BORDER_WIDTH(bottomRightHorizontal, borderRight);

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
            APPLY_SPREAD_DISTANCE(topLeftHorizontal);
            APPLY_SPREAD_DISTANCE(topRightHorizontal);
            APPLY_SPREAD_DISTANCE(topLeftVertical);
            APPLY_SPREAD_DISTANCE(bottomLeftVertical);
            APPLY_SPREAD_DISTANCE(topRightVertical);
            APPLY_SPREAD_DISTANCE(bottomRightVertical);
            APPLY_SPREAD_DISTANCE(bottomLeftHorizontal);
            APPLY_SPREAD_DISTANCE(bottomRightHorizontal);
#undef APPLY_SPREAD_DISTANCE
        }

        if (topLeftHorizontal + topRightHorizontal > maxWidthValue) {
            float a = topLeftHorizontal;
            float b = topRightHorizontal;
            topLeftHorizontal = a / (a + b) * maxWidthValue;
            topRightHorizontal = b / (a + b) * maxWidthValue;
        }

        if (topLeftVertical + bottomLeftVertical > maxHeightValue) {
            float a = topLeftVertical;
            float b = bottomLeftVertical;
            topLeftVertical = a / (a + b) * maxHeightValue;
            bottomLeftVertical = b / (a + b) * maxHeightValue;
        }

        if (topRightVertical + bottomRightVertical > maxHeightValue) {
            float a = topRightVertical;
            float b = bottomRightVertical;
            topRightVertical = a / (a + b) * maxHeightValue;
            bottomRightVertical = b / (a + b) * maxHeightValue;
        }

        if (bottomLeftHorizontal + bottomRightHorizontal > maxWidthValue) {
            float a = bottomLeftHorizontal;
            float b = bottomRightHorizontal;
            bottomLeftHorizontal = a / (a + b) * maxWidthValue;
            bottomRightHorizontal = b / (a + b) * maxWidthValue;
        }

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

void FrameBox::applyBorderRadiusClippingIfNeeds(Canvas* canvas,
                                                const LayoutRect& rect,
                                                float spreadDistance,
                                                bool inset)
{
    if (style()->hasBorderRadius()) {
        applyBorderRadius(canvas, rect, spreadDistance, inset);
        canvas->clipPath();
    }
}

void FrameBox::paintBackgroundAndBorders(Canvas* canvas)
{
    canvas->save();
    bool overflowApplied = shouldApplyOverflow();
    if (overflowApplied) {
        canvas->clip(Unit::Rect(0, 0, width(), height()));
    }

#if defined(PORT_GRAPHIC_BACKEND_EFL)
    Canvas* orgCanvas = canvas;
    bool cairoCanvasUsed = false;
#endif

// apply clip if border-radius exists
#if defined(PORT_GRAPHIC_BACKEND_EFL)
    if (style()->hasBorderRadius()) {
        const LayoutRect rect(0, 0, width(), height());
        auto br = style()->borderRadius();
        float arcR;
        float topLeftHorizontal =
            br.m_topLeftHorizontal.specifiedValue(width(), this);
        float topLeftVertical =
            br.m_topLeftVertical.specifiedValue(height(), this);
        float topRightHorizontal =
            br.m_topRightHorizontal.specifiedValue(width(), this);
        float topRightVertical =
            br.m_topRightVertical.specifiedValue(height(), this);
        float bottomLeftHorizontal =
            br.m_bottomLeftHorizontal.specifiedValue(width(), this);
        float bottomLeftVertical =
            br.m_bottomLeftVertical.specifiedValue(height(), this);
        float bottomRightHorizontal =
            br.m_bottomRightHorizontal.specifiedValue(width(), this);
        float bottomRightVertical =
            br.m_bottomRightVertical.specifiedValue(height(), this);

        cairoCanvasUsed = true;
        if (!ensureFrameBoxRareData()->m_bufferForBorderRadius ||
            frameBoxRareData()->m_bufferForBorderRadius->width() !=
                width().toUnsigned() ||
            frameBoxRareData()->m_bufferForBorderRadius->height() !=
                height().toUnsigned()) {
            frameBoxRareData()->m_bufferForBorderRadius =
                NativeImageData::create(width().toUnsigned(),
                                        height().toUnsigned());
        }

        canvas = Canvas::createGenericCanvas(
            node()->starFish(),
            frameBoxRareData()->m_bufferForBorderRadius->data(),
            frameBoxRareData()->m_bufferForBorderRadius->width(),
            frameBoxRareData()->m_bufferForBorderRadius->height());
    } else {
        BorderData border = style()->border();
        if (border.top().style() == BorderStyleValue::DashedBorderStyleValue ||
            border.right().style() ==
                BorderStyleValue::DashedBorderStyleValue ||
            border.bottom().style() ==
                BorderStyleValue::DashedBorderStyleValue ||
            border.left().style() == BorderStyleValue::DashedBorderStyleValue) {
            cairoCanvasUsed = true;
            if (!ensureFrameBoxRareData()->m_bufferForBorderRadius ||
                frameBoxRareData()->m_bufferForBorderRadius->width() !=
                    width().toUnsigned() ||
                frameBoxRareData()->m_bufferForBorderRadius->height() !=
                    height().toUnsigned()) {
                frameBoxRareData()->m_bufferForBorderRadius =
                    NativeImageData::create(width().toUnsigned(),
                                            height().toUnsigned());
            }

            canvas = Canvas::createGenericCanvas(
                node()->starFish(),
                frameBoxRareData()->m_bufferForBorderRadius->data(),
                frameBoxRareData()->m_bufferForBorderRadius->width(),
                frameBoxRareData()->m_bufferForBorderRadius->height());
        }
    }
#endif

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

#if defined(PORT_GRAPHIC_BACKEND_EFL)
    if (cairoCanvasUsed) {
        delete canvas;
        canvas = orgCanvas;
        canvas->drawImage(ensureFrameBoxRareData()->m_bufferForBorderRadius,
                          Unit::Rect(0, 0, width(), height()));
    }
#endif

    canvas->restore();
}

void FrameBox::paintBoxShadows(Canvas* canvas)
{
    ComputedStyle* s = style();

    if (s->visibility() != VisibilityValue::VisibleVisibilityValue) {
        return;
    }

    bool hasShadow = s->boxShadow().size() ? true : false;
    if (hasShadow) {
        canvas->save();
        CanvasShadowDataList list = s->boxShadow().toCanvasShadowDataList(this);

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

                float radiusOffset = 0.0f;
                if (shadow->radius()) {
                    radiusOffset = shadow->radius();
                    radiusOffset =
                        std::min(ShadowBlur::RADIUS_LIMIT, radiusOffset);
                    radiusOffset *= 2;
                }

                NativeImageData* nativeImage = NativeImageData::create(
                    ceil(shadowRect.width() + radiusOffset),
                    ceil(shadowRect.height() + radiusOffset));
                Canvas* cv = Canvas::createGenericCanvas(node()->starFish(),
                                                         nativeImage);
                cv->clearColor(Unit::Color(0, 0, 0, 0));

                if (shadow->hasColor()) {
                    cv->setColor(shadow->color());
                } else {
                    cv->setColor(s->color());
                }

                cv->translate(ceil(radiusOffset / 2), ceil(radiusOffset / 2));
                const LayoutRect clipRect(0, 0, shadowRect.width(),
                                          shadowRect.height());
                applyBorderRadiusClippingIfNeeds(cv, clipRect, sd);
                cv->drawRect(shadowRect);

                ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                              nativeImage->height(), nativeImage->stride());
                sb.process(shadow->radius());
                delete cv;

                float offset = ceil(radiusOffset / 2);
                Unit::Rect imageRect(-offset + shadow->offsetX() - sd,
                                     -offset + shadow->offsetY() - sd,
                                     nativeImage->width(),
                                     nativeImage->height());

                bool intersect = borderRect.intersects(imageRect);
                canvas->save();
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

                    int xx = 0, yy = 0, ww = 0, hh = 0;
                    LayoutUnit rx = borderRect.x();
                    LayoutUnit ry = borderRect.y();
                    xx = rx.floor();
                    yy = ry.floor();
                    ww = snapSizeToPixel(borderRect.width(), rx);
                    hh = snapSizeToPixel(borderRect.height(), ry);
                    Unit::Rect rect(xx, yy, ww, hh);

                    if (style()->hasBorderRadius()) {
                        const LayoutRect r(0, 0, rect.width(), rect.height());
                        applyBorderRadiusClippingIfNeeds(canvas, r);
                    } else {
                        canvas->setFillRule(false);
                        canvas->clip(rect);
                    }
                }
                canvas->drawImage(nativeImage, imageRect);
                canvas->restore();

                delete nativeImage;
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

    bool hasShadow = s->boxShadow().size() ? true : false;
    if (hasShadow) {
        canvas->save();
        CanvasShadowDataList list = s->boxShadow().toCanvasShadowDataList(this);

        for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
            float sd = shadow->spreadDistance();

            if (shadow->inset()) {
                Unit::Rect borderRect = makeRect(BoxValue::BorderBoxBoxValue);
                Unit::Rect paddingRect = makeRect(BoxValue::PaddingBoxBoxValue);
                Unit::Rect shadowRect(
                    paddingRect.x(), paddingRect.y(),
                    paddingRect.width() + abs(shadow->offsetX()),
                    paddingRect.height() + abs(shadow->offsetY()));

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
                Canvas* cv = Canvas::createGenericCanvas(node()->starFish(),
                                                         nativeImage);
                cv->clearColor(Unit::Color(0, 0, 0, 0));

                if (shadow->hasColor()) {
                    cv->setColor(shadow->color());
                } else {
                    cv->setColor(s->color());
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
                if (style()->hasBorderRadius()) {
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
                    cv->drawRect(rect);
                }
                cv->fill();

                ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                              nativeImage->height(), nativeImage->stride());
                sb.process(shadow->radius());
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
                if (style()->hasBorderRadius()) {
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
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
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

    if (rootOrBodyelement) {
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
    }

    if (!style->backgroundColor().isTransparent() &&
        style->visibility() == VisibilityValue::VisibleVisibilityValue) {
        canvas->save();
        Unit::Rect paintingRect;
        if (rootOrBodyelement) {
            Window* window = rootOrBodyelement->window();
            FrameDocument* doc = window->document()->frame()->asFrameDocument();
            paintingRect =
                Unit::Rect(doc->scrollLeft(), doc->scrollTop(),
                           window->innerWidth(), window->innerHeight());
        } else {
            unsigned int idx = style->backgroundLayerSize() - 1;
            paintingRect = box->makeRect(style->backgroundClip(idx));
        }
        canvas->setColor(style->backgroundColor());
        // FIXME: the results of drawRect(LayoutRect) and drawRect(Unit::Rect)
        // are different because inside function drawRect(LayoutRect), modifies
        // its x, y, width and height somehow.
        canvas->drawRect(LayoutRect(paintingRect.x(), paintingRect.y(),
                                    paintingRect.width(),
                                    paintingRect.height()));
        canvas->restore();
    }

    ImageRenderingValue imageRenderingValue = style->imageRendering();

    for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
        unsigned int idx = style->backgroundLayerSize() - i - 1;
        NativeImageData* id = style->backgroundImageData(idx);
        if (id && id->width() && id->height()) {
            Unit::Rect paintingRect;
            Unit::Rect positioningRect;
            BackgroundAttachmentValue attachment =
                style->backgroundAttachment(idx);
            if (rootOrBodyelement) {
                Window* window = rootOrBodyelement->window();
                FrameDocument* doc =
                    window->document()->frame()->asFrameDocument();
                paintingRect =
                    Unit::Rect(doc->scrollLeft(), doc->scrollTop(),
                               window->innerWidth(), window->innerHeight());
            }

            if (attachment == FixedBackgroundAttachmentValue) {
                FrameDocument* doc =
                    box->document()->frame()->asFrameDocument();
                LayoutLocation loc;
                if (!rootOrBodyelement) {
                    loc = box->absolutePoint(doc);
                }
                positioningRect = doc->makeRect(style->backgroundOrigin(idx));
                positioningRect.setX(-loc.x().toFloat() + doc->scrollLeft());
                positioningRect.setY(-loc.y().toFloat() + doc->scrollTop());
                if (!rootOrBodyelement) {
                    paintingRect = box->makeRect(style->backgroundClip(idx));
                }
            } else if (attachment == LocalBackgroundAttachmentValue &&
                       box->isFrameBlockBox()) {
                FrameBox scrollBox(box->node(), style);
                scrollBox.copyFrom(box, FrameBox::BorderCopy |
                                            FrameBox::PaddingCopy);
                scrollBox.setWidth(box->asFrameBlockBox()->scrollWidth());
                scrollBox.setHeight(box->asFrameBlockBox()->scrollHeight());
                positioningRect =
                    scrollBox.makeRect(style->backgroundOrigin(idx));
                positioningRect.setX(positioningRect.x() -
                                     box->asFrameBlockBox()->scrollLeft());
                positioningRect.setY(positioningRect.x() -
                                     box->asFrameBlockBox()->scrollTop());
                if (rootOrBodyelement) {
                    positioningRect.setX(positioningRect.x() + box->x());
                    positioningRect.setY(positioningRect.y() + box->y());
                } else {
                    paintingRect =
                        scrollBox.makeRect(style->backgroundClip(idx));
                }
            } else {
                positioningRect = box->makeRect(style->backgroundOrigin(idx));

                if (rootOrBodyelement) {
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
            float imgR = id->width() / (float)id->height();
            if (style->backgroundSizeIsLength(idx)) {
                LengthSize bgSize = style->backgroundSizeLengthValue(idx);
                if (bgSize.width().isAuto() && bgSize.height().isAuto()) {
                    imgW = id->width();
                    imgH = id->height();
                } else if (bgSize.width().isAuto() &&
                           !bgSize.height().isAuto()) {
                    imgH = bgSize.height().specifiedValue(positionH, box);
                    imgW = imgH * id->width() / id->height();
                } else if (!bgSize.width().isAuto() &&
                           bgSize.height().isAuto()) {
                    imgW = bgSize.width().specifiedValue(positionW, box);
                    imgH = imgW * id->height() / id->width();
                } else {
                    imgW = bgSize.width().specifiedValue(positionW, box);
                    imgH = bgSize.height().specifiedValue(positionH, box);
                }
            } else {
                BackgroundSizeValue bgSize =
                    style->backgroundSizeTypeValue(idx);
                if (bgSize == BackgroundSizeValue::CoverBackgroundSizeValue) {
                    if (boxR < imgR) {
                        imgW = positionH * imgR;
                    } else {
                        imgH = positionW / imgR;
                    }
                } else {
                    STARFISH_ASSERT(
                        bgSize ==
                        BackgroundSizeValue::ContainBackgroundSizeValue);
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
            if (repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
                repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
                canvas->drawRepeatImage(
                    id, Unit::Rect(x, y, paintingW, paintingH), imgW, imgH,
                    true, true, imageRenderingValue);
            } else if (repeatX == BackgroundRepeatValue::NoRepeatRepeatValue &&
                       repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, imgW, paintingH),
                                        imgW, imgH, false, true,
                                        imageRenderingValue);
            } else if (repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
                       repeatY == BackgroundRepeatValue::NoRepeatRepeatValue) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, paintingW, imgH),
                                        imgW, imgH, true, false,
                                        imageRenderingValue);
            } else {
                canvas->drawImage(id, Unit::Rect(x, y, imgW, imgH),
                                  imageRenderingValue);
            }

            canvas->restore();
        }
    }
}

void FrameBox::paintDottedLine(Canvas* canvas, const LayoutLocation& p1,
                               const LayoutLocation& p2,
                               const LayoutLocation& p3,
                               const LayoutLocation& p4,
                               const LayoutLocation& p5,
                               const LayoutLocation& p6, BoxSide side)
{
    // This is chromium's style for dotted.
    double squares[] = { 1.0, 1.0 };
    int nsquare = sizeof(squares) / sizeof(squares[0]);
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

    squares[0] *= width.toDouble();
    squares[1] *= width.toDouble();

    canvas->drawRect(p1, p2, p3, p3);
    canvas->save();
    canvas->setDash(squares, nsquare, offset);
    canvas->setStrokeWidth(width.toFloat());
    canvas->moveTo(x1.toDouble(), y1.toDouble());
    canvas->lineTo(x2.toDouble(), y2.toDouble());
    canvas->stroke();
    canvas->restore();
    canvas->drawRect(p4, p5, p6, p6);
}

void FrameBox::paintDashedLine(Canvas* canvas, const LayoutLocation& p1,
                               const LayoutLocation& p2,
                               const LayoutLocation& p3,
                               const LayoutLocation& p4,
                               const LayoutLocation& p5,
                               const LayoutLocation& p6, BoxSide side)
{
    double dashes[] = { 2.0, 1.0 };
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
    canvas->setDash(dashes, ndash, offset);
    canvas->setStrokeWidth(width.toFloat());
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

#if !defined(PORT_CANVAS_BACKEND_EFL)
        double bLWidth = border.left().width().specifiedValue(width(), this);
        double bTWidth = border.top().width().specifiedValue(height(), this);
        double bRWidth = border.right().width().specifiedValue(width(), this);
        double bBWidth = border.bottom().width().specifiedValue(height(), this);

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

        double value = std::min((float)width() / (bImgLWidth + bImgRWidth),
                                (float)height() / (bImgTWidth + bImgBWidth));
        if (value < 1) {
            bImgLWidth *= value;
            bImgRWidth *= value;
            bImgTWidth *= value;
            bImgBWidth *= value;
        }

        NativeImageData* imgData = border.image().imageData();
        canvas->setNeedsGoodQualityAntialias();

        ImageRenderingValue imageRenderingValue = style()->imageRendering();

        BorderInfo borderinfo = { 1.0, 1.0,
                                  BorderImageRepeatValue::StretchValue,
                                  BorderImageRepeatValue::StretchValue };
        // Four Corners
        // left-top
        if ((lSlice > 0 && bImgLWidth > 0) && (tSlice > 0 && bImgTWidth > 0)) {
            canvas->drawImage(imgData, Unit::Rect(0, 0, lSlice, tSlice),
                              Unit::Rect(0, 0, bImgLWidth, bImgTWidth),
                              borderinfo, imageRenderingValue);
        }
        // right-top
        if ((rSlice > 0 && bImgRWidth > 0) && (tSlice > 0 && bImgTWidth > 0)) {
            canvas->drawImage(
                imgData, Unit::Rect(imgWidth - rSlice, 0, rSlice, tSlice),
                Unit::Rect(width() - bImgRWidth, 0, bImgRWidth, bImgTWidth),
                borderinfo, imageRenderingValue);
        }
        // left-bottom
        if ((lSlice > 0 && bImgLWidth > 0) && (bSlice > 0 && bImgBWidth > 0)) {
            canvas->drawImage(
                imgData, Unit::Rect(0, imgHeight - bSlice, lSlice, bSlice),
                Unit::Rect(0, height() - bImgBWidth, bImgLWidth, bImgBWidth),
                borderinfo, imageRenderingValue);
        }

        // right-bottom
        if ((rSlice > 0 && bImgRWidth > 0) && (bSlice > 0 && bImgBWidth > 0)) {
            canvas->drawImage(
                imgData, Unit::Rect(imgWidth - rSlice, imgHeight - bSlice,
                                    rSlice, bSlice),
                Unit::Rect(width() - bImgRWidth, height() - bImgBWidth,
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
                    Unit::Rect(bImgLWidth, 0,
                               width() - (bImgLWidth + bImgRWidth), bImgTWidth),
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
                    Unit::Rect(bImgLWidth, height() - bImgBWidth,
                               width() - (bImgLWidth + bImgRWidth), bImgBWidth),
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
                    Unit::Rect(0, bImgTWidth, bImgLWidth,
                               height() - (bImgTWidth + bImgBWidth)),
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
                    Unit::Rect(width() - bImgRWidth, bImgTWidth, bImgRWidth,
                               height() - (bImgTWidth + bImgBWidth)),
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
            canvas->drawImage(imgData,
                              Unit::Rect(lSlice, tSlice,
                                         imgWidth - (lSlice + rSlice),
                                         imgHeight - (tSlice + bSlice)),
                              Unit::Rect(bImgLWidth, bImgTWidth,
                                         width() - (bImgLWidth + bImgRWidth),
                                         height() - (bImgTWidth + bImgBWidth)),
                              borderinfo, imageRenderingValue);
        }

        canvas->setNeedsFastAntialias();
#else
        double bWidth = border.top().width().specifiedValue(height(), this);
        double bImgWidth =
            border.image().widths().top().specifiedValue(bWidth, this);
        double bImgSlice =
            border.image().slices().top().computedBorderImageSlice(height(),
                                                                   this);

        size_t imgWidth = border.image().imageData()->width();
        size_t imgHeight = border.image().imageData()->height();

        size_t lSlice = border.image().slices().left().computedBorderImageSlice(
            imgWidth, this);
        size_t tSlice = border.image().slices().top().computedBorderImageSlice(
            imgHeight, this);
        size_t rSlice =
            border.image().slices().right().computedBorderImageSlice(imgWidth,
                                                                     this);
        size_t bSlice =
            border.image().slices().bottom().computedBorderImageSlice(imgHeight,
                                                                      this);

        NativeImageData* imgData = border.image().imageData();

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
            canvas->drawBorderImage(
                imgData, Unit::Rect(rect.x(), rect.y(), drawRect, drawRect),
                lSlice, tSlice, 0, 0, scale, isFill);

            // right-top
            canvas->drawBorderImage(imgData,
                                    Unit::Rect((float)rect.width() - drawRect,
                                               rect.y(), drawRect, drawRect),
                                    0, tSlice, rSlice, 0, scale, isFill);

            // right-bottom
            canvas->drawBorderImage(
                imgData, Unit::Rect((float)rect.width() - drawRect,
                                    (float)(rect.y() + height()) - drawRect,
                                    drawRect, drawRect),
                0, 0, rSlice, bSlice, scale, isFill);

            // left-bottom
            canvas->drawBorderImage(
                imgData,
                Unit::Rect(rect.x(), (float)(rect.y() + height()) - drawRect,
                           drawRect, drawRect),
                lSlice, 0, 0, bSlice, scale, isFill);
        } else {
            isFill = border.image().sliceFill();
            canvas->drawBorderImage(
                imgData,
                Unit::Rect(rect.x(), rect.y(), rect.width(), rect.height()),
                lSlice, tSlice, rSlice, bSlice, scale, isFill);
        }
#endif
    } else if (border.hasBorderStyle()) {
        if (style()->hasBorderRadius()) {
            float x, y;
            auto br = style()->borderRadius();

            float maxWidthValue = rect.width();
            float maxHeightValue = rect.height();

            if (!br.m_topLeftVertical.isPercent() ||
                !br.m_bottomLeftVertical.isPercent() ||
                !br.m_bottomLeftVertical.isPercent() ||
                !br.m_bottomRightVertical.isPercent() ||
                !br.m_bottomLeftHorizontal.isPercent() ||
                !br.m_bottomRightHorizontal.isPercent() ||
                !br.m_topLeftHorizontal.isPercent() ||
                !br.m_topRightHorizontal.isPercent()) {
                maxHeightValue = maxWidthValue =
                    std::min(maxHeightValue, maxWidthValue);
            }

            float topLeftHorizontal =
                br.m_topLeftHorizontal.specifiedValue(width(), this);
            float topRightHorizontal =
                br.m_topRightHorizontal.specifiedValue(width(), this);

            if (topLeftHorizontal + topRightHorizontal > maxWidthValue) {
                float a = topLeftHorizontal;
                float b = topRightHorizontal;
                topLeftHorizontal = a / (a + b) * maxWidthValue;
                topRightHorizontal = b / (a + b) * maxWidthValue;
            }

            float topLeftVertical =
                br.m_topLeftVertical.specifiedValue(height(), this);
            float bottomLeftVertical =
                br.m_bottomLeftVertical.specifiedValue(height(), this);

            if (topLeftVertical + bottomLeftVertical > maxHeightValue) {
                float a = topLeftVertical;
                float b = bottomLeftVertical;
                topLeftVertical = a / (a + b) * maxHeightValue;
                bottomLeftVertical = b / (a + b) * maxHeightValue;
            }

            float topRightVertical =
                br.m_topRightVertical.specifiedValue(height(), this);
            float bottomRightVertical =
                br.m_bottomRightVertical.specifiedValue(height(), this);

            if (topRightVertical + bottomRightVertical > maxHeightValue) {
                float a = topRightVertical;
                float b = bottomRightVertical;
                topRightVertical = a / (a + b) * maxHeightValue;
                bottomRightVertical = b / (a + b) * maxHeightValue;
            }

            float bottomLeftHorizontal =
                br.m_bottomLeftHorizontal.specifiedValue(width(), this);
            float bottomRightHorizontal =
                br.m_bottomRightHorizontal.specifiedValue(width(), this);

            if (bottomLeftHorizontal + bottomRightHorizontal > maxWidthValue) {
                float a = bottomLeftHorizontal;
                float b = bottomRightHorizontal;
                bottomLeftHorizontal = a / (a + b) * maxWidthValue;
                bottomRightHorizontal = b / (a + b) * maxWidthValue;
            }

            float arcR;

            {
                canvas->setColor(border.left().color());

                if (topLeftHorizontal && topLeftVertical) {
                    canvas->save();
                    canvas->translate(topLeftHorizontal + rect.x().toFloat(),
                                      topLeftVertical + rect.y().toFloat());
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
                        canvas->translate(topLeftHorizontal +
                                              rect.x().toFloat(),
                                          topLeftVertical + rect.y().toFloat());
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
                        canvas->lineTo(rect.x() + borderLeft(),
                                       rect.y() + borderTop());
                    }
                } else {
                    canvas->moveTo(rect.x(), rect.y());
                    canvas->lineTo(rect.x() + borderLeft(),
                                   rect.y() + borderTop());
                }

                if (bottomLeftHorizontal && bottomLeftVertical) {
                    if (bottomLeftHorizontal > borderLeft() &&
                        bottomLeftVertical > borderBottom()) {
                        canvas->save();
                        canvas->translate(rect.x() + bottomLeftHorizontal,
                                          rect.maxY() - bottomLeftVertical);

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
                        canvas->lineTo(rect.x() + borderLeft(),
                                       rect.maxY() - borderBottom());
                    }

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

                    canvas->arc(0, 0, arcR, M_PI - M_PI / 2 + M_PI / 4, M_PI);
                    canvas->restore();

                    x = rect.x();
                    y = rect.y() + bottomLeftVertical;
                    canvas->lineTo(x, y);
                } else {
                    canvas->lineTo(rect.x() + borderLeft(),
                                   rect.maxY() - borderTop());
                    canvas->lineTo(rect.x(), rect.maxY());
                }

                canvas->fill();
            }

            // draw border-top
            {
                canvas->setColor(border.top().color());

                if (topLeftHorizontal && topLeftVertical) {
                    if (topLeftHorizontal > borderLeft() &&
                        topLeftVertical > borderTop()) {
                        canvas->save();
                        canvas->translate(topLeftHorizontal +
                                              rect.x().toFloat(),
                                          topLeftVertical + rect.y().toFloat());

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
                        x = rect.x() + borderLeft();
                        y = rect.y() + borderTop();
                        canvas->moveTo(x, y);
                    }

                    canvas->save();
                    canvas->translate(topLeftHorizontal + rect.x().toFloat(),
                                      topLeftVertical + rect.y().toFloat());
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
                    canvas->moveTo(rect.x() + borderLeft(),
                                   rect.y() + borderTop());
                    canvas->lineTo(rect.x(), rect.y());
                }

                if (topRightHorizontal && topRightVertical) {
                    canvas->save();
                    canvas->translate(-topRightHorizontal +
                                          rect.maxX().toFloat(),
                                      topRightVertical + rect.y().toFloat());

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
                            -topRightHorizontal + rect.maxX().toFloat(),
                            topRightVertical + rect.y().toFloat());
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
                        x = rect.maxX() - borderRight();
                        y = rect.y() + borderTop();
                        canvas->lineTo(x, y);
                    }
                } else {
                    canvas->lineTo(rect.maxX(), rect.y());
                    canvas->lineTo(rect.maxX() - borderRight(),
                                   rect.y() + borderTop());
                }
                canvas->fill();
            }

            // draw border-right
            {
                canvas->setColor(border.right().color());

                if (topRightHorizontal && topRightVertical) {
                    if (topRightHorizontal > borderRight() &&
                        topRightVertical > borderTop()) {
                        canvas->save();
                        canvas->translate(
                            rect.maxX().toFloat() - topRightHorizontal,
                            rect.y().toFloat() + topRightVertical);

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
                        x = rect.maxX() - borderRight();
                        y = rect.y() + borderTop();
                        canvas->moveTo(x, y);
                    }

                    canvas->save();

                    canvas->translate(rect.maxX().toFloat() -
                                          topRightHorizontal,
                                      rect.y().toFloat() + topRightVertical);
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
                    canvas->moveTo(rect.maxX() - borderRight(),
                                   rect.y() + borderTop());
                    canvas->lineTo(rect.maxX(), rect.y());
                }

                if (bottomRightHorizontal && bottomRightVertical) {
                    x = rect.maxX();
                    y = rect.maxY() - bottomRightVertical;
                    canvas->lineTo(x, y);

                    canvas->save();
                    canvas->translate(rect.maxX() - bottomRightHorizontal,
                                      rect.maxY() - bottomRightVertical);
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
                        canvas->translate(rect.maxX() - bottomRightHorizontal,
                                          rect.maxY() - bottomRightVertical);

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
                        x = rect.maxX() - borderRight();
                        y = rect.maxY() - borderBottom();
                        canvas->lineTo(x, y);
                    }
                } else {
                    canvas->lineTo(rect.maxX(), rect.maxY());
                    canvas->lineTo(rect.maxX() - borderRight(),
                                   rect.maxY() - borderBottom());
                }

                canvas->fill();
            }

            // border-bottom
            {
                canvas->setColor(border.bottom().color());
                if (bottomRightHorizontal && bottomRightVertical) {
                    if (bottomRightHorizontal > borderRight() &&
                        bottomRightVertical > borderBottom()) {
                        canvas->save();
                        canvas->translate(rect.maxX() - bottomRightHorizontal,
                                          rect.maxY() - bottomRightVertical);

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
                        x = rect.maxX() - borderRight();
                        y = rect.maxY() - borderBottom();
                        canvas->moveTo(x, y);
                    }

                    canvas->save();
                    canvas->translate(rect.maxX() - bottomRightHorizontal,
                                      rect.maxY() - bottomRightVertical);
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
                    canvas->moveTo(rect.maxX() - borderRight(),
                                   rect.maxY() - borderBottom());
                    canvas->lineTo(rect.maxX(), rect.maxY());
                }

                if (bottomLeftHorizontal && bottomLeftVertical) {
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
                    canvas->arc(0, 0, arcR, M_PI - M_PI / 2,
                                M_PI + M_PI / 4 - M_PI / 2);
                    canvas->restore();

                    if (bottomLeftHorizontal > borderLeft() &&
                        bottomLeftVertical > borderBottom()) {
                        canvas->save();
                        canvas->translate(rect.x() + bottomLeftHorizontal,
                                          rect.maxY() - bottomLeftVertical);

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
                        x = rect.x() + borderLeft();
                        y = rect.maxY() - borderBottom();
                        canvas->lineTo(x, y);
                    }
                } else {
                    canvas->lineTo(rect.x(), rect.maxY());
                    canvas->lineTo(rect.x() + borderLeft(),
                                   rect.maxY() - borderBottom());
                }
                canvas->fill();
            }

        } else {
            if (style()->isFourSideBorderStyleValueSolid() &&
                (border.top().color() == border.right().color()) &&
                (border.right().color() == border.bottom().color()) &&
                (border.bottom().color() == border.left().color())) {
                canvas->setColor(border.top().color());
#ifdef PORT_GRAPHIC_BACKEND_EFL
                // Draw solid borders fast around the given rect
                // when all 4 colors are the same.
                //    _______________
                //   |_______________|
                //   | |           | |
                //   | |           | |
                //   | |           | |
                //   |_|___________|_|
                //   |_______________|
                //

                // top
                canvas->drawRect(
                    LayoutRect(rect.x(), rect.y(), rect.width(), borderTop()));

                // right
                canvas->drawRect(
                    LayoutRect(rect.x() + rect.width() - borderRight(),
                               rect.y() + borderTop(), borderRight(),
                               rect.height() - borderHeight()));

                // bottom
                canvas->drawRect(LayoutRect(rect.x(), rect.y() + rect.height() -
                                                          borderBottom(),
                                            width(), borderBottom()));

                // left
                canvas->drawRect(LayoutRect(rect.x(), rect.y() + borderTop(),
                                            borderLeft(),
                                            rect.height() - borderHeight()));
#else
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
#endif
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
                    canvas->setColor(border.top().color().getDarkerColor());
                } else if ((border.top().style() ==
                            BorderStyleValue::OutsetBorderStyleValue) &&
                           (border.top().color() == black)) {
                    canvas->setColor(
                        Unit::Color(238, 238, 238, border.top().color().a()));
                } else {
                    canvas->setColor(border.top().color());
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
                        canvas->setColor(border.top().color().getDarkerColor());
                    } else {
                        canvas->setColor(border.top().color());
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
                        canvas->setColor(border.top().color());
                    } else {
                        canvas->setColor(border.top().color().getDarkerColor());
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
                    canvas->setColor(
                        Unit::Color(238, 238, 238, border.right().color().a()));
                } else if (border.right().style() ==
                           BorderStyleValue::OutsetBorderStyleValue) {
                    canvas->setColor(border.right().color().getDarkerColor());
                } else {
                    canvas->setColor(border.right().color());
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
                        canvas->setColor(border.top().color());
                    } else {
                        canvas->setColor(border.top().color().getDarkerColor());
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
                        canvas->setColor(border.top().color().getDarkerColor());
                    } else {
                        canvas->setColor(border.top().color());
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
                    canvas->setColor(Unit::Color(238, 238, 238,
                                                 border.bottom().color().a()));
                } else if (border.bottom().style() ==
                           BorderStyleValue::OutsetBorderStyleValue) {
                    canvas->setColor(border.bottom().color().getDarkerColor());
                } else {
                    canvas->setColor(border.bottom().color());
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
                        canvas->setColor(border.top().color());
                    } else {
                        canvas->setColor(border.top().color().getDarkerColor());
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
                        canvas->setColor(border.top().color().getDarkerColor());
                    } else {
                        canvas->setColor(border.top().color());
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
                    canvas->setColor(border.left().color().getDarkerColor());
                } else if ((border.left().style() ==
                            BorderStyleValue::OutsetBorderStyleValue) &&
                           (border.left().color() == black)) {
                    canvas->setColor(
                        Unit::Color(238, 238, 238, border.left().color().a()));
                } else {
                    canvas->setColor(border.left().color());
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
                        canvas->setColor(border.top().color().getDarkerColor());
                    } else {
                        canvas->setColor(border.top().color());
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
                        canvas->setColor(border.top().color());
                    } else {
                        canvas->setColor(border.top().color().getDarkerColor());
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

void FrameBox::paintContent(PaintingContext& ctx)
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
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

void FrameBox::establishesStackingContextIfNeeds()
{
    if (isEstablishesStackingContext()) {
        STARFISH_ASSERT(isRootElement() || stackingContext() == nullptr);
        if (!isRootElement() || (isRootElement() &&
                                 !node()
                                      ->document()
                                      ->browsingContext()
                                      ->isTopLevelBrowsingContext())) {
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
                    } else if (p->style()->hasTransforms(p)) {
                        break;
                    } else if (p->style()->position() == FixedPositionValue) {
                        break;
                    } else if ((p->isPositioned() || p->isFlexItem()) &&
                               p->style()->isSpecifiedZIndex()) {
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

LayoutRect FrameBox::frameVisibleRect()
{
    LayoutRect out = frameVisibleOutlineRect();
    LayoutRect shadow = frameVisibleShadowsRect();
    out.unite(shadow);
    return out;
}

LayoutRect FrameBox::frameVisibleOutlineRect()
{
    LayoutRect r = frameRect();
    r.setX(0);
    r.setY(0);

    ComputedStyle* cs = style();

    if (cs && cs->outlineStyle() != BorderStyleValue::NoneBorderStyleValue) {
        LayoutUnit t = outlineThickness();
        r.setX(r.x() - t);
        r.setY(r.y() - t);
        r.setWidth(r.width() + t * 2);
        r.setHeight(r.height() + t * 2);
    }

    return r;
}
ALWAYS_INLINE LayoutRect computeVisibleShadowRect(
    const LayoutRect& owner, const CanvasShadowData& shadow)
{
    LayoutRect ret = owner;
    if (!shadow.inset()) {
        float radiusOffset = 0.0f;
        if (shadow.radius()) {
            radiusOffset = shadow.radius();
            radiusOffset = std::min(ShadowBlur::RADIUS_LIMIT, radiusOffset);
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

LayoutRect FrameBox::frameVisibleShadowsRect()
{
    LayoutRect owner = frameRect();
    owner.setX(0);
    owner.setY(0);

    LayoutRect ret = owner;

    ComputedStyle* cs = style();
    if (!cs) {
        return ret;
    }

    if (cs->boxShadow().size()) {
        CanvasShadowDataList list =
            cs->boxShadow().toCanvasShadowDataList(this);
        for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
            LayoutRect rect = computeVisibleShadowRect(owner, *shadow);
            ret.unite(rect);
        }
    }

    if (cs->textShadow().size()) {
        CanvasShadowDataList list =
            cs->textShadow().toCanvasShadowDataList(this);
        for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
            LayoutRect rect = computeVisibleShadowRect(owner, *shadow);
            ret.unite(rect);
        }
    }
    return ret;
}

bool FrameBox::tryUniteVisibleRect(Frame::ComputeVisibleRectContext& ctx)
{
    if (ctx.sourceStackingContext &&
        (this != ctx.sourceStackingContext->owner() && stackingContext() &&
         stackingContext()->needsGraphicsBuffer())) {
        return false;
    }

    ComputedStyle* cs = style();

    if (cs && cs->visibility() == HiddenVisibilityValue) {
        return true;
    }

    bool ret = !shouldApplyOverflow();
    LayoutRect outline = frameVisibleOutlineRect();

    if (ctx.purpose == Frame::ComputeVisibleRectContext::GraphicsBuffer &&
        isFrameBlockBox()) {
        BorderData border = cs->border();
        if (isAnonymous() ||
            (cs->backgroundColor().isTransparent() &&
             cs->backgroundLayerSize() == 0 && !border.hasBorderStyle() &&
             (cs->outlineStyle() == BorderStyleValue::NoneBorderStyleValue ||
              outlineThickness() == 0))) {
            outline.setWidth(0);
            outline.setHeight(0);
            ret = true;
        }
    }

    if (ctx.isForSpecialValueForTableCell &&
        ctx.purpose == Frame::ComputeVisibleRectContext::Scrolling &&
        isFrameFlexibleBox()) {
        if (cs->height().isDefinite(true)) {
            ctx.uniteRect(outline);
            ret = false;
        }
    }
    ctx.uniteRect(outline);

    for (int i = 0; cs && i < 2; ++i) {
        CanvasShadowDataList list;
        LayoutRect owner = frameRect();
        owner.setX(0);
        owner.setY(0);
        LayoutRect shadowsRect = owner;
        if (i == 0) {
            list = cs->boxShadow().toCanvasShadowDataList(this);
        } else {
            list = cs->textShadow().toCanvasShadowDataList(this);
        }
        for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
            if (ctx.purpose ==
                    Frame::ComputeVisibleRectContext::GraphicsBuffer &&
                isFrameBlockBox()) {
                if (shadow->hasColor() && !shadow->color().isTransparent()) {
                    LayoutRect rect = computeVisibleShadowRect(owner, *shadow);
                    shadowsRect.unite(rect);
                    ret = false;
                } else if (!shadow->hasColor() &&
                           !cs->color().isTransparent()) {
                    LayoutRect rect = computeVisibleShadowRect(owner, *shadow);
                    shadowsRect.unite(rect);
                    ret = false;
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

        if (shadowsRect != owner) {
            ctx.uniteRect(shadowsRect);
        }
    }

    return ret;
}

void FrameBox::computeVisibleRect(Frame::ComputeVisibleRectContext& ctx)
{
    Frame::ComputeVisibleRectContextFragment f(ctx, this);
    tryUniteVisibleRect(ctx);
}

void FrameBox::clearStackingContextIfNeeds(bool shouldDetachNativeBuffer)
{
    if (stackingContext()) {
        stackingContext()->clearGraphicsBuffer(shouldDetachNativeBuffer);
        frameBoxRareData()->m_stackingContext = nullptr;
    }
}

LayoutUnit FrameBox::minMaxWidthAppliedIfNeeds(
    LayoutContext& ctx, LayoutUnit width, LayoutUnit parentWidth,
    bool underComputingPreferredWidth)
{
    ComputedStyle* style = Frame::style();
    if (style->minWidth().isSpecified()) {
        if (style->minWidth().isDefinite(!underComputingPreferredWidth)) {
            LayoutUnit minWidth =
                style->minWidth().specifiedValue(parentWidth, this);
            minWidth = contentWidthApplyingBoxSizing(minWidth);

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
                    width = contentWidthApplyingBoxSizing(width);

                    minWidth = width;
                }
            }

            PreferredWidthMainContext mainContext;
            PreferredWidthContext p(ctx, mainContext, this, this,
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

            maxWidth = contentWidthApplyingBoxSizing(maxWidth);

            if (maxWidth >= 0 && maxWidth < width) {
                return maxWidth;
            }
        }
    }
    return width;
}

LayoutUnit FrameBox::minMaxHeightAppliedIfNeeds(LayoutContext& ctx,
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

        minHeight = contentHeightApplyingBoxSizing(minHeight);

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
                    height = contentHeightApplyingBoxSizing(height);

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

        maxHeight = contentHeightApplyingBoxSizing(maxHeight);

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
}
