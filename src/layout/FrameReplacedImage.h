#ifndef __StarFishFrameReplacedImage__
#define __StarFishFrameReplacedImage__

#include "layout/FrameReplaced.h"
#include "platform/canvas/image/ImageData.h"

namespace StarFish {

class FrameReplacedImage : public FrameReplaced {
public:
    FrameReplacedImage(Node* node, String* src)
        : FrameReplaced(node, nullptr)
    {
        if (src == String::emptyString || src->equals(String::emptyString)) {
            m_imageData = nullptr;
            return;
        }
        m_imageData = node->document()->window()->starFish()->fetchImage(node->document()->window()->starFish()->makeResourcePath(src));
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

    virtual Size intrinsicSize()
    {
        if (m_imageData)
            return Size(m_imageData->width(), m_imageData->height());
        else
            return Size(0, 0);
    }

protected:
    ImageData* m_imageData;
};
}

#endif
