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
        STARFISH_ASSERT(m_name.string()->length());
        return m_name;
    }

    String* value() const
    {
        STARFISH_ASSERT(m_name.string()->length());

        if (UNLIKELY(m_rareData && m_rareData->m_getter))
            return m_rareData->m_getter(m_rareData->m_element, this);
        return m_value;
    }

    String* valueWithoutCheckGetter() const
    {
        STARFISH_ASSERT(m_name.string()->length());
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

}


#endif
