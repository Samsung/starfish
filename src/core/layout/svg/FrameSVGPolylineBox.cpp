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
#include "FrameSVGPolylineBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

void* FrameSVGPolylineBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGPolylineBox)] = { 0 };
        FrameSVGPolylineBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGPolylineBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Optional<Path*> FrameSVGPolylineBox::path()
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

LayoutRect FrameSVGPolylineBox::boundingRect()
{
    auto p = path();
    if (p) {
        Unit::Rect rect = p.getValue()->boundingRect();
        return LayoutRect(rect.x(), rect.y(), rect.width(), rect.height());
    }
    return LayoutRect();
}
} // namespace Starfish
