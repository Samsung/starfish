/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/Node.h"
#include "core/dom/HTMLTableCellElement.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameTreeBuilder.h"

namespace StarFish {

FrameTableRowBox::FrameTableRowBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_rowIndex(0)
    , m_lastAbsoluteColumnIndex(0)
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
    LayoutUnit xSoFar = 0;
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        sectionBox()
            ->tableBox()
            ->style()
            ->horizontalBorderSpacing()
            .specifiedValue(unused, sectionBox()->tableBox());

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

            cell->asFrameTableCellBox()->layoutWidth(ctx);
            xSoFar += cellWidth;

            if (i < sectionBox()->tableBox()->columnWidths().size() - 1) {
                xSoFar += borderSpacing;
            }

            i += cell->updatedColspan();
        }
    }

    setWidth(xSoFar);
}

void FrameTableRowBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit maxCellHeightSoFar = 0;
    LayoutUnit maxRowHeightSoFar = 0;
    // 1. We make the second iteration of cells to layout cells
    //    and calculate the width of each cell
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        FrameTableCellBox* cell = c->asFrameTableCellBox();
        ctx.pushBlockBoxAligningAtFirstBaseline(cell);
        cell->layoutHeight(ctx);
        auto it = ctx.firstLineAscender(cell);
        if (it.hasValue()) {
            ctx.tempReigsterFirstLineAscender(cell, it.getValue());
        }
        ctx.popBlockBoxAligningAtFirstBaseline();
        LayoutUnit cellHeight = cell->height();
        maxCellHeightSoFar = std::max(maxCellHeightSoFar, cellHeight);

        // Cells with rowspan > 1 do not contribute to the height of a row
        if (cell->updatedRowspan() == 1) {
            maxRowHeightSoFar = std::max(maxRowHeightSoFar, cellHeight);
        }
    }

    // 2. The height of each cell is set to the max height of the cells
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            cell->setHeight(maxCellHeightSoFar);
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

    setHeight(std::max(maxRowHeightSoFar, specifiedHeight));

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
    while (child) {
        if (child->isFrameTableCellBox()) {
            FrameTableCellBox* cell = child->asFrameTableCellBox();

            LayoutRect rect(cell->x(), cell->y(), cell->frameRect().width(),
                            cell->frameRect().height());
            paintBackground(canvas, nearstNotAnonymousNode(), style(), rect,
                            rect, false);
        }
        child = child->next();
    }
    paintBorders(canvas, m_frameRect);
}
}
