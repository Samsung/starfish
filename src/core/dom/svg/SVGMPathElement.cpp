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

#include "SVGMPathElement.h"

#include "Starfish.h"
#include "StaticStrings.h"
namespace Starfish {

void* SVGMPathElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGMPathElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGMPathElement)] = { 0 };
        SVGMPathElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGMPathElement));

        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGMPathElement::SVGMPathElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
{
}

void SVGMPathElement::didAttributeChanged(QualifiedName name,
                                          Optional<String*> old, String* value,
                                          bool attributeCreated,
                                          bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_href == name || ss->m_xlinkHref == name ||
        (!name.hasPrefix() &&
         ss->m_xlinkHref.hasSameNamespaceURI(name.namespaceURI()) &&
         ss->m_xlinkHref.hasSameLocalName(name.localName()))) {
        if (attributeRemoved) {
            m_href.reset();
        } else {
            m_href = value;
        }
    }
}

} // namespace Starfish
