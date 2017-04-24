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
