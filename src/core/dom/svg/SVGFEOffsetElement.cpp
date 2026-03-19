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
#include "Starfish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGFEOffsetElement.h"
#include "core/dom/svg/SVGAnimatedNumber.h"

namespace Starfish {
SVGFEOffsetElement::SVGFEOffsetElement(Document* document,
                                       const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEOffsetElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEOffsetElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEOffsetElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEOffsetElement, m_in));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEOffsetElement, m_dx));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEOffsetElement, m_dy));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFEOffsetElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEOffsetElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGFilterPrimitiveStandardAttributes::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_dx == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_dy == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
    }
}

void SVGFEOffsetElement::didAttributeChanged(QualifiedName name,
                                             Optional<String*> old,
                                             String* value,
                                             bool attributeCreated,
                                             bool attributeRemoved)
{
    SVGFilterPrimitiveStandardAttributes::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        in()->setBaseVal(value, true);
    } else if (ss->m_dx == name) {
        dx()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_dy == name) {
        dy()->setBaseVal(String::parseFloat(value), true);
    }
}

void SVGFEOffsetElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        setAttribute(ss->m_in, in()->baseVal());
    } else if (ss->m_dx == name) {
        setAttribute(ss->m_dx, String::fromFloat(dx()->baseVal()));
    } else if (ss->m_dy == name) {
        setAttribute(ss->m_dy, String::fromFloat(dy()->baseVal()));
    }
}

void SVGFEOffsetElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGFilterPrimitiveStandardAttributes::styleForPresentationAttribute(
        cssValues, matchedRules, cssCustomValues);
}

SVGAnimatedString* SVGFEOffsetElement::in()
{
    if (!m_in.hasValue()) {
        m_in = new SVGAnimatedString(this, starfish()->staticStrings()->m_in,
                                     String::emptyString, String::emptyString);
    }
    return m_in.getValue();
}

SVGAnimatedNumber* SVGFEOffsetElement::dx()
{
    if (!m_dx.hasValue()) {
        m_dx =
            new SVGAnimatedNumber(this, starfish()->staticStrings()->m_dx, 0);
    }
    return m_dx.getValue();
}

SVGAnimatedNumber* SVGFEOffsetElement::dy()
{
    if (!m_dy.hasValue()) {
        m_dy =
            new SVGAnimatedNumber(this, starfish()->staticStrings()->m_dy, 0);
    }
    return m_dy.getValue();
}

} // namespace Starfish
