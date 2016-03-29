#ifndef __StarFishHTMLStyleElement__
#define __StarFishHTMLStyleElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLStyleElement : public HTMLElement {
public:
    HTMLStyleElement(Document* document)
        : HTMLElement(document)
        , m_generatedSheet(nullptr)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_styleLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLStyleElement() const
    {
        return true;
    }

    virtual void didNodeInsertedToDocumenTree();
    virtual void didNodeRemovedFromDocumenTree();

    void generateStyleSheet();
    void removeStyleSheet();

protected:
    CSSStyleSheet* m_generatedSheet;
};

}

#endif
