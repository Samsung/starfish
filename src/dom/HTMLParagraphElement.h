#ifndef __StarFishHTMLParagraphElement__
#define __StarFishHTMLParagraphElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLParagraphElement : public HTMLElement {
public:
    HTMLParagraphElement(Document* document)
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
        return document()->window()->starFish()->staticStrings()->m_pTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_pTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLParagraphElement() const
    {
        return true;
    }

protected:
};

}

#endif
