/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
    FrameReplacedImage(Node* node) : FrameReplaced(node, nullptr)
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
        ImageData* id = node()
                            ->asElement()
                            ->asHTMLElement()
                            ->asHTMLImageElement()
                            ->imageData();
        if (id) {
            canvas->drawImage(
                id,
                Rect(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                     width() - borderWidth() - paddingWidth(),
                     height() - borderHeight() - paddingHeight()));
        }
    }

    virtual IntrinsicSize intrinsicSize()
    {
        IntrinsicSize result;
        ImageData* id = node()
                            ->asElement()
                            ->asHTMLElement()
                            ->asHTMLImageElement()
                            ->imageData();
        if (id) {
            result.m_isContentExists = true;
            result.m_intrinsicContentSize =
                LayoutSize(id->width(), id->height());
        } else {
            result.m_isContentExists = false;
        }
        return result;
    }

protected:
};
}

#endif
