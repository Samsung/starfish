#ifndef __StarFishHTMLScriptElement__
#define __StarFishHTMLScriptElement__

#include "dom/HTMLElement.h"
#include "loader/TextResource.h"

namespace StarFish {

class HTMLScriptElement : public HTMLElement {
    friend class ScriptDownloadClient;
public:
    HTMLScriptElement(Document* document)
        : HTMLElement(document)
        , m_isAlreadyStarted(false)
        , m_isParserInserted(false)
        , m_didScriptExecuted(false)
    {
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("+++tag:script\n");
#endif
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

    String* text();
    void setText(String* s);

    virtual void didCharacterDataModified(String* before, String* after);
    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);

    virtual void didNodeInsertedToDocumenTree();
    virtual Node* clone();
    void markParserInserted()
    {
        m_isParserInserted = true;
    }
    void clearParserInserted()
    {
        m_isParserInserted = false;
    }
    bool didScriptExecuted()
    {
        return m_didScriptExecuted;
    }
    bool executeScript(bool forceSync = false, bool inParser = false);

protected:
    bool m_isAlreadyStarted;
    bool m_isParserInserted;
    bool m_didScriptExecuted;
};

}

#endif
