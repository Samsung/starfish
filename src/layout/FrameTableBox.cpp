/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
#include "FrameTableBox.h"

#include "FrameTreeBuilder.h"
#include "FrameTableCaptionBox.h"
#include "FrameTableSectionBox.h"
#include "FrameTableRowBox.h"
#include "FrameTableCellBox.h"
#include "FrameTableColBox.h"

namespace StarFish {

class TableFormattingContextBlock {
public:
    TableFormattingContextBlock(Frame* frm, LayoutContext& ctx)
        : m_ctx(ctx)
        , m_needs(false)
    {
        if (frm->isEstablishesBlockFormattingContext()) {
            m_needs = true;
            m_ctx.establishBlockFormattingContext(frm->isNormalFlow());
        }
    }

    ~TableFormattingContextBlock()
    {
        if (m_needs) {
            m_ctx.removeBlockFormattingContext();
        }
    }

    LayoutContext& m_ctx;
    bool m_needs;
};

FrameTableBox::FrameTableBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_tableRect(0, 0, 0, 0)
    , m_thead(nullptr)
    , m_tfoot(nullptr)
{
}

FrameTableBox* FrameTableBox::buildFrameTable(Node* current,
                                              FrameTreeBuilderContext& ctx,
                                              bool force)
{
    FrameTableBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isTable =
        (current->style()->display() == DisplayValue::TableDisplayValue) ||
        (current->style()->display() == DisplayValue::InlineTableDisplayValue);

    if (isTable) {
        currentFrame = new FrameTableBox(current, nullptr);
        current->setFrame(currentFrame);
        // Table establishes a new block context
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            currentFrame->addChild(c, ctx, force);
        }
    } else {
        // If the current node is not a table wrapper node, make either
        // * an anonymous table wrapper box, or
        // * use the last anonymous wrapper box if it has already been created
        //   by a previous (and continuous) sibling of the current node.
        Frame* before = parent->lastChild();

        // NEED TO DISCUSSION : StarFish generate anonymous block box which has
        // only whitespace, below code treat above situation
        while (before && before->isAnonymous() &&
               before->firstChild()->isFrameText() &&
               before->firstChild()
                   ->asFrameText()
                   ->text()
                   ->containsOnlyWhitespace()) {
            before = before->previous();
        }

        if (before && before->isAnonymous() && before->isFrameTableBox()) {
            currentFrame = before->asFrameTableBox();
        } else {
            currentFrame =
                FrameTableBox::createAnonymousWithParent(parent, current);
        }
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        currentFrame->addChild(current, ctx, force);
    }
    ctx.setCurrentBlockContainer(parent);
    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

FrameTableBox* FrameTableBox::createAnonymousWithParent(FrameBlockBox* parent,
                                                        Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableBox(nullptr, style);
}

void FrameTableBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                             bool force)
{
    bool wrapInAnnoymousSection = false;
    Frame* childFrame;
    DisplayValue display = child->style()->display();

    switch (display) {
    case DisplayValue::TableCaptionDisplayValue:
        childFrame =
            FrameTableCaptionBox::buildFrameTableCaptionBox(child, ctx, force);
        m_captions.push_back(childFrame->asFrameTableCaptionBox());
        STARFISH_ASSERT(!childFrame->parent());
        break;
    case DisplayValue::TableColumnGroupDisplayValue:
    case DisplayValue::TableColumnDisplayValue:
        // buildFrameTableColBox always return a pointer of Frame object
        // If childFrame is reused anonymous, it will already have a parent,
        // so only forms a parent-child relationship when there is no parent.
        childFrame = FrameTableColBox::buildFrameTableColBox(child, ctx, force);
        if (!childFrame->parent()) {
            ctx.currentBlockContainer()->appendChild(childFrame);
            m_colObjects.push_back(childFrame->asFrameTableColBox());
        }
        return;
    case DisplayValue::TableHeaderGroupDisplayValue:
    case DisplayValue::TableFooterGroupDisplayValue:
    case DisplayValue::TableRowGroupDisplayValue:
        childFrame =
            FrameTableSectionBox::buildFrameTableSectionBox(child, ctx, force);
        STARFISH_ASSERT(!childFrame->parent());
        break;
    default:
        wrapInAnnoymousSection = true;
    }

    if (!wrapInAnnoymousSection && !childFrame->parent()) {
        ctx.currentBlockContainer()->appendChild(childFrame);
        STARFISH_ASSERT(childFrame->parent());
        if (!m_thead &&
            (display == DisplayValue::TableHeaderGroupDisplayValue)) {
            m_thead = childFrame->asFrameTableSectionBox();
        }
        if (!m_tfoot &&
            (display == DisplayValue::TableFooterGroupDisplayValue)) {
            m_tfoot = childFrame->asFrameTableSectionBox();
        }
        return;
    }

    if (child->isCharacterData() || child->isComment()) {
        // TODO
        return;
    } else if (wrapInAnnoymousSection) {
        // If there are 2 continuous node which becomes internal table box
        // without any proper parent, we have a problem, because parser doesn't
        // form a group these with one parent.
        // so we handle this at buileFrameTableXXX. The first node's frame box
        // will be returned with a hierarchical anonymous table box,
        // the point is the second one. If the second one does the same with
        // the first one, then we might have duplication processing about
        // reused anonymous box so that currentBlockContainer has 2 children
        // which referencing the same thing.
        // To prevent this situation, we separate two cases with which returned
        // pointer has a parent or not.

        childFrame =
            FrameTableSectionBox::buildFrameTableSectionBox(child, ctx, force);
        if (!childFrame->parent()) {
            ctx.currentBlockContainer()->appendChild(childFrame);
            STARFISH_ASSERT(childFrame->parent());
        }
        return;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTableBox::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    // This method is called by FrameBlockBox::layout() to do table layout.
    // Table starts its own layout algorithm that has minimum interaction with
    // the existing layout algorithm.
    // In brief,
    // after establishes a table context, we calculate the width of the table,
    // and place cells in rows and columns. To do so, we calculate x positions
    // of cells first, and then calculate the y positions of cells.
    TableFormattingContextBlock context(this, ctx);

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        calCellWidth(ctx);
        layoutWidth(ctx);
    }
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        LayoutUnit top = paddingTop() + borderTop();
        LayoutUnit bottom = paddingBottom() + borderBottom();
        // The table always establishes block formatting context,
        // so set the third argument to true
        MarginInfo marginInfo(top, bottom, true, style()->height());
        setMarginInfo(&marginInfo);
        layoutHeight(ctx);
    }
}

void FrameTableBox::calCellWidth(LayoutContext& ctx)
{
    switch (style()->tableLayout()) {
    case TableLayoutValue::AutoTableLayoutValue:
        calCellWidthForAutoTableLayout(ctx);
        break;
    case TableLayoutValue::FixedTableLayoutValue:
        calCellWidthForFixedTableLayout(ctx);
        break;
    default:
        // TableLayout has the above two values only
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTableBox::calCellWidthForAutoTableLayout(LayoutContext& ctx)
{
    // 1. We traverse the table to calculate min/max cell widths of the table
    //    before we perform table layout.
    m_columnWidths.clear();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->calCellWidth(ctx);
            collectColumnWidths(m_columnWidths,
                                c->asFrameTableSectionBox()->columnWidths());
        }
    }

    // 2. Determine the column size of the table
    //    If the parent's width is smaller than the max table width,
    //    the with of the table is adjusted to somewhere between min/max table
    //    width.
    //    If the parent's width >= the max table width,
    //    max table width is used

    // 2.1 calculate the table width
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(style()->horizontalBorderSpacing().fixed());
    LayoutUnit tableWidth = 0;
    tableWidth += mbpWidth();
    tableWidth += borderSpacing;
    for (auto& colSize : m_columnWidths) {
        tableWidth += colSize.maxCellWidth + borderSpacing;
    }

    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    if (tableWidth > parentContentWidth) {
        LayoutUnit availableWidth = parentContentWidth;
        availableWidth -= mbpWidth();
        availableWidth -=
            borderSpacing + (borderSpacing * m_columnWidths.size());

        // 2.2 Calculate the ratio of which each column is to be reduced.
        //     The cell width is:
        //     minCellWidth <= cellWidth <= maxCellWidth
        LayoutUnit totalCellWidths = 0;
        for (auto& col : m_columnWidths) {
            totalCellWidths += col.maxCellWidth;
        }

        for (auto& col : m_columnWidths) {
            // FIXME: LayoutUnit has a rounding error bug when division is
            // performed. To workaround, we convert LayoutUnit to double,
            // do calculation, and convert back to LayoutUnit.
            LayoutUnit newCellWidth(
                availableWidth.toDouble() *
                (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));

            if (newCellWidth < col.minCellWidth) {
                col.cellWidth = col.minCellWidth;
                availableWidth -= col.minCellWidth;
                totalCellWidths -= col.maxCellWidth;
            }
        }

        for (auto& col : m_columnWidths) {
            if (col.cellWidth != col.minCellWidth) {
                // To workaround the rounding error in LayoutUnit
                LayoutUnit newCellWidth(
                    availableWidth.toDouble() *
                    (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));
                col.cellWidth = std::max(col.minCellWidth, newCellWidth);
            }
        }
    }
}

void FrameTableBox::calCellWidthForFixedTableLayout(LayoutContext& ctx)
{
    // 0. Get the cells in the first row. These are used to determine:
    // * the width of each cell, and
    // * the width property (i.e., auto or specified) in the table.
    m_cellsInTheFirstRow.clear();
    FrameTableSectionBox* firstSection = firstSectionBoxInVisualOrder();
    if (!firstSection || !firstSection->firstChild()) {
        // We stop layout for an empty table
        return;
    }

    FrameTableRowBox* row = firstSection->firstChild()->asFrameTableRowBox();
    for (Frame* c = row->firstChild(); c; c = c->next()) {
        m_cellsInTheFirstRow.push_back(c->asFrameTableCellBox());
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
        STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());
        FrameTableCellBox* cell = m_cellsInTheFirstRow[col.id];

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
        } else if (style()->width().isPercent()) {
            tableWidth =
                parentContentWidth.toInt() * style()->width().percent();
        } else {
            // should not be here
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

#ifndef NDEBUG
    // This is used only to run w3c test cases.
    // width attribute has the highest priority when defining the width of
    // a table.
    //
    // NOTE: width="0" is handled differently by Blink and Firefox.
    // When width="0" is given, Blink tries to set table width to 0.
    // Firefox ignores width="0". We ignore width="0"
    LayoutUnit widthAttribute = widthFromAttribute();
    if (widthAttribute > 0) {
        hasTableWidth = true;
        tableWidth = widthAttribute;
    }
#endif

    // 3. If a width of the table is given, we either increase or decrease the
    // cell widths to fit them into the width of table.
    if (hasTableWidth) {
        LayoutUnit availableWidth = tableWidth;
        availableWidth -= borderWidth() + paddingWidth();
        availableWidth -=
            (borderSpacing * m_columnWidths.size()) + borderSpacing;
        LayoutUnit sumOfSpecifiedCellWidths = 0;

        for (auto& c : cellsWithSpecifiedWidths) {
            ColSizeStruct& col = *c;
            STARFISH_ASSERT(col.id < m_cellsInTheFirstRow.size());
            FrameTableCellBox* cell = m_cellsInTheFirstRow[col.id];

            LayoutUnit specifiedWidth = 0;
            if (cell->style()->width().isFixed()) {
                specifiedWidth =
                    LayoutUnit::fromPixel(cell->style()->width().fixed());
                specifiedWidth += cell->borderWidth() + cell->paddingWidth();
                col.cellWidth = specifiedWidth;
            } else if (cell->style()->width().isPercent()) {
                specifiedWidth =
                    availableWidth * cell->style()->width().percent();
            }
            sumOfSpecifiedCellWidths += specifiedWidth;
        }

        if (sumOfSpecifiedCellWidths >= tableWidth) {
            // Set the widths of all cells with "width: auto" to 0, if any
            for (auto& c : cellsWithAutoWidths) {
                ColSizeStruct& col = *c;
                col.cellWidth = 0;
            }
        } else {
            LayoutUnit remainingWidth = tableWidth - sumOfSpecifiedCellWidths;
            remainingWidth -= borderWidth() + paddingWidth();
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
                    STARFISH_ASSERT(m_cellsInTheFirstRow.size() ==
                                    m_columnWidths.size());

                    for (auto& col : m_columnWidths) {
                        FrameTableCellBox* cell = m_cellsInTheFirstRow[col.id];

                        LayoutUnit cellWidth = cell->style()->width().fixed();
                        cellWidth += cell->borderWidth() + cell->paddingWidth();

                        LayoutUnit extraCellWidth =
                            LayoutUnit(cellWidth.toDouble() /
                                       sumOfSpecifiedCellWidths.toDouble() *
                                       remainingWidth.toDouble());
                        col.cellWidth += extraCellWidth;
                    }
                } else {
                    // Distribute available spaces equally among cells with
                    // "width: auto"
                    LayoutUnit newCellWidth = LayoutUnit(
                        remainingWidth.toDouble() / cellsWithAutoWidths.size());
                    for (auto& c : cellsWithAutoWidths) {
                        ColSizeStruct& col = *c;
                        col.cellWidth = newCellWidth;
                    }
                }
            }
        }
    } else {
        bool FOLLOW_SPEC = true;
        // 1. A cell width cannot be smaller than the min width of the
        // cell
        LayoutUnit sumOfAutoCellPreferredWidths = 0;
        LayoutUnit sumOfAdjustedSpecifiedCellWidths = 0;
        std::vector<ColSizeStruct*> columnsAdjustedToMinWidths;
        std::vector<ColSizeStruct*> columnsMayNeedToAdjustWidths;
        LayoutUnit sumOfColWidths = 0;
        for (auto& col : m_columnWidths) {
            STARFISH_ASSERT(col.id < m_columnWidths.size());
            FrameTableCellBox* cell = m_cellsInTheFirstRow[col.id];

            if (isCellWidthAuto(col.id)) {
                col.cellWidth = col.maxCellWidth;
                sumOfAutoCellPreferredWidths += col.cellWidth;
            } else {
                LayoutUnit specifiedWidth = 0;
                col.cellWidth = col.maxSpecifiedWidth;

                if (cell->style()->width().isFixed()) {
                    specifiedWidth =
                        LayoutUnit::fromPixel(cell->style()->width().fixed());
                    specifiedWidth +=
                        cell->borderWidth() + cell->paddingWidth();
                    col.cellWidth = specifiedWidth;
                }

                if (col.cellWidth.toDouble() <= col.minCellWidth.toDouble()) {
                    col.cellWidth = col.minCellWidth;
                    columnsAdjustedToMinWidths.push_back(&col);
                } else {
                    columnsMayNeedToAdjustWidths.push_back(&col);
                }
                sumOfAdjustedSpecifiedCellWidths += col.cellWidth;
            }

            sumOfColWidths += col.cellWidth;
        }

        LayoutUnit tableWidthByAddingColWidths = sumOfColWidths;
        tableWidthByAddingColWidths += borderWidth() + paddingWidth();
        tableWidthByAddingColWidths +=
            (borderSpacing * m_columnWidths.size()) + borderSpacing;

        tableWidth = parentContentWidth - marginWidth();

        // Distribute available spaces to cells with "width: auto" in
        // proportion to the cell's preferred width.
        // The cell width can either be increased or decreased depending
        // on the width of table and min width of a cell.
        if (cellsWithAutoWidths.size() > 0) {
            LayoutUnit remainingWidth = tableWidth;
            remainingWidth -= borderWidth() + paddingWidth();
            remainingWidth -=
                (borderSpacing * m_columnWidths.size()) + borderSpacing;
            remainingWidth -= sumOfAdjustedSpecifiedCellWidths;

            unsigned reducedToMinWidths = 0;
            LayoutUnit sumOfAutoCellMinWidths = 0;
            for (auto& c : cellsWithAutoWidths) {
                STARFISH_ASSERT(c->id < m_columnWidths.size());
                ColSizeStruct& col = m_columnWidths[c->id];
                LayoutUnit newCellWidth(
                    col.maxCellWidth.toDouble() /
                    sumOfAutoCellPreferredWidths.toDouble() *
                    remainingWidth.toDouble());
                col.cellWidth = std::max(newCellWidth, col.minCellWidth);

                if (FOLLOW_SPEC) {
                    col.cellWidth = std::min(col.cellWidth, col.maxCellWidth);
                }

                if (col.cellWidth == col.minCellWidth) {
                    columnsAdjustedToMinWidths.push_back(&col);
                    reducedToMinWidths++;
                    sumOfAutoCellMinWidths += col.cellWidth;
                }
            }

            // We further reduce column widths if all columns with
            // "width: auto" are reduced to their min preferred widths and
            // the columns other than "width: auto" still have rooms to reduce.
            if (reducedToMinWidths == cellsWithAutoWidths.size()) {
                remainingWidth = tableWidth;
                remainingWidth -= borderWidth() + paddingWidth();
                remainingWidth -=
                    (borderSpacing * m_columnWidths.size()) + borderSpacing;
                remainingWidth -= sumOfAutoCellMinWidths;

                for (auto& c : columnsMayNeedToAdjustWidths) {
                    ColSizeStruct& col = *c;
                    LayoutUnit newCellWidth(
                        remainingWidth.toDouble() *
                        (col.cellWidth.toDouble() /
                         sumOfAdjustedSpecifiedCellWidths.toDouble()));
                    col.cellWidth = std::max(col.minCellWidth, newCellWidth);
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
            if (tableWidth < tableWidthByAddingColWidths) {
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
                    FrameTableCellBox* cell = m_cellsInTheFirstRow[col.id];

                    LayoutUnit cellWidth = cell->style()->width().fixed();
                    cellWidth += cell->borderWidth() + cell->paddingWidth();

                    LayoutUnit newCellWidth(cellWidth.toDouble() /
                                            sumOfColWidths.toDouble() *
                                            availableWidth.toDouble());

                    col.cellWidth = std::max(col.minCellWidth, newCellWidth);
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
    LayoutUnit maxWidth = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->layoutWidth(ctx);
            maxWidth = std::max(maxWidth, c->asFrameBox()->width());
        } else if (c->isFrameTableCaptionBox()) {
            c->asFrameTableCaptionBox()->layout(
                ctx, Frame::LayoutWantToResolve::ResolveWidth);
            c->asFrameTableCaptionBox()->setX(
                c->asFrameTableCaptionBox()->marginLeft());
        } else if (c->isFrameTableColBox()) {
            // The FrameTableColBox must not be laid out.
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // The width of all table sections should be the same,
    // so getting the max width should be the same as the width of
    // any table sections.
    setWidth(maxWidth + paddingWidth() + borderWidth());
    computeBorderMarginPadding(width());
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
    m_tableRect.setHeight(ySoFar - m_tableRect.y());

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
}

void FrameTableBox::collectColumnWidths(
    GCVector<ColSizeStruct>& columnWidthsSoFar,
    GCVector<ColSizeStruct>& columnWidths)
{
    // FIXME: absolute at this stage
    // Need to consider absolute and logical columns
    if (columnWidthsSoFar.empty()) {
        columnWidthsSoFar = columnWidths;
    } else {
        // FIXME: Update to support colspans
        for (unsigned i = 0; i < columnWidths.size(); i++) {
            ColSizeStruct& col = columnWidths[i];
            if (columnWidthsSoFar.size() == i) {
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
    // table has specified width or not. We DO NOT read "table-layout" property.
    // (This is how Blink and Firefox work)
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

    if (style()->width().isAuto()) {
        // Table does not have a width
        if (i < m_cellsInTheFirstRow.size()) {
            STARFISH_ASSERT(i < m_columnWidths.size());
            if (m_cellsInTheFirstRow[i]->style()->width().isAuto() &&
                !(m_columnWidths[i].hasSpecifiedWidth())) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        // Table has a specified width
        if (i < m_cellsInTheFirstRow.size()) {
            return m_cellsInTheFirstRow[i]->style()->width().isAuto() ? true
                                                                      : false;
        } else {
            return true;
        }
    }
}

// Table draws the border around the TableFrameSections
void FrameTableBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Fill in the table with background color
    LayoutRect bgRect(m_tableRect.x() + borderLeft(),
                      m_tableRect.y() + borderTop(),
                      m_tableRect.width() - borderWidth(),
                      m_tableRect.height() - borderHeight());
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
// * Use the baseline of the first row if the first row has
//   'vertical-align: baseline'
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
    LineBox* tallestLineBox = nullptr;
    LayoutUnit maxLineBoxHeight = 0;

    for (size_t i = 0; i < firstRS.cells.size(); ++i) {
        FrameTableCellBox* c = firstRS.cells[i].cell;
        LineBox* firstLineBox = nullptr;

        if (c->hasBlockFlow() && c->firstChild()) {
            FrameBlockBox* firstBox = c->firstChild()->asFrameBlockBox();
            if (!firstBox->lineBoxes().empty()) {
                firstLineBox = firstBox->lineBoxes()[0];
            }
        } else if (!c->lineBoxes().empty()) {
            firstLineBox = c->lineBoxes()[0];
        }

        if (firstLineBox && maxLineBoxHeight < firstLineBox->height()) {
            tallestLineBox = firstLineBox;
            maxLineBoxHeight = firstLineBox->height();
        }
    }

    LayoutUnit ySoFar = firstSection->y();

    if (tallestLineBox) {
        ySoFar += firstRS.tableRow->y() + firstRS.cells[0].cell->y();
        if (firstRS.tableRow->style()->verticalAlign() ==
            VerticalAlignValue::BaselineVAlignValue) {
            return ySoFar + firstRS.tableRow->baseline();
        }
        return ySoFar + tallestLineBox->y() + tallestLineBox->height();
    } else if (firstRS.cells.size()) {
        // Empty cell
        return ySoFar + firstRS.tableRow->y() + firstRS.cells[0].cell->y() +
               (firstRS.cells[0].cell->height().toDouble() / 2);
    }
    // Empty first row
    return ySoFar;
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
