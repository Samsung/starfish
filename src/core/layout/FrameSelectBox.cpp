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

#include "FrameSelectBox.h"

#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/layout/FrameBlockBox.h"

namespace StarFish {

FrameSelectBox::FrameSelectBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

void FrameSelectBox::layout(LayoutContext& ctx,
                            Frame::LayoutWantToResolve resolveWhat)
{
    STARFISH_ASSERT(node()->isHTMLSelectElement());
    HTMLSelectElement* selectNode = node()->asHTMLSelectElement();
    HTMLOptionElement* selected = selectNode->firstSelectedOptionElement();

    if (selected) {
        selected->m_drawOptionBox = true;
    }

    FrameBlockBox::layout(ctx, resolveWhat);
}
}
