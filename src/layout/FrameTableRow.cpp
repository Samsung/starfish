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
#include "FrameTableCell.h"

namespace StarFish {

FrameTableRow::FrameTableRow(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr) || (node != nullptr && style == nullptr));
}

FrameTableRow* FrameTableRow::buildFrameTableRow(Node* rowNode,
                                                 FrameTreeBuilderContext& ctx,
                                                 bool force) {
    FrameTableRow* tableRow = new FrameTableRow(rowNode, nullptr);
    rowNode->setFrame(tableRow);

    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableRow);

    for (Node* c = rowNode->firstChild(); c; c = c->nextSibling()) {
        tableRow->addChild(c, ctx, force);
    }

    ctx.setCurrentBlockContainer(lastContext);

    return tableRow;
}

void FrameTableRow::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    Frame* childFrame;
    if (!child->isTableCell()) {
        // TODO
        if (child->isCharacterData() || child->isComment()) {
            return;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    childFrame = FrameTableCell::buildFrameTableCell(child, ctx, force);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(),
                                                 childFrame, child, ctx);
    STARFISH_ASSERT(childFrame->parent());
}

void FrameTableRow::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, resolveWhat);

}

}
