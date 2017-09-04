/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/Node.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameTreeBuilder.h"

namespace StarFish {

FrameTableRowBox::FrameTableRowBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_rowIndex(0)
    , m_lastAbsoluteColumnIndex(0)
{
}

void* FrameTableRowBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTableRowBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox, m_node));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableRowBox, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableRowBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox,
                                              m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableRowBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox,
                                              m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableRowBox,
                                              m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableRowBox, m_colsWithColspans));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableRowBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameTableRowBox::calCellWidth(LayoutContext& ctx)
{
    // We traverse the cells first to calculate min/max cell width
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        c->asFrameTableCellBox()->calCellWidth(
            ctx, Frame::LayoutWantToResolve::ResolveWidth);
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
        if (cell->updatedColspan() > 1) {
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

        i += cell->updatedColspan();
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
        ctx.pushBlockBoxAligningAtFirstBaseline(cell);
        cell->layoutHeight(ctx);
        auto it = ctx.firstLineAscender(cell);
        if (it.hasValue()) {
            ctx.tempReigsterFirstLineAscender(cell, it.getValue());
        }
        ctx.popBlockBoxAligningAtFirstBaseline();
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

    // layout absolute positioned blocks
    ctx.layoutRegisteredAbsolutePositionedBoxes(this);

    // layout relative positioned blocks
    ctx.layoutRegisteredRelativePositionedBoxes(this);
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

void FrameTableRowBox::applyVerticalAlign(LayoutContext& ctx)
{
    m_baseline = calBaseline(ctx);
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        c->asFrameTableCellBox()->applyVerticalAlign(ctx);
    }
}

LayoutUnit FrameTableRowBox::calBaseline(LayoutContext& ctx)
{
    LayoutUnit maxSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        STARFISH_ASSERT(c->isFrameTableCellBox());
        maxSoFar =
            std::max(maxSoFar, c->asFrameTableCellBox()->calBaseline(ctx));
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
