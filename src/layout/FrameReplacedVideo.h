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

#ifndef __StarFishFrameReplacedVideo__
#define __StarFishFrameReplacedVideo__

#include "layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedVideo : public FrameReplaced {
public:
    FrameReplacedVideo(Node* node)
        : FrameReplaced(node, nullptr)
    {
    }

    virtual bool isFrameReplacedVideo()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedVideo";
    }

    virtual void paintReplaced(Canvas* canvas)
    {
    }

    virtual IntrinsicSize intrinsicSize()
    {
        IntrinsicSize result;
        result.m_isContentExists = true;
        auto v = node()->asElement()->asHTMLElement()->asHTMLVideoElement();
        result.m_intrinsicContentSize = LayoutSize(v->videoSurface()->width(), v->videoSurface()->height());
        return result;
    }

    virtual bool hasStackingContextContentBuffer()
    {
        return true;
    }

    virtual CanvasSurface* gainStackingContextContentBuffer()
    {
        return node()->asElement()->asHTMLElement()->asHTMLVideoElement()->videoSurface();
    }

protected:
};
}

#endif
