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
#include "FrameSVGPolygonBox.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

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
    auto points =
        parsePointsFromString(node()->asElement()->getAttributeOrEmpty(
            node()->starFish()->staticStrings()->m_points));

    if (points.size()) {
        ctx.m_canvas->moveTo(points[0].first, points[0].second);
        for (size_t i = 1; i < points.size(); i++) {
            ctx.m_canvas->lineTo(points[i].first, points[i].second);
        }
        FrameBox* cb = layoutParent()->asFrameBox();

        ctx.m_canvas->setFillRule(style()->fillRule());
        ctx.m_canvas->setColor(style()->fill().color());
        ctx.m_canvas->fillPreserve();

        ctx.m_canvas->setStrokeWidth(
            style()->strokeWidth().specifiedValue(cb->width(), this));
        ctx.m_canvas->setColor(style()->stroke().color());
        ctx.m_canvas->stroke();
    }
}
}
