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
        setOnclick(value);
    } else if (name == ss->m_onload) {
        setOnload(value);
    } else if (name == ss->m_onunload) {
        setOnunload(value);
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
