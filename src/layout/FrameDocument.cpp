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
#include "FrameDocument.h"

namespace StarFish {

void FrameDocument::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    int w = node()->asDocument()->window()->width();
    int h = node()->asDocument()->window()->height();

    style()->setWidth(Length(Length::Fixed, w));
    style()->setHeight(Length(Length::Fixed, h));

    if (firstChild()) {
        STARFISH_ASSERT(firstChild() == lastChild());
        STARFISH_ASSERT(firstChild()->isRootElement());

        FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
    }
}

}
