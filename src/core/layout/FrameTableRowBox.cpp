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
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

FrameTableRowBox::FrameTableRowBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{
}

void* FrameTableRowBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTableRowBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox, m_node));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableRowBox, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableRowBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox,
                                              m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableRowBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox,
                                              m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox,
                                              m_treeItemModel.m_lastChild));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableRowBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool FrameTableRowBox::hasChildCells()
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            return true;
        }
    }

    return false;
}

void FrameTableRowBox::collectCellWidthInfo(LayoutContext& ctx)
{
    // We traverse the cells first to calculate min/max cell width
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        c->asFrameTableCellBox()->collectCellWidthInfo(
            ctx, Frame::LayoutWantToResolve::ResolveWidth);
    }
}

void FrameTableRowBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        sectionBox()
            ->tableBox()
            ->style()
            ->horizontalBorderSpacing()
            .specifiedValue(unused, sectionBox()->tableBox());
    LayoutUnit xSoFar = borderSpacing;

    unsigned i = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        FrameTableCellBox* cell = c->asFrameTableCellBox();

        // shift cells to the right as this column is occupied by
        // upper cell that has rowspan
        for (; i < cell->absoluteColumnIndex(); i++) {
            STARFISH_ASSERT(i <
                            sectionBox()->tableBox()->columnWidths().size());
            xSoFar += sectionBox()->tableBox()->columnWidths()[i].cellWidth;

            if (i < sectionBox()->tableBox()->columnWidths().size() - 1) {
                xSoFar += borderSpacing;
            }
        }

        if (cell->absoluteColumnIndex() == i) {
            cell->setX(xSoFar);

            LayoutUnit cellWidth = 0;
            LayoutUnit oldWidth = cell->width();
            if (cell->updatedColspan() > 1) {
                // The cell width has been set in
                // FrameTableBox::calCellWidthsWithColspans() already
                cellWidth = cell->width();
            } else {
                STARFISH_ASSERT(
                    i < sectionBox()->tableBox()->columnWidths().size());
                cellWidth =
                    sectionBox()->tableBox()->columnWidths()[i].cellWidth;
                cell->setWidth(cellWidth);
            }

            if (oldWidth != cell->width()) {
                cell->markNeedsLayout();
            }
            cell->asFrameTableCellBox()->layoutWidth(ctx);
            xSoFar += cellWidth;

            if (i < sectionBox()->tableBox()->columnWidths().size() - 1) {
                xSoFar += borderSpacing;
            }

            i += cell->updatedColspan();
        }
    }

    xSoFar += borderSpacing;

    setWidth(xSoFar);
}

void FrameTableRowBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit maxCellHeightSoFar = 0;
    // 1. We make the second iteration of cells to layout cells
    //    and calculate the width of each cell
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        FrameTableCellBox* cell = c->asFrameTableCellBox();
        cell->setY(0);
        ctx.pushBlockBoxAligningAtFirstBaseline(cell);
        cell->layoutHeight(ctx);
        auto it = ctx.firstLineAscender(cell);
        if (it.hasValue()) {
            ctx.tempReigsterFirstLineAscender(cell, it.getValue());
        }
        ctx.popBlockBoxAligningAtFirstBaseline();
        LayoutUnit cellHeight = cell->height();

        // Cells with rowspan > 1 do not contribute to the height of a row
        if (cell->updatedRowspan() == 1) {
            maxCellHeightSoFar = std::max(maxCellHeightSoFar, cellHeight);
        }
    }

    // 2. The height of each cell is set to the max height of the cells
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            if (maxCellHeightSoFar > cell->height()) {
                cell->setHeight(maxCellHeightSoFar);
            }
        }
    }

    LayoutUnit specifiedHeight = 0;
    Length height = style()->height();
    if (height.isDefinite(false)) {
        LayoutUnit unused;
        specifiedHeight = height.specifiedValue(unused, this);
    } else if (height.isPercent()) {
        // The spec does not define how to calculate the height when the height
        // is specified in percentage
    } else if (height.isCalc()) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    setHeight(std::max(maxCellHeightSoFar, specifiedHeight));

    // layout absolute positioned blocks
    ctx.layoutRegisteredAbsolutePositionedBoxes(this);

    // layout relative positioned blocks
    ctx.layoutRegisteredRelativePositionedBoxes(this);
}

void FrameTableRowBox::increaseCellHeightBy(LayoutUnit cellHeightOffset)
{
    LayoutUnit maxRowHeightSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        FrameTableCellBox* cellBox = c->asFrameTableCellBox();
        cellBox->setHeight(cellBox->height() + cellHeightOffset);

        if (cellBox->updatedRowspan() == 1) {
            maxRowHeightSoFar = std::max(maxRowHeightSoFar, cellBox->height());
        }
    }
    setHeight(maxRowHeightSoFar);
}

void FrameTableRowBox::applyVerticalAlign(LayoutContext& ctx)
{
    m_baseline = calBaseline(ctx);
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        c->asFrameTableCellBox()->applyVerticalAlign(ctx);
    }
}

LayoutUnit FrameTableRowBox::calBaseline(LayoutContext& ctx)
{
    LayoutUnit maxSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        maxSoFar =
            std::max(maxSoFar, c->asFrameTableCellBox()->calBaseline(ctx));
    }

    return maxSoFar;
}

void FrameTableRowBox::layout(LayoutContext& ctx,
                              Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void FrameTableRowBox::paintBackgroundAndBorders(Canvas* canvas)
{
    Frame* child = firstChild();
    FrameBox fakeRow(node(), style());
    paintBoxShadows(canvas);
    while (child) {
        if (child->isFrameTableCellBox()) {
            FrameTableCellBox* cell = child->asFrameTableCellBox();
            canvas->save();
            canvas->translate(cell->x(), cell->y());
            fakeRow.copyFrom(cell, FrameBox::BorderBoxCopy);
            paintBackground(canvas, &fakeRow, nullptr);
            canvas->restore();
        }
        child = child->next();
    }
    paintInsetBoxShadows(canvas);
    paintBorders(canvas, m_frameRect);
}
}
