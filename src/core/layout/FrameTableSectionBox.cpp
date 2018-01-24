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
#include "core/layout/FrameTableColBox.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

RowStruct::RowStruct(FrameTableRowBox* tableRow)
    : m_tableRow(tableRow)
{
}

unsigned RowStruct::logicalColumnSize()
{
    if (lastCell()) {
        FrameTableCellBox* cell = lastCell()->cell();
        return cell->absoluteColumnIndex() + cell->updatedColspan();
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

    for (size_t i = id; i <= id; i--) {
        if (i < m_cells.size()) {
            FrameTableCellBox* cell = m_cells[i].cell();
            if (cell->absoluteColumnIndex() <= id) {
                return cell;
            }
        }
    }

    // A cell does not exist at logical column `id`, when that cell's space
    // is occupied by a cell with rowspan.
    // e.g., given column id = 0
    //
    // | 0 |   |   |
    // |   +---+---+
    // |   | 1 | 2 |
    // +---+---+---+
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
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableSectionBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameTableSectionBox::paintBackgroundAndBorders(Canvas* canvas)
{
    FrameBox fakeSection(node(), style());

    for (auto& rowStruct : m_grid) {
        for (auto& cellStruct : rowStruct.cells()) {
            canvas->save();
            canvas->translate(
                rowStruct.tableRow()->x() + cellStruct.cell()->x(),
                rowStruct.tableRow()->y() + cellStruct.cell()->y());

            FrameTableColBox* col = tableBox()->columnAtAbsoluteColumnIndex(
                cellStruct.cell()->absoluteColumnIndex());
            if (col) {
                // Paint background using column-group style
                if (col->parent()->isFrameTableColBox()) {
                    FrameBox fakeColGroup(col->parent()->node(),
                                          col->parent()->style());
                    fakeColGroup.copyFrom(cellStruct.cell(),
                                          FrameBox::BorderBoxCopy);
                    paintBackground(canvas, &fakeColGroup, nullptr);
                }
                // Paint background using column style
                FrameBox fakeCol(col->node(), col->style());
                fakeCol.copyFrom(cellStruct.cell(), FrameBox::BorderBoxCopy);
                paintBackground(canvas, &fakeCol, nullptr);
            }
            // Paint background using section style
            fakeSection.copyFrom(cellStruct.cell(), FrameBox::BorderBoxCopy);
            paintBackground(canvas, &fakeSection, nullptr);

            canvas->restore();
        }
    }

    // TODO : Below should be fixed when 'border-collpase:collapse' is
    // supported.
    paintBorders(canvas, m_frameRect);
    return;
}

void FrameTableSectionBox::collectCellWidthInfo(LayoutContext& ctx)
{
    // 0. We traverse the cells first to determine min/max cell size
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableRowBox());
        c->asFrameTableRowBox()->collectCellWidthInfo(ctx);
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

            if (cell->updatedColspan() == 1) {
                col->minCellWidth =
                    std::max(col->minCellWidth, cell->minCellWidth());
                col->maxCellWidth =
                    std::max(col->maxCellWidth, cell->maxCellWidth());
                Length width = cell->style()->width();
                if (width.isDefinite(false)) {
                    LayoutUnit unused;
                    LayoutUnit w = width.specifiedValue(unused, this);
                    w += cell->borderWidth() + cell->paddingWidth();
                    col->maxSpecifiedWidth =
                        std::max(col->maxSpecifiedWidth, w);
                } else if (width.isPercent()) {
                    col->maxPercentageWidth =
                        std::max(col->maxPercentageWidth,
                                 (double)cell->style()->width().percent());
                } else if (width.isCalc()) {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                }
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

        for (auto& cellStruct : rowStruct.cells()) {
            FrameTableCellBox* cell = cellStruct.cell();
            if (cell->updatedColspan() > 1) {
                calAbsoluteColumnIndicesForCellsAffectedByColspan(cell, rowId);
            }
            if (cell->updatedRowspan() > 1) {
                calAbsoluteColumnIndicesForCellsAffectedByRowspan(cell, rowId);
            }
        }
        rowId++;
    }
}

void FrameTableSectionBox::calAbsoluteColumnIndicesForCellsAffectedByColspan(
    FrameTableCellBox* cell, size_t rowId)
{
    size_t shiftCellBy = cell->updatedColspan() - 1;
    size_t colId = cell->absoluteColumnIndex();
    RowStruct& rowStruct = grid()[rowId];
    for (size_t i = 0; i < rowStruct.cells().size(); i++) {
        CellStruct& cellStruct = rowStruct.cells()[i];
        FrameTableCellBox* curCell = cellStruct.cell();

        if (curCell->absoluteColumnIndex() > colId) {
            curCell->setAbsoluteColumnIndex(curCell->absoluteColumnIndex() +
                                            shiftCellBy);
        }
    }
}

void FrameTableSectionBox::calAbsoluteColumnIndicesForCellsAffectedByRowspan(
    FrameTableCellBox* cell, size_t rowId)
{
    size_t rowEnd = rowId + cell->updatedRowspan();
    size_t colId = cell->absoluteColumnIndex();
    for (size_t curRowId = rowId + 1; curRowId < rowEnd; curRowId++) {
        if (curRowId < grid().size()) {
            RowStruct& rowStruct = grid()[curRowId];

            for (size_t i = 0; i < rowStruct.cells().size(); i++) {
                CellStruct& cellStruct = rowStruct.cells()[i];
                FrameTableCellBox* curCell = cellStruct.cell();

                if (curCell->absoluteColumnIndex() >= colId) {
                    curCell->setAbsoluteColumnIndex(
                        curCell->absoluteColumnIndex() + 1);
                }
            }
        }
    }
}

void FrameTableSectionBox::calCellWidthsWithColspans()
{
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        tableBox()->style()->horizontalBorderSpacing().specifiedValue(unused,
                                                                      this);

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

            // increase the height of the cell if the cell's height
            // is smaller than the row's height
            if (cell->updatedRowspan() > 1) {
                LayoutUnit cellHeight =
                    calCellHeightWithRowspan(cell, rowId, colId);
                cell->setHeight(std::max(cell->height(), cellHeight));
            }

            colId++;
        }
        rowId++;
    }
}

LayoutUnit FrameTableSectionBox::calCellHeightWithRowspan(
    FrameTableCellBox* cell, size_t rowId, size_t colId)
{
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        tableBox()->style()->verticalBorderSpacing().specifiedValue(unused,
                                                                    tableBox());

    LayoutUnit cellHeight = 0;
    size_t rowspan = cell->updatedRowspan();
    size_t rowEnd = rowId + rowspan;
    for (size_t curRowId = rowId; curRowId < rowEnd; curRowId++) {
        if (curRowId < grid().size()) {
            RowStruct& rowStruct = grid()[curRowId];

            cellHeight += rowStruct.tableRow()->height();
            if (curRowId < rowEnd - 1) {
                cellHeight += borderSpacing;
            }
        }
    }

    return cellHeight;
}

void FrameTableSectionBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit maxRowWidth = 0;

    // There are no child nodes
    if (!firstChild()) {
        LayoutUnit unused;
        LayoutUnit borderSpacing =
            tableBox()->style()->horizontalBorderSpacing().specifiedValue(
                unused, tableBox());
        maxRowWidth += borderSpacing * 2;
    } else {
        for (Frame* c = firstChild(); c; c = c->next()) {
            STARFISH_ASSERT(c->isFrameTableRowBox());
            c->asFrameTableRowBox()->layoutWidth(ctx);
            maxRowWidth = std::max(maxRowWidth, c->asFrameBox()->width());
        }
    }

    // The width of all rows should be the same, so ideally, the maxWidth
    // should be the same as the width of any row.
    setWidth(maxRowWidth);
}

void FrameTableSectionBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit ySoFar = 0;
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        tableBox()->style()->verticalBorderSpacing().specifiedValue(unused,
                                                                    tableBox());

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

    calCellHeightsWithRowspans();

    // A cell's height can be greater than the row's height when the cell
    // has rowspan > 1. This results from not considering the cell's height
    // when the row's height is calculated. In this case, table section's
    // height is increased to include oversized cells, not row's height.
    //
    // Blink does not increase the row's height, while Firefox does.
    // We follow how Blink does (although what Firefox does looks correct)
    std::vector<LayoutUnit> sumOfCellHeightsSoFar(
        tableBox()->columnWidths().size(), 0);
    if (tableBox()->firstSectionBoxInVisualOrder() == this) {
        for (size_t i = 0; i < sumOfCellHeightsSoFar.size(); i++) {
            sumOfCellHeightsSoFar[i] += borderSpacing;
        }
    }

    // Traverse each column top to bottom, and calculate the sum of cell's
    // height
    for (size_t colId = 0; colId < sumOfCellHeightsSoFar.size(); colId++) {
        size_t rowId = 0;
        for (auto& rowStruct : grid()) {
            FrameTableCellBox* cell =
                rowStruct.physicalCellAtLogicalColumn(colId);

            if (cell && cell->absoluteColumnIndex() == colId) {
                sumOfCellHeightsSoFar[colId] += cell->height();

                if (rowId < grid().size()) {
                    sumOfCellHeightsSoFar[colId] += borderSpacing;
                }
            }

            rowId++;
        }
    }

    LayoutUnit maxSumOfCellHeights = 0;
    for (auto& sumOfCellHeight : sumOfCellHeightsSoFar) {
        maxSumOfCellHeights = std::max(maxSumOfCellHeights, sumOfCellHeight);
    }

    setHeight(std::max(ySoFar, maxSumOfCellHeights));
}

void FrameTableSectionBox::increaseRowHeightBy(LayoutUnit rowHeightOffset)
{
    LayoutUnit extraHeight = 0;
    LayoutUnit heightIncreasedBy = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableRowBox());
        FrameTableRowBox* row = c->asFrameTableRowBox();
        row->setY(row->y() + heightIncreasedBy);
        LayoutUnit heightBefore = row->height();
        c->asFrameTableRowBox()->increaseCellHeightBy(rowHeightOffset);
        LayoutUnit heightAfter = row->height();
        heightIncreasedBy = heightAfter - heightBefore;
        extraHeight += heightIncreasedBy;
    }
    setHeight(height() + extraHeight);
    calCellHeightsWithRowspans();
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
