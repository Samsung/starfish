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

void FrameSVGRectBox::paintSVG(PaintingContext& ctx)
{
    Path* newPath = path();
    float opacity = style()->opacity();
    Nullable<GradientDrawingInfo*> info = nullptr;
    Unit::Color fillColor;

    if (style()->fill()->hasUrl()) {
        info = makeGradientDrawingInfo(style()->fill()->url());
    } else {
        fillColor = style()->fill()->color();
    }

    if (info.hasValue()) {
        ctx.m_canvas->save();
        Unit::Rect rect = newPath->boundingRect(true).snapSizeToPixel();
        std::shared_ptr<NativeGradient> gradient =
            NativeGradient::create(info.getValue());
        if (info->type == GradientType::LinearGradient) {
            ctx.m_canvas->drawLinearGradient(rect, info.getValue(),
                                             gradient.get());
        } else {
            ctx.m_canvas->drawRadialGradient(rect, info.getValue(),
                                             gradient.get());
        }
        ctx.m_canvas->restore();
    } else {
        ctx.m_canvas->setFillColor(
            Unit::Color(fillColor.r(), fillColor.g(), fillColor.b(),
                        fillColor.a() * style()->fillOpacity() * opacity));
        Unit::Color strokeColor = style()->stroke()->color();
        ctx.m_canvas->setStrokeColor(
            Unit::Color(strokeColor.r(), strokeColor.g(), strokeColor.b(),
                        strokeColor.a() * style()->strokeOpacity() * opacity));
        ctx.m_canvas->fillPath(newPath);
        ctx.m_canvas->strokePath(newPath);
    }
}

Path* FrameSVGRectBox::path()
{
    Path* path = Path::create();

    float rx = m_rx, ry = m_ry;
    if (rx > width() / 2) {
        rx = width() / 2;
    }

    if (ry > height() / 2) {
        ry = height() / 2;
    }

    // ctx.m_canvas->beginPath();
    path->clear();

    if (rx == 0 && ry == 0) {
        path->moveTo(0, 0);
        path->lineTo((float)width(), 0);
        path->lineTo((float)width(), (float)height());
        path->lineTo(0, (float)height());
        path->lineTo(0, 0);
    } else {
        // perform an absolute moveto operation to location (x+rx,y),
        path->moveTo(rx, 0);
        // perform an absolute horizontal lineto operation to location
        // (x+width-rx,y)
        path->lineTo(width() - rx, 0);
        // perform an absolute elliptical arc operation to coordinate
        // (x+width,y+ry)
        paintPathArcCommand(path, width() - rx, 0, rx, ry, 0, false, true,
                            width(), ry);
        // perform a absolute vertical lineto to location
        // (x+width,y+height-ry)
        path->lineTo(width(), height() - ry);
        // perform an absolute elliptical arc operation to coordinate
        // (x+width-rx,y+height)
        paintPathArcCommand(path, width(), height() - ry, rx, ry, 0, false,
                            true, width() - rx, height());
        // perform an absolute horizontal lineto to location (x+rx,y+height)
        path->lineTo(rx, height());
        // perform an absolute elliptical arc operation to coordinate
        // (x,y+height-ry)
        paintPathArcCommand(path, rx, height(), rx, ry, 0, false, true, 0,
                            height() - ry);
        // perform an absolute absolute vertical lineto to location (x,y+ry)
        path->lineTo(0, ry);
        // perform an absolute elliptical arc operation to coordinate
        // (x+rx,y)
        paintPathArcCommand(path, 0, ry, rx, ry, 0, false, true, rx, 0);
    }

    return path;
}
}
