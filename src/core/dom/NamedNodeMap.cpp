/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
