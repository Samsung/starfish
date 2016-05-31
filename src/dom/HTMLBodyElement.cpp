#include "StarFishConfig.h"
#include "Document.h"
#include "HTMLBodyElement.h"

namespace StarFish {

void HTMLBodyElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_onload) {
        document()->window()->setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onunload) {
        document()->window()->setAttributeEventListener(ss->m_unload, value, this);
    }
}

}
