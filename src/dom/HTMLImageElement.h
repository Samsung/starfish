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

    /* HTMLImageElement related */

    void setSrc(String* src)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
        didAttributeChanged(document()->window()->starFish()->staticStrings()->m_src, String::emptyString, src);
    }

    String* src()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_src);
    }

    int width()
    {
        String* width = getAttribute(document()->window()->starFish()->staticStrings()->m_width);
        if (width->equals(String::emptyString)) {
            return -1; // indicates that width is not defined
        } else {
            return String::parseInt(width);
        }
    }

    void setWidth(int width)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_width, String::fromInt(width));
        didAttributeChanged(document()->window()->starFish()->staticStrings()->m_width, String::emptyString, String::fromInt(width));
    }

    int height()
    {
        String* height = getAttribute(document()->window()->starFish()->staticStrings()->m_height);
        if (height->equals(String::emptyString)) {
            return -1; // indicates that height is not defined
        } else {
            return String::parseInt(height);
        }
    }

    void setHeight(int height)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_height, String::fromInt(height));
        didAttributeChanged(document()->window()->starFish()->staticStrings()->m_height, String::emptyString, String::fromInt(height));
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLImageElement() const
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value);

private:

};

}

#endif
