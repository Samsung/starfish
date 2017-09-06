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
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableColBox.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameTreeBuilder.h"

namespace StarFish {

RowStruct::RowStruct(FrameTableRowBox* tableRow)
    : m_tableRow(tableRow)
{
    for (Frame* cell = tableRow->firstChild(); cell; cell = cell->next()) {
        if (cell->isFrameTableCellBox()) {
            m_cells.push_back(CellStruct(cell->asFrameTableCellBox()));
        }
    }
}

unsigned RowStruct::logicalColumnSize()
{
    if (lastCell()) {
        FrameTableCellBox* cell = lastCell()->cell();
        return cell->absoluteColumnIndex() + cell->colspan();
    }
    return 0;
}

FrameTableCellBox* RowStruct::physicalCellAtLogicalColumn(size_t id)
{
    if (id < m_cells.size()) {
        if (m_cells[id].cell()->absoluteColumnIndex() == id) {
            return m_cells[id].cell();
        }
    }

    for (size_t i = id; i >= 0; i--) {
        if (i < m_cells.size()) {
            FrameTableCellBox* cell = m_cells[i].cell();
            if (cell->absoluteColumnIndex() <= id) {
                return cell;
            }
        }
    }

    STARFISH_ASSERT(false);
    return nullptr;
}

FrameTableSectionBox::FrameTableSectionBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{
}

void* FrameTableSectionBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTableSectionBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox, m_node));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableSectionBox, m_layoutParent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox,
                                              m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox,
                                              m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox,
                                              m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox,
                                              m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox,
                                              m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableSectionBox, m_lineBoxes));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox, m_grid));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableSectionBox, m_columnWidths));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox,
                                              m_affectedRowsByRowspans));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableSectionBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameTableSectionBox::paintBackgroundAndBorders(Canvas* canvas)
{
    for (auto& rowStruct : m_grid) {
        for (auto& cellStruct : rowStruct.cells()) {
            LayoutRect rect(rowStruct.tableRow()->x() + cellStruct.cell()->x(),
                            rowStruct.tableRow()->y() + cellStruct.cell()->y(),
                            cellStruct.cell()->frameRect().width(),
                            cellStruct.cell()->frameRect().height());

            FrameTableColBox* col = tableBox()->columnAtAbsoluteColumnIndex(
                cellStruct.cell()->absoluteColumnIndex());
            if (col) {
                // Paint background using column-group style
                if (col->parent()->isFrameTableColBox()) {
                    paintBackground(canvas, col->parent()->style(), rect, rect,
                                    false);
                }
                // Paint background using column style
                paintBackground(canvas, col->style(), rect, rect, false);
            }
            // Paint background using section style
            paintBackground(canvas, style(), rect, rect, false);
        }
    }

    // TODO : Below should be fixed when 'border-collpase:collapse' is
    // supported.
    paintBorders(canvas, m_frameRect);
    return;
}

void FrameTableSectionBox::calCellWidth(LayoutContext& ctx)
{
    // 0. We traverse the cells first to determine min/max cell size
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableRowBox());
        c->asFrameTableRowBox()->calCellWidth(ctx);
    }

    // 1. get max logical column size
    size_t logicalColSize = 0;
    for (size_t i = 0; i < m_grid.size(); i++) {
        RowStruct& row = m_grid[i];
        unsigned colSize = row.logicalColumnSize();
        logicalColSize = std::max<size_t>(logicalColSize, colSize);
    }

    // 2. get min/max preferred column width for each column.
    m_columnWidths.clear();
    for (size_t i = 0; i < logicalColSize; i++) {
        ColSizeStruct col;
        col.id = i;
        m_columnWidths.push_back(col);
    }

    for (auto& rowStruct : m_grid) {
        for (auto& cellStruct : rowStruct.cells()) {
            FrameTableCellBox* cell = cellStruct.cell();
            size_t index = cell->absoluteColumnIndex();
            STARFISH_ASSERT(index < m_columnWidths.size());
            ColSizeStruct* col = &m_columnWidths[index];

            col->minCellWidth =
                std::max(col->minCellWidth, cell->minCellWidth());
            col->maxCellWidth =
                std::max(col->maxCellWidth, cell->maxCellWidth());
            if (cell->style()->width().isFixed()) {
                LayoutUnit width = cell->style()->width().fixed();
                width += cell->borderWidth() + cell->paddingWidth();
                col->maxSpecifiedWidth =
                    std::max(col->maxSpecifiedWidth, width);
            } else if (cell->style()->width().isPercent()) {
                col->maxPercentageWidth =
                    std::max(col->maxPercentageWidth,
                             (double)cell->style()->width().percent());
            }
            col->isNullCell = false;
        }
    }
}

void FrameTableSectionBox::calAbsoluteColumnIndicesForCells()
{
    // 1. Set to normal indices
    for (auto& rowStruct : grid()) {
        size_t colId = 0;
        for (auto& cellStruct : rowStruct.cells()) {
            FrameTableCellBox* cell = cellStruct.cell();
            cell->setAbsoluteColumnIndex(colId);
            colId++;
        }
    }

    // 2. Cal indices
    size_t rowId = 0;
    for (auto& rowStruct : grid()) {
        FrameTableRowBox* row = rowStruct.tableRow();

        size_t colId = 0;
        for (auto& cellStruct : rowStruct.cells()) {
            FrameTableCellBox* cell = cellStruct.cell();
            if (cell->colspan() > 1) {
                calAbsoluteColumnIndicesForCellsAffectedByColspan(cell, rowId,
                                                                  colId);
            }
            if (cell->rowspan() > 1) {
                calAbsoluteColumnIndicesForCellsAffectedByRowspan(cell, rowId,
                                                                  colId);
            }

            colId++;
        }
        rowId++;
    }
}

void FrameTableSectionBox::calAbsoluteColumnIndicesForCellsAffectedByColspan(
    FrameTableCellBox* cell, size_t rowId, size_t colId)
{
    size_t shiftCellBy = cell->colspan() - 1;
    RowStruct& rowStruct = grid()[rowId];
    for (size_t i = colId + 1; i < rowStruct.cells().size(); i++) {
        CellStruct& cellStruct = rowStruct.cells()[i];
        FrameTableCellBox* curCell = cellStruct.cell();
        curCell->setAbsoluteColumnIndex(curCell->absoluteColumnIndex() +
                                        shiftCellBy);
    }
}

void FrameTableSectionBox::calAbsoluteColumnIndicesForCellsAffectedByRowspan(
    FrameTableCellBox* cell, size_t rowId, size_t colId)
{
    size_t rowEnd = rowId + cell->rowspan();
    for (size_t curRowId = rowId + 1; curRowId < rowEnd; curRowId++) {
        if (curRowId < grid().size()) {
            RowStruct& rowStruct = grid()[curRowId];

            for (size_t i = colId; i < rowStruct.cells().size(); i++) {
                CellStruct& cellStruct = rowStruct.cells()[i];
                FrameTableCellBox* curCell = cellStruct.cell();
                curCell->setAbsoluteColumnIndex(curCell->absoluteColumnIndex() +
                                                1);
            }
        }
    }
}

void FrameTableSectionBox::calCellWidthsWithColspans()
{
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        tableBox()->style()->horizontalBorderSpacing().fixed());

    for (auto& rowStruct : grid()) {
        for (auto& cellStruct : rowStruct.cells()) {
            FrameTableCellBox* cell = cellStruct.cell();

            if (cell->updatedColspan() > 1) {
                LayoutUnit maxCellWidth = 0;
                LayoutUnit minCellWidth = 0;

                size_t colId = cell->absoluteColumnIndex();
                for (size_t i = colId; i < colId + cell->updatedColspan();
                     i++) {
                    STARFISH_ASSERT(i < tableBox()->columnWidths().size());
                    if (tableBox()->columnWidths()[i].hasSpecifiedWidth()) {
                        maxCellWidth +=
                            tableBox()->columnWidths()[i].maxSpecifiedWidth;
                    } else {
                        maxCellWidth +=
                            tableBox()->columnWidths()[i].maxCellWidth;
                    }

                    minCellWidth += tableBox()->columnWidths()[i].minCellWidth;

                    if (i < colId + cell->updatedColspan() - 1) {
                        maxCellWidth += borderSpacing;
                        minCellWidth += borderSpacing;
                    }
                }

                cell->setMaxCellWidth(
                    std::max(cell->maxCellWidth(), maxCellWidth));
                cell->setMinCellWidth(
                    std::max(cell->minCellWidth(), minCellWidth));
            }
        }
    }
}

void FrameTableSectionBox::calCellHeightsWithRowspans()
{
    size_t rowId = 0;
    for (auto& rowStruct : grid()) {
        size_t colId = 0;
        for (auto& cellStruct : rowStruct.cells()) {
            FrameTableCellBox* cell = cellStruct.cell();

            if (cell->updatedRowspan() > 1) {
                LayoutUnit cellHeight =
                    calCellHeightWithRowspan(cell, rowId, colId);
                cell->setHeight(cellHeight);
            }

            colId++;
        }
        rowId++;
    }
}

LayoutUnit FrameTableSectionBox::calCellHeightWithRowspan(
    FrameTableCellBox* cell, size_t rowId, size_t colId)
{
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        tableBox()->style()->verticalBorderSpacing().fixed());

    LayoutUnit cellHeight = 0;
    size_t rowspan = cell->updatedRowspan();
    size_t rowEnd = rowId + rowspan;
    for (size_t curRowId = rowId; curRowId < rowEnd; curRowId++) {
        if (curRowId < grid().size()) {
            RowStruct& rowStruct = grid()[curRowId];

            if (colId < rowStruct.cells().size()) {
                CellStruct& cellStruct = rowStruct.cells()[colId];
                cellHeight += cellStruct.cell()->height();

                if (curRowId < rowEnd - 1) {
                    cellHeight += borderSpacing;
                }
                if (curRowId > rowId) {
                    m_affectedRowsByRowspans.insert(&rowStruct);
                }
            }
        }
    }

    return cellHeight;
}

void FrameTableSectionBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        tableBox()->style()->horizontalBorderSpacing().fixed());

    LayoutUnit xSoFar = borderSpacing;
    LayoutUnit maxWidth = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableRowBox());
        c->asFrameTableRowBox()->layoutWidth(ctx);
        c->asFrameBox()->setX(xSoFar);
        maxWidth = std::max(maxWidth, c->asFrameBox()->width());
    }

    // The width of all rows should be the same, so ideally, the maxWidth
    // should be the same as the width of any row.
    setWidth(borderSpacing + maxWidth + borderSpacing);
}

void FrameTableSectionBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit ySoFar = 0;
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        tableBox()->style()->verticalBorderSpacing().fixed());

    if (tableBox()->firstSectionBoxInVisualOrder() == this) {
        ySoFar += borderSpacing;
    }

    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableRowBox());
        c->asFrameBox()->setY(ySoFar);
        c->asFrameTableRowBox()->layoutHeight(ctx);
        ySoFar += c->asFrameBox()->height();
        ySoFar += borderSpacing;
    }

    setHeight(ySoFar);

    calCellHeightsWithRowspans();
    m_affectedRowsByRowspans.clear();
}

void FrameTableSectionBox::increaseRowHeightBy(LayoutUnit rowHeightOffset)
{
    LayoutUnit extraHeight = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableRowBox());
        c->asFrameTableRowBox()->increaseCellHeightBy(rowHeightOffset);
        extraHeight += rowHeightOffset;
    }
    setHeight(height() + extraHeight);
}

void FrameTableSectionBox::applyVerticalAlign(LayoutContext& ctx)
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableRowBox());
        c->asFrameTableRowBox()->applyVerticalAlign(ctx);
    }
}

void FrameTableSectionBox::layout(LayoutContext& ctx,
                                  Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}
}
