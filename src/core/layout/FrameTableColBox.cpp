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

// https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#rules-for-parsing-non-negative-integers
// https://html.spec.whatwg.org/multipage/tables.html#forming-a-table
uint32_t FrameTableColBox::span()
{
    if (!node()) {
        return 1;
    }
    if (!node()->isHTMLElement()) {
        return 1;
    }

    HTMLElement* e = node()->asHTMLElement();
    if (e->isHTMLTableColElement()) {
        return e->asHTMLTableColElement()->span();
    }

    // span is only accepted when HTML element is either <col> or <colGroup>,
    // hence it is not applied when used in other elements.
    // e.g., <div style="display: table-column" span="2">
    // In this case, we ignore the span value
    return 1;
}

bool FrameTableColBox::hasChildColBox()
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableColBox()) {
            return true;
        }
    }

    return false;
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
