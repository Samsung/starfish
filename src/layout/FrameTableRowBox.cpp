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
#include "FrameTableRowBox.h"

#include "FrameTreeBuilder.h"
#include "FrameTableBox.h"
#include "FrameTableCellBox.h"
#include "FrameTableSectionBox.h"

namespace StarFish {

FrameTableRowBox::FrameTableRowBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{

}

FrameTableRowBox* FrameTableRowBox::buildFrameTableRow(Node* current,
    FrameTreeBuilderContext& ctx, bool force)
{
    FrameBlockBox* parent = ctx.currentBlockContainer();
    FrameTableRowBox* tableRow;
    bool isTableRow = current->style()->display() == DisplayValue::TableRowDisplayValue;

    if (isTableRow) {
        tableRow = new FrameTableRowBox(current, nullptr);
        current->setFrame(tableRow);
    } else {
        // current node is not tableRow then make anonymous Row or
        // reuse last anonymous Row
        Frame* before = parent->lastChild();
        if (before && before->isAnonymous() && before->isFrameTableRowBox()) {
            tableRow = before->asFrameTableRowBox();
        } else {
            tableRow = FrameTableRowBox::createAnonymousWithParent(parent, current);
        }
    }

    ctx.setCurrentBlockContainer(tableRow);
    ctx.mergeTextDecorationData(tableRow->style());

    FrameTableCellBox* tableCell;
    if (isTableRow) {
        unsigned i = 0;
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            tableCell = tableRow->addChild(c, ctx, force);
            // TODO: need to calculate absoluteColumnIndex
            // After implementing anonymous boxes, replace the null check with assert()
            if (tableCell) {
                tableCell->setAbsoluteColumnIndex(i);
                i += tableCell->colspan();
            }
        }
    } else if (tableRow->isAnonymous()) {
        // set cell index value if reuse last anonymous table row
        tableCell = tableRow->addChild(current, ctx, force);
        if (tableCell != nullptr) {
            unsigned cellIndex = 0;
            for (Frame* c = tableRow->firstChild(); c; c = c->next()) {
                if (c->isFrameTableCellBox())
                    cellIndex += c->asFrameTableCellBox()->colspan();
            }
            tableCell->setAbsoluteColumnIndex(cellIndex);
            STARFISH_ASSERT(tableCell->parent());
        }
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parent);
    if (tableRow->parent()) {
        parent->asFrameTableSectionBox()->grid()[tableRow->rowIndex()].cells.push_back(CellStruct(tableCell->asFrameTableCellBox()));
        return nullptr;
    }
    return tableRow;
}

FrameTableRowBox* FrameTableRowBox::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableRowBox(nullptr, style);
}

FrameTableCellBox* FrameTableRowBox::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableCellBox* childFrame;

    if (child->style()->display() == DisplayValue::TableCellDisplayValue) {
        childFrame = FrameTableCellBox::buildFrameTableCell(child, ctx, force);
        FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
        STARFISH_ASSERT(childFrame->parent());
        return childFrame;
    }

    // TODO
    if (child->isCharacterData() || child->isComment()) {
        return nullptr;
    } else {
        // return nullptr, if buildFrameTableCell resuse before anonymous cell
        childFrame = FrameTableCellBox::buildFrameTableCell(child, ctx, force);
        if (childFrame != nullptr) {
            ctx.currentBlockContainer()->appendChild(childFrame);
        }
        return childFrame;
    }
}

void FrameTableRowBox::calCellWidth(LayoutContext& ctx)
{
    // We traverse the cells first to calculate min/max cell width
    unsigned i = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            c->asFrameTableCellBox()->calCellWidth(ctx, i, Frame::LayoutWantToResolve::ResolveWidth);
            i++;
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

void FrameTableRowBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit xSoFar = 0;
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(sectionBox()->tableBox()->style()->borderSpacing().fixed());

    if (firstChild()) {
        xSoFar += borderSpacing;
    }

    unsigned i = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameBox* cell = c->asFrameTableCellBox();
            cell->setX(xSoFar);
            STARFISH_ASSERT(i < sectionBox()->tableBox()-> columnWidths().size());
            LayoutUnit cellWidth = sectionBox()->tableBox()-> columnWidths()[i].cellWidth;
            cell->setWidth(cellWidth);
            cell->asFrameTableCellBox()->layoutWidth(ctx);
            xSoFar += cellWidth;
            xSoFar += borderSpacing;
            i++;
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    setWidth(xSoFar);
}

void FrameTableRowBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit maxHeightSoFar = 0;
    // 1. We make the second iteration of cells to layout cells
    //    and calculate the width of each cell
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            cell->layoutHeight(ctx);
            LayoutUnit cellHeight = cell->height();
            maxHeightSoFar = std::max(maxHeightSoFar, cellHeight);
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // 2. The height of each cell is set to the max height of the cells
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            cell->setHeight(maxHeightSoFar);
        }
    }

    setHeight(maxHeightSoFar);

    // 3. Place the contents of each cell according to the vertical-align of
    //    each cell
    m_baseline = calBaseline();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            c->asFrameTableCellBox()->applyVerticalAlign();
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

LayoutUnit FrameTableRowBox::calBaseline()
{
    LayoutUnit maxSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            maxSoFar = std::max(maxSoFar, c->asFrameTableCellBox()->calBaseline());
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    return maxSoFar;
}

void FrameTableRowBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void FrameTableRowBox::paintBackgroundAndBorders(Canvas* canvas)
{
    Frame* child = firstChild();
    while (child) {
        if (child->isFrameTableCellBox()) {
            FrameTableCellBox* cell = child->asFrameTableCellBox();

            LayoutRect rect(
                cell->x(),
                cell->y(),
                cell->frameRect().width(),
                cell->frameRect().height()
            );
            paintBackground(canvas, style(), rect, rect, false);
        }
        child = child->next();
    }
    paintBorders(canvas, m_frameRect);
}

}
