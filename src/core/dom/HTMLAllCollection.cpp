/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Element.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLAllCollection.h"
#include "core/dom/Traverse.h"

namespace Starfish {

HTMLAllCollection::HTMLAllCollection(Document* document)
    : ScriptWrappable(this)
    , m_document(document)
    , m_elements(document->getElementsByTagName(QualifiedName(
          AtomicString::emptyAtomicString(),
          AtomicString::createAtomicString(document->starfish(), "*"))))
{
}

ScriptBindingInstance* HTMLAllCollection::scriptBindingInstance()
{
    return m_document->scriptBindingInstance();
}

size_t HTMLAllCollection::length() const
{
    return m_elements->length();
}

Optional<Element*> HTMLAllCollection::defaultIndexedGetter(uint32_t index)
{
    return m_elements->item(index);
}

// HTML limits lookup by name to the all-named elements. An id matches on
// elements in any namespace.
static bool matchesAllName(Node* node, void* data, GCVector<Node*>*)
{
    if (!node->isElement()) {
        return false;
    }
    auto* element = node->asElement();
    auto* name = static_cast<String*>(data);
    if (element->hasId() && element->id()->equals(name)) {
        return true;
    }
    if (!element->isHTMLElement()) {
        return false;
    }
    const auto attr =
        element->getAttribute(element->starfish()->staticStrings()->m_name);
    if (!attr || !attr.value()->equals(name)) {
        return false;
    }
    String* tag = element->name().localName();
    return tag->equals("a") || tag->equals("button") || tag->equals("embed") ||
           tag->equals("form") || tag->equals("frame") ||
           tag->equals("frameset") || tag->equals("iframe") ||
           tag->equals("img") || tag->equals("input") || tag->equals("map") ||
           tag->equals("meta") || tag->equals("object") ||
           tag->equals("select") || tag->equals("textarea");
}

Optional<HTMLCollectionOrElement> HTMLAllCollection::namedItem(String* name)
{
    if (!name->length()) {
        return {};
    }
    Optional<Element*> first;
    for (Element* e = m_document->firstElementChild(); e;
         e = Traverse::nextElement(e, m_document)) {
        if (matchesAllName(e, name, nullptr)) {
            if (first) {
                return HTMLCollectionOrElement::createHTMLCollection(
                    new HTMLCollection(m_document, matchesAllName, name));
            }
            first = e;
        }
    }
    if (first) {
        return HTMLCollectionOrElement::createElement(first.value());
    }
    return {};
}

Optional<HTMLCollectionOrElement> HTMLAllCollection::item(String* name)
{
    // Array index property names are canonical decimals below 2^32 - 1.
    if (!name->length() || name->length() > 10 ||
        (name->length() > 1 && name->charAt(0) == '0')) {
        return namedItem(name);
    }
    uint64_t index = 0;
    for (size_t i = 0; i < name->length(); ++i) {
        auto c = name->charAt(i);
        if (c < '0' || c > '9') {
            return namedItem(name);
        }
        index = index * 10 + (c - '0');
    }
    if (index >= UINT32_MAX) {
        return namedItem(name);
    }
    if (auto element = defaultIndexedGetter(index)) {
        return HTMLCollectionOrElement::createElement(element.value());
    }
    return {};
}

} // namespace Starfish
