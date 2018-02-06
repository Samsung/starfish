/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "FrameButtonBox.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLInputElement.h"
#include "core/page/Window.h"

namespace StarFish {

FrameButtonBox::FrameButtonBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

void FrameButtonBox::layout(LayoutContext& ctx,
                            Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, resolveWhat);

    if (resolveWhat & Frame::ResolveHeight) {
        if (!hasBlockFlow()) {
            if (lineBoxes().size() > 0) {
                LayoutUnit lineBoxesHeight;
                for (size_t i = 0; i < lineBoxes().size(); i++) {
                    lineBoxesHeight += lineBoxes()[i]->height();
                }
                LayoutUnit availableHeight = contentHeight() - lineBoxesHeight;

                LayoutUnit d;
                for (size_t i = 0; i < lineBoxes().size(); i++) {
                    lineBoxes()[i]->setY(availableHeight / 2 + paddingTop() +
                                         borderTop() + d);
                    d += lineBoxes()[i]->height();
                }
            }
        }
    }
}
}
