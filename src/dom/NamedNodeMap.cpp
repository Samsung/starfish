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
    if (m_element->getAttribute(name) != String::emptyString)
        return new Attr(m_element->document(), m_instance, m_element, name);
    return nullptr;
}

void NamedNodeMap::setNamedItem(Attr* attr)
{
    m_element->setAttribute(attr->name(), attr->value());
}

void NamedNodeMap::removeNamedItem(QualifiedName name)
{
    m_element->removeAttribute(name);
}

}
