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
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

FrameTableBox* FrameTableBox::buildFrameTable(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableBox* tableWrapper;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    if (current->isTable()) {
        // if current node is table then make wrapper and current node owns this wrapper
        tableWrapper = new FrameTableBox(current, nullptr);
        current->setFrame(tableWrapper);
    } else {
        // if current node is not table wrapper node then make anonymous wrapper or
        // reuse before anonymous wrapper
        FrameBlockBox* parent = ctx.currentBlockContainer();
        Frame* before = parent->lastChild();

        // NEED TO DISCUSSION : StarFish generate anonymous block box which has only wihtespace,
        // below code treat above situation
        while (before->isAnonymous() && before->firstChild()->isFrameText()
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

    if (current->isTable()) {
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

    if (child->isTableCaption()) {
        childFrame = FrameTableCaptionBox::buildFrameTableCaptionBox(child, ctx, force);
        m_captions.push_back(childFrame->asFrameTableCaptionBox());
    } else if (child->isTableCol()) {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else if (child->isTableSection()) {
        // Only [TableHeaderGroup | TableFooterGroup | TableRowGroup] should appear
        switch (child->style()->display()) {
        case DisplayValue::TableHeaderGroupDisplayValue:
        case DisplayValue::TableFooterGroupDisplayValue:
        case DisplayValue::TableRowGroupDisplayValue:
            childFrame = FrameTableSectionBox::buildFrameTableSectionBox(child, ctx, force);
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        wrapInAnnoymousSection = true;
    }

    if (!wrapInAnnoymousSection) {
        FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
        STARFISH_ASSERT(childFrame->parent());
        if (!m_thead && (child->style()->display() == DisplayValue::TableHeaderGroupDisplayValue)) {
            m_thead = childFrame->asFrameTableSectionBox();
        }
        if (!m_tfoot && (child->style()->display() == DisplayValue::TableFooterGroupDisplayValue)) {
            m_tfoot = childFrame->asFrameTableSectionBox();
        }
        return;
    }

    if (child->isCharacterData() || child->isComment()) {
        // TODO
        return;
    } else if (wrapInAnnoymousSection) {
        // return nullptr, if buildFrameTableSection reuse before anonymouse section
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

    // 2 calculate the table width
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(style()->borderSpacing().fixed());
    LayoutUnit tableWidth = 0;
    tableWidth += marginWidth() + borderWidth() + paddingWidth();
    tableWidth += borderSpacing;
    for (auto& colSize : m_columnWidths) {
        tableWidth += colSize.maxCellWidth + borderSpacing;
    }

    // 3 Adjust the width of each cell if the parent width is smaller than
    // the preferred width of the table.
    // The width of each cell is reduced to proportion to the ratio of
    // the width of the cell over the total table width.
    // Cells with "width: auto" are not affected here, but adjusted later
    // this function if further rooms are needed.
    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    if (tableWidth > parentContentWidth) {
        LayoutUnit availableWidth = parentContentWidth;
        availableWidth -= marginWidth() + borderWidth() + paddingWidth();
        availableWidth -= borderSpacing + (borderSpacing * m_columnWidths.size());

        // 3.1 Calculate the ratio of which each column is to be reduced.
        //     The cell width is:
        //     minCellWidth <= cellWidth <= maxCellWidth
        LayoutUnit totalCellWidths = 0;
        for (auto& col : m_columnWidths) {
            totalCellWidths += col.maxCellWidth;
        }

        // 3.2 Check whether it is ok to reduce the width of each cell.
        // The minimum width of a cell should be no less than
        // the preferred min width of the cell.
        std::vector<ColSizeStruct*> columnsWithUserDefinedWidths;
        std::vector<ColSizeStruct*> columnsAdjustedToMinWidths;
        std::vector<ColSizeStruct*> columnsToAdjustWidths;
        for (unsigned i = 0; i < m_columnWidths.size(); i++) {
            ColSizeStruct& col = m_columnWidths[i];
            LayoutUnit newCellWidth(availableWidth.toDouble() *
                (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));

            if (i < m_cellsInTheFirstRow.size() && !(m_cellsInTheFirstRow[i]->style()->width().isAuto())) {
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

        // 3.3 Now reduce the width of each cell
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

        // 3.4 Blink further reduces column widths if all columns with
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
    // 1. Captions that has property "caption-side: top"
    //    If there are multiple captions, place them in document order
    // 2. Table sections in document order
    // 3. Captions that has property "caption-side: bottom"
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

    // 2-1. place the first table header section
    if (m_thead) {
        m_thead->asFrameBox()->setX(paddingLeft());
        m_thead->asFrameTableSectionBox()->layoutHeight(ctx);
        m_thead->asFrameBox()->setY(ySoFar);
        ySoFar += m_thead->asFrameBox()->height();
    }

    // 2-2. place table-row-group and the rest
    // "table-header/footer-group" sections
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox() && (c != m_thead) && (c != m_tfoot)) {
            c->asFrameBox()->setX(paddingLeft());
            c->asFrameTableSectionBox()->layoutHeight(ctx);
            c->asFrameBox()->setY(ySoFar);
            ySoFar += c->asFrameBox()->height();
        }
    }

    // 2-3. place the first table footer section
    // Similar logic as table-header-group applies to table-footer-group.
    if (m_tfoot) {
        m_tfoot->asFrameBox()->setX(paddingLeft());
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

// Table draws the border around the TableFrameSections
void FrameTableBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Fill in the table with background color
    LayoutRect bgRect(m_tableRect.x() + borderLeft(), m_tableRect.y() + borderTop(),
        m_tableRect.width() - borderWidth(), m_tableRect.height() - borderHeight());
    paintBackground(canvas, style(), bgRect, m_tableRect, false);

    paintBorders(canvas, m_tableRect);
}

}
