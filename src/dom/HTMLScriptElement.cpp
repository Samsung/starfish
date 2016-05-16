#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLScriptElement.h"
#include "CharacterData.h"

#include "platform/message_loop/MessageLoop.h"
#include "loader/ElementResourceClient.h"

namespace StarFish {

class ScriptDownloadClient : public ResourceClient {
public:
    ScriptDownloadClient(HTMLScriptElement* script, Resource* res)
        : ResourceClient(res)
        , m_element(script)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        String* text = m_resource->asTextResource()->text();
        m_element->document()->window()->starFish()->evaluate(text);
    }
protected:
    HTMLScriptElement* m_element;
};

void HTMLScriptElement::executeScript()
{
    if (!m_isAlreadyStarted && isInDocumentScope()) {
        String* typeAttr = getAttribute(document()->window()->starFish()->staticStrings()->m_type);
        if (!typeAttr->equals(String::emptyString) && !typeAttr->equals("text/javascript"))
            return;
        size_t idx = hasAttribute(document()->window()->starFish()->staticStrings()->m_src);
        if (idx == SIZE_MAX) {
            if (!firstChild())
                return;
            String* script = text();
            m_isAlreadyStarted = true;
            document()->window()->starFish()->evaluate(script);
        } else {
            String* url = getAttribute(idx);
            m_isAlreadyStarted = true;

            if (!url->length())
                return;

            TextResource* res = document()->resourceLoader()->fetchText(URL(document()->documentURI().string(), url));
            res->addResourceClient(new ScriptDownloadClient(this, res));
            res->addResourceClient(new ElementResourceClient(this, res, true));
            if (m_isParserInserted) {
                res->request(true);
            } else {
                res->request(false);
            }
        }
    }
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
    return n;
}

}
