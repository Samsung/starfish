#ifndef __StarFishHTMLScriptElement__
#define __StarFishHTMLScriptElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLScriptElement : public HTMLElement {
public:
    HTMLScriptElement(Document* document)
        : HTMLElement(document)
        , m_isAlreadyStarted(false)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_scriptLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLScriptElement() const
    {
        return true;
    }

    void executeScript();

    virtual void didNodeInsertedToDocumenTree();

protected:
    bool m_isAlreadyStarted;
};

}

#endif
