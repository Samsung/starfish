#ifndef __StarFishHTMLImageElement__
#define __StarFishHTMLImageElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLImageElement : public HTMLElement {
public:
    HTMLImageElement(Document* document)
        : HTMLElement(document)
        , m_imageData(nullptr)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_imgLocalName;
    }

    /* HTMLImageElement related */

    void setSrc(String* src)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
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
    }

    LayoutSize intrinsicSize()
    {
        if (m_imageData) {
            int w = width() >= 0 ? width(): m_imageData->width();
            int h = height() >= 0 ? height(): m_imageData->height();
            return LayoutSize(w, h);
        } else {
            return LayoutSize(0, 0);
        }
    }

    ImageData* imageData()
    {
        return m_imageData;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLImageElement() const
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);

private:
    ImageData* m_imageData;
};

}

#endif
