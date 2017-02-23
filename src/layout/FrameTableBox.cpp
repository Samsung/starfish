/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

FrameTableBox* FrameTableBox::buildFrameTable(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableBox* tableWrapper;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isTable = (current->style()->display() == DisplayValue::TableDisplayValue)
        || (current->style()->display() == DisplayValue::InlineTableDisplayValue);

    if (isTable) {
        // if current node is table then make wrapper and current node owns this wrapper
        tableWrapper = new FrameTableBox(current, nullptr);
        current->setFrame(tableWrapper);
    } else {
        // If the current node is not a table wrapper node, make either
        // * an anonymous table wrapper box, or
        // * use the last anonymous wrapper box if it has already been created
        //   by a previous (and continuous) sibling of the current node.
        Frame* before = parent->lastChild();

        // NEED TO DISCUSSION : StarFish generate anonymous block box which has only whitespace,
        // below code treat above situation
        while (before && before->isAnonymous() && before->firstChild()->isFrameText()
            && before->firstChild()->asFrameText()->text()->containsOnlyWhitespace()) {
            before = before->previous();
        }

        if (before && before->isAnonymous() && before->isFrameTableBox()) {
            tableWrapper = before->asFrameTableBox();
        } else {
            tableWrapper = FrameTableBox::createAnonymousWithParent(parent, current);
        }
    }

    // Table establishes a new block context
    ctx.setCurrentBlockContainer(tableWrapper);
    ctx.mergeTextDecorationData(tableWrapper->style());

    if (isTable) {
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            tableWrapper->addChild(c, ctx, force);
        }
    } else if (tableWrapper->isAnonymous()) {
        tableWrapper->addChild(current, ctx, force);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parent);
    return tableWrapper->parent()? nullptr : tableWrapper;
}

FrameTableBox* FrameTableBox::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableBox(nullptr, style);
}

void FrameTableBox::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    bool wrapInAnnoymousSection = false;
    Frame* childFrame;
    DisplayValue display = child->style()->display();

    switch (display) {
    case DisplayValue::TableCaptionDisplayValue:
        childFrame = FrameTableCaptionBox::buildFrameTableCaptionBox(child, ctx, force);
        m_captions.push_back(childFrame->asFrameTableCaptionBox());
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
        childFrame = FrameTableSectionBox::buildFrameTableSectionBox(child, ctx, force);
        break;
    default:
        wrapInAnnoymousSection = true;
    }

    if (!wrapInAnnoymousSection) {
        FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
        STARFISH_ASSERT(childFrame->parent());
        if (!m_thead && (display == DisplayValue::TableHeaderGroupDisplayValue)) {
            m_thead = childFrame->asFrameTableSectionBox();
        }
        if (!m_tfoot && (display == DisplayValue::TableFooterGroupDisplayValue)) {
            m_tfoot = childFrame->asFrameTableSectionBox();
        }
        return;
    }

    if (child->isCharacterData() || child->isComment()) {
        // TODO
        return;
    } else if (wrapInAnnoymousSection) {
        // If there are 2 continuous node which becomes internal table box without any proper parent,
        // we have a problem, because parser doesn't form a group these with one parent.
        // so we handle this at buileFrameTableXXX. The first node's frame box will be returned with a
        // hierarchical anonymous table box, the point is the second one. If the second one does the same
        // with the first one, then we might have duplication processing about reused anonymous box
        // so that currentBlockContainer has 2 children which referencing the same thing.
        // To prevent this situation, we separate two cases with which returned pointer is nullptr.

        // TODO: but this behavior seems a bit confusing. So I am thinking of a better design.
        childFrame = FrameTableSectionBox::buildFrameTableSectionBox(child, ctx, force);
        if (childFrame != nullptr) {
            ctx.currentBlockContainer()->appendChild(childFrame);
            STARFISH_ASSERT(childFrame->parent());
        }
        return;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTableBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method is called by FrameBlockBox::layout() to do table layout.
    // Table starts its own layout algorithm that has minimum interaction with
    // the existing layout algorithm.
    //
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
            collectColumnWidths(m_columnWidths, c->asFrameTableSectionBox()->columnWidths());
        }
    }

    // 2. Determine the column size of the table
    //    If the parent's width is smaller than the max table width,
    //    the with of the table is adjusted to somewhere between min/max table
    //    width.
    //    If the parent's width >= the max table width,
    //    max table width is used

    // 2.1 calculate the table width
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(style()->borderSpacing().fixed());
    LayoutUnit tableWidth = 0;
    tableWidth += marginWidth() + borderWidth() + paddingWidth();
    tableWidth += borderSpacing;
    for (auto& colSize : m_columnWidths) {
        tableWidth += colSize.maxCellWidth + borderSpacing;
    }

    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    if (tableWidth > parentContentWidth) {
        LayoutUnit availableWidth = parentContentWidth;
        availableWidth -= marginWidth() + borderWidth() + paddingWidth();
        availableWidth -= borderSpacing + (borderSpacing * m_columnWidths.size());

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
            // LayoutUnit newCellWidth = availableWidth * (col.maxCellWidth / totalCellWidths);
            LayoutUnit newCellWidth(availableWidth.toDouble() *
                (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));

            if (newCellWidth.round() < col.minCellWidth) {
                col.cellWidth = col.minCellWidth;
                availableWidth -= col.minCellWidth;
                totalCellWidths -= col.maxCellWidth;
            }
        }

        for (auto& col : m_columnWidths) {
            if (col.cellWidth != col.minCellWidth) {
                // To workaround the rounding error in LayoutUnit
                // LayoutUnit newCellWidth = availableWidth * (col.maxCellWidth / totalCellWidths);
                LayoutUnit newCellWidth(availableWidth.toDouble() *
                    (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));
                col.cellWidth = std::max(col.minCellWidth.toInt(), newCellWidth.round());
            }
        }
    }
}

void FrameTableBox::calCellWidthForFixedTableLayout(LayoutContext& ctx)
{
    // 0. Get the cells in the first row. These are used to
    // determine the width of each cell in the table later.
    m_cellsInTheFirstRow.clear();
    for (Frame* s = firstChild(); s; s = s->next()) {
        if (s->isFrameTableSectionBox()) {
            FrameTableSectionBox* section = s->asFrameTableSectionBox();
            if (section && section->firstChild()) {
                FrameTableRowBox* row = section->firstChild()->asFrameTableRowBox();
                if (row) {
                    for (Frame* c = row->firstChild(); c; c = c->next()) {
                        FrameTableCellBox* cell = c->asFrameTableCellBox();
                        m_cellsInTheFirstRow.push_back(cell);
                    }
                }
            }

            break;
        }
    }

    // 1. We traverse the table to calculate:
    //  * min/max cell widths of the table if "width: auto", OR
    //  * fixed cell widths if the cell width is not "width: auto".
    // before performing the layout.
    m_columnWidths.clear();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->calCellWidth(ctx);
            collectColumnWidths(m_columnWidths, c->asFrameTableSectionBox()->columnWidths());
        }
    }

    // 2. Calculate the table width
    // TODO: Need to consider the widths of captions too
    //
    // Followed the algorithm from
    // https://www.w3.org/TR/2016/WD-css-tables-3-20161025/#width-distribution
    // except when table width is given, we simple use the width (following how
    // blink works)
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(style()->borderSpacing().fixed());
    LayoutUnit maxTableWidth = 0;
    LayoutUnit minTableWidth = 0;
    maxTableWidth += marginWidth() + borderWidth() + paddingWidth();
    maxTableWidth += borderSpacing;
    minTableWidth = maxTableWidth;
    for (auto& colSize : m_columnWidths) {
        maxTableWidth += colSize.maxCellWidth + borderSpacing;
        minTableWidth += colSize.minCellWidth + borderSpacing;
    }

    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    LayoutUnit tableWidth;
    bool hasTableWidth = false;
    if (style()->width().isAuto()) {
        LayoutUnit preferredWidth = std::min(maxTableWidth, parentContentWidth);
        tableWidth = std::min(preferredWidth, minTableWidth);
    } else {
        // The width of table is explicitly given
        hasTableWidth = true;
        if (style()->width().isFixed()) {
            // Following Blinks behaviour here
            // tableWidth = std::max(minTableWidth, LayoutUnit::fromPixel(style()->width().fixed()));
            tableWidth = LayoutUnit::fromPixel(style()->width().fixed());
        } else if (style()->width().isPercent()) {
            // Not implemented yet
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else {
            // should not be here
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // 3. Increase or reduce the width of each cell.
    if (tableWidth < maxTableWidth) {
        // 3.1 Reduce the width of each cell if table width is smaller
        // than the sum of max column widths. There are two types of table
        // width reduction:
        //
        // * When tableWidth specified
        // * When tableWidth is not specified
        //
        // When tableWidth is given, find the sum of cell widths that have fixed
        // width. If the total sum is greater than the table width, 0 is given
        // to all cells with "width: auto". Else, the remaining spaces are
        // distributed to among these cells.
        //
        // When tableWidth is not given, reduce the cells that have "width: auto".
        // Each width of these cells is reduced in proportion to the ratio of
        // the width of the cell over the total table width.
        // Cells with "width: auto" are not affected here, but adjusted later
        // this function if further rooms are needed.
        //
        // Note: The spec does not say how to reduce the width of cells.
        // Our table width reducing behaviour mimics how blink works.

        if (hasTableWidth) {
            // 1. Get the sum of all fixed cell widths.
            LayoutUnit totalFixedCellWidths = 0;
            std::vector<ColSizeStruct*> cellsWithAutoWidths;
            for (unsigned i = 0; i < m_columnWidths.size(); i++) {
                ColSizeStruct& col = m_columnWidths[i];

                if (!isCellWidthAuto(i)) {
                    STARFISH_ASSERT(i < m_cellsInTheFirstRow.size());
                    totalFixedCellWidths += m_cellsInTheFirstRow[i]->style()->width().fixed();
                } else {
                    cellsWithAutoWidths.push_back(&col);
                }
            }

            if (totalFixedCellWidths >= tableWidth) {
                // Set the widths of all "width: auto" cells to 0
                for (auto& c : cellsWithAutoWidths) {
                    ColSizeStruct& col = *c;
                    col.cellWidth = 0;
                }
            } else {
                // Distribute available spaces among cells width "width: auto"
                LayoutUnit availableWidth = tableWidth - totalFixedCellWidths;
                availableWidth -= borderWidth() + paddingWidth();
                availableWidth -= (borderSpacing * m_columnWidths.size()) - borderSpacing;
                LayoutUnit newCellWidth(availableWidth.toDouble() / cellsWithAutoWidths.size());

                for (auto& c : cellsWithAutoWidths) {
                    ColSizeStruct& col = *c;
                    col.cellWidth = newCellWidth.round();
                }
            }
        } else {
            LayoutUnit availableWidth = tableWidth;
            availableWidth -= marginWidth() + borderWidth() + paddingWidth();
            availableWidth -= borderSpacing + (borderSpacing * m_columnWidths.size());

            // 3.1.1 Calculate the ratio of which each column is to be reduced.
            //     The cell width is:
            //     minCellWidth <= cellWidth <= maxCellWidth
            LayoutUnit totalCellWidths = 0;
            for (auto& col : m_columnWidths) {
                totalCellWidths += col.maxCellWidth;
            }

            // 3.1.2 Check whether it is ok to reduce the width of each cell.
            // The minimum width of a cell should be no less than
            // the preferred min width of the cell.
            std::vector<ColSizeStruct*> columnsWithUserDefinedWidths;
            std::vector<ColSizeStruct*> columnsAdjustedToMinWidths;
            std::vector<ColSizeStruct*> columnsToAdjustWidths;
            for (unsigned i = 0; i < m_columnWidths.size(); i++) {
                ColSizeStruct& col = m_columnWidths[i];
                LayoutUnit newCellWidth(availableWidth.toDouble() *
                    (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));

                if (!isCellWidthAuto(i)) {
                    // Use the user-supplied width
                    col.cellWidth = col.maxCellWidth;
                    availableWidth -= col.cellWidth;
                    totalCellWidths -= col.cellWidth;
                    columnsWithUserDefinedWidths.push_back(&col);
                } else {
                    // This cell has "width: auto". If the scaled width is
                    // less than the min preferred cell width, use the
                    // min preferred width for the cell
                    if (newCellWidth.round() <= col.minCellWidth) {
                        col.cellWidth = col.minCellWidth;
                        availableWidth -= col.minCellWidth;
                        totalCellWidths -= col.maxCellWidth;
                        columnsAdjustedToMinWidths.push_back(&col);
                    } else {
                        columnsToAdjustWidths.push_back(&col);
                    }
                }
            }

            // 3.1.3 Now reduce the width of each cell
            for (auto& c : columnsToAdjustWidths) {
                ColSizeStruct& col = *c;
                // To workaround the rounding error in LayoutUnit
                // LayoutUnit newCellWidth = availableWidth * (col.maxCellWidth / totalCellWidths);
                LayoutUnit newCellWidth(availableWidth.toDouble() *
                    (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));
                col.cellWidth = std::max(col.minCellWidth.toInt(), newCellWidth.round());

                if (col.cellWidth == col.minCellWidth) {
                    columnsAdjustedToMinWidths.push_back(&col);
                }
            }

            // 3.1.4 Blink further reduces column widths if all columns with
            // "width: auto" are reduced to their min preferred widths and
            // the columns other than "width: auto" still have rooms to reduce.
            // The spec does not say anything about this behaviour.
            if (columnsWithUserDefinedWidths.size() + columnsAdjustedToMinWidths.size() == m_columnWidths.size()) {
                for (auto& col : m_columnWidths) {
                    availableWidth += col.cellWidth;
                    totalCellWidths += col.cellWidth;
                }
                for (auto& c : columnsWithUserDefinedWidths) {
                    ColSizeStruct& col = *c;
                    LayoutUnit newCellWidth(availableWidth.toDouble() *
                        (col.cellWidth.toDouble() / totalCellWidths.toDouble()));
                    col.cellWidth = std::max(col.minCellWidth.toInt(), newCellWidth.round());
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
            c->asFrameTableCaptionBox()->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        } else if (c->isFrameTableColBox()) {
            // The FrameTableColBox must not be laid out.
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // The width of all table sections should be the same,
    // so getting the max width should be the same as the width of
    // any table sections.
    setWidth(maxWidth);
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

    LayoutUnit ySoFar = marginTop();

    // 1. place captions with caption-side: top
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::TopCaptionSideValue) {
            caption->setWidth(paddingLeft() + width() + paddingRight());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
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
    m_tableRect.setWidth(paddingLeft() + width() + paddingRight());
    m_tableRect.setHeight(ySoFar - m_tableRect.y());

    // 3. place captions with caption-side: bottom
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::BottomCaptionSideValue) {
            caption->setWidth(paddingLeft() + width() + paddingRight());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
        }
    }
    ySoFar += marginBottom();

    setHeight(ySoFar);
}

void FrameTableBox::collectColumnWidths(GCVector<ColSizeStruct>& columnWidthsSoFar, GCVector<ColSizeStruct>& columnWidths)
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
            colSoFar.maxCellWidth = std::max(colSoFar.maxCellWidth, col.maxCellWidth);
            colSoFar.minCellWidth = std::max(colSoFar.minCellWidth, col.minCellWidth);
            colSoFar.cellWidth = colSoFar.maxCellWidth;
        }
    }
}

bool FrameTableBox::isCellWidthAuto(unsigned i)
{
    if (i < m_cellsInTheFirstRow.size()) {
        return m_cellsInTheFirstRow[i]->style()->width().isAuto()? true: false;
    } else {
        return true;
    }
}

// Table draws the border around the TableFrameSections
void FrameTableBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Fill in the table with background color
    LayoutRect bgRect(m_tableRect.x() + borderLeft(), m_tableRect.y() + borderTop(),
        m_tableRect.width() - borderWidth(), m_tableRect.height() - borderHeight());
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
        if (c != m_tfoot && c->isFrameTableSectionBox() && c->asFrameTableSectionBox()->grid().size()) {
            return c->asFrameTableSectionBox();
        }
    }

    if (m_tfoot && m_tfoot->grid().size()) {
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
    STARFISH_ASSERT(style()->display() == DisplayValue::InlineTableDisplayValue);

    FrameTableSectionBox* firstSection = firstNonEmptySectionBoxInVisualOrder();
    if (!firstSection) {
        return height();
    }

    RowStruct& firstRS = firstSection->grid()[0];
    LineBox* tallestLineBox = nullptr;
    LayoutUnit maxLineBoxHeight = 0;

    for (size_t i = 0 ; i < firstRS.cells.size(); ++i) {
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
        if (firstRS.tableRow->style()->verticalAlign() == VerticalAlignValue::BaselineVAlignValue) {
            return ySoFar + firstRS.tableRow->baseline();
        }
        return ySoFar + tallestLineBox->y() + tallestLineBox->height();
    } else if (firstRS.cells.size()) {
        // Empty cell
        return ySoFar + firstRS.tableRow->y() + firstRS.cells[0].cell->y() + (firstRS.cells[0].cell->height().toDouble() / 2);
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
