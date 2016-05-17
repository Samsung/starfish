#ifndef __StarFishHTMLBodyElement__
#define __StarFishHTMLBodyElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLBodyElement : public HTMLElement {
public:
    HTMLBodyElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_bodyTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_bodyTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLBodyElement() const
    {
        return true;
    }

    virtual void didComputedStyleChanged(ComputedStyle* oldStyle, ComputedStyle* newStyle)
    {
        HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
        if (!newStyle->backgroundColor().isTransparent() || newStyle->backgroundImageData()) {
            document()->window()->m_hasBodyElementBackground = true;
        } else {
            document()->window()->m_hasBodyElementBackground = false;
        }
    }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);
protected:
};

}

#endif
