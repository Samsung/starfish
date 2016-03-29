#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLStyleElement.h"

#include "style/CSSParser.h"

namespace StarFish {

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
    CSSParser parser(document());
    CSSStyleSheet* sheet = parser.parseStyleSheet(textContent());
    if (sheet) {
        m_generatedSheet = sheet;
        document()->window()->styleResolver()->addSheet(sheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
    }
}

void HTMLStyleElement::removeStyleSheet()
{
    if (m_generatedSheet) {
        document()->window()->styleResolver()->removeSheet(m_generatedSheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

}
