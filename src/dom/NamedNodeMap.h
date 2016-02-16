#ifndef __StarFishNamedNodeMap__
#define __StarFishNamedNodeMap__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Element;
class Attr;
class NamedNodeMap : public ScriptWrappable {
public:
    NamedNodeMap(ScriptBindingInstance* instance, Element* element)
        : ScriptWrappable(this), m_element(element), m_instance(instance)
    {
        initScriptWrappable(this, instance);
    }
    unsigned long length();
    Attr* item(unsigned long index);
    Attr* getNamedItem(QualifiedName name);
    void setNamedItem(Attr* attr);
    void removeNamedItem(QualifiedName name);
    ScriptBindingInstance* striptBindingInstance() { return m_instance; }
    Element* element() { return m_element; }
private:
    Element* m_element;
    ScriptBindingInstance* m_instance;
};

}

#endif
