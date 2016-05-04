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
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
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

    virtual void didCharacterDataModified(String* before, String* after);
    virtual void didNodeInsertedToDocumenTree();
    virtual void didNodeRemovedFromDocumenTree();
    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);
    virtual void finishParsing()
    {
        HTMLElement::finishParsing();
        generateStyleSheet();
    }

    void generateStyleSheet();
    void removeStyleSheet();
    CSSStyleSheet* generatedSheet()
    {
        return m_generatedSheet;
    }

protected:
    CSSStyleSheet* m_generatedSheet;
};

}

#endif
