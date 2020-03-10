/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "FrameSVGEllipseBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

void* FrameSVGEllipseBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGEllipseBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGEllipseBox)] = { 0 };
        FrameSVGEllipseBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGEllipseBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGEllipseBox::paintSVG(PaintingContext& ctx)
{
    FrameBox* cb = layoutParent()->asFrameBox();

    Path* newPath = path();
    if (newPath) {
        ctx.m_canvas->save();
        float opacity = style()->opacity();
        Unit::Color fillColor = style()->fill().color();
        ctx.m_canvas->setFillColor(
            Unit::Color(fillColor.r(), fillColor.g(), fillColor.b(),
                        fillColor.a() * style()->fillOpacity() * opacity));
        Unit::Color strokeColor = style()->stroke().color();
        ctx.m_canvas->setStrokeColor(
            Unit::Color(strokeColor.r(), strokeColor.g(), strokeColor.b(),
                        strokeColor.a() * style()->strokeOpacity() * opacity));
        ctx.m_canvas->setFillRule(style()->fillRule());
        ctx.m_canvas->fillPath(newPath);
        ctx.m_canvas->setLineWidth(
            style()->strokeWidth().specifiedValue(cb->width(), this));
        ctx.m_canvas->strokePath(newPath);

        ctx.m_canvas->restore();
    }
}

Path* FrameSVGEllipseBox::path()
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
    double rx = 0;
    if (style()->rx().isSpecified()) {
        rx = style()->rx().specifiedValue(cb->width(), this);
    }
    double ry = 0;
    if (style()->ry().isSpecified()) {
        ry = style()->ry().specifiedValue(cb->height(), this);
    }
    if (rx && ry) {
        path->ellipse(cx, cy, rx, ry, 0, 0, 2 * M_PI);
    } else {
        return nullptr;
    }

    return path;
}
}
