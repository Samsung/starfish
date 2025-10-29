/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#include "SVGAnimateMotionElement.h"

#include "Starfish.h"
#include "StaticStrings.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"

#include "core/dom/svg/SVGAnimateElement.h"
#include "core/dom/svg/SVGPathElement.h"

#include "core/modules/canvas/Path.h"

namespace Starfish {

void* SVGAnimateMotionElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGAnimateMotionElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGAnimateMotionElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGAnimateMotionElement, m_pointList));
        SVGAnimateElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGAnimateMotionElement));

        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGAnimateMotionElement::SVGAnimateMotionElement(Document* document,
                                                 const QualifiedName& qname)
    : SVGAnimationElement(document, qname)
{
}

void SVGAnimateMotionElement::didAttributeChanged(QualifiedName name,
                                                  Optional<String*> old,
                                                  String* value,
                                                  bool attributeCreated,
                                                  bool attributeRemoved)
{
    SVGAnimationElement::didAttributeChanged(name, old, value, attributeCreated,
                                             attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (name == ss->m_path) {
        auto path = Path::create();
        SVGPathElement::parsePath(value, path);
        m_pointList = path->pointList();
    }
}

void SVGAnimateMotionElement::beginElementAt(float offset)
{
    if (!m_pointList.size()) {
        return;
    }

    GCVector<CSSStyleValuePair> v;
    auto vec = new GCAtomicVector<Unit::FloatPoint>(m_pointList);
    CSSStyleValuePair p;
    p.setAnimateMotionValue(vec);
    v.push_back(p);
    v.push_back(p);
    beginElementAtInternal(offset, CSSStyleValuePair::KeyKind::Unknown,
                           NullOption, NullOption, v);
}

Unit::FloatPoint SVGAnimateMotionElement::computePoint(
    const GCAtomicVector<Unit::FloatPoint>& pl, float progress)
{
    float totalDistance = 0;
    Unit::FloatPoint last = pl.size() ? *pl.begin() : Unit::FloatPoint();
    for (size_t i = 1; i < pl.size(); i++) {
        totalDistance +=
            std::sqrt((last.x() - pl[i].x()) * (last.x() - pl[i].x()) +
                      (last.y() - pl[i].y()) * (last.y() - pl[i].y()));
        last = pl[i];
    }

    float targetDistance = totalDistance * progress;
    float distance = 0;
    for (size_t i = 1; i < pl.size(); i++) {
        float currentDistance =
            std::sqrt((last.x() - pl[i].x()) * (last.x() - pl[i].x()) +
                      (last.y() - pl[i].y()) * (last.y() - pl[i].y()));
        float remain = targetDistance - (distance + currentDistance);
        if (remain <= 0) {
            float c = currentDistance + remain;
            float x = last.x() + c / currentDistance * (pl[i].x() - last.x());
            float y = last.y() + c / currentDistance * (pl[i].y() - last.y());
            return Unit::FloatPoint(x, y);
        }
        last = pl[i];
        distance += currentDistance;
    }

    return last;
}

} // namespace Starfish
