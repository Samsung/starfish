/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "dom/Node.h"
#include "dom/HTMLVideoElement.h"
#include "layout/FrameReplacedVideo.h"
#include "platform/canvas/Canvas.h"
#include "platform/multimedia/MediaPlayer.h"

namespace StarFish {

IntrinsicSize FrameReplacedVideo::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    auto v = node()->asHTMLVideoElement();
    unsigned long videoWidth = v->videoWidth();
    unsigned long videoHeight = v->videoHeight();
    result.m_intrinsicContentSize = LayoutSize(videoWidth, videoHeight);
    return result;
}

void FrameReplacedVideo::didCompsiteStackingContext(Canvas* c)
{
    STARFISH_ASSERT(node()->isHTMLVideoElement());
    auto v = node()->asHTMLVideoElement();
    LayoutRect videoRect(borderLeft() + paddingLeft(),
                         borderTop() + paddingTop(), contentWidth(),
                         contentHeight());
    LayoutRect absVideoRect(videoRect);
    c->applyMatrixTo(absVideoRect);
    if (v->mediaPlayer()) {
        v->mediaPlayer()->drawVideo(c, videoRect, absVideoRect);
    }
}
}
