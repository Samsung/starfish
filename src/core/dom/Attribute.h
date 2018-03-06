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

#ifndef __StarFishAttribute__
#define __StarFishAttribute__

namespace StarFish {

class Attribute;
class Element;
class AtomicHTMLToken;

typedef String* (*AttributeValueGetter)(Element* element,
                                        const Attribute* const attr);

extern void adjustForeignAttributes(AtomicHTMLToken* token);
class AttributeRareData : public gc {
public:
    AttributeRareData(Element* e)
    {
        m_element = e;
        m_getter = nullptr;
    }
    Element* m_element;
    AttributeValueGetter m_getter;
};

class Attribute {
public:
    Attribute(QualifiedName name, String* value)
        : m_name(name)
    {
        STARFISH_ASSERT(m_name.localName()->length());
        m_value = value;
        m_rareData = nullptr;
    }

    QualifiedName name() const
    {
        STARFISH_ASSERT(m_name.localName()->length());
        return m_name;
    }

    String* value() const
    {
        STARFISH_ASSERT(m_name.localName()->length());

        if (UNLIKELY(m_rareData && m_rareData->m_getter)) {
            return m_rareData->m_getter(m_rareData->m_element, this);
        }
        return m_value;
    }

    String* valueWithoutCheckGetter() const
    {
        STARFISH_ASSERT(m_name.localName()->length());
        return m_value;
    }

    void setValue(String* v)
    {
        m_value = v;
    }

    void registerGetterCallback(Element* element,
                                AttributeValueGetter getter) const
    {
        setupAttributeRareData(element);
        m_rareData->m_getter = getter;
    }
    friend void adjustForeignAttributes(AtomicHTMLToken* token);

private:
    void setupAttributeRareData(Element* element) const
    {
        m_rareData = new AttributeRareData(element);
    }

    void setName(QualifiedName& name)
    {
        m_name = name;
    }

    QualifiedName m_name;
    String* m_value;
    mutable AttributeRareData* m_rareData;
};

Attribute* findAttributeInVector(GCVector<Attribute>& attr,
                                 const QualifiedName& attributeName);
}

#endif
