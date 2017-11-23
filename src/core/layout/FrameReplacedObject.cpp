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
#include "core/dom/HTMLObjectElement.h"
#include "core/layout/FrameReplacedObject.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"

namespace StarFish {

IntrinsicSize FrameReplacedObject::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    auto v = node()->asHTMLObjectElement();
    if (v->content()) {
        result.m_hasAspectRatio = true;
        result.m_intrinsicContentSize =
            LayoutSize(v->content()->width(), v->content()->height());
    } else {
        result.m_hasAspectRatio = false;
        result.m_intrinsicContentSize = LayoutSize(1, 1);
    }
    return result;
}

void FrameReplacedObject::didCompsiteStackingContext(Compositor* c)
{
    auto v = node()->asHTMLObjectElement();
    LayoutRect contentRect(borderLeft() + paddingLeft(),
                           borderTop() + paddingTop(), contentWidth(),
                           contentHeight());
    LayoutRect absContentRect(contentRect);
    c->applyMatrixTo(absContentRect);
    if (v->content()) {
        v->content()->drawContent(c, contentRect, absContentRect);
    }
}

void FrameReplacedObject::computeStyleFlags()
{
    FrameReplaced::computeStyleFlags();
    m_flags.m_isEstablishesStackingContext = true;
    if (node()->asHTMLObjectElement()->content()) {
        m_flags.m_needsGraphicsBuffer =
            node()->asHTMLObjectElement()->content()->needsGraphicsBuffer();
    }
}
}
