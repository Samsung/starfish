#ifndef __StarFishHTMLUnknownElement__
#define __StarFishHTMLUnknownElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLUnknownElement : public HTMLElement {
public:
    HTMLUnknownElement(Document* document, QualifiedName localName)
        : HTMLElement(document)
        , m_name(localName)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return m_name.localName();
    }

    virtual QualifiedName name()
    {
        return m_name;
    }

    /* Other methods (not in DOM API) */
    virtual bool isHTMLUnknownElement() const
    {
        return true;
    }

protected:
    QualifiedName m_name;
};

}

#endif
