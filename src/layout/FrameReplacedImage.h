/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
