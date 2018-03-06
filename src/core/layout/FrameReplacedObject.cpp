/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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
