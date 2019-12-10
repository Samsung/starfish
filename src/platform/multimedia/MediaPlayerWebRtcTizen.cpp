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
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "platform/multimedia/MediaPlayerWebRtcTizen.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"

#include "core/dom/HTMLMediaElement.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/layout/FrameReplacedVideo.h"

namespace Starfish {

MediaPlayer* MediaPlayerWebRtc::create(HTMLMediaElement* element)
{
    return new MediaPlayerWebRtcTizen(element);
}

MediaPlayerWebRtcTizen::MediaPlayerWebRtcTizen(HTMLMediaElement* element)
    : MediaPlayerWebRtc(element)
{
    if (element->isHTMLVideoElement()) {
        HTMLVideoElement* elem = element->asHTMLVideoElement();
        m_canvasSurface =
            CanvasSurface::create(m_container->webView()->platformWindow(),
                                  elem->width(), elem->height());
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((MediaPlayerWebRtcTizen*)obj)->~MediaPlayerWebRtcTizen();
        },
        NULL, NULL, NULL);
}

MediaPlayerWebRtcTizen::~MediaPlayerWebRtcTizen()
{
    destroy();
}

void MediaPlayerWebRtcTizen::destroy()
{
    PLAYER_LOGI("%s\n", __func__);
}

void MediaPlayerWebRtcTizen::play()
{
    PLAYER_LOGI("%s\n", __func__);

    GCVector<MediaStreamTrack*> tracks = m_mediaProvider->getVideoTracks();
    if (!tracks.empty()) {
        m_mediaProvider->startPlayVideoTrack(this, tracks[0]);
    }
}

void MediaPlayerWebRtcTizen::prepare(MediaProvider* mediaProvider)
{
    m_mediaProvider = mediaProvider;

    MessageLoop* msgLoop = m_container->webView()->messageLoop();
    msgLoop->addIdler(
        m_container->window(),
        [](size_t, void* data) {
            MediaPlayerWebRtcTizen* self = (MediaPlayerWebRtcTizen*)data;
            self->processNextOperationQueueInContainer();
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_METADATA);
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_ENOUGH_DATA);
        },
        this);
}

void MediaPlayerWebRtcTizen::errorHandlerMediaFormat(int err, std::string msg)
{
    if (err == MEDIA_FORMAT_ERROR_NONE) {
        STARFISH_LOG_INFO("%s: Success: %s\n", __func__, msg.data());
    } else if (err == MEDIA_FORMAT_ERROR_INVALID_PARAMETER) {
        STARFISH_LOG_INFO("%s: Invalid Params: %s\n", __func__, msg.data());
    } else if (err == MEDIA_FORMAT_ERROR_OUT_OF_MEMORY) {
        STARFISH_LOG_INFO("%s: Out of memory: %s\n", __func__, msg.data());
    } else if (err == MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
        STARFISH_LOG_INFO("%s: Invalid operation: %s\n", __func__, msg.data());
    } else {
        STARFISH_LOG_INFO("%s: Unknown error: %s\n", __func__, msg.data());
    }
}
void MediaPlayerWebRtcTizen::errorHandlerMediaPacket(int err, std::string msg)
{
    if (err == MEDIA_PACKET_ERROR_NONE) {
        STARFISH_LOG_INFO("%s: Success: %s\n", __func__, msg.data());
    } else if (err == MEDIA_PACKET_ERROR_INVALID_PARAMETER) {
        STARFISH_LOG_INFO("%s: Invalid Params: %s\n", __func__, msg.data());
    } else if (err == MEDIA_PACKET_ERROR_OUT_OF_MEMORY) {
        STARFISH_LOG_INFO("%s: Out of memory: %s\n", __func__, msg.data());
    } else if (err == MEDIA_PACKET_ERROR_INVALID_OPERATION) {
        STARFISH_LOG_INFO("%s: Invalid operation: %s\n", __func__, msg.data());
    } else {
        STARFISH_LOG_INFO("%s: Unknown error: %s\n", __func__, msg.data());
    }
}

void MediaPlayerWebRtcTizen::onFrame(uint8_t* image)
{
    if (!isMainThread()) {
        struct Params {
            MediaPlayerWebRtcTizen* self;
            uint8_t* image;
        };

        Params* p = new Params{ this, image };

        container()
            ->webView()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                container()->window(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;
                    p->self->onFrame(p->image);
                    delete p;
                },
                p);
        return;
    }

    STARFISH_ASSERT(image != nullptr);

    FrameReplaced* frame = container()->frame()->asFrameReplaced();
    BrowsingContext* b = container()->window()->browsingContext();
    auto ptr = m_canvasSurface->mapBuffer();

    memcpy(ptr, image,
           m_canvasSurface->bufferStride() * m_canvasSurface->height());
    m_canvasSurface->unmapBufferAndNotifyUpdatedRegion(
        frame->x().toInt(), frame->y().toInt(), m_canvasSurface->width(),
        m_canvasSurface->height());

    b->setNeedsComposite();
}

} // namespace Starfish

#endif
#endif
