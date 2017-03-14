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
#include "FrameTableSectionBox.h"

#include "FrameTreeBuilder.h"
#include "FrameTableBox.h"
#include "FrameTableRowBox.h"
#include "FrameTableCellBox.h"
#include "FrameTableColBox.h"

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

FrameTableSectionBox* FrameTableSectionBox::buildFrameTableSectionBox(
    Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableSectionBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isTableSection = (current->style()->display() ==
                           DisplayValue::TableRowGroupDisplayValue) ||
                          (current->style()->display() ==
                           DisplayValue::TableHeaderGroupDisplayValue) ||
                          (current->style()->display() ==
                           DisplayValue::TableFooterGroupDisplayValue);

    if (isTableSection) {
        currentFrame = new FrameTableSectionBox(current, nullptr);
        current->setFrame(currentFrame);
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            currentFrame->addChild(c, ctx, force);
        }
    } else {
        // please read comment in FrameTableBox::buildFrameTable
        Frame* before = parent->lastChild();

        if (before && before->isAnonymous() &&
            before->isFrameTableSectionBox()) {
            currentFrame = before->asFrameTableSectionBox();
        } else {
            currentFrame = FrameTableSectionBox::createAnonymousWithParent(
                parent, current);
        }
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        currentFrame->addChild(current, ctx, force);
    }

    ctx.setCurrentBlockContainer(parent);
    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

FrameTableSectionBox* FrameTableSectionBox::createAnonymousWithParent(
    FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowGroupDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableSectionBox(nullptr, style);
}

void FrameTableSectionBox::paintBackgroundAndBorders(Canvas* canvas)
{
    for (const auto& rowStruct : m_grid) {
        for (auto& cellStruct : rowStruct.cells) {
            LayoutRect rect(rowStruct.tableRow->x() + cellStruct.cell->x(),
                            rowStruct.tableRow->y() + cellStruct.cell->y(),
                            cellStruct.cell->frameRect().width(),
                            cellStruct.cell->frameRect().height());

            FrameTableColBox* col = tableBox()->columnAtAbsoluteColumnIndex(
                cellStruct.cell->absoluteColumnIndex());
            if (col) {
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

void FrameTableSectionBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                    bool force)
{
    STARFISH_ASSERT(ctx.currentBlockContainer()->isFrameTableSectionBox());
    if ((child->style()->display() == DisplayValue::TableRowDisplayValue) ||
        (child->style()->display() == DisplayValue::TableCellDisplayValue)) {
        FrameTableSectionBox* parentSection =
            ctx.currentBlockContainer()->asFrameTableSectionBox();
        FrameTableRowBox* childFrame =
            FrameTableRowBox::buildFrameTableRow(child, ctx, force);

        if (!childFrame->parent()) {
            childFrame->setRowIndex(parentSection->grid().size());
            RowStruct row(childFrame);
            parentSection->grid().push_back(row);
            ctx.currentBlockContainer()->appendChild(childFrame);
        }
        STARFISH_ASSERT(childFrame->parent());
    } else if (child->isCharacterData() || child->isComment()) {
        // TODO, do not use assert!
    } else {
        // TODO
        STARFISH_ASSERT_NOT_REACHED();
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
    size_t logicalColSize = 0;
    for (size_t i = 0; i < m_grid.size(); i++) {
        RowStruct& row = m_grid[i];
        logicalColSize = std::max<size_t>(logicalColSize, row.cells.size());
    }

    // 2. get min/max column width for each column that does not have a colspan
    m_columnWidths.clear();
    for (size_t c = 0; c < logicalColSize; c++) {
        LayoutUnit minCellWidthSoFar = 0;
        LayoutUnit maxCellWidthSoFar = 0;
        LayoutUnit maxSpecifiedWidth = 0;
        for (size_t r = 0; r < m_grid.size(); r++) {
            RowStruct& row = m_grid[r];
            if (c < row.cells.size()) {
                FrameTableCellBox* cell = row.cells[c].cell;
                minCellWidthSoFar =
                    std::max(minCellWidthSoFar, cell->minCellWidth());
                maxCellWidthSoFar =
                    std::max(maxCellWidthSoFar, cell->maxCellWidth());
                if (cell->style()->width().isFixed()) {
                    LayoutUnit width = cell->style()->width().fixed();
                    width += cell->borderWidth() + cell->paddingWidth();
                    maxSpecifiedWidth = std::max(maxSpecifiedWidth, width);
                } else if (cell->style()->width().isPercent()) {
                    // Not doing anything at this stage
                }
            }
        }
        ColSizeStruct col;
        col.id = c;
        col.maxSpecifiedWidth = maxSpecifiedWidth;
        col.minCellWidth = minCellWidthSoFar;
        col.maxCellWidth = maxCellWidthSoFar;
        col.cellWidth = maxCellWidthSoFar;
        m_columnWidths.push_back(col);
    }

    // 3. TODO: increase column widths to fit the columns with colspans.
}

void FrameTableSectionBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        tableBox()->style()->horizontalBorderSpacing().fixed());

    LayoutUnit xSoFar = borderSpacing;
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
    setWidth(borderSpacing + maxWidth + borderSpacing);
}

void FrameTableSectionBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit ySoFar = 0;
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        tableBox()->style()->verticalBorderSpacing().fixed());
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

void FrameTableSectionBox::layout(LayoutContext& ctx,
                                  Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}
}
