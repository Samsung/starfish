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

#include "FrameTreeBuilder.h"
#include "FrameTableCaption.h"
#include "FrameTableSection.h"

namespace StarFish {

FrameTable::FrameTable(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr) ||
                    (node != nullptr && style == nullptr));
}

FrameTable* FrameTable::buildFrameTable(Node* tableNode,
                                        FrameTreeBuilderContext& ctx,
                                        bool force)
{
    FrameTable* tableWrapper = new FrameTable(tableNode, nullptr);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(),
                                                 tableWrapper, tableNode, ctx);
    tableNode->setFrame(tableWrapper);

    // Table establishes a new block context
    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableWrapper);
    ctx.mergeTextDecorationData(tableWrapper->style());

    for (Node* c = tableNode->firstChild(); c; c = c->nextSibling()) {
        tableWrapper->addChild(c, ctx, force);
    }

    ctx.setCurrentBlockContainer(lastContext);

    return tableWrapper;
}

void FrameTable::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    bool wrapInAnnoymousSection = false;
    Frame* childFrame;

    if (child->isTableCaption()) {
        childFrame = FrameTableCaption::buildFrameTableCaption(child, ctx, force);
        m_captions.push_back(childFrame->asFrameTableCaption());
    } else if (child->isTableCol()) {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else if (child->isTableSection()) {
        switch(child->style()->display()) {
        case DisplayValue::TableHeaderGroupDisplayValue:
            // TODO
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            break;
        case DisplayValue::TableFooterGroupDisplayValue:
            // TODO
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            break;
        case DisplayValue::TableRowGroupDisplayValue:
            childFrame = FrameTableSection::buildFrameTableSection(child, ctx, force);
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        wrapInAnnoymousSection = true;
    }

    if (!wrapInAnnoymousSection) {
        FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(),
                                                     childFrame, child, ctx);
        STARFISH_ASSERT(childFrame->parent());
    }

}

void FrameTable::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, resolveWhat);
}

}
