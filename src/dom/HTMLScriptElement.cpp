#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLScriptElement.h"
#include "CharacterData.h"

#include "platform/message_loop/MessageLoop.h"
#include "loader/ElementResourceClient.h"

#include "dom/builder/html/HTMLDocumentBuilder.h"
#include "dom/parser/HTMLParser.h"

namespace StarFish {

class ScriptDownloadClient : public ResourceClient {
public:
    ScriptDownloadClient(HTMLScriptElement* script, Resource* res, bool forceSync, bool inParser)
        : ResourceClient(res)
        , m_element(script)
        , m_forceSync(forceSync)
        , m_inParser(inParser)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        didScriptLoaded();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        String* text = m_resource->asTextResource()->text();
        m_element->document()->window()->starFish()->evaluate(text);
        didScriptLoaded();
    }

    void didScriptLoaded()
    {
        m_element->m_didScriptExecuted = true;
        if (m_inParser && !m_forceSync)
            m_element->document()->resumeDocumentParsing();
    }
protected:
    HTMLScriptElement* m_element;
    bool m_forceSync;
    bool m_inParser;
};

bool HTMLScriptElement::executeScript(bool forceSync, bool inParser)
{
    if (m_isParserInserted)
        return false;

    if (!m_isAlreadyStarted && isInDocumentScope()) {
        String* typeAttr = getAttribute(document()->window()->starFish()->staticStrings()->m_type);
        if (!typeAttr->equals(String::emptyString) && !typeAttr->equals("text/javascript"))
            return false;
        size_t idx = hasAttribute(document()->window()->starFish()->staticStrings()->m_src);
        if (idx == SIZE_MAX) {
            if (!firstChild()) {
                return false;
            }
            String* script = text();
            m_isAlreadyStarted = true;
            document()->window()->starFish()->evaluate(script);
            m_didScriptExecuted = true;
            return false;
        } else {
            String* url = getAttribute(idx);
            m_isAlreadyStarted = true;

            if (!url->length())
                return false;

            TextResource* res = document()->resourceLoader()->fetchText(URL(document()->documentURI().string(), url));
            res->addResourceClient(new ScriptDownloadClient(this, res, forceSync, inParser));
            res->addResourceClient(new ElementResourceClient(this, res, true));
            res->request(forceSync);
            return true;
        }
    }
    return false;
}

void HTMLScriptElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        executeScript();
    }
}

void HTMLScriptElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    executeScript();
}

void HTMLScriptElement::didCharacterDataModified(String* before, String* after)
{
    HTMLElement::didCharacterDataModified(before, after);
    executeScript();
}

void HTMLScriptElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    executeScript();
}

String* HTMLScriptElement::text()
{
    String* str = String::createASCIIString("");
    for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
        if (child->nodeType() == TEXT_NODE) {
            str = str->concat(child->textContent());
        }
    }
    return str;
}

void HTMLScriptElement::setText(String* s)
{
    if (firstChild() && firstChild()->isText()) {
        firstChild()->asCharacterData()->setData(s);
    } else {
        setTextContent(s);
    }
}

Node* HTMLScriptElement::clone()
{
    HTMLScriptElement* n = HTMLElement::clone()->asElement()->asHTMLElement()->asHTMLScriptElement();
    n->m_isAlreadyStarted = m_isAlreadyStarted;
    n->m_didScriptExecuted = m_didScriptExecuted;
    return n;
}

}
