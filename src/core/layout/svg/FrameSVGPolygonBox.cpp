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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameSVGPolygonBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

void* FrameSVGPolygonBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGPolygonBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGPolygonBox)] = { 0 };
        FrameSVGPolygonBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGPolygonBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGPolygonBox::paintSVG(PaintingContext& ctx)
{
    Path* newPath = path();
    if (newPath) {
        FrameBox* cb = layoutParent()->asFrameBox();

        ctx.m_canvas->setFillRule(style()->fillRule());
        Unit::Color fillColor = style()->fill().color();
        ctx.m_canvas->setFillColor(
            Unit::Color(fillColor.r(), fillColor.g(), fillColor.b(),
                        fillColor.a() * style()->fillOpacity()));
        ctx.m_canvas->fillPath(newPath);

        ctx.m_canvas->setLineWidth(
            style()->strokeWidth().specifiedValue(cb->width(), this));
        ctx.m_canvas->setStrokeColor(style()->stroke().color());
        ctx.m_canvas->strokePath(newPath);
    }
}

Path* FrameSVGPolygonBox::path()
{
    auto points =
        parsePointsFromString(node()->asElement()->getAttributeOrEmpty(
            node()->starfish()->staticStrings()->m_points));

    if (points.size()) {
        Path* path = Path::create();
        path->moveTo(points[0].first, points[0].second);
        for (size_t i = 1; i < points.size(); i++) {
            path->lineTo(points[i].first, points[i].second);
        }
        return path;
    }
    return nullptr;
}
}
