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
#include "core/dom/svg/SVGUseElement.h"

namespace Starfish {

void* SVGUseElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGUseElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGUseElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGUseElement, m_href));
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

void SVGUseElement::didAttributeChanged(QualifiedName name,
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
        m_href = value;
        setNeedsStyleRecalc();
        setNeedsFrameTreeBuild();
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
