#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLMetaElement.h"

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
#endif

void HTMLMetaElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_name) {
#ifdef STARFISH_ENABLE_TEST
        if (value->equals("pixel-test")) {
            g_enablePixelTest = true;
            document()->setStyle(document()->styleResolver()->resolveDocumentStyle(document()));
            document()->window()->setWholeDocumentNeedsStyleRecalc();
        }
#endif
    }

}

}
