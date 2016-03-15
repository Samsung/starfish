#ifndef __StarFishFrameReplacedImage__
#define __StarFishFrameReplacedImage__

#include "layout/FrameReplaced.h"
#include "platform/canvas/image/ImageData.h"

namespace StarFish {

class FrameReplacedImage : public FrameReplaced {
public:
    FrameReplacedImage(Node* node, String* src)
        : FrameReplaced(node, nullptr)
        , m_imageData(nullptr)
    {
        replaceImageData(node, src);
    }

    virtual bool isFrameReplacedImage()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedImage";
    }

    virtual void paintReplaced(Canvas* canvas)
    {
        if (m_imageData)
            canvas->drawImage(m_imageData, Rect(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
            width() - borderWidth() - paddingWidth(), height() - borderHeight() - paddingHeight()));
    }

    virtual LayoutSize intrinsicSize()
    {
        if (m_imageData) {
            STARFISH_ASSERT(node()->asElement()->asHTMLElement()->isHTMLImageElement());
            HTMLImageElement* imgNode = node()->asElement()->asHTMLElement()->asHTMLImageElement();
            int width = imgNode->width() >= 0? imgNode->width(): m_imageData->width();
            int height = imgNode->height() >= 0? imgNode->height(): m_imageData->height();
            return LayoutSize(width, height);
        } else {
            return LayoutSize(0, 0);
        }
    }

    void replaceImageData(Node* node, String* src)
    {
        if (src == String::emptyString || src->equals(String::emptyString)) {
            m_imageData = nullptr;
        } else {
            m_imageData = node->document()->window()->starFish()->fetchImage(node->document()->window()->starFish()->makeResourcePath(src));
        }
    }

protected:
    ImageData* m_imageData;
};
}

#endif
