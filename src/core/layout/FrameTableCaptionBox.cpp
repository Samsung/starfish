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
#include "core/layout/FrameTableCaptionBox.h"
#include "core/layout/FrameTreeBuilder.h"

namespace StarFish {

FrameTableCaptionBox::FrameTableCaptionBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_minCaptionWidth(0)
    , m_maxCaptionWidth(0)
{
}

void FrameTableCaptionBox::layoutWidth(LayoutContext& ctx)
{
    layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);

    PreferredWidthContext p(ctx, LayoutUnit::max());
    computePreferredWidth(p);
    p.finishLine(false);
    m_minCaptionWidth = p.preferredMinWidth() + borderWidth() + paddingWidth();
    m_maxCaptionWidth = p.preferredWidth() + borderWidth() + paddingWidth();
}
}
