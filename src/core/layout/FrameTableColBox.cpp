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
#include "core/dom/HTMLTableColElement.h"
#include "core/layout/FrameTableColBox.h"

namespace StarFish {

FrameTableColBox::FrameTableColBox(Node* node, ComputedStyle* style)
    : FrameTableCellBox(node, style)
{
}

unsigned FrameTableColBox::span()
{
    uint32_t ret = 0;

    if (!(node() && node()->isHTMLElement())) {
        return ret;
    }

    HTMLElement* e = node()->asHTMLElement();
    if (e->isHTMLTableColElement()) {
        ret = e->asHTMLTableColElement()->span();
    }
    // span is only accepted when HTML element is either <col> or <colGroup>,
    // hence it is not applied when used in other elements.
    // e.g., <div style="display: table-column" span="2">
    // In this case, we ignore the span value

    // If span is not defined, use 1 as the default value
    return ret <= 0 ? 1 : ret;
}

void FrameTableColBox::layout(LayoutContext& ctx,
                              Frame::LayoutWantToResolve resolveWhat)
{
    STARFISH_ASSERT_NOT_REACHED();
}

void FrameTableColBox::paintContent(PaintingContext& ctx)
{
    // FrameTableCol should only exist logically and ignore paint.
}
}
