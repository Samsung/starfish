/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "core/dom/Attr.h"
#include "core/dom/Attribute.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/dom/NamedNodeMap.h"
#include "core/modules/window/Window.h"

namespace StarFish {

ScriptBindingInstance* NamedNodeMap::scriptBindingInstance()
{
    return m_element->document()->scriptBindingInstance();
}

size_t NamedNodeMap::length()
{
    return m_element->attributeCount(); // The localName have to be excepted
}

Attr* NamedNodeMap::item(unsigned long index)
{
    // The localName is considered
    if (index < m_element->attributeCount()) {
        return m_element->ensureAttr(m_element->getAssuredAttributeName(index));
    } else {
        return nullptr;
    }
}

Attr* NamedNodeMap::getNamedItem(String* name)
{
    return getNamedItem(element()->document()->createAttributeName(name));
}

Attr* NamedNodeMap::getNamedItem(QualifiedName name)
{
    size_t index = m_element->hasAttribute(name);
    if (index < m_element->attributeCount()) {
        return m_element->ensureAttr(name);
    } else {
        return nullptr;
    }
}

Attr* NamedNodeMap::setNamedItem(Attr* attr)
{
    m_element->setAttribute(attr->qname(), attr->value());
    Attr* storedAttr = m_element->attr(attr->qname());
    if (!storedAttr)
        m_element->addAttr(attr);
    return storedAttr;
}

Attr* NamedNodeMap::removeNamedItem(String* name)
{
    StarFish* starfish = element()->starFish();
    QualifiedName qname(AtomicString::emptyAtomicString(),
                        AtomicString::createAttrAtomicString(starfish, name));
    Attr* old = getNamedItem(qname);
    if (old == nullptr) {
        throw new DOMException(element()->document(),
                               DOMException::Code::NOT_FOUND_ERR, nullptr);
    }
    Attr* toReturn = new Attr(old->document(), qname, old->value());
    m_element->removeAttribute(qname);
    return toReturn;
}
}
