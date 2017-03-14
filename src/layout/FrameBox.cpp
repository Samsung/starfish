/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

void FrameBox::paintStackingContextContent(Canvas* canvas)
{
    PaintingContext ctx(canvas);
    // the in-flow, non-inline-level, non-positioned descendants.
    ctx.m_paintingStage = PaintingNormalFlowBlock;
    ctx.m_paintingInlineStage = PaintingInlineLevelElements;
    paintChildrenWith(ctx);

    // the non-positioned float
    ctx.m_paintingStage = PaintingNonPositionedFloats;
    paintChildrenWith(ctx);

    // the in-flow, inline-level, non-positioned descendants, including inline
    // tables and inline blocks.
    ctx.m_paintingStage = PaintingNormalFlowInline;
    paintChildrenWith(ctx);

    // the child stacking contexts with stack level 0 and the positioned
    // descendants with stack level 0.
    ctx.m_paintingStage = PaintingPositionedElements;
    paintChildrenWith(ctx);
}

bool FrameBox::tryUniteVisibleRect(StackingContext* sCtx, LayoutLocation& loc)
{
    if (this != sCtx->owner() && stackingContext() &&
        stackingContext()->needsOwnBuffer()) {
        return false;
    }

    LayoutRect r = frameRect();
    r.setX(r.x() + loc.x());
    r.setY(r.y() + loc.y());
    sCtx->unite(r);

    if (style()->overflow() == OverflowValue::HiddenOverflow) {
        return false;
    }
    return true;
}

void FrameBox::computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc)
{
    tryUniteVisibleRect(sCtx, loc);
}
}
