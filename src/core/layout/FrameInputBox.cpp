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

#include "FrameInputBox.h"

#include "core/dom/HTMLInputElement.h"
#include "core/dom/Text.h"
#include "core/dom/PseudoElement.h"

#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameTreeBuilder.h"
namespace StarFish {

FrameInputBox::FrameInputBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

FrameInputBox* FrameInputBox::buildFrameTree(Node* current,
                                             FrameTreeBuilderContext& ctx,
                                             bool force)
{
    FrameInputBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    if (current->needsFrameTreeBuild() || force) {
        currentFrame = new FrameInputBox(current, nullptr);
        force = true;
    } else {
        currentFrame = current->frame()->asFrameInputBox();
    }

    ctx.setCurrentBlockContainer(currentFrame);

    STARFISH_ASSERT(current->isHTMLInputElement());

    {
        // TODO: Input boxes of "type='text'" requires a GUI component from
        // the backend library. For time being, direct user text inputs to
        // input boxes is not supported. Input boxes display
        // the text value assigned to "value" attribute only. To integrate with
        // current layout, a tmp pseudo element is created to display the
        // text value.
        PseudoElement* textElement = new PseudoElement(
            parent->document(),
            StyleResolver::PseudoElementType::PseudoElementFormOnly);
        textElement->setParentNode(current);
        textElement->setStyle(current->style());

        String* userVal = current->asHTMLInputElement()->value();
        Text* textNode = new Text(current->document(), userVal);
        textNode->setStyle(current->style());
        textNode->setParentNode(textElement);

        FrameText* frameText = new FrameText(textNode, current->style());
        textNode->setFrame(frameText);
        currentFrame->appendChild(frameText);
    }

    ctx.setCurrentBlockContainer(parent);

    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}
}
