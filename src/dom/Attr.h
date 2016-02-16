#ifndef __StarFishAttr__
#define __StarFishAttr__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Element;
class Attr : public ScriptWrappable {
public:
    Attr(ScriptBindingInstance* instance, Element* element, QualifiedName name)
        : ScriptWrappable(this), m_element(element), m_name(name), m_standAloneValue(nullptr)
    {
        initScriptWrappable(this, instance);
    }

    Attr(ScriptBindingInstance* instance, QualifiedName name)
        : ScriptWrappable(this), m_element(nullptr), m_name(name), m_standAloneValue(nullptr)
    {
        initScriptWrappable(this, instance);
    }

    Attr(ScriptBindingInstance* instance, QualifiedName name, String* value)
        : ScriptWrappable(this), m_element(nullptr), m_name(name), m_standAloneValue(value)
    {
        initScriptWrappable(this, instance);
    }

    QualifiedName name()
    {
        return m_name;
    }

    String* value()
    {
        if (m_element)
            return m_element->getAttribute(m_name);
        return m_standAloneValue;
    }

    void setValue(String* value)
    {
        if (m_element)
            m_element->setAttribute(m_name, value);
        else
            m_standAloneValue = value;
    }

    Element* ownerElement() { return m_element; }

private:
    Element* m_element;
    QualifiedName m_name;
    String* m_standAloneValue;
};

}

#endif
