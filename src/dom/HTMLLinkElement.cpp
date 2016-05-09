#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLLinkElement.h"

#include "platform/message_loop/MessageLoop.h"
#include "style/CSSParser.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

URL HTMLLinkElement::href()
{
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);
    if (href != SIZE_MAX) {
        String* url = getAttribute(href);
        return URL(url);
    }
    return URL();
}

void HTMLLinkElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    checkLoadStyleSheet();
}

void HTMLLinkElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    unloadStyleSheetIfExists();
}

void HTMLLinkElement::checkLoadStyleSheet()
{
    if (!isInDocumentScope()) {
        unloadStyleSheetIfExists();
        return;
    }

    size_t type = hasAttribute(document()->window()->starFish()->staticStrings()->m_type);
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);
    size_t rel = hasAttribute(document()->window()->starFish()->staticStrings()->m_rel);

    if (type != SIZE_MAX && getAttribute(type)->toLower()->equals("text/css")
        && href != SIZE_MAX
        && rel != SIZE_MAX && getAttribute(rel)->toLower()->equals("stylesheet")) {
        loadStyleSheet();
    } else {
        unloadStyleSheetIfExists();
    }
}

void HTMLLinkElement::loadStyleSheet()
{
    unloadStyleSheetIfExists();
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);

    String* url = getAttribute(href);
    FileIO* fio = FileIO::create();
    if (fio->open(document()->window()->starFish()->makeResourcePath(url))) {
        size_t siz = fio->length();

        char* fileContents = (char*)malloc(siz + 1);
        fio->read(fileContents, sizeof(char), siz);

        fileContents[siz] = 0;
        String* source = String::fromUTF8(fileContents);
        free(fileContents);
        fio->close();

        CSSParser parser(document());
        CSSStyleSheet* sheet = parser.parseStyleSheet(source, this);
        if (sheet) {
            m_generatedSheet = sheet;
            document()->styleResolver()->addSheet(sheet);
            document()->window()->setWholeDocumentNeedsStyleRecalc();
        }

        document()->window()->starFish()->messageLoop()->addIdler([](void* data) {
            HTMLLinkElement* element = (HTMLLinkElement*)data;
            String* eventType = element->document()->window()->starFish()->staticStrings()->m_load.localName();
            Event* e = new Event(eventType, EventInit(false, false));
            element->EventTarget::dispatchEvent(element, e);
        }, this);
    }
}

void HTMLLinkElement::unloadStyleSheetIfExists()
{
    if (m_generatedSheet) {
        document()->styleResolver()->removeSheet(m_generatedSheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLLinkElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_href) {
        checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_type) {
        checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_rel) {
        checkLoadStyleSheet();
    }
}

}
