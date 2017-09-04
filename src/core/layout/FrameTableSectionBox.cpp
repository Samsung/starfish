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
    unsigned i = 0;
    for (Frame* cell = tableRow->firstChild(); cell; cell = cell->next()) {
        if (cell->isFrameTableCellBox()) {
            m_cells.push_back(CellStruct(cell->asFrameTableCellBox(), i));
            i += cell->asFrameTableCellBox()->colspan();
        }
    }
}

unsigned RowStruct::logicalColumnSize()
{
    if (lastCell()) {
        return lastCell()->id() + lastCell()->cell()->colspan();
    }
    return 0;
}

CellStruct* RowStruct::logicalCellStructAt(size_t id)
{
    if (id < m_cells.size()) {
        if (m_cells[id].id() == id) {
            return &m_cells[id];
        }
    }

    size_t logicalId = 0;
    for (size_t i = 0; i < m_cells.size(); i++) {
        if (id < logicalId) {
            break;
        }

        CellStruct* cell = &m_cells[i];
        if (cell->id() == id) {
            return cell;
        }

        logicalId += cell->cell()->colspan();
    }

    return nullptr;
}

FrameTableCellBox* RowStruct::physicalCellAtLogicalColumn(size_t id)
{
    if (id < m_cells.size()) {
        if (m_cells[id].id() == id) {
            return m_cells[id].cell();
        }
    }

    for (size_t i = 0; i < m_cells.size(); i++) {
        if (id <= m_cells[i].id()) {
            STARFISH_ASSERT(m_cells.size() - i > 0);
            return m_cells[i - 1].cell();
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

    // 2. get min/max column width for each column that does not have a colspan.
    // Empty columns are filled with empty ColSizeStruct.
    m_columnWidths.clear();
    for (size_t c = 0; c < logicalColSize; c++) {
        LayoutUnit minCellWidthSoFar = 0;
        LayoutUnit maxCellWidthSoFar = 0;
        LayoutUnit maxSpecifiedWidth = 0;
        float maxPercentageWidth = 0;

        ColSizeStruct col;

        // TODO: rowspan is not yet supported
        for (size_t r = 0; r < m_grid.size(); r++) {
            RowStruct& row = m_grid[r];
            if (c < row.logicalColumnSize()) {
                FrameTableCellBox* cell = row.logicalCellAt(c);

                if (cell) {
                    minCellWidthSoFar =
                        std::max(minCellWidthSoFar, cell->minCellWidth());
                    maxCellWidthSoFar =
                        std::max(maxCellWidthSoFar, cell->maxCellWidth());
                    if (cell->style()->width().isFixed()) {
                        LayoutUnit width = cell->style()->width().fixed();
                        width += cell->borderWidth() + cell->paddingWidth();
                        maxSpecifiedWidth = std::max(maxSpecifiedWidth, width);
                    } else if (cell->style()->width().isPercent()) {
                        maxPercentageWidth =
                            std::max(maxPercentageWidth,
                                     cell->style()->width().percent());
                    }
                    col.isNullCell = false;
                }
            }
        }

        col.id = c;
        col.maxSpecifiedWidth = maxSpecifiedWidth;
        col.maxPercentageWidth = maxPercentageWidth;
        col.minCellWidth = minCellWidthSoFar;
        col.maxCellWidth = maxCellWidthSoFar;
        col.cellWidth = maxCellWidthSoFar;
        m_columnWidths.push_back(col);
    }
}

void FrameTableSectionBox::calCellWidthsWithColspans()
{
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        tableBox()->style()->horizontalBorderSpacing().fixed());

    for (auto& rowStruct : grid()) {
        FrameTableRowBox* row = rowStruct.tableRow();
        row->colsWithColspans().clear();

        unsigned id = 0;
        for (auto& cellStruct : rowStruct.cells()) {
            FrameTableCellBox* cell = cellStruct.cell();

            if (cell->updatedColspan() > 1) {
                LayoutUnit maxCellWidth = 0;
                LayoutUnit minCellWidth = 0;

                for (size_t i = id; i < id + cell->updatedColspan(); i++) {
                    if (tableBox()->columnWidths()[i].hasSpecifiedWidth()) {
                        maxCellWidth +=
                            tableBox()->columnWidths()[i].maxSpecifiedWidth;
                    } else {
                        maxCellWidth +=
                            tableBox()->columnWidths()[i].maxCellWidth;
                    }

                    minCellWidth += tableBox()->columnWidths()[i].minCellWidth;

                    if (i < id + cell->updatedColspan() - 1) {
                        maxCellWidth += borderSpacing;
                        minCellWidth += borderSpacing;
                    }
                }

                ColSizeStruct col;
                col.id = id;
                col.maxCellWidth = std::max(cell->maxCellWidth(), maxCellWidth);
                col.minCellWidth = std::max(cell->minCellWidth(), minCellWidth);
                row->colsWithColspans().push_back(col);
            }

            id += cell->updatedColspan();
        }
    }
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
