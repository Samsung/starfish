/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameSVGLineBox.h"

namespace Starfish {

void* FrameSVGLineBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGLineBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGLineBox)] = { 0 };
        FrameSVGLineBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGLineBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGLineBox::paintSVG(PaintingContext& ctx)
{
    FrameBox* cb = layoutParent()->asFrameBox();
    Path* newPath = path();

    ctx.m_canvas->setFillColor(style()->fill().color());
    ctx.m_canvas->fillPath(newPath);

    ctx.m_canvas->setLineWidth(
        style()->strokeWidth().specifiedValue(cb->width(), this));
    ctx.m_canvas->setStrokeColor(style()->stroke().color());
    ctx.m_canvas->strokePath(newPath);
}

Path* FrameSVGLineBox::path()
{
    Path* path = Path::create();

    path->clear();

    path->moveTo(m_x1, m_y1);
    path->lineTo(m_x2, m_y2);

    return path;
}
}
