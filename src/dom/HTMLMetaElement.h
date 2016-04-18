#ifndef __StarFishHTMLMetaElement__
#define __StarFishHTMLMetaElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLMetaElement : public HTMLElement {
public:
    HTMLMetaElement(Document* document)
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
        return document()->window()->starFish()->staticStrings()->m_metaLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLMetaElement() const
    {
        return true;
    }

protected:
};

}

#endif
