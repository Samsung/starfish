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
#include "FrameBox.h"

#include "StackingContext.h"

namespace StarFish {

LayoutLocation FrameBox::absolutePointWithoutRelativePosition(FrameBox* top)
{
    LayoutLocation l(0, 0);
    Frame* p = this;
    while (top != p) {
        l.setX(l.x() + p->asFrameBox()->x());
        l.setY(l.y() + p->asFrameBox()->y());
        if (p->isPositionedElement() && p->style()->position() == RelativePositionValue) {
            l.setX(l.x() - p->asFrameBox()->m_positionRelativeOffset.x());
            l.setY(l.y() - p->asFrameBox()->m_positionRelativeOffset.y());
        }
        p = p->layoutParent();
    }
    return l;
}

void FrameBox::paintStackingContextContent(Canvas* canvas)
{
    // the in-flow, non-inline-level, non-positioned descendants.
    paintChildrenWith(canvas, PaintingNormalFlowBlock);

    // TODO the non-positioned floats.
    // paintChildrenWith(canvas, ctx, PaintingNonPositionedFloats);

    // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
    paintChildrenWith(canvas, PaintingNormalFlowInline);

    // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
    paintChildrenWith(canvas, PaintingPositionedElements);
}

}
