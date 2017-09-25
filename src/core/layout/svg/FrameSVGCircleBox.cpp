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
#include "FrameSVGCircleBox.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

void FrameSVGCircleBox::paintSVG(PaintingContext& ctx)
{
    FrameBox* cb = layoutParent()->asFrameBox();
    LayoutUnit viewportWidth =
        node()->document()->frame()->style()->width().fixed();
    LayoutUnit viewportHeight =
        node()->document()->frame()->style()->height().fixed();

    double cx =
        resolveLengthFromAttribute(node()->starFish()->staticStrings()->m_cx);
    double cy =
        resolveLengthFromAttribute(node()->starFish()->staticStrings()->m_cy);
    double r =
        resolveLengthFromAttribute(node()->starFish()->staticStrings()->m_r);

    ctx.m_canvas->arc(cx, cy, r, 0.0, 2 * M_PI);

    ctx.m_canvas->setFillRule(style()->fillRule());
    ctx.m_canvas->setColor(style()->fill().color());
    ctx.m_canvas->fillPreserve();

    ctx.m_canvas->setStrokeWidth(style()->strokeWidth().specifiedValue(
        cb->width(), viewportWidth, viewportHeight));
    ctx.m_canvas->setColor(style()->stroke().color());
    ctx.m_canvas->stroke();
}
}
