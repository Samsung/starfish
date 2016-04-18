#ifndef __StarFishHTMLDivElement__
#define __StarFishHTMLDivElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLDivElement : public HTMLElement {
public:
    HTMLDivElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_divLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLDivElement() const
    {
        return true;
    }

protected:
};

}

#endif
