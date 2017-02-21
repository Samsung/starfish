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
#include "FrameTableColGroupBox.h"
#include "FrameTableColBox.h"
#include "FrameTreeBuilder.h"

namespace StarFish {

FrameTableColGroupBox::FrameTableColGroupBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{

}

FrameTableColGroupBox* FrameTableColGroupBox::buildFrameTableColGroupBox(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    STARFISH_ASSERT(ctx.currentBlockContainer()->isFrameTableBox());

    FrameTableColGroupBox* colGroupBox;
    FrameBlockBox* parentWrapperBox = ctx.currentBlockContainer();
    bool isColGroup = current->style()->display() == DisplayValue::TableColumnGroupDisplayValue;

    if (isColGroup) {
        colGroupBox = new FrameTableColGroupBox(current, nullptr);
        current->setFrame(colGroupBox);
    } else {
        // If the current node is not a colGroup node, make either
        // * an anonymous table colGroup box, or
        // * use the last anonymous table colGroup box if it has already been created
        //   by a previous (and continuous) sibling of the current node.
        Frame* last = parentWrapperBox->lastChild();
        if (last && last->isAnonymous() && last->isFrameTableColGroupBox()) {
            colGroupBox = last->asFrameTableColGroupBox();
        } else {
            colGroupBox = FrameTableColGroupBox::createAnonymousWithParent(parentWrapperBox, current);
        }
    }

    ctx.setCurrentBlockContainer(colGroupBox);
    ctx.mergeTextDecorationData(colGroupBox->style());

    if (isColGroup) {
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            colGroupBox->addChild(c, ctx, force);
        }
    } else if (colGroupBox->isAnonymous()) {
        colGroupBox->addChild(current, ctx, force);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parentWrapperBox);

    // allways return a pointer of Frame object
    return colGroupBox;
}

FrameTableColGroupBox* FrameTableColGroupBox::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableColumnGroupDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableColGroupBox(nullptr, style);
}

void FrameTableColGroupBox::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableColBox* childFrame;

    if (child->style()->display() == DisplayValue::TableColumnDisplayValue) {
        // FrameTableCol must have no children frame, so don't build the sub frame-tree
        childFrame = new FrameTableColBox(child, nullptr);
        ctx.currentBlockContainer()->appendChild(childFrame);
        child->setFrame(childFrame);
        STARFISH_ASSERT(childFrame->parent());
        return;
    }
    // Ignore the child node which display value is not 'TableColumnDisplayValue'.
    return;
}

unsigned FrameTableColGroupBox::span()
{
    unsigned ret = 0;
    if (node() && node()->asElement()->asHTMLElement()->isHTMLColGroupElement()) {
        String* span = node()->asElement()->asHTMLElement()->asHTMLColGroupElement()->span();
        ret = String::parseInt(span);
    }
    // span is only accepted when HTML element is either <col> or <colGroup>,
    // hence it is not applied when used in other elements.
    // e.g., <div style="display: table-column" span="2">
    // In this case, we ignore the span value

    // If span is not defined, use 1 as the default value
    return ret == 0 ? 1 : ret;
}

void FrameTableColGroupBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    STARFISH_ASSERT_NOT_REACHED();
}

}
