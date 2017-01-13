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
#include "FrameTable.h"

#include "FrameTreeBuilder.h"
#include "FrameTableCaption.h"
#include "FrameTableSection.h"

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

FrameTable::FrameTable(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

FrameTable* FrameTable::buildFrameTable(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTable* tableWrapper;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    if (current->isTable()) {
        // if current node is table then make wrapper and current node owns this wrapper
        tableWrapper = new FrameTable(current, nullptr);
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

        if (before && before->isAnonymous() && before->isFrameTable()) {
            tableWrapper = before->asFrameTable();
        } else {
            tableWrapper = FrameTable::createAnonymousWithParent(parent, current);
        }

    }

    // Table establishes a new block context
    ctx.setCurrentBlockContainer(tableWrapper);
    ctx.mergeTextDecorationData(tableWrapper->style());

    if (current->isTable()) {
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            tableWrapper->addChild(c, ctx, force);
        }
    } else if (tableWrapper->isAnonymous()){
        tableWrapper->addChild(current, ctx, force);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parent);
    return tableWrapper->parent()? nullptr : tableWrapper;
}

FrameTable* FrameTable::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTable(nullptr, style);
}

void FrameTable::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    bool wrapInAnnoymousSection = false;
    Frame* childFrame;

    if (child->isTableCaption()) {
        childFrame = FrameTableCaption::buildFrameTableCaption(child, ctx, force);
        m_captions.push_back(childFrame->asFrameTableCaption());
    } else if (child->isTableCol()) {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else if (child->isTableSection()) {
        // Only [TableHeaderGroup | TableFooterGroup | TableRowGroup] should appear
        switch (child->style()->display()) {
        case DisplayValue::TableHeaderGroupDisplayValue:
        case DisplayValue::TableFooterGroupDisplayValue:
        case DisplayValue::TableRowGroupDisplayValue:
            childFrame = FrameTableSection::buildFrameTableSection(child, ctx, force);
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
        return;
    }

    if (child->isCharacterData() || child->isComment()) {
        // TODO
        return;
    } else if (wrapInAnnoymousSection) {
        // return nullptr, if buildFrameTableSection reuse before anonymouse section
        childFrame = FrameTableSection::buildFrameTableSection(child, ctx, force);
        if (childFrame != nullptr) {
            ctx.currentBlockContainer()->appendChild(childFrame);
            STARFISH_ASSERT(childFrame->parent());
        }
        return;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTable::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
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

void FrameTable::calCellWidth(LayoutContext& ctx)
{
    // 1. We traverse the table to calculate min/max cell widths of the table
    //    before we perform table layout.
    m_columnWidths.clear();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            c->asFrameTableSection()->calCellWidth(ctx);
            collectColumnWidths(m_columnWidths, c->asFrameTableSection()->columnWidths());
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
    tableWidth += LayoutUnit::fromPixel(style()->borderLeftWidth().fixed());
    tableWidth += LayoutUnit::fromPixel(style()->borderRightWidth().fixed());
    tableWidth += borderSpacing;
    for (auto& colSize : m_columnWidths) {
        tableWidth += colSize.maxCellWidth + borderSpacing;
    }

    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    if (tableWidth > parentContentWidth) {
        LayoutUnit availableWidth = parentContentWidth;
        availableWidth -= LayoutUnit::fromPixel(style()->borderLeftWidth().fixed());
        availableWidth -= LayoutUnit::fromPixel(style()->borderRightWidth().fixed());
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

            if (newCellWidth.floor() < col.minCellWidth) {
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
                col.cellWidth = std::max(col.minCellWidth.toInt(), newCellWidth.floor());
            }
        }
    }
}

void FrameTable::layoutWidth(LayoutContext& ctx)
{
    // The width of the caption is limited by the max width of the
    // FrameTableSection. Hence, captions can only be placed after calculating
    // the width of the table, which has already been done by calContentWidth()
    LayoutUnit maxWidth = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            c->asFrameTableSection()->layoutWidth(ctx);
            maxWidth = std::max(maxWidth, c->asFrameBox()->width());
        } else if (c->isFrameTableCaption()) {
            c->asFrameTableCaption()->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
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

void FrameTable::layoutHeight(LayoutContext& ctx)
{
    // Table is placed in the following order:
    // 1. Captions that has property "caption-side: top"
    //    If there are multiple captions, place them in document order
    // 2. Table sections in document order
    // 3. Captions that has property "caption-side: bottom"
    //    If there are multiple captions, place them in document order

    LayoutUnit ySoFar = LayoutUnit::fromPixel(style()->borderTopWidth().fixed());
    LayoutUnit topCaptionHeightsSoFar = 0;

    // 1. place captions with caption-side: top
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::TopCaptionSideValue) {
            caption->setWidth(width());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
            topCaptionHeightsSoFar += caption->height();
        }
    }

    // 2. place table sections
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            c->asFrameTableSection()->layoutHeight(ctx);
            c->asFrameBox()->setY(ySoFar);
            ySoFar += c->asFrameBox()->height();
        }
    }

    // 3. place captions with caption-side: bottom
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::BottomCaptionSideValue) {
            caption->setWidth(width());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
        }
    }

    ySoFar += LayoutUnit::fromPixel(style()->borderBottomWidth().fixed());
    setHeight(ySoFar);
}

void FrameTable::collectColumnWidths(GCVector<ColSizeStruct>& columnWidthsSoFar, GCVector<ColSizeStruct>& columnWidths)
{
    // FIXME: absolute at this stage
    // Need to consider absolute and logical columns
    if (columnWidthsSoFar.empty()) {
        columnWidthsSoFar = columnWidths;
    } else {
        // Update to support colspans
        STARFISH_ASSERT(columnWidthsSoFar.size() == columnWidths.size());
        for (unsigned i = 0; i < columnWidths.size(); i++) {
            ColSizeStruct& colSoFar = columnWidthsSoFar[i];
            ColSizeStruct& col = columnWidths[i];
            colSoFar.maxCellWidth = std::max(colSoFar.maxCellWidth, col.maxCellWidth);
            colSoFar.minCellWidth = std::max(colSoFar.minCellWidth, col.minCellWidth);
            colSoFar.cellWidth = colSoFar.maxCellWidth;
        }
    }
}

}
