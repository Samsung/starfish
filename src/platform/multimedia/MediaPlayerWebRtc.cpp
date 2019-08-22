/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "MediaPlayerWebRtc.h"

#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/dom/HTMLMediaElement.h"
#include "core/layout/FrameReplacedVideo.h"

namespace Starfish {

MediaPlayerWebRtc::MediaPlayerWebRtc(HTMLMediaElement* element)
    : MediaPlayer(element)
{
    if (element->isHTMLVideoElement()) {
        HTMLVideoElement* elem = element->asHTMLVideoElement();
        m_canvasSurface =
            CanvasSurface::create(m_container->webView()->platformWindow(),
                                  elem->width(), elem->height());
    }
}

void MediaPlayerWebRtc::play()
{
}

// https://html.spec.whatwg.org/multipage/media.html#concept-media-load-algorithm
void MediaPlayerWebRtc::prepare(MediaProvider* mediaProvider)
{
    STARFISH_LOG_INFO("%s\n", __func__);
    STARFISH_ASSERT(mediaProvider);

    // TODO: Impl resource selection algorithm
    // TODO: The spec assumes there is one video track
    GCVector<MediaStreamTrack*> tracks = mediaProvider->getVideoTracks();
    if (!tracks.empty()) {
        mediaProvider->startPlayVideoTrack(this, (VideoStreamTrack*)tracks[0]);
    }
}

void MediaPlayerWebRtc::prepareMediaSource()
{
    STARFISH_LOG_INFO("%s\n", __func__);
}

void MediaPlayerWebRtc::willDrawVideo(Compositor* canvas,
                                      const LayoutRect& videoRect)
{
    STARFISH_ASSERT(canvas != nullptr);
}

void MediaPlayerWebRtc::onFrame(uint8_t* image)
{
    STARFISH_ASSERT(image != nullptr);

    m_container->window()
        ->webView()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data1, void* data2) {
                MediaPlayerWebRtc* self = (MediaPlayerWebRtc*)data1;
                BrowsingContext* b =
                    self->container()->window()->browsingContext();
                uint8_t* image = (uint8_t*)data2;
                CanvasSurface* canvasSurface = self->canvasSurface();
                FrameReplaced* frame =
                    self->container()->frame()->asFrameReplaced();
                auto ptr = canvasSurface->mapBuffer();
                if (image != nullptr) {
                    memcpy(ptr, image, canvasSurface->bufferStride() *
                                           canvasSurface->height());
                    canvasSurface->unmapBufferAndNotifyUpdatedRegion(
                        frame->x().toInt(), frame->y().toInt(),
                        canvasSurface->width(), canvasSurface->height());
                    b->setNeedsComposite();
                }
            },
            this, image);
}

void MediaPlayerWebRtc::didDrawVideo(Compositor* canvas,
                                     const LayoutRect& videoRect,
                                     const LayoutRect& absVideoRect)
{
}

MediaPlayer* MediaPlayerWebRtc::create(HTMLMediaElement* element)
{
    return new MediaPlayerWebRtc(element);
}
} // namespace Starfish

#endif
