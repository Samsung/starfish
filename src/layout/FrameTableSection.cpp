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

FrameTableSection* FrameTableSection::buildFrameTableSection(Node* sectionNode, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableSection* tableSection = new FrameTableSection(sectionNode, nullptr);
    sectionNode->setFrame(tableSection);

    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableSection);

    unsigned i = 0;
    for (Node* c = sectionNode->firstChild(); c; c = c->nextSibling()) {
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

    ctx.setCurrentBlockContainer(lastContext);

    return tableSection;
}

FrameTableSection* FrameTableSection::createAnonymousWithParent(FrameBlockBox* parent, Node* parentNode)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowGroupDisplayValue);
    style->loadResources(parentNode);
    style->arrangeStyleValues(parent->style(), parentNode);

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
            // simple case to generate anonymous table object
            if (child->isTableCell()) {
                Frame* last = ctx.currentBlockContainer()->lastChild();
                FrameTableRow* anonymous;
                if (last == nullptr || last->node() != nullptr) {
                    anonymous = FrameTableRow::createAnonymousWithParent(ctx.currentBlockContainer(), ctx.currentBlockContainer()->node());
                    ctx.currentBlockContainer()->appendChild(anonymous);
                    childFrame = anonymous;
                } else if (last && last->isFrameTableRow() && last->node() == nullptr) {
                    // last node was placed at anonymous table row
                    // and current node that is tableCell must be placed at same anonymous table row
                    anonymous = last->asFrameTableRow();
                    childFrame = nullptr;
                } else {
                    // TODO
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                FrameBlockBox* lastContext = ctx.currentBlockContainer();
                ctx.setCurrentBlockContainer(anonymous);
                ctx.mergeTextDecorationData(anonymous->style());
                FrameTableCell* childFrameCell = FrameTableCell::buildFrameTableCell(child, ctx, force);

                FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrameCell, child, ctx);
                ctx.setCurrentBlockContainer(lastContext);
                STARFISH_ASSERT(childFrameCell->parent());
                return childFrame;
            } else {
                // TODO
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    childFrame = FrameTableRow::buildFrameTableRow(child, ctx, force);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
    STARFISH_ASSERT(childFrame->parent());
    return childFrame;
}

void FrameTableSection::calContentWidth(LayoutContext& ctx)
{
    // 0. We traverse the cells first to determine min/max cell size
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableRow()) {
            c->asFrameTableRow()->calContentWidth(ctx);
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
        LayoutUnit minContentWidthSoFar = 0;
        LayoutUnit maxContentWidthSoFar = 0;
        for (unsigned r = 0; r < m_grid.size(); r++) {
            RowStruct& row = m_grid[r];
            if (c < row.cells.size()) {
                FrameTableCell* cell = row.cells[c].cell;
                minContentWidthSoFar =
                    std::max(minContentWidthSoFar, cell->minContentWidth());
                maxContentWidthSoFar =
                    std::max(maxContentWidthSoFar, cell->maxContentWidth());
            }
        }
        ColStruct col;
        col.minContentWidth = minContentWidthSoFar;
        col.maxContentWidth = maxContentWidthSoFar;
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
    LayoutUnit ySoFar =
        LayoutUnit::fromPixel(table()->style()->borderTopWidth().fixed());
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(table()->style()->borderSpacing().fixed());

    if (firstChild()) {
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

    ySoFar += LayoutUnit::fromPixel(table()->style()->borderBottomWidth().fixed());
    setHeight(ySoFar);
}

void FrameTableSection::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

}
