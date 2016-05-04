#ifndef __StarFishHTMLLinkElement__
#define __StarFishHTMLLinkElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLLinkElement : public HTMLElement {
public:
    HTMLLinkElement(Document* document)
        : HTMLElement(document)
        , m_generatedSheet(nullptr)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_linkLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLLinkElement() const
    {
        return true;
    }

    URL href();

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);
    virtual void didNodeInsertedToDocumenTree();
    virtual void didNodeRemovedFromDocumenTree();
    void checkLoadStyleSheet();
    void loadStyleSheet();
    void unloadStyleSheetIfExists();
    CSSStyleSheet* generatedSheet()
    {
        return m_generatedSheet;
    }
protected:
    CSSStyleSheet* m_generatedSheet;
};

}

#endif
