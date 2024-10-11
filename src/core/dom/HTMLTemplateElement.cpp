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
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLTemplateElement.h"
#include "core/dom/MutationObservationScope.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/xml/XMLSerializer.h"

namespace Starfish {

HTMLTemplateElement::HTMLTemplateElement(Document* document,
                                         const QualifiedName& qname)
    : HTMLElement(document, qname)
    , m_content(new DocumentFragment(document))
{
}

void* HTMLTemplateElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLTemplateElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLTemplateElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLTemplateElement, m_content));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLTemplateElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* HTMLTemplateElement::shadowRootMode()
{
    QualifiedName q(
        AtomicString::createAtomicString(starfish(), "shadowrootmode"));
    auto attr = getAttribute(q);
    if (attr) {
        if (attr->equals("open")) {
            return attr.value();
        } else if (attr->equals("closed")) {
            return attr.value();
        }
    }
    return String::emptyString;
}

void HTMLTemplateElement::setShadowRootMode(String* s)
{
    QualifiedName q(
        AtomicString::createAtomicString(starfish(), "shadowrootmode"));
    setAttribute(q, s);
}

void HTMLTemplateElement::finishParsing()
{
    HTMLElement::finishParsing();

    String* mode = shadowRootMode();
    if (mode->length()) {
        if (parentElement()) {
            Element* parent = parentElement();
            parent->removeChild(this);

            ShadowRootInit init;
            init.setMode(mode);
            parent->attachShadow(init);
            parent->internalShadowRoot()->appendChild(content());
        }
    }
}

String* HTMLTemplateElement::innerHTML()
{
    return XMLSerializer::serializeToXML(m_content, false);
}

void HTMLTemplateElement::setInnerHTML(String* html)
{
    ChildListMutationObservationScope scope;
    scope.startChildListMutationScope(m_content);
    while (m_content->firstChild()) {
        m_content->removeChild(m_content->firstChild());
    }

    DocumentFragment* df = fragmentParsingAlgorithm(document(), html, this);
    m_content->appendChild(df);
}

} // namespace Starfish
