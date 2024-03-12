/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#include "core/modules/message_loop/Timer.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"

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
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s", __func__);

    if (element->isHTMLVideoElement()) {
        HTMLVideoElement* elem = element->asHTMLVideoElement();
        m_canvasSurface = CanvasSurface::create(
            m_container->webView()->renderer(), elem->width(), elem->height());
    }
}

MediaPlayerWebRtcTizen::~MediaPlayerWebRtcTizen()
{
    try {
        destroy();
    } catch (...) {
        PLAYER_LOGE("MediaPlayerWebRtcTizen::%s", __func__);
    }
}

void MediaPlayerWebRtcTizen::destroy()
{
    STARFISH_RELEASE_ASSERT(isMainThread());
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s", __func__);

    m_alive = false;
    pause();
    if (m_mediaProvider) {
        m_mediaProvider->stopAudioTrack();
        m_mediaProvider->stopVideoTrack();
        m_mediaProvider->setMediaPlayer(nullptr);
    }
    m_mediaProvider = nullptr;

    if (m_container) {
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_NOTHING);
    }
    m_container = nullptr;

    m_seekState = SEEKSTATE_NO_SEEK;
    m_playbackState = PLAYBACK_STATE_NONE;
}

void MediaPlayerWebRtcTizen::play()
{
    STARFISH_RELEASE_ASSERT(isMainThread());
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s", __func__);

    // TODO: Impl resource selection algorithm
    // TODO: The spec assumes there is one video track
    // TODO: Plays the first audio track.
    STARFISH_ASSERT(m_mediaProvider);
    GCVector<MediaStreamTrack*> videoTracks = m_mediaProvider->getVideoTracks();
    if (!videoTracks.empty()) {
        STARFISH_ASSERT(m_mediaProvider);
        m_mediaProvider->playVideoTrack(videoTracks[0]);
    }
    GCVector<MediaStreamTrack*> audioTracks = m_mediaProvider->getAudioTracks();
    if (!audioTracks.empty()) {
        STARFISH_ASSERT(m_mediaProvider);
        m_mediaProvider->playAudioTrack(audioTracks[0]);
    }
}

void MediaPlayerWebRtcTizen::pause()
{
    if (m_playbackState == PLAYBACK_STATE_PLAYING) {
        m_playbackState = PLAYBACK_STATE_PAUSED;
        m_container->executionContext()->removePointerFromRootSet(this);
        window()->clearInterval(m_currentTimeUpdateTimer);
        m_currentTimeUpdateTimer = TimerInvalidID;
    }
}

void MediaPlayerWebRtcTizen::prepare(MediaProvider* mediaProvider)
{
    STARFISH_RELEASE_ASSERT(isMainThread());
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s", __func__);

    STARFISH_ASSERT(mediaProvider);
    m_mediaProvider = mediaProvider;
    m_mediaProvider->setMediaPlayer(this);

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

void MediaPlayerWebRtcTizen::onFrame(MediaStream::VideoFrameObserver* observer)
{
    if (!isMainThread()) {
        struct Params {
            MediaPlayerWebRtcTizen* self;
            MediaStream::VideoFrameObserver* observer;
        };

        Params* p = new Params{ this, observer };

        container()
            ->webView()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                container()->window(),
                [](size_t, void* data) {
                    if (data) {
                        Params* p = (Params*)data;
                        if (p->self && p->observer) {
                            p->self->onFrame(p->observer);
                        }
                        delete p;
                    }
                },
                p);
        return;
    }

    STARFISH_ASSERT(observer && observer->image());

    int videoFrameWidth = observer->width();
    int videoFrameHeight = observer->height();

    if (!m_canvasSurface || m_canvasSurface->width() != videoFrameWidth ||
        m_canvasSurface->height() != videoFrameHeight) {
        if (m_canvasSurface != nullptr) {
            m_canvasSurface->detachNativeBuffer();
            m_canvasSurface = nullptr;
        }
        if (container()->isHTMLVideoElement()) {
            HTMLVideoElement* elem = container()->asHTMLVideoElement();
            {
                // FIXME!! : mong
                elem->setWidth(videoFrameWidth);
                elem->setHeight(videoFrameHeight);
            }
            m_canvasSurface =
                CanvasSurface::create(container()->webView()->renderer(),
                                      videoFrameWidth, videoFrameHeight);
        }
    }

    if (m_canvasSurface && observer && observer->image()) {
        Locker<Mutex> lock(*observer->imageLock());
        FrameReplaced* frame = container()->frame()->asFrameReplaced();
        BrowsingContext* b = container()->window()->browsingContext();
        auto ptr = m_canvasSurface->mapBuffer();

        memcpy(ptr, observer->image(),
               videoFrameWidth * videoFrameHeight * observer->pixelStride());
        m_canvasSurface->unmapBufferAndNotifyUpdatedRegion(
            0, 0, m_canvasSurface->width(), m_canvasSurface->height());
        b->setNeedsComposite();
    }
}

void MediaPlayerWebRtcTizen::onData(MediaStream::AudioTrackObserver* observer)
{
}

} // namespace Starfish

#endif
#endif
