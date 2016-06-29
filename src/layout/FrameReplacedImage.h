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
        if (node()->asElement()->asHTMLElement()->asHTMLImageElement()->imageData())
            canvas->drawImage(node()->asElement()->asHTMLElement()->asHTMLImageElement()->imageData(), Rect(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
            width() - borderWidth() - paddingWidth(), height() - borderHeight() - paddingHeight()));
    }

    virtual std::pair<Length, Length> intrinsicSize()
    {
        return node()->asElement()->asHTMLElement()->asHTMLImageElement()->intrinsicSize();
    }

    ImageData* imageData()
    {
        return nullptr;
    }
protected:
};
}

#endif
