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

#include "FrameSelectBox.h"

#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/layout/FrameBlockBox.h"

namespace StarFish {

FrameSelectBox::FrameSelectBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

void FrameSelectBox::layout(LayoutContext& ctx,
                            Frame::LayoutWantToResolve resolveWhat)
{
    STARFISH_ASSERT(node()->isHTMLSelectElement());
    HTMLSelectElement* selectNode = node()->asHTMLSelectElement();
    HTMLOptionElement* selected = selectNode->firstSelectedOptionElement();

    if (selected) {
        selected->m_drawOptionBox = true;
    }

    FrameBlockBox::layout(ctx, resolveWhat);
}
}
