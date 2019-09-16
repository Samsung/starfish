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
        GC_set_bit(desc, GC_WORD_OFFSET(SVGUseElement, m_targetElementURL));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGUseElement, m_targetElement));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGUseElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGUseElement::SVGUseElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_targetElementURL(nullptr)
    , m_targetElement(nullptr)
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
            document()->unregisterUseElement(this);
            m_targetElementURL = nullptr;
        } else {
            document()->registerUseElement(this);
            m_targetElementURL = new ResourceURL(value, document()->baseURI());
        }
    } else if (ss->m_rx == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_ry == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    }
}

void SVGUseElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);
}

void SVGUseElement::updateShadowTree()
{
    if (m_targetElement && !m_targetElement->needsFrameTreeBuild()) {
        return;
    }
    shadowRoot()->clear();
    if (m_targetElementURL) {
        String* fragmentIdentifier = m_targetElementURL->hash();
        if (fragmentIdentifier && fragmentIdentifier->length() != 0) {
            Element* element =
                document()->getElementById(fragmentIdentifier->substring(
                    1, fragmentIdentifier->length() - 1));
            if (element && element->isSVGElement()) {
                Node* newClonedElement = element->makeShadowClone();
                if (newClonedElement && newClonedElement->isSVGElement()) {
                    shadowRoot()->appendChild(newClonedElement);
                    m_targetElement = element->asSVGElement();
                }
            }
        }
    }
}
}
