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
#include "FrameTableRow.h"

#include "FrameTreeBuilder.h"
#include "FrameTable.h"
#include "FrameTableSection.h"
#include "FrameTableCell.h"

namespace StarFish {

FrameTableRow::FrameTableRow(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr) ||
                    (node != nullptr && style == nullptr));
}

FrameTableRow* FrameTableRow::buildFrameTableRow(Node* rowNode,
                                                 FrameTreeBuilderContext& ctx,
                                                 bool force) {
    FrameTableRow* tableRow = new FrameTableRow(rowNode, nullptr);
    rowNode->setFrame(tableRow);

    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableRow);

    unsigned i = 0;
    for (Node* c = rowNode->firstChild(); c; c = c->nextSibling()) {
        FrameTableCell* tableCell = tableRow->addChild(c, ctx, force);
        // TODO: need to calculate absoluteColumnIndex
        // After implementing anonymous boxes, replace the null check with assert()
        if (tableCell) {
            tableCell->setAbsoluteColumnIndex(i);
            i += tableCell->colspan();
        }
    }

    ctx.setCurrentBlockContainer(lastContext);

    return tableRow;
}

FrameTableRow* FrameTableRow::createAnonymousWithParent(FrameBlockBox* parent, Node* parentNode)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(parentNode);
    style->arrangeStyleValues(parent->style(), parentNode);

    return new FrameTableRow(nullptr, style);
}

FrameTableCell* FrameTableRow::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableCell* childFrame;
    if (!child->isTableCell()) {
        // TODO
        if (child->isCharacterData() || child->isComment()) {
            return nullptr;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    childFrame = FrameTableCell::buildFrameTableCell(child, ctx, force);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(),
                                                 childFrame, child, ctx);
    STARFISH_ASSERT(childFrame->parent());
    return childFrame;
}

void FrameTableRow::calContentWidth(LayoutContext& ctx)
{
    // We traverse the cells first to calculate min/max cell width
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCell()) {
            c->asFrameTableCell()->calContentWidth(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

void FrameTableRow::layoutWidth(LayoutContext& ctx)
{
    int i=0;
    LayoutUnit xSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCell()) {
            c->asFrameBox()->setX(xSoFar);
            LayoutUnit cellWidth = tableSection()->table()->
                                       columnWidths()[i].maxContentWidth;
            c->asFrameTableCell()->setContentWidth(cellWidth);
            xSoFar += cellWidth;
            i++;
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

void FrameTableRow::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit maxYSoFar = 0;
    // We traverse the cells first to calculate min/max cell width
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCell()) {
            c->asFrameTableCell()->layoutHeight(ctx);
            maxYSoFar = std::max(maxYSoFar, c->asFrameBox()->height());
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    setHeight(maxYSoFar);
}

void FrameTableRow::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

}
