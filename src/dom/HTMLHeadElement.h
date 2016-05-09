#ifndef __StarFishHTMLHeadElement__
#define __StarFishHTMLHeadElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLHeadElement : public HTMLElement {
public:
    HTMLHeadElement(Document* document)
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
        return document()->window()->starFish()->staticStrings()->m_headTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_headTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLHeadElement() const
    {
        return true;
    }

protected:
};

}

#endif
