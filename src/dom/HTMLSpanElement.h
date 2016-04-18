#ifndef __StarFishHTMLSpanElement__
#define __StarFishHTMLSpanElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLSpanElement : public HTMLElement {
public:
    HTMLSpanElement(Document* document)
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
        return document()->window()->starFish()->staticStrings()->m_spanLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLSpanElement() const
    {
        return true;
    }

protected:
};

}

#endif
