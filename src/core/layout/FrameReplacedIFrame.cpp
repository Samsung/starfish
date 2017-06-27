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
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/Document.h"
#include "core/layout/FrameReplacedIFrame.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

IntrinsicSize FrameReplacedIFrame::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    result.m_hasAspectRatio = false;
    auto v = node()->asHTMLIFrameElement();
    result.m_intrinsicContentSize =
        LayoutSize(v->frameWidth(), v->frameHeight());
    return result;
}

void FrameReplacedIFrame::layout(LayoutContext& ctx,
                                 Frame::LayoutWantToResolve resolveWhat)
{
    FrameReplaced::layout(ctx, resolveWhat);
    if (resolveWhat & ResolveHeight) {
        HTMLIFrameElement* v = node()->asHTMLIFrameElement();
        if (v->browsingContext()) {
            if (v->browsingContext()->window()) {
                v->browsingContext()->window()->resize(contentWidth(),
                                                       contentHeight());
                v->browsingContext()->layoutIfNeeds();
            }
        }
    }
}

void FrameReplacedIFrame::paintReplaced(Canvas* canvas)
{
    HTMLIFrameElement* v = node()->asHTMLIFrameElement();
    if (v->browsingContext()) {
        if (v->browsingContext()->window()) {
            canvas->save();
            canvas->clip(Unit::Rect(paddingLeft() + borderLeft(),
                                    paddingTop() + borderTop(), contentWidth(),
                                    contentHeight()));

            PaintingContext ctx(canvas);
            ctx.m_paintingStage = PaintingStageEnd;
            v->browsingContext()->document()->frame()->paint(ctx);

            canvas->restore();
        }
    }
}

void FrameReplacedIFrame::establishesStackingContextIfNeeds()
{
    HTMLIFrameElement* v = node()->asHTMLIFrameElement();
    if (v->browsingContext()) {
        if (v->browsingContext()->window()) {
            v->browsingContext()
                ->window()
                ->document()
                ->frame()
                ->establishesStackingContextIfNeeds();
        }
    }
}
}
