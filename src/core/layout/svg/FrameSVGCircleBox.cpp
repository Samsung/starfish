/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameSVGCircleBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

void* FrameSVGCircleBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGCircleBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGCircleBox)] = { 0 };
        FrameSVGCircleBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGCircleBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Path* FrameSVGCircleBox::path()
{
    Path* path = Path::create();

    FrameBox* cb = layoutParent()->asFrameBox();

    double cx = 0;
    if (style()->cx().isSpecified()) {
        cx = style()->cx().specifiedValue(cb->width(), this);
    }
    double cy = 0;
    if (style()->cy().isSpecified()) {
        cy = style()->cy().specifiedValue(cb->height(), this);
    }
    double r = 0;
    if (style()->r().isSpecified()) {
        r = style()->r().specifiedValue(cb->width(), this);
    }

    path->arc(cx, cy, r, 0.0, 2 * M_PI);

    return path;
}
} // namespace Starfish
