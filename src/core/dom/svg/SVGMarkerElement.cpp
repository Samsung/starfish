/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#include "core/dom/svg/SVGMarkerElement.h"
#include "StarfishBase.h"
namespace Starfish {

SVGMarkerElement::SVGMarkerElement(Document* document,
                                   const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_orientAngle(nullptr)
{
}

void SVGMarkerElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_orient == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    }
}

void* SVGMarkerElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGMarkerElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGMarkerElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMarkerElement, m_orientAngle));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGMarkerElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGAnimatedAngle* SVGMarkerElement::orientAngle()
{
    if (!m_orientAngle) {
        SVGAngle* baseVal = new SVGAngle(this, staticStrings()->m_orient);
        m_orientAngle = new SVGAnimatedAngle(document(), baseVal, nullptr);
    }
    return m_orientAngle;
}
} // namespace Starfish
