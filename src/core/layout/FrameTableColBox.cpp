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
#include "core/dom/Node.h"
#include "core/dom/HTMLTableColElement.h"
#include "core/dom/HTMLTableColGroupElement.h"
#include "core/layout/FrameTableColBox.h"
#include "core/layout/FrameTreeBuilder.h"

namespace StarFish {

FrameTableColBox::FrameTableColBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{
}

unsigned FrameTableColBox::span()
{
    uint32_t ret = 0;

    if (!(node() && node()->isHTMLElement())) {
        return ret;
    }

    HTMLElement* e = node()->asHTMLElement();
    if (e->isHTMLTableColElement()) {
        ret = e->asHTMLTableColElement()->span();
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

void FrameTableColBox::paintContent(PaintingContext& ctx)
{
    // FrameTableCol should only exist logically and ignore paint.
}
}
