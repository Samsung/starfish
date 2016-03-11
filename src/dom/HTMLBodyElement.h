#ifndef __StarFishHTMLBodyElement__
#define __StarFishHTMLBodyElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLBodyElement : public HTMLElement {
public:
    HTMLBodyElement(Document* document)
        : HTMLElement(document)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_bodyLocalName;
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

protected:
};

}

#endif
