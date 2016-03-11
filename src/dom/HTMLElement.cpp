#include "StarFishConfig.h"
#include "Document.h"
#include "Text.h"
#include "HTMLElement.h"


namespace StarFish {

void HTMLElement::didAttributeChanged(QualifiedName name, String* old, String* value)
{
    Element::didAttributeChanged(name, old, value);
    if (name == document()->window()->starFish()->staticStrings()->m_onclick) {
        String* name[] = {String::createASCIIString("event")};
        setOnclick(createScriptFunction(name, 1, value));
    } else if (name == document()->window()->starFish()->staticStrings()->m_onload) {
        String* name[] = {String::createASCIIString("event")};
        setOnload(createScriptFunction(name, 1, value));
    } else if (name == document()->window()->starFish()->staticStrings()->m_onunload) {
        String* name[] = {String::createASCIIString("event")};
        setOnunload(createScriptFunction(name, 1, value));
    }
}

}
