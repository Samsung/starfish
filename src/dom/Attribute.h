#ifndef __StarFishAttribute__
#define __StarFishAttribute__

#include "util/String.h"
#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Attribute {
public:

    Attribute()
        : m_name(QualifiedName::emptyQualifiedName())
    {
        m_value = String::emptyString;
    }

    Attribute(QualifiedName name, String* value)
        : m_name(name)
    {
        m_value = value;
    }

    QualifiedName name()
    {
        STARFISH_ASSERT(m_name.string()->length());
        return m_name;
    }

    String* value()
    {
        STARFISH_ASSERT(m_name.string()->length());
        return m_value;
    }

    void setValue(String* v)
    {
        m_value = v;
    }
protected:
    QualifiedName m_name;
    String* m_value;
};

}

#endif
