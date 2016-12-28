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
#include "FrameTableCell.h"

#include "FrameTreeBuilder.h"

namespace StarFish {

FrameTableCell::FrameTableCell(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr) || (node != nullptr && style == nullptr));
}

FrameTableCell* FrameTableCell::buildFrameTableCell(Node* cellNode,
                                                    FrameTreeBuilderContext& ctx,
                                                    bool force) {
    printf("FrameTableCell::buildFrameTableCell\n");
    FrameTableCell* tableCell = new FrameTableCell(cellNode, nullptr);
    cellNode->setFrame(tableCell);

    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableCell);

    for(Node* c = cellNode->firstChild(); c; c = c->nextSibling()) {
        FrameTreeBuilder::buildTree(c, ctx, force);
    }

    ctx.setCurrentBlockContainer(lastContext);

    return tableCell;
}

void FrameTableCell::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    printf("FrameTableCell::layout\n");
    FrameBlockBox::layout(ctx, resolveWhat);
}


}
