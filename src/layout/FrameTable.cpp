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
#include "FrameTable.h"

#include "FrameTableCaption.h"
#include "FrameTreeBuilder.h"

namespace StarFish {

FrameTable::FrameTable(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr) ||
                    (node != nullptr && style == nullptr));
}

FrameTable* FrameTable::buildFrameTable(Node* tableNode,
                                        FrameTreeBuilderContext& ctx,
                                        bool force = false)
{
    FrameTable* tableWrapper = new FrameTable(tableNode, nullptr);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(),
                                                 tableWrapper, tableNode, ctx);
    tableNode->setFrame(tableWrapper);

    // Table establishes a new block context
    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableWrapper);
    ctx.mergeTextDecorationData(tableWrapper->style());

    // Create either FrameTableCaptions or a FrameTableSection
    for (Node* c = tableNode->firstChild(); c; c = c->nextSibling()) {
        if (c->style()->display() == DisplayValue::TableCaptionDisplayValue) {
            FrameTableCaption* captionFrame =
                    FrameTableCaption::buildFrameTableCaption(c, ctx, force);

            FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(),
                                                         captionFrame, c, ctx);
            STARFISH_ASSERT(captionFrame->parent());
            tableWrapper->m_captions.push_back(captionFrame);
        }
    }

    ctx.setCurrentBlockContainer(lastContext);

    return tableWrapper;
}

void FrameTable::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, resolveWhat);
}

}
