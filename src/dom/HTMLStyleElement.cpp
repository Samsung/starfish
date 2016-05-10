#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLStyleElement.h"
#include "dom/Text.h"
#include "dom/Traverse.h"

#include "style/CSSParser.h"

namespace StarFish {

void HTMLStyleElement::didCharacterDataModified(String* before, String* after)
{
    HTMLElement::didCharacterDataModified(before, after);
    if (isInDocumentScope()) {
        // FIXME
        // parse style sheet every modified-time is bad idea
        // should we parse stylesheet in rendering-time?
        removeStyleSheet();
        generateStyleSheet();
    }
}

void HTMLStyleElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    if (isInDocumentScope()) {
        // FIXME
        // parse style sheet every modified-time is bad idea
        // should we parse stylesheet in rendering-time?
        removeStyleSheet();
        generateStyleSheet();
    }
}

void HTMLStyleElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    HTMLElement::didNodeInserted(parent, oldChild);
    if (isInDocumentScope()) {
        // FIXME
        // parse style sheet every modified-time is bad idea
        // should we parse stylesheet in rendering-time?
        removeStyleSheet();
        generateStyleSheet();
    }
}

void HTMLStyleElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    generateStyleSheet();
    dispatchLoadEvent();
}

void HTMLStyleElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    removeStyleSheet();
}

void HTMLStyleElement::generateStyleSheet()
{
    STARFISH_ASSERT(isInDocumentScope());

    if (m_inParsing)
        return;

    if (m_generatedSheet)
        removeStyleSheet();

    CSSParser parser(document());

    String* str = String::emptyString;

    Node* child = firstChild();
    while (child) {
        if (child->isCharacterData() && child->asCharacterData()->isText()) {
            str = str->concat(child->asCharacterData()->data());
        }
        child = child->nextSibling();
    }

    CSSStyleSheet* sheet = parser.parseStyleSheet(str, this);
    m_generatedSheet = sheet;
    document()->styleResolver()->addSheet(sheet);
    document()->window()->setWholeDocumentNeedsStyleRecalc();
}

void HTMLStyleElement::removeStyleSheet()
{
    if (m_generatedSheet) {
        document()->styleResolver()->removeSheet(m_generatedSheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLStyleElement::dispatchLoadEvent()
{
    String* eventType = document()->window()->starFish()->staticStrings()->m_load.localName();
    Event* e = new Event(eventType, EventInit(false, false));
    dispatchEvent(e);
}

}
