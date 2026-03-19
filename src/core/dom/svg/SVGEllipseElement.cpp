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
#include "Starfish.h"
#include "core/style/CSSParser.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGEllipseElement.h"

namespace Starfish {

void SVGEllipseElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGElement::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_cx == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_cy == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_rx == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_ry == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    }
}

void SVGEllipseElement::didAttributeChanged(QualifiedName name,
                                            Optional<String*> old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
}

void SVGEllipseElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_cx == name) {
        setAttribute(ss->m_cx, cx()->baseVal()->valueAsString());
    } else if (ss->m_cy == name) {
        setAttribute(ss->m_cy, cy()->baseVal()->valueAsString());
    } else if (ss->m_rx == name) {
        setAttribute(ss->m_rx, rx()->baseVal()->valueAsString());
    } else if (ss->m_ry == name) {
        setAttribute(ss->m_ry, ry()->baseVal()->valueAsString());
    }
}

void* SVGEllipseElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGEllipseElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGEllipseElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGEllipseElement, m_cx));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGEllipseElement, m_cy));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGEllipseElement, m_rx));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGEllipseElement, m_ry));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGEllipseElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGEllipseElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, matchedRules,
                                              cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cx, CX, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cy, CY, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(rx, RX, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(ry, RY, cssCustomValues);
}
} // namespace Starfish
