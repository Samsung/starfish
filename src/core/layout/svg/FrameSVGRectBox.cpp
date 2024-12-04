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
#include "FrameSVGRectBox.h"

#include "core/style/GradientData.h"
#include "core/style/CSSGradientValue.h"
#include "core/modules/canvas/NativeGradient.h"

namespace Starfish {

void* FrameSVGRectBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGRectBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGRectBox)] = { 0 };
        FrameSVGRectBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGRectBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Optional<Path*> FrameSVGRectBox::path()
{
    Path* path = Path::create();

    auto vp = viewport();
    auto styleSize = resolveStyleSize(vp);

    float rx = m_rx, ry = m_ry;
    if (rx > styleSize.width() / 2) {
        rx = styleSize.width() / 2;
    }

    if (ry > styleSize.height() / 2) {
        ry = styleSize.height() / 2;
    }

    path->clear();

    if (rx == 0 && ry == 0) {
        path->moveTo(0, 0);
        path->lineTo((float)styleSize.width(), 0);
        path->lineTo((float)styleSize.width(), (float)styleSize.height());
        path->lineTo(0, (float)styleSize.height());
        path->lineTo(0, 0);
    } else {
        // perform an absolute moveto operation to location (x+rx,y),
        path->moveTo(rx, 0);
        // perform an absolute horizontal lineto operation to location
        // (x+width-rx,y)
        path->lineTo(styleSize.width() - rx, 0);
        // perform an absolute elliptical arc operation to coordinate
        // (x+width,y+ry)
        paintPathArcCommand(path, styleSize.width() - rx, 0, rx, ry, 0, false, true,
                styleSize.width(), ry);
        // perform a absolute vertical lineto to location
        // (x+width,y+height-ry)
        path->lineTo(styleSize.width(), styleSize.height() - ry);
        // perform an absolute elliptical arc operation to coordinate
        // (x+width-rx,y+height)
        paintPathArcCommand(path, styleSize.width(), styleSize.height() - ry, rx, ry, 0, false,
                            true, styleSize.width() - rx, styleSize.height());
        // perform an absolute horizontal lineto to location (x+rx,y+height)
        path->lineTo(rx, styleSize.height());
        // perform an absolute elliptical arc operation to coordinate
        // (x,y+height-ry)
        paintPathArcCommand(path, rx, styleSize.height(), rx, ry, 0, false, true, 0,
                styleSize.height() - ry);
        // perform an absolute absolute vertical lineto to location (x,y+ry)
        path->lineTo(0, ry);
        // perform an absolute elliptical arc operation to coordinate
        // (x+rx,y)
        paintPathArcCommand(path, 0, ry, rx, ry, 0, false, true, rx, 0);
    }

    auto stylePos = resolveStylePosition(vp);
    path->translate(stylePos.x(), stylePos.y());

    return path;
}
} // namespace Starfish
