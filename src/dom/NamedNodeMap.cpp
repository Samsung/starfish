/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "NamedNodeMap.h"
#include "dom/Document.h"
#include "dom/Element.h"
#include "dom/Attribute.h"
#include "dom/Attr.h"

namespace StarFish {

unsigned long NamedNodeMap::length()
{
    return m_element->attributeCount(); // The localName have to be excepted
}

Attr* NamedNodeMap::item(unsigned long index)
{
    // The localName is considered
    if (index < m_element->attributeCount()) {
        return m_element->ensureAttr(m_element->getAttributeName(index));
    } else
        return nullptr;
}

Attr* NamedNodeMap::getNamedItem(QualifiedName name)
{
    size_t index = m_element->hasAttribute(name);
    if (index < m_element->attributeCount()) {
        return m_element->ensureAttr(name);
    } else
        return nullptr;
}

Attr* NamedNodeMap::setNamedItem(Attr* attr)
{
    m_element->setAttribute(attr->name(), attr->value());
    Attr* storedAttr = m_element->attr(attr->name());
    if (!storedAttr)
        m_element->addAttr(attr);
    return storedAttr;
}

void NamedNodeMap::removeNamedItem(QualifiedName name)
{
    m_element->removeAttribute(name);
}

}
