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
        return document()->window()->starFish()->staticStrings()->m_scriptTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_scriptTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLScriptElement() const
    {
        return true;
    }

    void executeScript();
    String* text();
    void setText(String* s);

    virtual void didCharacterDataModified(String* before, String* after);
    virtual void didNodeInserted(Node* parent, Node* newChild);

    virtual void didNodeInsertedToDocumenTree();
    virtual void finishParsing()
    {
        HTMLElement::finishParsing();
        executeScript();
    }

    virtual Node* clone();

protected:
    bool m_isAlreadyStarted;
};

}

#endif
