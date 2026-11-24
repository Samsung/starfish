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
#include "core/dom/Document.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/svg/SVGAnimatedString.h"
#include "core/dom/svg/SVGUseElement.h"

namespace Starfish {

void* SVGUseElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGUseElement));
    bool& typeInited = GCDescriptorCache<SVGUseElement>::inited;
    GC_descr& descr = GCDescriptorCache<SVGUseElement>::descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGUseElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGUseElement, m_href));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGUseElement, m_hrefObject));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGUseElement, m_target));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGUseElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGUseElement::SVGUseElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_href(String::emptyString)
{
}

String* SVGUseElement::effectiveHrefAttributeValue()
{
    StaticStrings* ss = starfish()->staticStrings();
    if (hasAttribute(ss->m_href) != SIZE_MAX) {
        return getAttributeOrEmpty(ss->m_href);
    }
    Optional<String*> xlinkVal =
        getAttributeNS(ss->m_xlinkHref.namespaceURI().getValue().string(),
                       ss->m_href.localName());
    return xlinkVal.hasValue() ? xlinkVal.getValue() : String::emptyString;
}

SVGAnimatedString* SVGUseElement::href()
{
    if (!m_hrefObject.hasValue()) {
        StaticStrings* ss = starfish()->staticStrings();
        String* value = effectiveHrefAttributeValue();
        m_hrefObject = new SVGAnimatedString(this, ss->m_href, value, value);
    }
    return m_hrefObject.getValue();
}

void SVGUseElement::didAttributeChanged(QualifiedName name,
                                        Optional<String*> old, String* value,
                                        bool attributeCreated,
                                        bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();
    bool isPlainHref = (ss->m_href == name);
    bool isXLinkHref =
        (ss->m_xlinkHref == name ||
         (ss->m_xlinkHref.hasSameNamespaceURI(name.namespaceURI()) &&
          ss->m_xlinkHref.hasSameLocalName(name.localName())));
    if (isPlainHref || isXLinkHref) {
        String* effectiveValue = effectiveHrefAttributeValue();
        if (m_hrefObject.hasValue()) {
            m_hrefObject->setBaseVal(effectiveValue, true);
        }
        bool hasPlainHref = hasAttribute(ss->m_href) != SIZE_MAX;
        if (isXLinkHref && hasPlainHref) {
            // Plain "href" takes precedence over "xlink:href".
            return;
        }
        m_href = effectiveValue;
        setNeedsStyleRecalc();
        setNeedsFrameTreeBuild();
    }
}

void SVGUseElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_href == name) {
        // https://svgwg.org/svg2-draft/linking.html#XLinkRefAttrs
        // If "href" is not set but an XLink "href" attribute is present,
        // reflect mutations into the existing XLink attribute.
        if (hasAttribute(ss->m_href) == SIZE_MAX &&
            hasAttributeNS(ss->m_xlinkHref.namespaceURI().getValue().string(),
                           ss->m_href.localName())) {
            for (size_t i = 0; i < attributeCount(); i++) {
                QualifiedName attrName = getAssuredAttributeName(i);
                if (ss->m_xlinkHref.hasSameNamespaceURI(
                        attrName.namespaceURI()) &&
                    ss->m_xlinkHref.hasSameLocalName(attrName.localName())) {
                    setAttribute(attrName, href()->baseVal());
                    return;
                }
            }
        }
        setAttribute(ss->m_href, href()->baseVal());
    }
}

Optional<SVGElement*> SVGUseElement::updateShadowTree()
{
    Optional<ResourceURL*> targetElementURL;
    ShadowRoot* shadowRoot = internalEnsureShadowRoot();

    auto oldTarget = m_target;
    Optional<SVGElement*> newTarget;
    Optional<Node*> oldClonedTarget = shadowRoot->firstChild();
    Optional<Node*> newClonedTarget;

    auto targetElement = findHrefTarget(m_href);
    if (targetElement && !targetElement->contains(this)) {
        auto newClonedElement = targetElement->makeShadowClone();
        if (newClonedElement && newClonedElement->isSVGElement()) {
            newTarget = targetElement->asSVGElement();
            newClonedTarget = newClonedElement;
        }
    }

    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        bool shouldUpdateShadowRootContents = true;
        if (oldTarget != newTarget) {
            m_target = newTarget;
        } else if (oldClonedTarget &&
                   oldClonedTarget->isEqualNode(newClonedTarget.value())) {
            shouldUpdateShadowRootContents = false;
        }

        if (shouldUpdateShadowRootContents) {
            while (shadowRoot->hasChildNodes()) {
                shadowRoot->removeChild(shadowRoot->firstChild());
            }
            if (newClonedTarget) {
                shadowRoot->appendChild(newClonedTarget.value());
            }
        }

#ifndef NDEBUG
        if (m_target) {
            STARFISH_ASSERT(internalEnsureShadowRoot()->firstElementChild());
        }
#endif
    }

    return m_target;
}
} // namespace Starfish
