#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLScriptElement.h"
#include "CharacterData.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

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
            String* script = textContent();
            m_isAlreadyStarted = true;
            document()->window()->starFish()->evaluate(script);
        } else {
            String* url = getAttribute(idx);
            FileIO* fio = FileIO::create();
            if (fio->open(document()->window()->starFish()->makeResourcePath(url))) {
                size_t siz = fio->length();

                char* fileContents = (char*)malloc(siz + 1);
                fio->read(fileContents, sizeof(char), siz);

                fileContents[siz] = 0;

                m_isAlreadyStarted = true;
                document()->window()->starFish()->evaluate(String::fromUTF8(fileContents));

                free(fileContents);
                fio->close();

                document()->window()->starFish()->messageLoop()->addIdler([](void* data) {
                    HTMLScriptElement* element = (HTMLScriptElement*)data;
                    String* eventType = element->document()->window()->starFish()->staticStrings()->m_load.localName();
                    Event* e = new Event(eventType, EventInit(false, false));
                    element->EventTarget::dispatchEvent(element, e);
                }, this);
            }
        }
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
    return textContent();
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
