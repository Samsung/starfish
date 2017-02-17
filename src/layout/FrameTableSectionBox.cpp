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
#include "FrameTableSectionBox.h"

#include "FrameTreeBuilder.h"
#include "FrameTableBox.h"
#include "FrameTableRowBox.h"
#include "FrameTableCellBox.h"

namespace StarFish {

RowStruct::RowStruct(FrameTableRowBox* tableRow_)
    : tableRow(tableRow_)
{
    for (Frame* cell = tableRow->firstChild(); cell; cell = cell->next()) {
        if (cell->isFrameTableCellBox()) {
            cells.push_back(CellStruct(cell->asFrameTableCellBox()));
        }
    }
}

FrameTableSectionBox::FrameTableSectionBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{

}

FrameTableSectionBox* FrameTableSectionBox::buildFrameTableSectionBox(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableSectionBox* tableSection;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isTableSection = current->style()->display() == DisplayValue::TableRowGroupDisplayValue
        || current->style()->display() == DisplayValue::TableHeaderGroupDisplayValue
        || current->style()->display() == DisplayValue::TableFooterGroupDisplayValue;

    if (isTableSection) {
        tableSection = new FrameTableSectionBox(current, nullptr);
        current->setFrame(tableSection);
    } else {
        // If the current node is not a table row-group node, make either
        // * an anonymous table row-group box, or
        // * use the last anonymous row-group box if it has already been created
        //   by a previous (and continuous) sibling of the current node.
        Frame* before = parent->lastChild();

        if (before && before->isAnonymous() && before->isFrameTableSectionBox()) {
            tableSection = before->asFrameTableSectionBox();
        } else {
            tableSection = FrameTableSectionBox::createAnonymousWithParent(parent, current);
        }
    }

    ctx.setCurrentBlockContainer(tableSection);
    ctx.mergeTextDecorationData(tableSection->style());

    if (isTableSection) {
        unsigned i = 0;
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            FrameTableRowBox* tableRow = tableSection->addChild(c, ctx, force);
            // TODO: After implementing anonymous boxes, replace the null check
            // with assert()
            if (tableRow) {
                tableRow->setRowIndex(i);
                i++;
                RowStruct row(tableRow);
                tableSection->grid().push_back(row);
            }
        }
    } else if (tableSection->isAnonymous()) {
        FrameTableRowBox* tableRow;
        tableRow = tableSection->addChild(current, ctx, force);
        if (tableRow != nullptr) {
            tableRow->setRowIndex(tableSection->grid().size());
            RowStruct row(tableRow);
            tableSection->grid().push_back(row);
            STARFISH_ASSERT(tableRow->parent());
        }
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parent);
    return tableSection->parent() ? nullptr : tableSection;
}

FrameTableCellBox* FrameTableCellBox::createAnonymousWithParent(FrameBlockBox* parent, Node* parentNode)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(parentNode);
    style->arrangeStyleValues(parent->style(), parentNode);

    return new FrameTableCellBox(nullptr, style);
}

FrameTableSectionBox* FrameTableSectionBox::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowGroupDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableSectionBox(nullptr, style);
}

FrameTableRowBox* FrameTableSectionBox::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableRowBox* childFrame;

    if (child->style()->display() == DisplayValue::TableRowDisplayValue) {
        childFrame = FrameTableRowBox::buildFrameTableRow(child, ctx, force);
        FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
        STARFISH_ASSERT(childFrame->parent());
        return childFrame;
    }

    // TODO
    if (child->isCharacterData() || child->isComment()) {
        return nullptr;
    } else {
        // please read comment in FrameTableBox::addChild
        childFrame = FrameTableRowBox::buildFrameTableRow(child, ctx, force);
        if (childFrame != nullptr) {
            FrameTableSectionBox* tableSection = ctx.currentBlockContainer()->asFrameTableSectionBox();
            tableSection->appendChild(childFrame);
            childFrame->setRowIndex(tableSection->grid().size());
            RowStruct row(childFrame);
            tableSection->grid().push_back(row);
            STARFISH_ASSERT(childFrame->parent());
        }
        return childFrame;
    }
}

void FrameTableSectionBox::calCellWidth(LayoutContext& ctx)
{
    // 0. We traverse the cells first to determine min/max cell size
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRowBox()) {
            c->asFrameTableRowBox()->calCellWidth(ctx);
        } else {
            // Only FrameTableRow should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // 1. get max logical column size
    unsigned logicalColSize = 0;
    for (unsigned i = 0; i < m_grid.size(); i++) {
        RowStruct& row = m_grid[i];
        logicalColSize = std::max<unsigned>(logicalColSize, row.cells.size());
    }

    // 2. get min/max column width for each column that does not have a colspan
    m_columnWidths.clear();
    for (unsigned c = 0; c < logicalColSize; c++) {
        LayoutUnit minCellWidthSoFar = 0;
        LayoutUnit maxCellWidthSoFar = 0;
        for (unsigned r = 0; r < m_grid.size(); r++) {
            RowStruct& row = m_grid[r];
            if (c < row.cells.size()) {
                FrameTableCellBox* cell = row.cells[c].cell;
                minCellWidthSoFar =
                    std::max(minCellWidthSoFar, cell->minCellWidth());
                maxCellWidthSoFar =
                    std::max(maxCellWidthSoFar, cell->maxCellWidth());
            }
        }
        ColSizeStruct col;
        col.minCellWidth = minCellWidthSoFar;
        col.maxCellWidth = maxCellWidthSoFar;
        col.cellWidth = maxCellWidthSoFar;
        m_columnWidths.push_back(col);
    }

    // 3. TODO: increase column widths to fit the columns with colspans.
}

void FrameTableSectionBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit xSoFar = 0;
    LayoutUnit maxWidth = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRowBox()) {
            c->asFrameTableRowBox()->layoutWidth(ctx);
            c->asFrameBox()->setX(xSoFar);
            maxWidth = std::max(maxWidth, c->asFrameBox()->width());
        } else {
            // Only FrameTableRow should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // The width of all rows should be the same, so ideally, the maxWidth
    // should be the same as the width of any row.
    setWidth(tableBox()->borderLeft() + maxWidth + tableBox()->borderRight());
}

void FrameTableSectionBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit ySoFar = 0;
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(tableBox()->style()->borderSpacing().fixed());

    if (isFirstTableSection()) {
        ySoFar += borderSpacing;
    }

    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRowBox()) {
            c->asFrameBox()->setY(ySoFar);
            c->asFrameTableRowBox()->layoutHeight(ctx);
            ySoFar += c->asFrameBox()->height();
            ySoFar += borderSpacing;
        } else {
            // Only FrameTableRow should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    setHeight(ySoFar);
}

bool FrameTableSectionBox::isFirstTableSection()
{
    for (Frame* c = tableBox()->firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            if (c == this) {
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

void FrameTableSectionBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

}
