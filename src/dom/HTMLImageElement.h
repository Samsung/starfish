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

    int width() { return m_width; }
    void setWidth(int width) { m_width = width; }
    int height() { return m_height; }
    void setHeight(int height) { m_height = height; }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value);

protected:
    virtual Node* clone()
    {
        HTMLImageElement* newNode = static_cast<HTMLImageElement*>(Element::clone());
        newNode->m_src = m_src;
        return newNode;
    }

    String* m_src;
    int m_width;
    int m_height;
};

}

#endif
