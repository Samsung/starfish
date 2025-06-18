/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

Optional<Path*> FrameSVGLineBox::path()
{
    Path* path = Path::create();

    path->clear();

    path->moveTo(m_x1, m_y1);
    path->lineTo(m_x2, m_y2);

    return path;
}

LayoutRect FrameSVGLineBox::boundingRect()
{
    auto p = path();
    if (p) {
        Unit::Rect rect = p.getValue()->boundingRect();
        return LayoutRect(rect.x(), rect.y(), rect.width(), rect.height());
    }
    return LayoutRect();
}
} // namespace Starfish
