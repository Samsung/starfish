#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLStyleElement.h"

#include "style/CSSParser.h"

namespace StarFish {

void HTMLStyleElement::didCharacterDataModified(String* before, String* after)
{
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
}

void HTMLStyleElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    removeStyleSheet();
}

void HTMLStyleElement::generateStyleSheet()
{
    STARFISH_ASSERT(isInDocumentScope());
    CSSParser parser(document());
    CSSStyleSheet* sheet = parser.parseStyleSheet(textContent(), this);
    if (sheet) {
        m_generatedSheet = sheet;
        document()->styleResolver()->addSheet(sheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
    }
}

void HTMLStyleElement::removeStyleSheet()
{
    if (m_generatedSheet) {
        document()->styleResolver()->removeSheet(m_generatedSheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

}
