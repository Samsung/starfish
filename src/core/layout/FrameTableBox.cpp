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
#include "core/dom/HTMLTableElement.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCaptionBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableColBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/style/CSSParser.h"

namespace StarFish {

FrameTableBox::FrameTableBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_tableRect(0, 0, 0, 0)
    , m_thead(nullptr)
    , m_tfoot(nullptr)
    , m_candidateWidth(0)
{
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
        for (size_t i = id - 1; i >= 0; i--) {
            if (m_cellsInTheFirstRow[i]) {
                return m_cellsInTheFirstRow[i];
            }
        }
        // empty cells in the first row
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

void FrameTableBox::calCellWidth(LayoutContext& ctx)
{
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
            for (unsigned i = 1; i < cell->colspan(); i++) {
                m_cellsInTheFirstRow.push_back(nullptr);
            }
        }
    }

    // 1. The spec says to look at the first row only to get the width for each
    // cell. But, there are cases where the following rows contains more cells
    // than the first row. In this case, the spec leaves what to do to
    // implementors. We try to obtain the width of those cells similar to
    // "table-layout: auto", i.e., we perform the following
    // to get:
    //  * min/max cell widths of the table if "width: auto"
    m_columnWidths.clear();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->calCellWidth(ctx);
            collectColumnWidths(m_columnWidths,
                                c->asFrameTableSectionBox()->columnWidths());
        }
    }

    // We iterate columns and calculate the following:
    //   * sum of all cells with specified widths.
    //   * calculate initial column width for cells with specified width
    //   * min/max table width
    //     - TODO: Need to consider caption widths too
    //   * collect auto and specified width cells for later calculation
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(style()->horizontalBorderSpacing().fixed());
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

    // Followed the algorithm from
    // https://www.w3.org/TR/2016/WD-css-tables-3-20161025/#width-distribution
    // except when table width is given, we simple use the width (following how
    // blink works)
    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    LayoutUnit tableWidth;
    bool hasTableWidth = false;
    if (style()->width().isAuto()) {
        tableWidth = std::max(minTableWidth, parentContentWidth);
        tableWidth -= marginWidth();
    } else {
        // The width of table is explicitly given
        hasTableWidth = true;
        if (style()->width().isFixed()) {
            tableWidth = LayoutUnit::fromPixel(style()->width().fixed());

            // For CSS table, width is the table content width EXCLUDING
            // border and padding.
            // For HTML table, width is the table width INCLUDNIG border and
            // padding.
            // For our implementation, tableWidth refers to CSS table convention
            if (!isAnonymous() && node()->isHTMLTableElement()) {
                tableWidth -= borderWidth() + paddingWidth();
            }
        } else if (style()->width().isPercent()) {
            tableWidth =
                parentContentWidth.toInt() * style()->width().percent();
            tableWidth -= borderWidth() + paddingWidth();
        } else {
            // should not be here
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // We support <table width="xx"> unofficially, as it is used to run
    // w3c test cases, and real-world web sites.
    // width attribute has the highest priority when defining the width of
    // a table.
    //
    // NOTE: width="0" is handled differently by Blink and Firefox.
    // When width="0" is given, Blink tries to set table width to 0.
    // Firefox ignores width="0". We ignore width="0"
    if (!isAnonymous() && node()->isHTMLTableElement()) {
        LayoutUnit widthAttribute = widthFromAttribute(parentContentWidth);
        if (widthAttribute > 0) {
            hasTableWidth = true;
            tableWidth = widthAttribute;
            tableWidth -= borderWidth() + paddingWidth();
        }
    }

    if (hasTableWidth) {
        m_candidateWidth = tableWidth;
    }

    // 3. If a width of the table is given, we either increase or decrease the
    // cell widths to fit them into the width of table.
    if (hasTableWidth) {
        LayoutUnit availableWidth = tableWidth;
        availableWidth -=
            (borderSpacing * m_columnWidths.size()) + borderSpacing;
        LayoutUnit sumOfSpecifiedCellWidths = 0;

        double sumOfWidthPercentage = 0;
        for (auto& c : cellsWithSpecifiedWidths) {
            ColSizeStruct& col = *c;
            STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());
            FrameTableCellBox* cell = cellInTheFirstRowAt(col.id);

            LayoutUnit specifiedWidth = 0;
            if (cell->style()->width().isFixed()) {
                specifiedWidth =
                    LayoutUnit::fromPixel(cell->style()->width().fixed());
                specifiedWidth += cell->borderWidth() + cell->paddingWidth();
                col.cellWidth = specifiedWidth;
            } else if (cell->style()->width().isPercent()) {
                specifiedWidth =
                    availableWidth * cell->style()->width().percent();
                col.cellWidth = specifiedWidth;
                sumOfWidthPercentage += cell->style()->width().percent();
            }
            sumOfSpecifiedCellWidths += specifiedWidth;
        }

        if (sumOfWidthPercentage > 1) {
            // Sum of all widths specified in percentage is greater than 100%.
            // In this case, we reduce each cell width in proportion to its
            // width over the sum of all widths.
            sumOfSpecifiedCellWidths = 0;
            for (auto& c : cellsWithSpecifiedWidths) {
                ColSizeStruct& col = *c;
                STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());
                FrameTableCellBox* cell = cellInTheFirstRowAt(col.id);

                if (cell->style()->width().isPercent()) {
                    LayoutUnit specifiedWidth =
                        availableWidth * (cell->style()->width().percent() /
                                          sumOfWidthPercentage);
                    col.cellWidth = specifiedWidth;
                }
                sumOfSpecifiedCellWidths += col.cellWidth;
            }
        }

        if (sumOfSpecifiedCellWidths + LayoutUnit::epsilon() >=
            availableWidth) {
            // Set the widths of all cells with "width: auto" to 0, if any
            for (auto& c : cellsWithAutoWidths) {
                ColSizeStruct& col = *c;
                col.cellWidth = 0;
            }
        } else {
            LayoutUnit remainingWidth = tableWidth - sumOfSpecifiedCellWidths;
            remainingWidth -=
                (borderSpacing * m_columnWidths.size()) + borderSpacing;

            // Specified table width is bigger than the sum of all specified
            // cell widths, if
            // * the sum of all actual specified cell widths is smaller than
            //   the specified table width OR
            // * there are cells with "width: auto" that have no widths
            //   calculated yet
            if (remainingWidth > 0) {
                // All cells have fixed width. In this case, distribute
                // available spaces among cells. The extra space for each cell
                // is proportional to the width of each cell.
                if (cellsWithAutoWidths.empty()) {
                    for (auto& col : m_columnWidths) {
                        FrameTableCellBox* cell = cellInTheFirstRowAt(col.id);

                        if (cell->style()->width().isFixed()) {
                            LayoutUnit cellWidth =
                                cell->style()->width().fixed();

                            cellWidth +=
                                cell->borderWidth() + cell->paddingWidth();

                            LayoutUnit extraCellWidth =
                                LayoutUnit(cellWidth.toDouble() /
                                           sumOfSpecifiedCellWidths.toDouble() *
                                           remainingWidth.toDouble());
                            col.cellWidth += extraCellWidth;
                        }
                    }
                } else {
                    if (style()->tableLayout() ==
                        TableLayoutValue::FixedTableLayoutValue) {
                        // Distribute available spaces equally among cells with
                        // "layout-layout: fixed"
                        LayoutUnit newCellWidth =
                            LayoutUnit(remainingWidth.toDouble() /
                                       cellsWithAutoWidths.size());
                        for (auto& c : cellsWithAutoWidths) {
                            ColSizeStruct& col = *c;
                            col.cellWidth = newCellWidth;
                        }
                    } else {
                        // Adjust cell width in proportion to its preferred
                        // width
                        LayoutUnit sumOfAutoCellPreferredWidths = 0;
                        LayoutUnit sumOfAdjustedSpecifiedCellWidths = 0;
                        std::vector<ColSizeStruct*> columnsAdjustedToMinWidths;
                        std::vector<ColSizeStruct*>
                            columnsMayNeedToAdjustWidths;
                        LayoutUnit sumOfColWidths = 0;

                        setCandidateCellWidthsAndReturnCellInfo(
                            remainingWidth, &sumOfAutoCellPreferredWidths,
                            &sumOfAdjustedSpecifiedCellWidths,
                            &columnsAdjustedToMinWidths,
                            &columnsMayNeedToAdjustWidths, &sumOfColWidths);

                        LayoutUnit newEqualCellWidth =
                            LayoutUnit(remainingWidth.toDouble() /
                                       cellsWithAutoWidths.size());
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
                        }
                    }
                }
            }
        }
    } else {
        // 1. A cell width cannot be smaller than the min width of the
        // cell
        LayoutUnit sumOfAutoCellPreferredWidths = 0;
        LayoutUnit sumOfAdjustedSpecifiedCellWidths = 0;
        std::vector<ColSizeStruct*> columnsAdjustedToMinWidths;
        std::vector<ColSizeStruct*> columnsMayNeedToAdjustWidths;
        LayoutUnit sumOfColWidths = 0;

        setCandidateCellWidthsAndReturnCellInfo(
            tableWidth - marginWidth(), &sumOfAutoCellPreferredWidths,
            &sumOfAdjustedSpecifiedCellWidths, &columnsAdjustedToMinWidths,
            &columnsMayNeedToAdjustWidths, &sumOfColWidths);

        LayoutUnit tableWidthByAddingColWidths = sumOfColWidths;
        tableWidthByAddingColWidths += borderWidth() + paddingWidth();
        tableWidthByAddingColWidths +=
            (borderSpacing * m_columnWidths.size()) + borderSpacing;

        // Distribute available spaces to cells with "width: auto" in
        // proportion to the cell's preferred width.
        // The cell width can either be increased or decreased depending
        // on the width of table and min width of a cell.

        if (cellsWithAutoWidths.size() > 0) {
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

                LayoutUnit remainingWidth = tableWidth;
                remainingWidth -= borderWidth() + paddingWidth();
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
            if (numOfReducedToMinWidths == cellsWithAutoWidths.size()) {
                LayoutUnit remainingWidth = tableWidth;
                remainingWidth -= borderWidth() + paddingWidth();
                remainingWidth -=
                    (borderSpacing * m_columnWidths.size()) + borderSpacing;

                // Cal widths specified in percentage
                secondRunOrMore = false;
                LayoutUnit sumOfPercentageWidth = 0;
                do {
                    reducedToMinWidth = false;
                    LayoutUnit remainingWidthForPercentageWidth =
                        remainingWidth - sumOfPercentageWidth;
                    double sumOfPercentageSoFar = 0;
                    for (auto& c : columnsMayNeedToAdjustWidths) {
                        ColSizeStruct& col = *c;

                        if (secondRunOrMore &&
                            (col.cellWidth == col.minCellWidth)) {
                            continue;
                        }

                        if (col.hasPercentageWidth()) {
                            if (sumOfPercentageSoFar < 1) {
                                LayoutUnit newCellWidth =
                                    remainingWidthForPercentageWidth
                                        .toDouble() *
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

                            sumOfPercentageWidth += col.cellWidth;
                        }
                    }
                    secondRunOrMore = true;
                } while (reducedToMinWidth);

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
        } else {
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
                LayoutUnit availableWidth = tableWidth;
                availableWidth -= borderWidth() + paddingWidth();
                availableWidth -=
                    (borderSpacing * m_columnWidths.size()) + borderSpacing;

                for (auto& c : columnsAdjustedToMinWidths) {
                    ColSizeStruct& col = *c;
                    availableWidth -= col.cellWidth;
                    sumOfColWidths -= col.cellWidth;
                }

                for (auto& c : columnsMayNeedToAdjustWidths) {
                    ColSizeStruct& col = *c;
                    STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());
                    LayoutUnit newCellWidth = LayoutUnit(
                        col.cellWidth.toDouble() / sumOfColWidths.toDouble() *
                        availableWidth.toDouble());

                    col.cellWidth = std::max(col.minCellWidth, newCellWidth);
                }
            }
        }
    }
}

// All input parameters are used as out parameters
void FrameTableBox::setCandidateCellWidthsAndReturnCellInfo(
    LayoutUnit remainingWidth, LayoutUnit* sumOfAutoCellPreferredWidths,
    LayoutUnit* sumOfAdjustedSpecifiedCellWidths,
    std::vector<ColSizeStruct*>* columnsAdjustedToMinWidths,
    std::vector<ColSizeStruct*>* columnsMayNeedToAdjustWidths,
    LayoutUnit* sumOfColWidths)
{
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(style()->horizontalBorderSpacing().fixed());

    for (auto& col : m_columnWidths) {
        STARFISH_ASSERT(col.id < m_columnWidths.size());
        FrameTableCellBox* cell = cellInTheFirstRowAt(col.id);

        if (!cell) {
            continue;
        }

        if (cell->colspan() > 1) {
            if (col.hasSpecifiedWidth()) {
                LayoutUnit specifiedWidth = col.maxSpecifiedWidth;
                specifiedWidth += cell->borderWidth() + cell->paddingWidth();
                col.cellWidth = specifiedWidth;
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
            col.cellWidth = col.maxSpecifiedWidth;

            if (cell->style()->width().isFixed()) {
                specifiedWidth =
                    LayoutUnit::fromPixel(cell->style()->width().fixed());
                specifiedWidth += cell->borderWidth() + cell->paddingWidth();
                col.cellWidth = specifiedWidth;
            } else if (col.hasPercentageWidth()) {
                specifiedWidth =
                    remainingWidth.toInt() * col.maxPercentageWidth;
                col.cellWidth = specifiedWidth;
            }

            // A cell width cannot be smaller than the min width of the cell
            if (col.cellWidth < col.minCellWidth) {
                col.cellWidth = col.minCellWidth;
                columnsAdjustedToMinWidths->push_back(&col);
            } else {
                columnsMayNeedToAdjustWidths->push_back(&col);
            }

            if (col.hasSpecifiedWidth()) {
                *sumOfAdjustedSpecifiedCellWidths += col.cellWidth;
            }
        }

        *sumOfColWidths += col.cellWidth;
    }
}

void FrameTableBox::calCellWidthsWithColspans()
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            FrameTableSectionBox* section = c->asFrameTableSectionBox();
            section->calCellWidthsWithColspans();
        }
    }

    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(style()->horizontalBorderSpacing().fixed());

    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            FrameTableSectionBox* section = c->asFrameTableSectionBox();

            for (auto& rowStruct : section->grid()) {
                FrameTableRowBox* row = rowStruct.tableRow();

                unsigned id = 0;
                for (auto& cellStruct : rowStruct.cells()) {
                    FrameTableCellBox* cell = cellStruct.cell();

                    if (cell->colspan() > 1) {
                        ColSizeStruct* colSize = row->colWithColspanAt(id);

                        LayoutUnit sumOfCellWidth = 0;
                        for (size_t i = id; i < id + cell->colspan(); i++) {
                            sumOfCellWidth += m_columnWidths[i].cellWidth;
                            if (i < id + cell->colspan() - 1) {
                                sumOfCellWidth += borderSpacing;
                            }
                        }

                        if (m_columnWidths[id].hasSpecifiedWidth()) {
                            colSize->cellWidth =
                                std::max(sumOfCellWidth,
                                         m_columnWidths[id].maxSpecifiedWidth);
                        } else {
                            colSize->cellWidth = sumOfCellWidth;
                        }
                    }
                    id += cell->colspan();
                }
            }
        }
    }
}

void FrameTableBox::layoutWidth(LayoutContext& ctx)
{
    // The width of the caption is limited by the max width of the
    // FrameTableSection. Hence, captions can only be placed after calculating
    // the width of the table, which has already been done by calContentWidth()
    LayoutUnit maxWidthSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->layoutWidth(ctx);
            maxWidthSoFar = std::max(maxWidthSoFar, c->asFrameBox()->width());
        } else if (c->isFrameTableCaptionBox()) {
            c->asFrameTableCaptionBox()->layout(
                ctx, Frame::LayoutWantToResolve::ResolveWidth);
            c->asFrameTableCaptionBox()->setX(
                c->asFrameTableCaptionBox()->marginLeft());
            if (c->asFrameBox()->style()->width().isFixed()) {
                maxWidthSoFar =
                    std::max(maxWidthSoFar, c->asFrameBox()->width());
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
    LayoutUnit tableWidth = maxWidthSoFar;
    if (m_candidateWidth > tableWidth) {
        tableWidth = m_candidateWidth;
    }
    setWidth(tableWidth + paddingWidth() + borderWidth());
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
    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    MarginInfo marginInfo(top, bottom, true, style()->height());
    setMarginInfo(&marginInfo);

    // 1. place captions with caption-side: top
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() ==
            CaptionSideValue::TopCaptionSideValue) {
            caption->setWidth(width() - caption->marginWidth());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar + caption->marginTop());
            ySoFar += caption->height() + caption->marginHeight();
        }
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

    // 2-1. place the first table header section
    if (m_thead) {
        m_thead->asFrameBox()->setX(xPosOfSection);
        m_thead->asFrameTableSectionBox()->layoutHeight(ctx);
        m_thead->asFrameBox()->setY(ySoFar);
        ySoFar += m_thead->asFrameBox()->height();
    }

    // 2-2. place table-row-group and the rest
    // "table-header/footer-group" sections
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox() && (c != m_thead) && (c != m_tfoot)) {
            c->asFrameBox()->setX(xPosOfSection);
            c->asFrameTableSectionBox()->layoutHeight(ctx);
            c->asFrameBox()->setY(ySoFar);
            ySoFar += c->asFrameBox()->height();
        }
    }

    // 2-3. place the first table footer section
    // Similar logic as the table-header-group applies to table-footer-group.
    if (m_tfoot) {
        m_tfoot->asFrameBox()->setX(xPosOfSection);
        m_tfoot->asFrameTableSectionBox()->layoutHeight(ctx);
        m_tfoot->asFrameBox()->setY(ySoFar);
        ySoFar += m_tfoot->asFrameBox()->height();
    }

    ySoFar += paddingBottom();
    ySoFar += borderBottom();
    m_tableRect.setWidth(width());

    LayoutUnit specifiedHeight = 0;
    bool hasTableHeight = false;
    if (style()->height().isFixed()) {
        hasTableHeight = true;
        specifiedHeight = LayoutUnit::fromPixel(style()->height().fixed());
        specifiedHeight += borderHeight() + paddingHeight();
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
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() ==
            CaptionSideValue::BottomCaptionSideValue) {
            caption->setWidth(width() - caption->marginWidth());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar + caption->marginTop());
            ySoFar += caption->height() + caption->marginHeight();
        }
    }
    setHeight(ySoFar);

    // 4. Apply vertical-align to each cell
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->applyVerticalAlign();
        }
    }
}

void FrameTableBox::collectColumnWidths(
    GCVector<ColSizeStruct>& columnWidthsSoFar,
    GCVector<ColSizeStruct>& columnWidths)
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
        // FIXME: Update to support colspans
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

LayoutUnit FrameTableBox::widthFromAttribute(LayoutUnit parentContentWidth)
{
    LayoutUnit tableWidth = -1;
    if (isAnonymous() || !node()->isHTMLTableElement()) {
        return tableWidth;
    }

    String* w = node()->asHTMLTableElement()->width();
    if (w && !w->equals(String::emptyString)) {
        // Use px as the default unit
        if (!w->contains("px") && !w->contains("%")) {
            w = w->concat(String::createASCIIString("px"));
        }
    }

    CSSStyleValuePair pair;
    CSSPropertyParser::parseLengthOrPercent(w->utf8Data(), false, &pair);

    switch (pair.valueKind()) {
    case CSSStyleValuePair::ValueKind::Length: {
        CSSLength len = pair.lengthValue();
        tableWidth = LayoutUnit::fromPixel(len.value());
        break;
    }
    case CSSStyleValuePair::ValueKind::Percentage:
        tableWidth = parentContentWidth * pair.percentageValue();
        break;
    default:
        tableWidth = -1;
    }

    // It is ok to use -1 to indicate both "doesn't exist" and
    // actual negative width, as negative width is invalid.
    // FYI, Blink and Firefox ignore a negative width for table
    return tableWidth;
}

// Table draws the border around the TableFrameSections
void FrameTableBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Fill in the table with background color
    LayoutRect bgRect(m_tableRect.x() + borderLeft(),
                      m_tableRect.y() + borderTop(),
                      m_tableRect.width() - borderWidth(),
                      m_tableRect.height() - borderHeight());

    Unit::Color bgColor;
    if (bgColorFromAttribute(&bgColor)) {
        style()->setBackgroundColor(bgColor);
    }

    paintBackground(canvas, style(), bgRect, m_tableRect, false);

    paintBorders(canvas, m_tableRect);
}

// The layout result of the table may be different from the document order.
// So we have to consider the visual order.
// And if there is no row in the section, the section is an empty section.
FrameTableSectionBox* FrameTableBox::firstNonEmptySectionBoxInVisualOrder()
{
    if (m_thead && m_thead->grid().size()) {
        return m_thead;
    }

    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c != m_tfoot && c->isFrameTableSectionBox() &&
            c->asFrameTableSectionBox()->grid().size()) {
            return c->asFrameTableSectionBox();
        }
    }

    if (m_tfoot && m_tfoot->grid().size()) {
        return m_tfoot;
    }
    return nullptr;
}

FrameTableSectionBox* FrameTableBox::firstSectionBoxInVisualOrder()
{
    if (m_thead) {
        return m_thead;
    }

    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c != m_tfoot && c->isFrameTableSectionBox()) {
            return c->asFrameTableSectionBox();
        }
    }

    if (m_tfoot) {
        return m_tfoot;
    }

    return nullptr;
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

LayoutUnit FrameTableBox::calBaseline()
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
        LineBox* flb = c->firstLineBox();

        if (flb && (!tallestLB || (tallestLB->height() < flb->height()))) {
            tallestLB = flb;
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
    if (m_colObjects.size() == 0) {
        return nullptr;
    }

    unsigned l = 0, r = 0;
    for (auto colGroup : m_colObjects) {
        if (colGroup->firstChild() == nullptr) {
            STARFISH_ASSERT(colGroup->isFrameTableColBox());
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
    return nullptr;
}
}
