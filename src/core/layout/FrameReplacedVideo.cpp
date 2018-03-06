/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
#ifdef STARFISH_ENABLE_MULTIMEDIA

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
#endif
