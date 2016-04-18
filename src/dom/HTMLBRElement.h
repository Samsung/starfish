#ifndef __StarFishHTMLBRElement__
#define __StarFishHTMLBRElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLBRElement : public HTMLElement {
public:
    HTMLBRElement(Document* document)
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
        return document()->window()->starFish()->staticStrings()->m_brLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLBRElement() const
    {
        return true;
    }

protected:
};

}

#endif
