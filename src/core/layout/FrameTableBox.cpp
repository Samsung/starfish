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
#include "core/dom/Node.h"
#include "core/dom/HTMLTableElement.h"
#include "core/dom/HTMLTableCellElement.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCaptionBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableColBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/style/CSSParser.h"

namespace StarFish {

FrameTableBox::FrameTableBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_tableRect(0, 0, 0, 0)
{
    m_table = new Table();
}

void* FrameTableBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTableBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableBox, m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableBox, m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableBox, m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_lineBoxes));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_table));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_captions));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_colObjects));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_columnWidths));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableBox, m_cellsInTheFirstRow));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_colBoxes));

        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameTableBox::computeTableWidth(LayoutContext& ctx)
{
    formingATable();

    calCellWidth(ctx);
    calCellWidthsWithColspans();
    layoutWidth(ctx);
}

void FrameTableBox::layoutTable(LayoutContext& ctx)
{
    // Table starts its own layout algorithm that has minimum
    // interaction with the existing layout algorithm.
    // In brief, after establishes a table context, we calculate
    // the width of the table, and place cells in rows and columns.
    // To do so, we calculate x positions of cells first, and then
    // calculate the y positions of cells.
    layoutHeight(ctx);
}

// https://html.spec.whatwg.org/multipage/tables.html#forming-a-table
// We run the algorithm on a table frame tree, so that we do not need to worry
// about HTML and CSS tables.
void FrameTableBox::formingATable()
{
    // 1-4
    size_t xWidth = 0;
    size_t yHeight = 0;
    std::vector<FrameTableSectionBox*> tfootSectionBoxes;
    m_table->clear();

    // 5
    if (firstChild() == nullptr) {
        return;
    }

    // 6-9: Generate ColGroups if exist
    FrameTableCaptionBox* firstCaption = nullptr;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCaptionBox() && !firstCaption) {
            firstCaption = c->asFrameTableCaptionBox();
        } else if (c->isFrameTableColBox()) {
            // either colgroup or col
            ColGroup* colGroup = nullptr;
            FrameTableColBox* colParentBox = c->asFrameTableColBox();
            if (colParentBox->hasChildColBox()) {
                int xStart = xWidth;
                colGroup = new ColGroup(xStart);
                for (Frame* col = colParentBox->firstChild(); col;
                     col = col->next()) {
                    if (col->isFrameTableColBox()) {
                        FrameTableColBox* colChildBox =
                            col->asFrameTableColBox();
                        xWidth += colChildBox->span();
                    }
                }
                colGroup->m_slotWidth = xWidth - xStart;
            } else {
                int span = colParentBox->span();
                colGroup = new ColGroup(xWidth);
                colGroup->m_slotWidth = span;
                xWidth += span;
            }
            m_table->m_colGroups.push_back(colGroup);
        }
    }

    // 10-
    size_t yCurrent = 0;
    GCVector<Cell*>* downwardGrowingCells = new GCVector<Cell*>();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRowBox()) {
            processRow(c->asFrameTableRowBox(), yCurrent, xWidth, yHeight);
        } else if (c->style()->display() ==
                   DisplayValue::TableFooterGroupDisplayValue) {
            tfootSectionBoxes.push_back(c->asFrameTableSectionBox());
        } else if ((c->style()->display() ==
                    DisplayValue::TableHeaderGroupDisplayValue) ||
                   (c->style()->display() ==
                    DisplayValue::TableRowGroupDisplayValue)) {
            size_t yStart = yHeight;
            for (Frame* tr = c->firstChild(); tr; tr = tr->next()) {
                if (tr->isFrameTableRowBox()) {
                    processRow(tr->asFrameTableRowBox(), yCurrent, xWidth,
                               yHeight);
                    if (yHeight > yStart) {
                        // TODO: form a new row group
                        // This part can be skipped if a row group is not needed
                        // later part of the algorithm.
                        // Come back this part later
                    }
                }
            }
        }
    }

    for (FrameTableSectionBox* tfoot : tfootSectionBoxes) {
        size_t yStart = yHeight;
        for (Frame* tr = tfoot->firstChild(); tr; tr = tr->next()) {
            processRow(tr->asFrameTableRowBox(), yCurrent, xWidth, yHeight);
        }
    }
    m_table->m_width = xWidth;
    m_table->m_height = yHeight;
}

void FrameTableBox::processRow(FrameTableRowBox* rowBox, size_t& yCurrent,
                               size_t& xWidth, size_t& yHeight)
{
    // 1-4
    if (yHeight == yCurrent) {
        yHeight++;
    }
    size_t xCurrent = 0;
    // TODO: Run the algorithm for growing downward-growing cells.
    if (!rowBox->hasChildCells()) {
        yCurrent++;
        return;
    }
    // 5-18
    Row* row = new Row();
    m_table->m_rows.push_back(row);
    for (Frame* c = rowBox->firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();

            while (xCurrent < xWidth && isSlotOccupied(xCurrent, yCurrent)) {
                xCurrent++;
            }

            if (xCurrent == xWidth) {
                xWidth++;
            }

            size_t colspan = cell->colspan();
            size_t rowspan = cell->rowspan();

            // TODO: 10.
            // rowspan cannot be 0 by definition. But this part of spec
            // asks to make rowspan=1 if rowspan=0, and perform
            // growingDownwardCell algo. Come back later this part.

            if (xWidth < xCurrent + colspan) {
                xWidth = xCurrent + colspan;
            }
            if (yHeight < yCurrent + rowspan) {
                yHeight = yCurrent + rowspan;
            }

            Cell* newCell =
                new Cell(cell, xCurrent, yCurrent, colspan, rowspan);
            row->m_cells.push_back(newCell);
            xCurrent += colspan;

            // 13 Assigning header cells
            if (cell->isHTMLTHElement()) {
                // TODO: TH is not supported yet
                // assigningHeaderCells(newCell);
            }
        }
    }
    yCurrent++;
}

Cell* FrameTableBox::cellAtSlot(size_t x, size_t y)
{
    if (y < m_table->m_rows.size()) {
        // 1. We check whether a cell occupies the given slot.
        Row* row = m_table->m_rows[y];
        if (x < row->m_cells.size()) {
            Cell* cell = row->m_cells[x];
            if ((cell->m_slotX == x) && (cell->m_slotY == y)) {
                return cell;
            }
        }
    }

    // 2. We check the entire cells in the table
    for (size_t i = y; i < m_table->m_rows.size(); i--) {
        Row* row = m_table->m_rows[i];
        for (size_t j = 0; j < row->m_cells.size(); j++) {
            Cell* cell = row->m_cells[j];
            if ((cell->m_slotX <= x && x < cell->m_slotX + cell->m_width) &&
                (cell->m_slotY <= y && y < cell->m_slotY + cell->m_height)) {
                return cell;
            }
        }
    }

    return nullptr;
}

// https://html.spec.whatwg.org/multipage/tables.html#table-processing-model
// To reduce memory usage, we do not explicitly create slots for a table.
// Instead, we calculate whether a cell created so far occupies the given slot
bool FrameTableBox::isSlotOccupied(size_t x, size_t y)
{
    return cellAtSlot(x, y) != nullptr ? true : false;
}

// https://html.spec.whatwg.org/multipage/tables.html#algorithm-for-assigning-header-cells
void FrameTableBox::assigningHeaderCells(Cell* principalCell)
{
    GCVector<Cell*> headers;
    size_t principalX = principalCell->m_slotX;
    size_t principalY = principalCell->m_slotY;
    size_t principalWidth = principalCell->m_width;
    size_t principalHeight = principalCell->m_height;

    // if principal cell does not have a headers attribute specified
    for (size_t y = principalY; y < principalY + principalHeight; y++) {
        scanningAndAssigningHeaderCells(principalCell, headers, -1, 0);
    }

    for (size_t x = principalX; x < principalX + principalWidth; x++) {
        scanningAndAssigningHeaderCells(principalCell, headers, 0, -1);
    }
}

// https://html.spec.whatwg.org/multipage/tables.html#internal-algorithm-for-scanning-and-assigning-header-cells
void FrameTableBox::scanningAndAssigningHeaderCells(Cell* principalCell,
                                                    GCVector<Cell*>& headers,
                                                    int deltaX, int deltaY)
{
    GCVector<Cell*> opaqueHeaders;

    bool inHeaderBlock = false;
    GCVector<Cell*> headersFromCurrentHeaderBlock;
    if ((principalCell->m_cellBox->isHTMLTHElement())) {
        inHeaderBlock = true;
        headersFromCurrentHeaderBlock.push_back(principalCell);
    }

    for (size_t x = principalCell->m_slotX, y = principalCell->m_slotY;
         (x <= principalCell->m_slotX) && (y <= principalCell->m_slotY);
         x += deltaX, y += deltaY) {
        // TODO: If there is no cell covering slot (x, y), or if there is more
        // than one cell covering slot (x, y), skip it.
        Cell* curCell = cellAtSlot(x, y);
        if (!curCell) {
            continue;
        }

        if (curCell->m_cellBox->isHTMLTHElement()) {
            inHeaderBlock = true;
            headersFromCurrentHeaderBlock.push_back(curCell);
            // TODO: traverse top and left direction and look for th element
        } else if (!curCell->m_cellBox->isHTMLTHElement() && inHeaderBlock) {
            inHeaderBlock = false;
            opaqueHeaders.insert(opaqueHeaders.end(),
                                 headersFromCurrentHeaderBlock.begin(),
                                 headersFromCurrentHeaderBlock.end());
            headersFromCurrentHeaderBlock.clear();
        }
    }
}

FrameTableCellBox* FrameTableBox::cellInTheFirstRowAt(unsigned id)
{
    // There are more cells in the following rows than the first row
    // This case happens when following rows have anonymous cells
    if (id >= m_cellsInTheFirstRow.size()) {
        return nullptr;
    }

    FrameTableCellBox* cell = m_cellsInTheFirstRow[id];
    if (cell) {
        return cell;
    } else {
        for (size_t i = id - 1; i <= id - 1; i--) {
            if (m_cellsInTheFirstRow[i]) {
                return m_cellsInTheFirstRow[i];
            }
        }
        // empty cells in the first row
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

void FrameTableBox::resetIfNeeds(LayoutContext& ctx)
{
    if (ctx.didResetTable(this)) {
        return;
    }
    ctx.markDidResetTable(this);

    Frame* sectionChild = firstChild();
    while (sectionChild) {
        if (sectionChild->isFrameTableSectionBox()) {
            FrameTableSectionBox* section =
                sectionChild->asFrameTableSectionBox();
            section->grid().clear();
            Frame* rowChild = section->firstChild();
            while (rowChild) {
                if (rowChild->isFrameTableRowBox()) {
                    FrameTableRowBox* row = rowChild->asFrameTableRowBox();
                    RowStruct rowStruct(row);

                    Frame* cellChild = row->firstChild();
                    unsigned lastAbsoluteColumnIndex = 0;
                    while (cellChild) {
                        if (cellChild->isFrameTableCellBox()) {
                            FrameTableCellBox* cell =
                                cellChild->asFrameTableCellBox();
                            cell->setAbsoluteColumnIndex(
                                lastAbsoluteColumnIndex);
                            lastAbsoluteColumnIndex += cell->colspan();
                            rowStruct.cells().push_back(CellStruct(cell));
                        }
                        cellChild = cellChild->next();
                    }

                    section->grid().push_back(rowStruct);
                }
                rowChild = rowChild->next();
            }
        }
        sectionChild = sectionChild->next();
    }
}

void FrameTableBox::calCellWidth(LayoutContext& ctx)
{
    resetIfNeeds(ctx);

    for (int i = 0; i < 2; i++) {
        // 0. Calculate absoluteColumnIndex for cells
        for (Frame* c = firstChild(); c; c = c->next()) {
            if (c->isFrameTableSectionBox()) {
                c->asFrameTableSectionBox()->calAbsoluteColumnIndicesForCells();
            }
        }

        // 0. The spec says to look at the first row only to get the width for
        // each cell. But, there are cases where the following rows contains
        // more cells than the first row. In this case, the spec leaves what to
        // do to implementors. We try to obtain the width of those cells
        // similar to "table-layout: auto", i.e., we perform the following
        // to get:
        //  * min/max cell widths of the table if "width: auto"
        m_columnWidths.clear();
        for (Frame* c = firstChild(); c; c = c->next()) {
            if (c->isFrameTableSectionBox()) {
                c->asFrameTableSectionBox()->collectCellWidthInfo(ctx);
                collectColumnWidths(
                    m_columnWidths,
                    c->asFrameTableSectionBox()->columnWidths());
            }
        }

        // Do not need to run the following steps in the second run
        if (i == 1) {
            break;
        }

        // 0. Remove unneeded colspan="x", where x > 1, if colspan does not
        // collapse any cells.
        // i.e., all columns have the same colspan="x", x > 1.
        // If there are cells that have updated colspans to "colspan=1", we
        // need to recalculate absoluteColumnIndexes for the cells
        // one more time.
        bool colspanUpdated = resetColspanIfPossible();
        if (!colspanUpdated) {
            break;
        }
    }

    collectColBoxes();

    // 0. Get the cells in the first row. These are used to determine:
    // * the width of each cell, and
    // * the width property (i.e., auto or specified) in the table.
    m_cellsInTheFirstRow.clear();
    FrameTableSectionBox* firstSection = firstSectionBoxInVisualOrder();
    if (firstSection && firstSection->firstChild()) {
        // We use nullptr to occupy spaces for non-existing cells because of
        // previous colspans
        FrameTableRowBox* row =
            firstSection->firstChild()->asFrameTableRowBox();
        for (Frame* c = row->firstChild(); c; c = c->next()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            m_cellsInTheFirstRow.push_back(cell);
            for (unsigned i = 1; i < cell->updatedColspan(); i++) {
                m_cellsInTheFirstRow.push_back(nullptr);
            }
        }
    }

    // We iterate columns and calculate the following:
    //   * sum of all cells with specified widths.
    //   * calculate initial column width for cells with specified width
    //   * min/max table width
    //     - TODO: Need to consider caption widths too
    //   * collect auto and specified width cells for later calculation
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        style()->horizontalBorderSpacing().specifiedValue(unused, this);

    LayoutUnit minTableWidth = 0;
    LayoutUnit maxTableWidth = 0;
    minTableWidth += mbpWidth();
    minTableWidth += borderSpacing;
    maxTableWidth = minTableWidth;

    std::vector<ColSizeStruct*> cellsWithAutoWidths;
    std::vector<ColSizeStruct*> cellsWithSpecifiedWidths;
    for (auto& col : m_columnWidths) {
        if (isCellWidthAuto(col.id)) {
            cellsWithAutoWidths.push_back(&col);
        } else {
            cellsWithSpecifiedWidths.push_back(&col);
        }

        minTableWidth += col.minCellWidth + borderSpacing;
        maxTableWidth += col.maxCellWidth + borderSpacing;
    }

    // https://www.w3.org/TR/CSS2/tables.html#width-layout
    // Applying rules in Section 17.5.2.1
    bool tableLayoutFixed = false;
    if ((style()->display() == DisplayValue::TableDisplayValue ||
         style()->display() == DisplayValue::InlineTableDisplayValue) &&
        (style()->tableLayout() == TableLayoutValue::FixedTableLayoutValue)) {
        // TODO: Need to consider when (!hasTableWidth && tableLayoutFixed)
        tableLayoutFixed = true;
    }

    // Followed the algorithm from
    // https://www.w3.org/TR/2016/WD-css-tables-3-20161025/#width-distribution
    // except when table width is given, we simple use the width (following how
    // blink works)
    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    LayoutUnit tableWidth;
    bool hasTableWidth = false;
    Length width = style()->width();
    if (width.isAuto()) {
        tableWidth = std::max(minTableWidth, parentContentWidth);
        tableWidth -= marginWidth();
    } else {
        // The width of table is explicitly given
        hasTableWidth = true;

        if (width.isDefinite(false)) {
            LayoutUnit unused;
            tableWidth = width.specifiedValue(unused, this);

            // For CSS table, width refers to the table content width.
            // For HTML table, width refers to table content width + border +
            // padding.
            if (!isAnonymous() && !node()->isHTMLTableElement()) {
                tableWidth += borderWidth() + paddingWidth();
            }
        } else if (width.isPercent()) {
            tableWidth = width.percentValue(parentContentWidth);
        } else if (width.isCalc()) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
    }

    LayoutUnit tableContentWidth = tableWidth - borderWidth() - paddingWidth();

    // The values from <col> have higher priority
    for (auto& col : m_columnWidths) {
        if (tableLayoutFixed && !isCellWidthAuto(col.id) && hasColBox(col.id)) {
            FrameTableColBox* colBox = m_colBoxes[col.id];
            Length width = colBox->style()->width();

            if (width.isDefinite(false)) {
                LayoutUnit unused;
                LayoutUnit specifiedWidth = width.specifiedValue(unused, this);
                specifiedWidth +=
                    colBox->borderWidth() + colBox->paddingWidth();
                col.maxSpecifiedWidth = specifiedWidth;
            } else if (width.isPercent()) {
                col.maxPercentageWidth = width.percent();
            }
        }
    }

    LayoutUnit sumOfAutoCellPreferredWidths = 0;
    LayoutUnit sumOfAdjustedSpecifiedCellWidths = 0;
    std::vector<ColSizeStruct*> columnsAdjustedToMinWidths;
    std::vector<ColSizeStruct*> columnsMayNeedToAdjustWidths;
    LayoutUnit sumOfColWidths = 0;

    LayoutUnit availableWidth = tableContentWidth;
    availableWidth -= (borderSpacing * m_columnWidths.size()) + borderSpacing;

    setCandidateCellWidthsAndReturnCellInfo(
        ctx, availableWidth, hasTableWidth, tableLayoutFixed,
        &sumOfAutoCellPreferredWidths, &sumOfAdjustedSpecifiedCellWidths,
        &columnsAdjustedToMinWidths, &columnsMayNeedToAdjustWidths,
        &sumOfColWidths);

    // 3. If a width of the table is given, we either increase or decrease the
    // cell widths to fit them into the width of table.
    if (hasTableWidth) {
        LayoutUnit sumOfSpecifiedCellWidths = sumOfAdjustedSpecifiedCellWidths;

        double sumOfWidthPercentage = 0;
        for (auto& c : cellsWithSpecifiedWidths) {
            ColSizeStruct& col = *c;
            STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());

            Length width = cellFromFirstRowOrColGroup(tableLayoutFixed, col.id)
                               ->style()
                               ->width();
            if (width.isPercent()) {
                sumOfWidthPercentage += width.percent();
            } else if (width.isCalc()) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        }

        if (sumOfWidthPercentage > 1) {
            // Sum of all widths specified in percentage is greater than 100%.
            // In this case, we reduce each cell width in proportion to its
            // width over the sum of all widths.
            sumOfSpecifiedCellWidths = 0;
            for (auto& c : cellsWithSpecifiedWidths) {
                ColSizeStruct& col = *c;
                STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());
                Length width =
                    cellFromFirstRowOrColGroup(tableLayoutFixed, col.id)
                        ->style()
                        ->width();

                if (width.isPercent()) {
                    LayoutUnit specifiedWidth =
                        width.percentValue(availableWidth) /
                        sumOfWidthPercentage;
                    col.cellWidth = specifiedWidth;
                } else if (width.isCalc()) {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                }
                sumOfSpecifiedCellWidths += col.cellWidth;
            }
        }

        {
            // Adjust cell widths to fit into the specified widths.
            // At this stage, the sum of all specified cell widths can be
            // smaller or bigger than the specified widths.
            // * Also, there may be cells with "width: auto" that have no widths
            //   calculated yet

            // All cells have fixed width. In this case, distribute
            // available spaces among cells. The extra space for each cell
            // is proportional to the width of each cell.
            if (cellsWithAutoWidths.empty()) {
                bool shouldUseContentWidth = false;
                if (tableLayoutFixed) {
                    bool isEveryCellHasNonPercentageWidth = true;
                    LayoutUnit sumSoFar;
                    for (auto& col : m_columnWidths) {
                        LayoutUnit specifiedWidth = 0;
                        FrameTableCellBox* cellBox = cellFromFirstRowOrColGroup(
                            tableLayoutFixed, col.id);
                        Length width = cellBox->style()->width();

                        if (width.isDefinite(false)) {
                            LayoutUnit unused;
                            sumSoFar += width.specifiedValue(unused, this);
                        } else {
                            isEveryCellHasNonPercentageWidth = false;
                        }
                    }
                    if (isEveryCellHasNonPercentageWidth &&
                        availableWidth <= sumOfAdjustedSpecifiedCellWidths) {
                        sumOfSpecifiedCellWidths = sumSoFar;
                        shouldUseContentWidth = true;
                    }
                }

                for (auto& col : m_columnWidths) {
                    FrameTableCellBox* cell = cellInTheFirstRowAt(col.id);
                    Length width =
                        cellFromFirstRowOrColGroup(tableLayoutFixed, col.id)
                            ->style()
                            ->width();

                    if (width.isDefinite(false)) {
                        LayoutUnit unused;
                        LayoutUnit cellWidth =
                            width.specifiedValue(unused, this);
                        if (!shouldUseContentWidth) {
                            cellWidth +=
                                cell->borderWidth() + cell->paddingWidth();
                        }

                        LayoutUnit newCellWidth =
                            LayoutUnit(cellWidth.toDouble() /
                                       sumOfSpecifiedCellWidths.toDouble() *
                                       availableWidth.toDouble());
                        col.cellWidth = newCellWidth;
                        if (tableLayoutFixed && newCellWidth < cellWidth) {
                            col.cellWidth = cellWidth;
                        }
                    }
                }
            } else {
                LayoutUnit remainingWidth = availableWidth;
                remainingWidth -= sumOfSpecifiedCellWidths;

                if (hasTableWidth && tableLayoutFixed) {
                    // Distribute available spaces equally among cells with
                    // "table-layout: fixed"
                    LayoutUnit newCellWidth = LayoutUnit(
                        remainingWidth.toDouble() / cellsWithAutoWidths.size());
                    for (auto& c : cellsWithAutoWidths) {
                        ColSizeStruct& col = *c;

                        if ((sumOfSpecifiedCellWidths + LayoutUnit::epsilon() >=
                             availableWidth) &&
                            tableLayoutFixed) {
                            col.cellWidth = 0;
                        } else {
                            col.cellWidth = newCellWidth;
                        }
                    }
                } else {
                    // Adjust cell width in proportion to its preferred
                    // width
                    LayoutUnit newEqualCellWidth = LayoutUnit(
                        remainingWidth.toDouble() / cellsWithAutoWidths.size());

                    LayoutUnit sumOfAutoCellWidths = 0;
                    for (auto& c : cellsWithAutoWidths) {
                        ColSizeStruct& col = *c;
                        LayoutUnit newCellWidth = 0;
                        if (col.isEmptyCell()) {
                            newCellWidth = newEqualCellWidth;
                        } else {
                            newCellWidth = LayoutUnit(
                                col.maxCellWidth.toDouble() /
                                sumOfAutoCellPreferredWidths.toDouble() *
                                remainingWidth.toDouble());
                        }

                        col.cellWidth =
                            std::max(col.minCellWidth, newCellWidth);
                        if (!hasTableWidth && tableLayoutFixed) {
                            col.cellWidth =
                                std::min(col.cellWidth, col.maxCellWidth);
                        }
                        sumOfAutoCellWidths += col.cellWidth;
                    }

                    // After adjusting the widths of cells with
                    // "width: auto", the sum of each col width could be
                    // greater than the specified table width. In this case,
                    // the size of cells are adjusted once more.
                    if (sumOfAutoCellWidths > remainingWidth) {
                        remainingWidth = tableContentWidth;
                        remainingWidth -= sumOfAutoCellWidths;
                        remainingWidth -=
                            (borderSpacing * m_columnWidths.size()) +
                            borderSpacing;

                        LayoutUnit sumOfPercentageWidth = 0;
                        calCellWidthsWithPercentageWidths(
                            remainingWidth, columnsMayNeedToAdjustWidths,
                            &sumOfPercentageWidth);
                    }
                }
            }
        }
    } else {
        // 1. A cell width cannot be smaller than the min width of the
        // cell
        LayoutUnit tableWidthByAddingColWidths = sumOfColWidths;
        tableWidthByAddingColWidths += borderWidth() + paddingWidth();
        tableWidthByAddingColWidths +=
            (borderSpacing * m_columnWidths.size()) + borderSpacing;

        if (cellsWithAutoWidths.empty()) {
            // Decrease the width of each cell in proportion to their
            // preferred widths if the parent width is smaller than
            // the sum of specified width.
            //
            // NOTE: We do not increase the width of a cell to its preferred
            // width if the parent width is larger than the sum of specified
            // width.
            if ((0 < tableWidth) &&
                (tableWidth < tableWidthByAddingColWidths)) {
                // Decrease the cell widths
                LayoutUnit remainingWidth = availableWidth;

                for (auto& c : columnsAdjustedToMinWidths) {
                    ColSizeStruct& col = *c;
                    remainingWidth -= col.cellWidth;
                    sumOfColWidths -= col.cellWidth;
                }

                for (auto& c : columnsMayNeedToAdjustWidths) {
                    ColSizeStruct& col = *c;
                    STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());
                    LayoutUnit newCellWidth = LayoutUnit(
                        col.cellWidth.toDouble() / sumOfColWidths.toDouble() *
                        remainingWidth.toDouble());

                    col.cellWidth = std::max(col.minCellWidth, newCellWidth);
                }
            }
        } else {
            // Distribute available spaces to cells with "width: auto" in
            // proportion to the cell's preferred width.
            // The cell width can either be increased or decreased depending
            // on the width of table and min width of a cell.

            bool reducedToMinWidth = false;
            unsigned numOfReducedToMinWidths = 0;
            LayoutUnit sumOfAutoCellMinWidths = 0;
            bool secondRunOrMore = false;

            // We repeatedly calculate each table cell width.
            // In the first run, we calculate initial cell width. If the cell
            // width is less than min cell width, it is adjusted to min cell
            // width. If any of the cell is adjusted to min cell width, we
            // need to repeat the width calculation again, because width
            // calculation involves the total preferred width, but if the cell
            // is adjusted to min cell width, its width is excluded from the
            // total preferred width.
            do {
                reducedToMinWidth = false;

                LayoutUnit remainingWidth = tableContentWidth;
                remainingWidth -=
                    (borderSpacing * m_columnWidths.size()) + borderSpacing;
                remainingWidth -= sumOfAdjustedSpecifiedCellWidths;
                remainingWidth -= sumOfAutoCellMinWidths;

                LayoutUnit widthToAdjust = sumOfAutoCellPreferredWidths;
                for (auto& c : cellsWithAutoWidths) {
                    STARFISH_ASSERT(c->id < m_columnWidths.size());
                    ColSizeStruct& col = m_columnWidths[c->id];

                    if (secondRunOrMore &&
                        (col.cellWidth == col.minCellWidth)) {
                        continue;
                    }

                    col.cellWidth =
                        LayoutUnit(col.maxCellWidth.toDouble() /
                                   sumOfAutoCellPreferredWidths.toDouble() *
                                   remainingWidth.toDouble());

                    if (col.cellWidth < col.minCellWidth) {
                        col.cellWidth = col.minCellWidth;
                        columnsAdjustedToMinWidths.push_back(&col);
                        numOfReducedToMinWidths++;
                        sumOfAutoCellMinWidths += col.cellWidth;
                        widthToAdjust -= col.maxCellWidth;
                        reducedToMinWidth = true;
                    } else {
                        col.cellWidth =
                            std::min(col.cellWidth, col.maxCellWidth);
                    }
                }

                sumOfAutoCellPreferredWidths = widthToAdjust;
                secondRunOrMore = true;
            } while (reducedToMinWidth);

            // We further reduce column widths if all columns with
            // "width: auto" are reduced to their min preferred widths and
            // the columns other than "width: auto" still have rooms to reduce.
            if ((numOfReducedToMinWidths == cellsWithAutoWidths.size()) ||
                (columnsMayNeedToAdjustWidths.size() > 0)) {
                LayoutUnit remainingWidth = tableContentWidth;
                remainingWidth -=
                    (borderSpacing * m_columnWidths.size()) + borderSpacing;

                // Cal widths specified in percentage
                LayoutUnit sumOfPercentageWidth = 0;
                calCellWidthsWithPercentageWidths(remainingWidth,
                                                  columnsMayNeedToAdjustWidths,
                                                  &sumOfPercentageWidth);

                remainingWidth -= sumOfAutoCellMinWidths;
                remainingWidth -= sumOfPercentageWidth;

                // cal widths specified in pixels
                LayoutUnit sumOfFixedWidth = 0;
                for (auto& c : columnsMayNeedToAdjustWidths) {
                    ColSizeStruct& col = *c;

                    if (col.hasSpecifiedWidth()) {
                        LayoutUnit newCellWidth(
                            remainingWidth.toDouble() *
                            (col.cellWidth.toDouble() /
                             sumOfAdjustedSpecifiedCellWidths.toDouble()));

                        if (newCellWidth < col.cellWidth) {
                            // Cells with specified width can only reduce its
                            // width if there is no room. It cannot grow larger
                            // than its specified width
                            col.cellWidth =
                                std::max(col.minCellWidth, newCellWidth);
                        }
                        sumOfFixedWidth += col.cellWidth;
                    }
                }

                // Adjust widths specified in percentage
                // Reduce cell widths specified in percentage if all other
                // cells are reduced to min width, and there are rooms in the
                // cells specified in percentage.
                if (remainingWidth - sumOfFixedWidth < 0) {
                    LayoutUnit remainingWidthForPercentageWidth =
                        remainingWidth - sumOfFixedWidth + sumOfPercentageWidth;

                    double sumOfPercentage = 0;
                    for (auto& c : columnsMayNeedToAdjustWidths) {
                        ColSizeStruct& col = *c;

                        if (col.hasPercentageWidth()) {
                            if (col.cellWidth > col.minCellWidth) {
                                sumOfPercentage += col.maxPercentageWidth;
                            } else {
                                remainingWidthForPercentageWidth +=
                                    col.cellWidth;
                            }
                        }
                    }

                    for (auto& c : columnsMayNeedToAdjustWidths) {
                        ColSizeStruct& col = *c;

                        if (col.hasPercentageWidth() &&
                            (col.cellWidth > col.minCellWidth)) {
                            LayoutUnit newCellWidth =
                                remainingWidthForPercentageWidth.toDouble() *
                                col.maxPercentageWidth / sumOfPercentage;
                            col.cellWidth =
                                std::max(col.minCellWidth, newCellWidth);
                        }
                    }
                }
            }
        }
    }
}

template <typename Func>
void FrameTableBox::forEachRowStruct(Func filter)
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            FrameTableSectionBox* section = c->asFrameTableSectionBox();

            size_t rowId = 0;
            for (auto& row : section->grid()) {
                filter(&row, rowId);
                rowId++;
            }
        }
    }
}

bool FrameTableBox::resetColspanIfPossible()
{
    bool colspanUpdated = false;
    for (size_t i = 0; i < m_columnWidths.size(); i++) {
        if (m_columnWidths[i].isNullCell) {
            forEachRowStruct(
                [i, &colspanUpdated](RowStruct* rowStruct, size_t _rowId) {
                    FrameTableCellBox* cell =
                        rowStruct->physicalCellAtLogicalColumn(i);
                    cell->resetColspanForLayout();
                    colspanUpdated = true;
                });
        }
    }

    return colspanUpdated;
}

// All input parameters are used as out parameters
void FrameTableBox::setCandidateCellWidthsAndReturnCellInfo(
    LayoutContext& ctx, LayoutUnit remainingWidth, bool hasTableWidth,
    bool tableLayoutFixed, LayoutUnit* sumOfAutoCellPreferredWidths,
    LayoutUnit* sumOfAdjustedSpecifiedCellWidths,
    std::vector<ColSizeStruct*>* columnsAdjustedToMinWidths,
    std::vector<ColSizeStruct*>* columnsMayNeedToAdjustWidths,
    LayoutUnit* sumOfColWidths)
{
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        style()->horizontalBorderSpacing().specifiedValue(unused, this);

    for (auto& col : m_columnWidths) {
        STARFISH_ASSERT(col.id < m_columnWidths.size());
        FrameTableCellBox* cell = cellInTheFirstRowAt(col.id);

        if (!cell) {
            continue;
        }

        if (cell->updatedColspan() > 1) {
            if (col.hasSpecifiedWidth()) {
                col.cellWidth = col.maxSpecifiedWidth;
            } else {
                col.cellWidth = col.maxCellWidth;
            }

            *sumOfColWidths += col.cellWidth;
            if (isCellWidthAuto(col.id)) {
                *sumOfAutoCellPreferredWidths += col.cellWidth;
                if (m_cellsInTheFirstRow[col.id] != cell) {
                    *sumOfAutoCellPreferredWidths += borderSpacing;
                }
            }
            continue;
        }

        if (isCellWidthAuto(col.id)) {
            col.cellWidth = col.maxCellWidth;
            *sumOfAutoCellPreferredWidths += col.cellWidth;
        } else {
            LayoutUnit specifiedWidth = 0;
            FrameTableCellBox* cellBox =
                cellFromFirstRowOrColGroup(tableLayoutFixed, col.id);
            Length width = cellBox->style()->width();

            if (width.isDefinite(false)) {
                LayoutUnit unused;
                specifiedWidth = width.specifiedValue(unused, this);
                specifiedWidth +=
                    cellBox->borderWidth() + cellBox->paddingWidth();
                col.cellWidth = specifiedWidth;
            } else if (width.isPercent()) {
                col.cellWidth = remainingWidth * width.percent();
            } else {
                col.cellWidth = col.maxSpecifiedWidth;
            }

            // A cell width cannot be smaller than the min width of the cell
            if (!tableLayoutFixed && col.cellWidth < col.minCellWidth) {
                col.cellWidth = col.minCellWidth;
                columnsAdjustedToMinWidths->push_back(&col);
            } else {
                columnsMayNeedToAdjustWidths->push_back(&col);
            }

            if (col.hasSpecifiedWidth() ||
                (hasTableWidth && col.hasPercentageWidth())) {
                *sumOfAdjustedSpecifiedCellWidths += col.cellWidth;
            }
        }

        *sumOfColWidths += col.cellWidth;
    }
}

void FrameTableBox::collectColBoxes()
{
    m_colBoxes.clear();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableColBox()) {
            FrameTableColBox* colBox = c->asFrameTableColBox();

            if (colBox->firstChild()) {
                for (Frame* b = colBox->firstChild(); b; b = b->next()) {
                    FrameTableColBox* child = b->asFrameTableColBox();
                    if (child->style()->display() ==
                        DisplayValue::TableColumnDisplayValue) {
                        m_colBoxes.push_back(child);
                    }
                }
            } else {
                m_colBoxes.push_back(colBox);
            }
        }
    }
}

bool FrameTableBox::hasColBox(size_t i)
{
    return i < m_colBoxes.size();
}

FrameTableCellBox* FrameTableBox::cellFromFirstRowOrColGroup(
    bool tableLayoutFixed, size_t i)
{
    if (tableLayoutFixed && hasColBox(i)) {
        return m_colBoxes[i];
    } else {
        return cellInTheFirstRowAt(i);
    }
}

void FrameTableBox::calCellWidthsWithPercentageWidths(
    LayoutUnit remainingWidth,
    std::vector<ColSizeStruct*> columnsMayNeedToAdjustWidths,
    LayoutUnit* sumOfPercentageWidth)
{
    bool secondRunOrMore = false;
    *sumOfPercentageWidth = 0;
    bool reducedToMinWidth = false;
    do {
        reducedToMinWidth = false;
        LayoutUnit remainingWidthForPercentageWidth =
            remainingWidth - *sumOfPercentageWidth;
        double sumOfPercentageSoFar = 0;

        for (auto& c : columnsMayNeedToAdjustWidths) {
            ColSizeStruct& col = *c;

            if (secondRunOrMore && (col.cellWidth == col.minCellWidth)) {
                continue;
            }

            if (col.hasPercentageWidth()) {
                if (sumOfPercentageSoFar < 1) {
                    LayoutUnit newCellWidth =
                        remainingWidthForPercentageWidth.toDouble() *
                        col.maxPercentageWidth;
                    col.cellWidth = newCellWidth;
                    sumOfPercentageSoFar += col.maxPercentageWidth;
                } else {
                    col.cellWidth = col.minCellWidth;
                }

                if (col.cellWidth <= col.minCellWidth) {
                    col.cellWidth = col.minCellWidth;
                    reducedToMinWidth = true;
                }

                *sumOfPercentageWidth += col.cellWidth;
            }
        }
        secondRunOrMore = true;
    } while (reducedToMinWidth);
}

void FrameTableBox::calCellWidthsWithColspans()
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            FrameTableSectionBox* section = c->asFrameTableSectionBox();
            section->calCellWidthsWithColspans();
        }
    }

    LayoutUnit unused;
    LayoutUnit borderSpacing =
        style()->horizontalBorderSpacing().specifiedValue(unused, this);

    forEachRowStruct(
        [this, borderSpacing](RowStruct* rowStruct, size_t _rowId) {
            for (auto& cellStruct : rowStruct->cells()) {
                FrameTableCellBox* cell = cellStruct.cell();

                if (cell->updatedColspan() > 1) {
                    LayoutUnit sumOfCellWidth = 0;
                    size_t colId = cell->absoluteColumnIndex();
                    STARFISH_ASSERT(colId < m_columnWidths.size());

                    for (size_t i = colId; i < colId + cell->updatedColspan();
                         i++) {
                        STARFISH_ASSERT(i < m_columnWidths.size());
                        sumOfCellWidth += m_columnWidths[i].cellWidth;
                        if (i < colId + cell->updatedColspan() - 1) {
                            sumOfCellWidth += borderSpacing;
                        }
                    }

                    if (m_columnWidths[colId].hasSpecifiedWidth()) {
                        LayoutUnit cellWidth =
                            std::max(sumOfCellWidth,
                                     m_columnWidths[colId].maxSpecifiedWidth);
                        cell->setWidth(cellWidth);
                    } else {
                        cell->setWidth(sumOfCellWidth);
                    }
                }
            }
        });
}

void FrameTableBox::layoutWidth(LayoutContext& ctx)
{
    // The width of the caption is limited by the max width of the
    // FrameTableSection. Hence, captions can only be placed after calculating
    // the width of the table, which has already been done by calContentWidth()
    LayoutUnit minCaptionWidthSoFar = 0;
    LayoutUnit maxRowWidthSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->layoutWidth(ctx);
            maxRowWidthSoFar =
                std::max(maxRowWidthSoFar, c->asFrameBox()->width());
        } else if (c->isFrameTableCaptionBox()) {
            c->asFrameTableCaptionBox()->layoutWidth(ctx);
            minCaptionWidthSoFar = std::max(
                minCaptionWidthSoFar, c->asFrameTableCaptionBox()->width());
            c->asFrameTableCaptionBox()->setX(
                c->asFrameTableCaptionBox()->marginLeft());
            if (c->style()->width().isDefinite(false)) {
                maxRowWidthSoFar =
                    std::max(maxRowWidthSoFar, c->asFrameBox()->width());
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else if (c->isFrameTableColBox()) {
            // The FrameTableColBox must not be laid out.
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // The width of all table sections should be the same,
    // so getting the max width should be the same as the width of
    // any table sections.
    LayoutUnit tableContentWidth = maxRowWidthSoFar;
    tableContentWidth = std::max(tableContentWidth, minCaptionWidthSoFar);

    Length width = style()->width();
    if (!width.isAuto()) {
        LayoutUnit tableWidth;
        if (width.isDefinite(false)) {
            LayoutUnit unused;
            tableWidth = width.specifiedValue(unused, this);
            if (!isAnonymous() && !node()->isHTMLTableElement()) {
                tableWidth += borderWidth() + paddingWidth();
            }
        } else if (width.isPercent()) {
            tableWidth = width.percentValue(ctx.parentContentWidth(this));
        } else if (width.isCalc()) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }

        tableContentWidth = std::max(
            tableContentWidth, tableWidth - borderWidth() - paddingWidth());
    }

    setWidth(tableContentWidth + paddingWidth() + borderWidth());
}

void FrameTableBox::layoutHeight(LayoutContext& ctx)
{
    // Table is placed in the following order:
    // 1. Captions that have property "caption-side: top"
    //    If there are multiple captions, place them in document order
    // 2. Table sections in document order
    // 3. Captions that have property "caption-side: bottom"
    //    If there are multiple captions, place them in document order

    LayoutUnit ySoFar = 0;

    // 1. place captions with caption-side: top
    Frame* child = firstChild();
    while (child) {
        if (child->isFrameTableCaptionBox()) {
            FrameTableCaptionBox* caption = child->asFrameTableCaptionBox();
            if (caption->style()->captionSide() ==
                CaptionSideValue::TopCaptionSideValue) {
                LayoutUnit oldWidth = caption->width();
                caption->setWidth(width() - caption->marginWidth());
                if (caption->width() != oldWidth) {
                    caption->markNeedsLayout();
                }
                caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
                caption->setY(ySoFar + caption->marginTop());
                ySoFar += caption->height() + caption->marginHeight();
            }
        }

        child = child->next();
    }

    // 2. place table sections
    // The table can have multiple section elements with
    // 'display:table-header-group. In this case, only the first
    // table-header-group is rendered as the table-header-group
    // Other table-header-groups are treated as 'display:table-row-group'
    m_tableRect.setX(0);
    m_tableRect.setY(ySoFar);
    ySoFar += borderTop();
    ySoFar += paddingTop();
    LayoutUnit xPosOfSection = borderLeft() + paddingLeft();
    FrameTableSectionBox* thead = this->thead();
    FrameTableSectionBox* tfoot = this->tfoot();

    // 2-1. place the first table header section
    if (thead) {
        thead->asFrameBox()->setX(xPosOfSection);
        thead->asFrameTableSectionBox()->layoutHeight(ctx);
        thead->asFrameBox()->setY(ySoFar);
        ySoFar += thead->asFrameBox()->height();
    }

    // 2-2. place table-row-group and the rest
    // "table-header/footer-group" sections
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox() && (c != thead) && (c != tfoot)) {
            c->asFrameBox()->setX(xPosOfSection);
            c->asFrameTableSectionBox()->layoutHeight(ctx);
            c->asFrameBox()->setY(ySoFar);
            ySoFar += c->asFrameBox()->height();
        }
    }

    // 2-3. place the first table footer section
    // Similar logic as the table-header-group applies to table-footer-group.
    if (tfoot) {
        tfoot->asFrameBox()->setX(xPosOfSection);
        tfoot->asFrameTableSectionBox()->layoutHeight(ctx);
        tfoot->asFrameBox()->setY(ySoFar);
        ySoFar += tfoot->asFrameBox()->height();
    }

    ySoFar += paddingBottom();
    ySoFar += borderBottom();
    m_tableRect.setWidth(width());

    LayoutUnit specifiedHeight = 0;
    bool hasTableHeight = false;
    Length height = style()->height();
    if (!height.isAuto()) {
        hasTableHeight = true;

        if (height.isDefinite(false)) {
            LayoutUnit unused;
            specifiedHeight = height.specifiedValue(unused, this);
            specifiedHeight += borderHeight() + paddingHeight();
        } else if (height.isPercent()) {
            if (ctx.parentHasFixedHeight(this)) {
                specifiedHeight =
                    height.percentValue(ctx.parentFixedHeight(this));
            }
        } else if (height.isCalc()) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
    }

    LayoutUnit sectionHeight = ySoFar - m_tableRect.y();
    if (sectionHeight < specifiedHeight) {
        // table height is specified, and it is greater than the sum of
        // all row heights. In this case, distribute remaining spaces equally
        // among rows.
        LayoutUnit extraSpace = specifiedHeight - sectionHeight;
        size_t rowCount = numOfRowsInTheTable();
        for (Frame* c = firstChild(); c; c = c->next()) {
            if (c->isFrameTableSectionBox()) {
                c->asFrameTableSectionBox()->increaseRowHeightBy(
                    LayoutUnit(extraSpace.toDouble() / rowCount));
            }
        }

        ySoFar += extraSpace;
        sectionHeight = specifiedHeight;
    }

    m_tableRect.setHeight(sectionHeight);

    // 3. place captions with caption-side: bottom
    child = firstChild();
    while (child) {
        if (child->isFrameTableCaptionBox()) {
            FrameTableCaptionBox* caption = child->asFrameTableCaptionBox();
            if (caption->style()->captionSide() ==
                CaptionSideValue::BottomCaptionSideValue) {
                LayoutUnit oldWidth = caption->width();
                caption->setWidth(width() - caption->marginWidth());
                if (oldWidth != caption->width()) {
                    caption->markNeedsLayout();
                }
                caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
                caption->setY(ySoFar + caption->marginTop());
                ySoFar += caption->height() + caption->marginHeight();
            }
        }

        child = child->next();
    }
    setHeight(ySoFar);

    // 4. Apply vertical-align to each cell
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->applyVerticalAlign(ctx);
        }
    }
}

void FrameTableBox::collectColumnWidths(
    GCAtomicVector<ColSizeStruct>& columnWidthsSoFar,
    GCAtomicVector<ColSizeStruct>& columnWidths)
{
    if (columnWidthsSoFar.empty()) {
        for (auto& col : columnWidths) {
            // Add empty ColSizeStruct as place holders
            while (columnWidthsSoFar.size() < col.id) {
                columnWidthsSoFar.push_back(ColSizeStruct());
            }
            columnWidthsSoFar.push_back(col);
        }

    } else {
        for (unsigned i = 0; i < columnWidths.size(); i++) {
            ColSizeStruct& col = columnWidths[i];

            // Add empty ColSizeStruct as place holders
            while (columnWidthsSoFar.size() <= col.id) {
                columnWidthsSoFar.push_back(ColSizeStruct());
            }

            ColSizeStruct& colSoFar = columnWidthsSoFar[i];
            colSoFar.maxCellWidth =
                std::max(colSoFar.maxCellWidth, col.maxCellWidth);
            colSoFar.minCellWidth =
                std::max(colSoFar.minCellWidth, col.minCellWidth);
            colSoFar.cellWidth = colSoFar.maxCellWidth;

            if (!col.isNullCell) {
                colSoFar.isNullCell = false;
            }
        }
    }
}

bool FrameTableBox::isCellWidthAuto(unsigned i)
{
    // Determining whether a cell's width is "auto" or "specified" depends on
    // whether we are performing "auto" or "fixed" layout. To determine whether
    // we are doing "auto" or "fixed" layout, we need to check whether the
    // table has specified width or not.
    //
    // If the table's width is not given:
    // * If a column has a specified width anywhere in the row, the column will
    //   have its width the maximum specified width of all cells in the column.
    // * Otherwise, the width becomes auto.
    //
    // If the table's width is given:
    // * If the cell in the first row has a specified width, the with becomes
    //   the width of the column.
    // * Otherwise, the width becomes auto. In this case, all other specified
    //   width anywhere in the column except the first row are ignored.

    if (i >= m_cellsInTheFirstRow.size()) {
        return true;
    }

    if (style()->width().isAuto()) {
        // Table does not have a width

        // matching cell is empty because of colspan of previous cell
        if (m_cellsInTheFirstRow[i] == nullptr) {
            return !m_columnWidths[i].hasSpecifiedWidth();
        }

        if (i < m_cellsInTheFirstRow.size()) {
            STARFISH_ASSERT(i < m_columnWidths.size());
            if (m_cellsInTheFirstRow[i]->style()->width().isAuto() &&
                !(m_columnWidths[i].hasSpecifiedWidth())) {
                return true;
            } else {
                return false;
            }
        }
        return true;
    } else {
        // Table has a specified width

        if (i < m_colBoxes.size() &&
            !m_colBoxes[i]->style()->width().isAuto()) {
            return false;
        }

        // matching cell is empty because of colspan of previous cell
        // TODO: Do we need to consider specified widths in the rows in the
        // same column? Check with Blink
        if (m_cellsInTheFirstRow[i] == nullptr) {
            return true;
        }

        if (i < m_cellsInTheFirstRow.size()) {
            return m_cellsInTheFirstRow[i]->style()->width().isAuto();
        }
        return true;
    }
}

size_t FrameTableBox::numOfRowsInTheTable()
{
    size_t rowCount = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            rowCount += c->asFrameTableSectionBox()->grid().size();
        }
    }

    return rowCount;
}

// Table draws the border around the TableFrameSections
void FrameTableBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Fill in the table with background color
    paintBoxShadows(canvas);
    paintBackground(canvas, this, nullptr);
    paintInsetBoxShadows(canvas);
    paintBorders(canvas, m_tableRect);
}

Unit::Rect FrameTableBox::makeRect(BoxValue box)
{
    LayoutRect tableRect = asFrameTableBox()->tableRect();
    float x, y, w, h;

    switch (box) {
    case BoxValue::BorderBoxBoxValue:
        x = tableRect.x();
        y = tableRect.y();
        w = tableRect.width();
        h = tableRect.height();
        break;
    case BoxValue::PaddingBoxBoxValue:
        x = tableRect.x() + borderLeft();
        y = tableRect.y() + borderTop();
        w = tableRect.width() - borderWidth();
        h = tableRect.height() - borderHeight();
        break;
    case BoxValue::ContentBoxBoxValue:
        x = tableRect.x() + borderLeft() + paddingLeft();
        y = tableRect.y() + borderTop() + paddingTop();
        w = tableRect.width() - borderWidth() - paddingWidth();
        h = tableRect.height() - borderHeight() - paddingHeight();
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    return Unit::Rect(x, y, w, h);
}

// The layout result of the table may be different from the document order.
// So we have to consider the visual order.
// And if there is no row in the section, the section is an empty section.
FrameTableSectionBox* FrameTableBox::firstNonEmptySectionBoxInVisualOrder()
{
    FrameTableSectionBox* thead = this->thead();
    if (thead && thead->grid().size()) {
        return thead;
    }

    FrameTableSectionBox* tfoot = this->tfoot();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c != tfoot && c->isFrameTableSectionBox() &&
            c->asFrameTableSectionBox()->grid().size()) {
            return c->asFrameTableSectionBox();
        }
    }

    if (tfoot && tfoot->grid().size()) {
        return tfoot;
    }
    return nullptr;
}

FrameTableSectionBox* FrameTableBox::firstSectionBoxInVisualOrder()
{
    FrameTableSectionBox* thead = this->thead();
    if (thead) {
        return thead;
    }

    FrameTableSectionBox* tfoot = this->tfoot();

    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c != tfoot && c->isFrameTableSectionBox()) {
            return c->asFrameTableSectionBox();
        }
    }

    return tfoot;
}

// -----------------------------------------------------------------------------
// The baseline of an inline-table is needed to calculate the vertical alignment
// of inline elements. The spec
// https://www.w3.org/TR/CSS21/visudet.html#propdef-vertical-align
// says "The baseline of an 'inline-table' is the baseline of the first row of
// the table." But the spec omits details, and there are many cases to consider
// when calculating the baseline. For undefined behaviours, we try to mimic
// Blink's behaviour.
// -----------------------------------------------------------------------------
// NOTE : The implementation is in progress.
// The baseline is calculated as follows.
// * Use the tables wrapperbox height if there are only empty sections
// * Use the vertical-align value of the cell that has the tallest line box
//   in the first row
// * Use the tallest linebox height if the first row has other
//   "vertical-align" values.
// * Use Y position of the center of the cell if the first row has only
//   empty cells
// * Use Y position of the first row if the first row is empty

LayoutUnit FrameTableBox::calBaseline(LayoutContext& ctx)
{
    STARFISH_ASSERT(style()->display() ==
                    DisplayValue::InlineTableDisplayValue);

    FrameTableSectionBox* firstSection = firstNonEmptySectionBoxInVisualOrder();
    if (!firstSection) {
        return height();
    }

    RowStruct& firstRS = firstSection->grid()[0];
    LineBox* tallestLB = nullptr;
    bool isBaseLine = false;
    for (size_t i = 0; i < firstRS.cells().size(); ++i) {
        FrameTableCellBox* c = firstRS.cells()[i].cell();
        auto it = ctx.tempFirstLineAscender(c);

        if (it.hasValue() && (!tallestLB || (tallestLB->height() <
                                             it.getValue().first->height()))) {
            tallestLB = it.getValue().first;
            isBaseLine = c->style()->verticalAlign() ==
                         VerticalAlignValue::BaselineVAlignValue;
        }
    }

    if (tallestLB) {
        if (isBaseLine) {
            return firstRS.tableRow()->absolutePoint(this).y() +
                   firstRS.tableRow()->baseline();
        }
        return tallestLB->absolutePoint(this).y() + tallestLB->height();
    } else if (firstRS.cells().size()) {
        // Empty cell
        return firstRS.cells()[0].cell()->absolutePoint(this).y() +
               (firstRS.cells()[0].cell()->height().toDouble() / 2);
    }
    // Empty first row
    return firstRS.tableRow()->absolutePoint(this).y();
}

FrameTableColBox* FrameTableBox::columnAtAbsoluteColumnIndex(unsigned index)
{
    Frame* child = firstChild();
    unsigned l = 0, r = 0;
    while (child) {
        if (child->isFrameTableColBox()) {
            FrameTableColBox* colGroup = child->asFrameTableColBox();
            if (colGroup->firstChild() == nullptr) {
                r = l + colGroup->span();
                if (l <= index && index < r) {
                    return colGroup;
                }
                l = r;
            } else {
                for (Frame* p = colGroup->firstChild(); p; p = p->next()) {
                    STARFISH_ASSERT(p->isFrameTableColBox());
                    r = l + p->asFrameTableColBox()->span();
                    if (l <= index && index < r) {
                        return p->asFrameTableColBox();
                    }
                    l = r;
                }
            }
        }

        child = child->next();
    }

    return nullptr;
}
}
