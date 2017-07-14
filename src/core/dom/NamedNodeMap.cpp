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
#include "core/page/Window.h"
#include "core/util/AttributeName.h"

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
    return m_element->getAttributeNode(name);
}

Attr* NamedNodeMap::getNamedItemNS(Nullable<String*> ns, String* localName)
{
    return m_element->getAttributeNodeNS(ns, localName);
}

Attr* NamedNodeMap::setNamedItem(Attr* attr)
{
    return m_element->setAttributeNode(attr);
}

Attr* NamedNodeMap::setNamedItemNS(Attr* attr)
{
    return m_element->setAttributeNodeNS(attr);
}

Attr* NamedNodeMap::removeNamedItem(String* name)
{
    Attr* old = getNamedItem(name);
    if (old == nullptr) {
        throw new DOMException(element()->document(),
                               DOMException::Code::NOT_FOUND_ERR, nullptr);
    }
    return m_element->removeAttributeNode(old);
}

Attr* NamedNodeMap::removeNamedItemNS(Nullable<String*> ns, String* localName)
{
    Attr* old = getNamedItemNS(ns, localName);
    if (old == nullptr) {
        throw new DOMException(element()->document(),
                               DOMException::Code::NOT_FOUND_ERR, nullptr);
    }
    return m_element->removeAttributeNode(old);
}
}
