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
#include "FrameTableRowBox.h"

#include "FrameTreeBuilder.h"
#include "FrameTableBox.h"
#include "FrameTableCellBox.h"
#include "FrameTableSectionBox.h"

namespace StarFish {

FrameTableRowBox::FrameTableRowBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_rowIndex(0)
    , m_lastAbsoluteColumnIndex(0)
{
}

FrameTableRowBox* FrameTableRowBox::buildFrameTableRow(
    Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableRowBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isTableRow =
        current->style()->display() == DisplayValue::TableRowDisplayValue;

    if (isTableRow) {
        currentFrame = new FrameTableRowBox(current, nullptr);
        current->setFrame(currentFrame);
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            currentFrame->addChild(c, ctx, force);
        }
    } else {
        // please read comment in FrameTableBox::buildFrameTable
        Frame* before = parent->lastChild();
        if (before && before->isAnonymous() && before->isFrameTableRowBox()) {
            currentFrame = before->asFrameTableRowBox();
        } else {
            currentFrame =
                FrameTableRowBox::createAnonymousWithParent(parent, current);
        }
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        currentFrame->addChild(current, ctx, force);
    }

    ctx.setCurrentBlockContainer(parent);
    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

FrameTableRowBox* FrameTableRowBox::createAnonymousWithParent(
    FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableRowBox(nullptr, style);
}

void FrameTableRowBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                bool force)
{
    STARFISH_ASSERT(ctx.currentBlockContainer()->isFrameTableRowBox());
    if (child->isComment() ||
        (child->isCharacterData() &&
         child->textContent()->containsOnlyWhitespace())) {
        // TODO, do not use assert!
        return;
    } else {
        FrameTableRowBox* parentRow =
            ctx.currentBlockContainer()->asFrameTableRowBox();
        FrameTableCellBox* childFrame =
            FrameTableCellBox::buildFrameTableCell(child, ctx, force);

        if (!childFrame->parent()) {
            childFrame->setAbsoluteColumnIndex(m_lastAbsoluteColumnIndex);
            m_lastAbsoluteColumnIndex += childFrame->colspan();
            ctx.currentBlockContainer()->appendChild(childFrame);
            // Put the cell manually into the rowstruct If the row is
            // an anonymous row that has already been created
            if (parentRow->isAnonymous() && parentRow->parent()) {
                RowStruct& row = parentRow->parent()
                                     ->asFrameTableSectionBox()
                                     ->grid()[parentRow->rowIndex()];
                unsigned curId = row.logicalColumnSize();
                row.cells().push_back(
                    CellStruct(childFrame->asFrameTableCellBox(), curId));
            }
        }
        STARFISH_ASSERT(childFrame->parent());
        return;
    }
}

void FrameTableRowBox::calCellWidth(LayoutContext& ctx)
{
    // We traverse the cells first to calculate min/max cell width
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            c->asFrameTableCellBox()->calCellWidth(
                ctx, Frame::LayoutWantToResolve::ResolveWidth);
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

ColSizeStruct* FrameTableRowBox::colWithColspanAt(unsigned id)
{
    if (id < m_colsWithColspans.size()) {
        if (id == m_colsWithColspans[id].id) {
            return &m_colsWithColspans[id];
        }
    }

    for (size_t i = 0; i < m_colsWithColspans.size(); i++) {
        ColSizeStruct* col = &m_colsWithColspans[i];
        if (col->id == id) {
            return col;
        }
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

void FrameTableRowBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit xSoFar = 0;
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(
        sectionBox()->tableBox()->style()->horizontalBorderSpacing().fixed());

    unsigned i = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        FrameTableCellBox* cell = c->asFrameTableCellBox();
        cell->setX(xSoFar);
        STARFISH_ASSERT(i < sectionBox()->tableBox()->columnWidths().size());

        LayoutUnit cellWidth = 0;
        if (cell->colspan() > 1) {
            cellWidth = colWithColspanAt(i)->cellWidth;
        } else {
            cellWidth = sectionBox()->tableBox()->columnWidths()[i].cellWidth;
        }

        cell->setWidth(cellWidth);
        cell->asFrameTableCellBox()->layoutWidth(ctx);
        xSoFar += cellWidth;

        if (i < sectionBox()->tableBox()->columnWidths().size() - 1) {
            xSoFar += borderSpacing;
        }

        i += cell->colspan();
    }

    setWidth(xSoFar);
}

void FrameTableRowBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit maxHeightSoFar = 0;
    // 1. We make the second iteration of cells to layout cells
    //    and calculate the width of each cell
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        FrameTableCellBox* cell = c->asFrameTableCellBox();
        cell->layoutHeight(ctx);
        LayoutUnit cellHeight = cell->height();
        maxHeightSoFar = std::max(maxHeightSoFar, cellHeight);
    }

    // 2. The height of each cell is set to the max height of the cells
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            cell->setHeight(maxHeightSoFar);
        }
    }

    LayoutUnit specifiedHeight = 0;
    if (style()->height().isFixed()) {
        specifiedHeight = LayoutUnit::fromPixel(style()->height().fixed());
    } else if (style()->height().isPercent()) {
        // The spec does not define how to calculate the height when the height
        // is specified in percentage
    }

    setHeight(std::max(maxHeightSoFar, specifiedHeight));
}

void FrameTableRowBox::increaseCellHeightBy(LayoutUnit cellHeightOffset)
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        FrameTableCellBox* cellBox = c->asFrameTableCellBox();
        cellBox->setHeight(cellBox->height() + cellHeightOffset);
    }
    setHeight(height() + cellHeightOffset);
}

void FrameTableRowBox::applyVerticalAlign()
{
    m_baseline = calBaseline();
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        c->asFrameTableCellBox()->applyVerticalAlign();
    }
}

LayoutUnit FrameTableRowBox::calBaseline()
{
    LayoutUnit maxSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        maxSoFar = std::max(maxSoFar, c->asFrameTableCellBox()->calBaseline());
    }

    return maxSoFar;
}

void FrameTableRowBox::layout(LayoutContext& ctx,
                              Frame::LayoutWantToResolve resolveWhat)
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

            LayoutRect rect(cell->x(), cell->y(), cell->frameRect().width(),
                            cell->frameRect().height());
            paintBackground(canvas, style(), rect, rect, false);
        }
        child = child->next();
    }
    paintBorders(canvas, m_frameRect);
}
}
