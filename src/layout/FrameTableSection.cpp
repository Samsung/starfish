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
#include "FrameTableSection.h"

#include "FrameTreeBuilder.h"
#include "FrameTable.h"
#include "FrameTableRow.h"
#include "FrameTableCell.h"

namespace StarFish {

RowStruct::RowStruct(FrameTableRow* tableRow_)
    : tableRow(tableRow_)
{
    for (Frame* cell = tableRow->firstChild(); cell; cell = cell->next()) {
        if (cell->isFrameTableCell()) {
            cells.push_back(CellStruct(cell->asFrameTableCell()));
        }
    }
}

FrameTableSection::FrameTableSection(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr) || (node != nullptr && style == nullptr));
}

FrameTableSection* FrameTableSection::buildFrameTableSection(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableSection* tableSection;
    FrameBlockBox* parent = ctx.currentBlockContainer();

    if (current->isTableSection()) {
        tableSection = new FrameTableSection(current, nullptr);
        current->setFrame(tableSection);
    } else {
        // current node is not tablesection then make anonymous section or
        // reuse before anonymous section
        Frame* before = parent->lastChild();

        if (before && before->isAnonymous() && before->isFrameTableSection()) {
            tableSection = before->asFrameTableSection();
        } else {
            tableSection = FrameTableSection::createAnonymousWithParent(parent, current);
        }
    }

    ctx.setCurrentBlockContainer(tableSection);
    ctx.mergeTextDecorationData(tableSection->style());

    if (current->isTableSection()) {
        unsigned i = 0;
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            FrameTableRow* tableRow = tableSection->addChild(c, ctx, force);
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
        FrameTableRow* tableRow;
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

FrameTableCell* FrameTableCell::createAnonymousWithParent(FrameBlockBox* parent, Node* parentNode)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(parentNode);
    style->arrangeStyleValues(parent->style(), parentNode);

    return new FrameTableCell(nullptr, style);
}

FrameTableSection* FrameTableSection::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowGroupDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableSection(nullptr, style);
}

FrameTableRow* FrameTableSection::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableRow* childFrame;
    if (!child->isTableRow()) {
        // TODO
        if (child->isCharacterData() || child->isComment()) {
            return nullptr;
        } else {
            // return nullptr, if buildFrameTableRow reuse before anonymous row
            childFrame = FrameTableRow::buildFrameTableRow(child, ctx, force);
            if (childFrame != nullptr) {
                FrameTableSection* tableSection = ctx.currentBlockContainer()->asFrameTableSection();
                tableSection->appendChild(childFrame);
                childFrame->setRowIndex(tableSection->grid().size());
                RowStruct row(childFrame);
                tableSection->grid().push_back(row);
                STARFISH_ASSERT(childFrame->parent());
            }
            return childFrame;
        }
    }

    childFrame = FrameTableRow::buildFrameTableRow(child, ctx, force);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
    STARFISH_ASSERT(childFrame->parent());
    return childFrame;
}

void FrameTableSection::calCellWidth(LayoutContext& ctx)
{
    // 0. We traverse the cells first to determine min/max cell size
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRow()) {
            c->asFrameTableRow()->calCellWidth(ctx);
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
                FrameTableCell* cell = row.cells[c].cell;
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

void FrameTableSection::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit xSoFar =
        LayoutUnit::fromPixel(table()->style()->borderLeftWidth().fixed());
    LayoutUnit maxWidth = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRow()) {
            c->asFrameTableRow()->layoutWidth(ctx);
            c->asFrameBox()->setX(xSoFar);
            maxWidth = std::max(maxWidth, c->asFrameBox()->width());
        } else {
            // Only FrameTableRow should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // The width of all rows should be the same, so ideally, the maxWidth
    // should be the same as the width of any row.
    setWidth(table()->style()->borderLeftWidth().fixed() +
        maxWidth + table()->style()->borderRightWidth().fixed());
}

void FrameTableSection::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit ySoFar = 0;
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(table()->style()->borderSpacing().fixed());

    if (isFirstTableSection()) {
        ySoFar += borderSpacing;
    }

    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRow()) {
            c->asFrameBox()->setY(ySoFar);
            c->asFrameTableRow()->layoutHeight(ctx);
            ySoFar += c->asFrameBox()->height();
            ySoFar += borderSpacing;
        } else {
            // Only FrameTableRow should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    setHeight(ySoFar);
}

bool FrameTableSection::isFirstTableSection()
{
    for (Frame* c = table()->firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            if (c == this) {
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

void FrameTableSection::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

}
