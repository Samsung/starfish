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
#include "FrameSVGTextBox.h"
#include "core/dom/Document.h"
#include "core/layout/FrameBlockBox.h"

namespace StarFish {

void FrameSVGTextBox::layoutSVG()
{
    LayoutContext ctx(node()->starFish(),
                      node()->document()->frame()->asFrameDocument());
    firstChild()->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
}

void FrameSVGTextBox::paintSVG(PaintingContext& ctx)
{
    PaintingContext newCtx(ctx.m_canvas);
    newCtx.m_paintingStage = PaintingNormalFlowInline;
    newCtx.m_canvas->save();
    newCtx.m_canvas->translate(0,
                               -(float)style()->font()->metrics().m_ascender);
    firstChild()->asFrameBlockBox()->paintContent(newCtx);
    newCtx.m_canvas->restore();
}
}
