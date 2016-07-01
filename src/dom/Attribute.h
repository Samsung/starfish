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

#ifndef __StarFishAttribute__
#define __StarFishAttribute__

#include "util/String.h"
#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Attribute;
typedef String* (*AttributeValueGetter)(Element* element, const Attribute * const attr);

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

        if (UNLIKELY(m_rareData && m_rareData->m_getter))
            return m_rareData->m_getter(m_rareData->m_element, this);
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

    void registerGetterCallback(Element* element, AttributeValueGetter getter) const
    {
        setupAttributeRareData(element);
        m_rareData->m_getter = getter;
    }
private:
    void setupAttributeRareData(Element* element) const
    {
        m_rareData = new AttributeRareData(element);
    }
    QualifiedName m_name;
    String* m_value;
    mutable AttributeRareData* m_rareData;
};

typedef std::vector<Attribute, gc_allocator<Attribute>> AttributeVector;

Attribute* findAttributeInVector(AttributeVector& attr, const QualifiedName& attributeName);
}


#endif
