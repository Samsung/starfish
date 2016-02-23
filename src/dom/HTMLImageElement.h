#ifndef __StarFishHTMLImageElement__
#define __StarFishHTMLImageElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLImageElement : public HTMLElement {
public:
    HTMLImageElement(Document* document)
        : HTMLElement(document)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_imageLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLImageElement() const
    {
        return true;
    }

    void setSrc(String* src)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
    }

    String* src()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_src);
    }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value)
    {
        HTMLElement::didAttributeChanged(name, old, value);
        if (name == document()->window()->starFish()->staticStrings()->m_src) {
            m_src = value;
            setNeedsFrameTreeBuild();
        }
    }

protected:
    String* m_src;
};

}

#endif
