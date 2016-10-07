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
#include "platform/multimedia/MediaPlayer.h"

namespace StarFish {

class FrameReplacedVideo : public FrameReplaced {
public:
    FrameReplacedVideo(Node* node)
        : FrameReplaced(node, nullptr)
    {
        computeStyleFlags();
    }

    virtual void computeStyleFlags()
    {
        FrameReplaced::computeStyleFlags();
        m_flags.m_isEstablishesStackingContext = true;
        m_flags.m_needsGraphicsBuffer = true;
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
        unsigned long videoWidth = v->videoWidth();
        unsigned long videoHeight = v->videoHeight();
        result.m_intrinsicContentSize = LayoutSize(videoWidth, videoHeight);
        return result;
    }

    virtual void willCompsiteStackingContext(Canvas* c)
    {
    }
    virtual void didCompsiteStackingContext(Canvas* c)
    {
        STARFISH_ASSERT(node()->isElement());
        STARFISH_ASSERT(node()->asElement()->isHTMLElement());
        STARFISH_ASSERT(node()->asElement()->asHTMLElement()->isHTMLVideoElement());
        auto v = node()->asElement()->asHTMLElement()->asHTMLVideoElement();
        LayoutRect videoRect(borderLeft() + paddingLeft(), borderTop() + paddingTop(), contentWidth(), contentHeight());
        LayoutRect absVideoRect(videoRect);
        c->applyMatrixTo(absVideoRect);
        if (v->mediaPlayer())
            v->mediaPlayer()->drawVideo(c, videoRect, absVideoRect);
    }

protected:
};
}

#endif
