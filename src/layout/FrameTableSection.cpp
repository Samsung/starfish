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
    STARFISH_ASSERT((node == nullptr && style != nullptr) ||
                    (node != nullptr && style == nullptr));
}

FrameTableSection* FrameTableSection::buildFrameTableSection(Node* sectionNode,
                                                             FrameTreeBuilderContext& ctx,
                                                             bool force) {
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

FrameTableRow* FrameTableSection::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableRow* childFrame;
    if (!child->isTableRow()) {
        // TODO
        if (child->isCharacterData() || child->isComment()) {
            return nullptr;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    childFrame = FrameTableRow::buildFrameTableRow(child, ctx, force);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(),
                                                 childFrame, child, ctx);
    STARFISH_ASSERT(childFrame->parent());
    return childFrame;
}

}
