/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Node.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/layout/FrameReplacedVideo.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "platform/multimedia/MediaPlayer.h"

namespace StarFish {

IntrinsicSize FrameReplacedVideo::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    result.m_hasAspectRatio = true;
    auto v = node()->asHTMLVideoElement();
    unsigned long videoWidth = v->videoWidth();
    unsigned long videoHeight = v->videoHeight();
    result.m_intrinsicContentSize = LayoutSize(videoWidth, videoHeight);
    return result;
}

void FrameReplacedVideo::didCompsiteStackingContext(Compositor* c)
{
    STARFISH_ASSERT(node()->isHTMLVideoElement());
    auto v = node()->asHTMLVideoElement();
    LayoutRect videoRect(borderLeft() + paddingLeft(),
                         borderTop() + paddingTop(), contentWidth(),
                         contentHeight());
    LayoutRect absVideoRect(videoRect);
    c->applyMatrixTo(absVideoRect);
    if (v->activeMediaPlayer()) {
        v->activeMediaPlayer()->drawVideo(c, videoRect, absVideoRect);
    }
}

void FrameReplacedVideo::createGraphicsBuffer(CanvasSurface** surfaceHolder,
                                              size_t visibleWidth,
                                              size_t visibleHeight)
{
    if (!(*surfaceHolder)) {
        auto v = node()->asHTMLVideoElement();
        if (v->activeMediaPlayer()) {
            *surfaceHolder = v->activeMediaPlayer()->createGraphicsBuffer(
                visibleWidth, visibleHeight);
        } else {
            *surfaceHolder = CanvasSurface::create(
                node()->starFish()->platformWindow(), 1, 1);
        }
    }
}
}
