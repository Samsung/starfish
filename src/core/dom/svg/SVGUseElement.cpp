/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/svg/SVGUseElement.h"

namespace Starfish {

void* SVGUseElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGUseElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGUseElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGUseElement, m_targetElement));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGUseElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGUseElement::SVGUseElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
{
}

void SVGUseElement::didAttributeChanged(QualifiedName name, String* old,
                                        String* value, bool attributeCreated,
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
            document()->unRegisterUseElement(this);
            m_targetElement = nullptr;
        } else {
            document()->registerUseElement(this);
            m_targetElement = new ResourceURL(value, document()->baseURI());
        }
    }
}

void SVGUseElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);
}

void SVGUseElement::updateShadowTree()
{
    shadowRoot()->clear();
    if (m_targetElement) {
        Element* element = document()->getElementById(m_targetElement->hash());
        if (element && element->isSVGElement()) {
            SVGElement* newClonedElement =
                element->cloneNode(true)->asSVGElement();
            shadowRoot()->appendChild(newClonedElement);
        }
    }
}
}
