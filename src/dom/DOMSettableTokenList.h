#ifndef __StarFishDOMSettableTokenList__
#define __StarFishDOMSettableTokenList__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Element;
class DOMSettableTokenList : public DOMTokenList {
public:
    DOMSettableTokenList(ScriptBindingInstance* instance, Element* element, QualifiedName localName)
        : DOMTokenList(instance, element, localName)
    {
        initScriptWrappable(this, instance);
    }

    /* 7.2. Interface DOMSettableTokenList */

    // FIXME:mh.byun(wrong behavior)
    String* value()
    {
        return m_value;
    }

    // FIXME:mh.byun(wrong behavior)
    void setValue(String* value)
    {
        m_value = value;
    }

private:
    String* m_value;
};
}

#endif
