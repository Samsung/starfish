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
#include "FrameTableColBox.h"
#include "FrameTreeBuilder.h"

namespace StarFish {

FrameTableColBox::FrameTableColBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{
}

FrameTableColBox* FrameTableColBox::buildFrameTableColBox(
    Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    STARFISH_ASSERT(ctx.currentBlockContainer()->isFrameTableBox());

    FrameTableColBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isColGroup = current->style()->display() ==
                      DisplayValue::TableColumnGroupDisplayValue;

    if (isColGroup) {
        currentFrame = new FrameTableColBox(current, nullptr);
        current->setFrame(currentFrame);
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            currentFrame->addChild(c, ctx, force);
        }
        current->clearNeedsFrameTreeBuild();
        current->clearChildNeedsFrameTreeBuild();
    } else {
        if (current->needsFrameTreeBuild()) {
            // If the current node is not a colGroup node, make either
            // * an anonymous table colGroup box, or
            // * use the last anonymous table colGroup box if it has already
            // been created by a previous (and continuous) sibling of
            // the current node.
            Frame* last = parent->lastChild();
            // Always treat as colgroupbox, if the box is anonymous and
            // FrameTableColBox
            if (last && last->isAnonymous() && last->isFrameTableColBox()) {
                currentFrame = last->asFrameTableColBox();
            } else {
                currentFrame = FrameTableColBox::createAnonymousWithParent(
                    parent, current);
            }
            ctx.setCurrentBlockContainer(currentFrame);
            ctx.mergeTextDecorationData(currentFrame->style());
            currentFrame->addChild(current, ctx, force);
        } else if (current->childNeedsFrameTreeBuild()) {
            for (Frame* f = current->frame(); f; f = f->parent()) {
                if (f->parent() == parent) {
                    STARFISH_ASSERT(f->isAnonymous());
                    STARFISH_ASSERT(f->isFrameTableColBox());
                    currentFrame = f->asFrameTableColBox();
                    break;
                }
            }
            ctx.setCurrentBlockContainer(currentFrame);
            ctx.mergeTextDecorationData(currentFrame->style());
            currentFrame->addChild(current, ctx, force);
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }

    ctx.setCurrentBlockContainer(parent);
    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

FrameTableColBox* FrameTableColBox::createAnonymousWithParent(
    FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableColumnGroupDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableColBox(nullptr, style);
}

void FrameTableColBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                bool force)
{
    if (child->style()->display() == DisplayValue::TableColumnDisplayValue) {
        // column must have no children, so don't build the sub frame-tree
        FrameTableColBox* childFrame = new FrameTableColBox(child, nullptr);
        ctx.currentBlockContainer()->appendChild(childFrame);
        child->setFrame(childFrame);
        child->clearNeedsFrameTreeBuild();
        child->clearChildNeedsFrameTreeBuild();
        STARFISH_ASSERT(childFrame->parent());
        return;
    }
    // Ignore the other child node which display value is not
    // 'TableColumnDisplayValue'.
    return;
}

unsigned FrameTableColBox::span()
{
    int ret = 0;

    if (!(node() && node()->isElement() &&
          node()->asElement()->isHTMLElement())) {
        return ret;
    }

    HTMLElement* e = node()->asElement()->asHTMLElement();
    if (e->isHTMLColGroupElement()) {
        String* span = e->asHTMLColGroupElement()->span();
        ret = String::parseInt(span);
    } else if (e->isHTMLColElement()) {
        String* span = e->asHTMLColElement()->span();
        ret = String::parseInt(span);
    }
    // span is only accepted when HTML element is either <col> or <colGroup>,
    // hence it is not applied when used in other elements.
    // e.g., <div style="display: table-column" span="2">
    // In this case, we ignore the span value

    // If span is not defined, use 1 as the default value
    return ret <= 0 ? 1 : ret;
}

void FrameTableColBox::layout(LayoutContext& ctx,
                              Frame::LayoutWantToResolve resolveWhat)
{
    STARFISH_ASSERT_NOT_REACHED();
}

void FrameTableColBox::paint(PaintingContext& ctx)
{
    // FrameTableCol should only exist logically and ignore paint.
}
}
