#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLLinkElement.h"

#include "platform/message_loop/MessageLoop.h"
#include "style/CSSParser.h"

namespace StarFish {

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
    FILE* fp = fopen(document()->window()->starFish()->makeResourcePath(url)->utf8Data(), "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size_t siz = ftell(fp);
        rewind(fp);

        char* fileContents = (char*)malloc(siz + 1);
        fread(fileContents, sizeof(char), siz, fp);

        fileContents[siz] = 0;
        String* source = String::fromUTF8(fileContents);
        free(fileContents);
        fclose(fp);

        CSSParser parser(document());
        CSSStyleSheet* sheet = parser.parseStyleSheet(source);
        if (sheet) {
            m_generatedSheet = sheet;
            document()->window()->styleResolver()->addSheet(sheet);
            document()->window()->setWholeDocumentNeedsStyleRecalc();
        }

        document()->window()->starFish()->messageLoop()->addIdler([](void* data) {
            HTMLLinkElement* element = (HTMLLinkElement*)data;
            QualifiedName eventType = element->document()->window()->starFish()->staticStrings()->m_load;
            Event* e = new Event(eventType, EventInit(false, false));
            element->EventTarget::dispatchEvent(element, e);
        }, this);
    }
}

void HTMLLinkElement::unloadStyleSheetIfExists()
{
    if (m_generatedSheet) {
        document()->window()->styleResolver()->removeSheet(m_generatedSheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLLinkElement::didAttributeChanged(QualifiedName name, String* old, String* value)
{
    HTMLElement::didAttributeChanged(name, old, value);
    if (name == document()->window()->starFish()->staticStrings()->m_href) {
        checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_type) {
        checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_rel) {
        checkLoadStyleSheet();
    }
}

}
