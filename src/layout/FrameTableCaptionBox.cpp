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
#include "FrameTableCaptionBox.h"

#include "FrameTreeBuilder.h"

namespace StarFish {

FrameTableCaptionBox::FrameTableCaptionBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{
}

FrameTableCaptionBox* FrameTableCaptionBox::buildFrameTableCaptionBox(
    Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableCaptionBox* currentFrame = nullptr;

    if (current->needsFrameTreeBuild() && !current->frame()) {
        currentFrame = new FrameTableCaptionBox(current, nullptr);
        current->setFrame(currentFrame);
    } else {
        STARFISH_ASSERT(current->frame());
        currentFrame = current->frame()->asFrameTableCaptionBox();
    }

    // Caption establishes a new block context
    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(currentFrame);
    ctx.mergeTextDecorationData(currentFrame->style());

    FrameTreeBuilder::createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementBefore,
        ctx);

    if (current->childNeedsFrameTreeBuild() || force) {
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            FrameTreeBuilder::buildTree(c, ctx, force);
        }
    }

    current->clearNeedsFrameTreeBuild();
    current->clearChildNeedsFrameTreeBuild();

    FrameTreeBuilder::createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementAfter, ctx);

    ctx.setCurrentBlockContainer(lastContext);

    FrameTreeBuilder::createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementFirstLetter,
        ctx);

    return currentFrame;
}

void FrameTableCaptionBox::layout(LayoutContext& ctx,
                                  Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, resolveWhat);
}
}
