#include "StarFishConfig.h"
#include "Document.h"
#include "Text.h"
#include "HTMLElement.h"


namespace StarFish {

void HTMLElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    Element::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_onclick) {
        String* name[] = {String::createASCIIString("event")};
        setOnclick(createScriptFunction(name, 1, value));
    } else if (name == ss->m_onload) {
        String* name[] = {String::createASCIIString("event")};
        setOnload(createScriptFunction(name, 1, value));
    } else if (name == ss->m_onunload) {
        String* name[] = {String::createASCIIString("event")};
        setOnunload(createScriptFunction(name, 1, value));
    } else if (name == ss->m_dir) {
        if (attributeCreated)
            m_hasDirAttribute = true;
        if (attributeRemoved)
            m_hasDirAttribute = false;
        String* orgValue = value;
        value = value->toLower();
        if (value->equals("")) {
            return;
        } else if (value->equals("ltr") || value->equals("rtl")) {
            if (!orgValue->equals(value))
                setAttribute(ss->m_dir, value);
        } else {
            setAttribute(ss->m_dir, String::emptyString);
        }
    }
}

}
