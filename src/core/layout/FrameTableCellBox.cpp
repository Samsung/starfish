/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/HTMLTDElement.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableRowBox.h"

namespace StarFish {

FrameTableCellBox::FrameTableCellBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_absoluteColumnIndex(0)
    , m_minCellWidth(0)
    , m_maxCellWidth(0)
    , m_updatedColspan(0)
    , m_updatedRowspan(0)
{
    m_updatedColspan = colspan();
    m_updatedRowspan = rowspan();
}

bool FrameTableCellBox::isHTMLTHElement()
{
    if (node() && node()->isHTMLTHElement()) {
        return true;
    }

    return false;
}

// In CSS, the width of a cell refers to the content width, which excludes the
// border and padding, e.g., <td style="width: 100px">
//
//  +--------------------------+
//  |          border          |
//  |  +--------------------+  |
//  |  |  Width of the cell |  |
//  |  |        100px       |  |
//  |  |<------------------>|  |
//  |  |                    |  |
//  |  +--------------------+  |
//  |                          |
//  +--------------------------+
//
void FrameTableCellBox::collectCellWidthInfo(
    LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    PreferredWidthContext p(ctx, nullptr, this, this, LayoutUnit::max());
    p.computePreferredWidth();
    m_minCellWidth = p.preferredMinWidth() + borderWidth() + paddingWidth();
    m_maxCellWidth = p.preferredWidth() + borderWidth() + paddingWidth();

    Length minWidth = style()->minWidth();
    Length maxWidth = style()->maxWidth();
    LayoutUnit unused;
    if (minWidth.isDefinite(false)) {
        LayoutUnit mw = minWidth.specifiedValue(unused, this);
        if (mw < m_minCellWidth) {
            m_minCellWidth = mw;
        }
    }
    if (maxWidth.isDefinite(false)) {
        LayoutUnit mw = maxWidth.specifiedValue(unused, this);
        if (mw < m_maxCellWidth) {
            m_maxCellWidth = mw;
        }
    }

    setContentWidth(m_maxCellWidth - borderWidth() - paddingWidth());
}

void FrameTableCellBox::layoutWidth(LayoutContext& ctx)
{
    // layout blockboxes to fit them into the width of this cell.
    // The width of cell has been calculated in calCellWidth()
    // in the first iteration
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameBlockBox()) {
            c->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        }
    }
}

void FrameTableCellBox::layoutHeight(LayoutContext& ctx)
{
    FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);

    if (hasBiggerContentThanFrameHeight()) {
        auto visibleRect = computeVisibleRectForScroll(true);
        LayoutUnit cellContentHeight = visibleRect.height();
        if (visibleRect.y() < 0) {
            cellContentHeight += visibleRect.y();
        }

        LayoutUnit h = cellContentHeight - (height() - borderHeight());
        if (h > 0) {
            // expand height if table cell is smaller than content of cell
            setHeight(contentHeight() + paddingHeight() + borderHeight() + h);
        }
    }
}

bool FrameTableCellBox::emptyContent()
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameText()) {
            if (c->asFrameText()->text()->trim()->equals(String::emptyString)) {
                if ((style()->whiteSpace() ==
                     WhiteSpaceValue::NormalWhiteSpaceValue) ||
                    (style()->whiteSpace() ==
                     WhiteSpaceValue::NoWrapWhiteSpaceValue)) {
                    continue;
                }
            }
        }
        return false;
    }
    return true;
}

// Table draws the border around the TableFrameSections
void FrameTableCellBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Do not print borders and background if "emptyCells: hide", and
    // there's no content
    if (style()->emptyCells() == EmptyCellsValue::HideEmptyCellsValue &&
        emptyContent()) {
        return;
    }

    // Print borders and background if the cell width is > 0
    if (width() > 0) {
        FrameBox::paintBackgroundAndBorders(canvas);
    }
}

void FrameTableCellBox::applyVerticalAlign(LayoutContext& ctx)
{
    // 1. Cal the content height of all child boxes
    //    Also calculate the ascender of the first line. It is used
    //    in "vertical-align: baseline"
    // 2. Cal y pos where the first child box will be positioned
    LayoutUnit yPosOffset = 0;
    switch (style()->verticalAlign()) {
    case VerticalAlignValue::TopVAlignValue:
        yPosOffset = 0;
        break;
    case VerticalAlignValue::BottomVAlignValue:
        yPosOffset = contentHeight() - ctx.contentHeight(this);
        break;
    case VerticalAlignValue::MiddleVAlignValue: {
        LayoutUnit halfCellContentHeight =
            LayoutUnit(contentHeight().toDouble() / 2);
        yPosOffset = halfCellContentHeight.toDouble() -
                     (ctx.contentHeight(this).toDouble() / 2);
        break;
    }
    case VerticalAlignValue::BaselineVAlignValue:
    case VerticalAlignValue::SubVAlignValue:
    case VerticalAlignValue::SuperVAlignValue:
    case VerticalAlignValue::TextTopVAlignValue:
    case VerticalAlignValue::TextBottomVAlignValue:
    case VerticalAlignValue::NumericVAlignValue:
        yPosOffset = rowBox()->baseline() - calBaseline(ctx);
        break;
    default:
        break;
    }

    // 3. Move all child boxes by yPosOffset
    if (hasBlockFlow()) {
        for (Frame* c = firstChild(); c; c = c->next()) {
            if (c->isFrameBlockBox()) {
                c->asFrameBox()->moveY(yPosOffset);
            }
        }
    } else {
        for (auto& b : m_lineBoxes) {
            b->moveY(yPosOffset);
        }
    }
}

LayoutUnit FrameTableCellBox::calBaseline(LayoutContext& ctx)
{
    // To reduce unneeded computation, baseline is only calculated when
    // this cell has "vertical-align: baseline" or equivalent

    // The baseline of a cell is the baseline of the first in-flow line box
    // in the cell, or the first in-flow table-row in the cell, whichever comes
    // first. If there is no such line box or table-row, the baseline is the
    // bottom of content edge of the cell box.
    switch (style()->verticalAlign()) {
    case VerticalAlignValue::BaselineVAlignValue:
    case VerticalAlignValue::SubVAlignValue:
    case VerticalAlignValue::SuperVAlignValue:
    case VerticalAlignValue::TextTopVAlignValue:
    case VerticalAlignValue::TextBottomVAlignValue:
    case VerticalAlignValue::NumericVAlignValue: {
        auto it = ctx.tempFirstLineAscender(this);
        if (it.hasValue()) {
            LineBox* flb = it.getValue().first;
            return flb->absolutePoint(this).y() + it.getValue().second;
        }
        break;
    }
    default:
        break;
    }

    if (rowBox()) {
        return rowBox()->baseline();
    }

    return contentHeight();
}

size_t FrameTableCellBox::colspan()
{
    if (!(node() && node()->isHTMLElement())) {
        return 1;
    }

    HTMLElement* e = node()->asHTMLElement();
    if (e->isHTMLTableCellElement()) {
        return e->asHTMLTableCellElement()->colSpan();
    }

    // colspan is only accepted when HTML element is either <td> or <th>,
    // hence it is not applied when used in other elements.
    // e.g., <div style="display: table-cell" colspan="2">
    // In this case, we ignore the colspan value
    return 1;
}

size_t FrameTableCellBox::rowspan()
{
    if (!(node() && node()->isHTMLElement())) {
        return 1;
    }

    HTMLElement* e = node()->asHTMLElement();
    if (e->isHTMLTableCellElement()) {
        return e->asHTMLTableCellElement()->rowSpan();
    }

    // rowspan is only accepted when HTML element is either <td> or <th>,
    // hence it is not applied when used in other elements.
    // e.g., <div style="display: table-cell" rowspan="2">
    // In this case, we ignore the rowspan value
    return 1;
}

size_t FrameTableCellBox::updatedColspan()
{
    return m_updatedColspan;
}

size_t FrameTableCellBox::updatedRowspan()
{
    return m_updatedRowspan;
}

void FrameTableCellBox::updateColspanForLayout(size_t colspan)
{
    m_updatedColspan = colspan;
}

void FrameTableCellBox::resetColspanForLayout()
{
    updateColspanForLayout(1);
}

void* FrameTableCellBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameTableCellBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTableCellBox)] = { 0 };
        FrameTableCellBox::fillGCDescriptor(obj_bitmap);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableCellBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
}
