#ifndef __StarFishHTMLImageElement__
#define __StarFishHTMLImageElement__

#include "dom/HTMLElement.h"
#include "loader/ImageResource.h"

namespace StarFish {

class HTMLImageElement : public HTMLElement {
    friend class ImageDownloadClient;
public:
    HTMLImageElement(Document* document)
        : HTMLElement(document)
        , m_imageResource(nullptr)
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
        return document()->window()->starFish()->staticStrings()->m_imgTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_imgTagName;
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

    String* width()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_width);
    }

    void setWidth(int width)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_width, String::fromInt(width));
    }

    String* height()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_height);
    }

    void setHeight(int height)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_height, String::fromInt(height));
    }

    LayoutSize intrinsicSize()
    {
        if (m_imageData) {
            String* widthString = width();
            String* heightString = height();
            bool widthIsEmpty = widthString->equals(String::emptyString);
            bool heightIsEmpty = heightString->equals(String::emptyString);
            if (widthIsEmpty && heightIsEmpty) {
                return LayoutSize(m_imageData->width(), m_imageData->height());
            } else if (widthIsEmpty) {
                int h = String::parseInt(heightString);
                return LayoutSize(h, h);
            } else if (heightIsEmpty) {
                int w = String::parseInt(widthString);
                return LayoutSize(w, w);
            } else {
                int w = String::parseInt(widthString);
                int h = String::parseInt(heightString);
                return LayoutSize(w, h);
            }
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
    ImageResource* m_imageResource;
    ImageData* m_imageData;
};

}

#endif
