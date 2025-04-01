/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "SVGAnimateElement.h"

#include "Starfish.h"
#include "StaticStrings.h"

namespace Starfish {

SVGAnimateElement::SVGAnimateElement(Document* document,
                                     const QualifiedName& qname)
    : SVGAnimationElement(document, qname)
{
}

void* SVGAnimateElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGAnimateElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGAnimateElement)] = { 0 };
        SVGAnimateElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGAnimateElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGAnimateElement::didAttributeChanged(QualifiedName name,
                                            Optional<String*> old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    SVGAnimationElement::didAttributeChanged(name, old, value, attributeCreated,
                                             attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (m_attributeName.hasValue()) {
        if (ss->m_from == name) {
            CSSStyleValuePair from;
            if (parseFromTo(m_attributeName.value(), value, from)) {
                if (!m_from.hasValue() || m_from.value() != from) {
                    m_from = from;
                }
            }
        } else if (ss->m_to == name) {
            CSSStyleValuePair to;
            if (parseFromTo(m_attributeName.value(), value, to)) {
                if (!m_to.hasValue() || m_to.value() != to) {
                    m_to = to;
                }
            }
        } else if (ss->m_values == name) {
            GCVector<CSSStyleValuePair> values;
            if (parseValues(m_attributeName.value(), value, values)) {
                if (!m_values.hasValue() ||
                    m_values.value().size() != values.size() ||
                    !std::equal(m_values.value().begin(),
                                m_values.value().end(), values.begin())) {
                    m_values = std::move(values);
                }
            }
        }
    }
}

bool SVGAnimateElement::parseFromTo(CSSStyleValuePair::KeyKind keyKind,
                                    const String* value,
                                    CSSStyleValuePair& output)
{
    return parseValue(keyKind, value, output);
}

} // namespace Starfish
