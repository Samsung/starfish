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
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s\n", __func__);

    if (element->isHTMLVideoElement()) {
        HTMLVideoElement* elem = element->asHTMLVideoElement();
        m_canvasSurface =
            CanvasSurface::create(m_container->webView()->platformWindow(),
                                  elem->width(), elem->height());
    }
}

MediaPlayerWebRtcTizen::~MediaPlayerWebRtcTizen()
{
    destroy();
}

void MediaPlayerWebRtcTizen::destroy()
{
    STARFISH_RELEASE_ASSERT(isMainThread());
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s\n", __func__);

    if (m_player) {
        player_state_e state;
        player_get_state(m_player, &state);
        if (state == PLAYER_STATE_PLAYING) {
            player_stop(m_player);
        }
        checkStatusPlayer(player_unprepare(m_player), "playerUnprepare");
        player_unset_media_stream_buffer_status_cb(m_player,
                                                   PLAYER_STREAM_TYPE_AUDIO);
        player_unset_buffering_cb(m_player);
        checkStatusPlayer(player_destroy(m_player), "playerDestroy");
        checkStatusMediaFormat(media_format_unref(m_audioFormat),
                               "mediaFormatUnref");
    }

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
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s\n", __func__);

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

        STARFISH_LOG_INFO("%s: <playerStart>\n", __func__);
        checkStatusPlayer(player_start(m_player), "playerStart");
        STARFISH_LOG_INFO("%s: </playerStart>\n", __func__);
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

static void bufferingCb(int percent, void* data)
{
    STARFISH_LOG_INFO("buffering: %d\n", percent);
}

static void mediaStreamBufferStatusCb(
    player_media_stream_buffer_status_e status, void* data)
{
    if (status == PLAYER_MEDIA_STREAM_BUFFER_UNDERRUN) {
        STARFISH_LOG_WARN("MediaStreamBufferUnderrun\n");
    } else if (status == PLAYER_MEDIA_STREAM_BUFFER_OVERFLOW) {
        STARFISH_LOG_WARN("MediaStreamBufferOverflow\n");
    }
}

void MediaPlayerWebRtcTizen::prepare(MediaProvider* mediaProvider)
{
    STARFISH_RELEASE_ASSERT(isMainThread());
    PLAYER_LOGI("MediaPlayerWebRtcTizen::%s\n", __func__);

    STARFISH_ASSERT(mediaProvider);
    m_mediaProvider = mediaProvider;
    m_mediaProvider->setMediaPlayer(this);

    if (!mediaProvider->getAudioTracks().empty()) {
        if (m_player) {
            player_state_e state;
            player_get_state(m_player, &state);
            if (state == PLAYER_STATE_PLAYING) {
                player_stop(m_player);
            }
            checkStatusPlayer(player_unprepare(m_player), "playerUnprepare");
            checkStatusPlayer(player_destroy(m_player), "playerDestroy");
        }

        checkStatusPlayer(player_create(&m_player), "playerCreate");

        checkStatusMediaFormat(media_format_create(&m_audioFormat),
                               "mediaFormatCreate");
        checkStatusMediaFormat(
            media_format_set_audio_mime(m_audioFormat, MEDIA_FORMAT_PCM_S16LE),
            "");
        checkStatusMediaFormat(
            media_format_set_audio_channel(m_audioFormat, AUDIO_CHANNELS), "");
        checkStatusMediaFormat(
            media_format_set_audio_samplerate(m_audioFormat, AUDIO_SAMPLE_RATE),
            "");
        checkStatusPlayer(player_set_media_stream_info(m_player,
                                                       PLAYER_STREAM_TYPE_AUDIO,
                                                       m_audioFormat),
                          "");
        checkStatusPlayer(
            player_set_buffering_cb(m_player, bufferingCb, nullptr),
            "setBufferingCb");
        checkStatusPlayer(player_set_media_stream_buffer_status_cb(
                              m_player, PLAYER_STREAM_TYPE_AUDIO,
                              mediaStreamBufferStatusCb, nullptr),
                          "setMediaStreamBufferStatusCb");

        checkStatusPlayer(player_prepare(m_player), "playerPrepare");
    }

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

bool MediaPlayerWebRtcTizen::checkStatusPlayer(int err, std::string msg)
{
    if (err == PLAYER_ERROR_NONE) {
        return true;
    }

    if (err == PLAYER_ERROR_INVALID_PARAMETER) {
        STARFISH_LOG_ERROR("Invalid params: %s\n", msg.data());
    } else if (err == PLAYER_ERROR_INVALID_STATE) {
        STARFISH_LOG_ERROR("Invalid state: %s\n", msg.data());
    } else if (err == PLAYER_ERROR_NOT_SUPPORTED_FILE) {
        STARFISH_LOG_ERROR("Not supported file: %s\n", msg.data());
    } else if (err == PLAYER_ERROR_BUFFER_SPACE) {
        STARFISH_LOG_ERROR("Buffer space: %s\n", msg.data());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    } else if (err == PLAYER_ERROR_OUT_OF_MEMORY) {
        STARFISH_LOG_ERROR("Out of memory: %s\n", msg.data());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    } else if (err == PLAYER_ERROR_INVALID_OPERATION) {
        STARFISH_LOG_ERROR("Invalid operation: %s\n", msg.data());
    } else if (err == PLAYER_ERROR_RESOURCE_LIMIT) {
        STARFISH_LOG_ERROR("Resource limited: %s\n", msg.data());
    } else {
        STARFISH_LOG_ERROR("Unknown error: %s\n", msg.data());
    }

    return false;
}

bool MediaPlayerWebRtcTizen::checkStatusMediaFormat(int err, std::string msg)
{
    if (err == MEDIA_FORMAT_ERROR_NONE) {
        return true;
    }

    if (err == MEDIA_FORMAT_ERROR_INVALID_PARAMETER) {
        STARFISH_LOG_ERROR("Invalid Params: %s\n", msg.data());
    } else if (err == MEDIA_FORMAT_ERROR_OUT_OF_MEMORY) {
        STARFISH_LOG_ERROR("Out of memory: %s\n", msg.data());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    } else if (err == MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
        STARFISH_LOG_ERROR("Invalid operation: %s\n", msg.data());
    } else {
        STARFISH_LOG_ERROR("Unknown error: %s\n", msg.data());
    }

    return false;
}
bool MediaPlayerWebRtcTizen::checkStatusMediaPacket(int err, std::string msg)
{
    if (err == MEDIA_PACKET_ERROR_NONE) {
        return true;
    }

    if (err == MEDIA_PACKET_ERROR_INVALID_PARAMETER) {
        STARFISH_LOG_ERROR("Invalid Params: %s\n", msg.data());
    } else if (err == MEDIA_PACKET_ERROR_OUT_OF_MEMORY) {
        STARFISH_LOG_ERROR("Out of memory: %s\n", msg.data());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    } else if (err == MEDIA_PACKET_ERROR_INVALID_OPERATION) {
        STARFISH_LOG_ERROR("Invalid operation: %s\n", msg.data());
    } else {
        STARFISH_LOG_ERROR("Unknown error: %s\n", msg.data());
    }

    return false;
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
                    Params* p = (Params*)data;
                    p->self->onFrame(p->observer);
                    delete p;
                },
                p);
        return;
    }

    STARFISH_ASSERT(observer && observer->image());

    FrameReplaced* frame = container()->frame()->asFrameReplaced();
    BrowsingContext* b = container()->window()->browsingContext();
    auto ptr = m_canvasSurface->mapBuffer();

    int canvasBufferSize =
        m_canvasSurface->bufferStride() * m_canvasSurface->height();
    {
        Locker<Mutex> lock(*observer->imageLock());
        int videoFrameSize =
            observer->width() * observer->height() * observer->pixelStride();

        if (videoFrameSize != canvasBufferSize) {
            STARFISH_LOG_WARN("videoFrameSize: %d != canvasBufferSize: %d\n",
                              videoFrameSize, canvasBufferSize);
        }

        memcpy(ptr, observer->image(),
               std::min(videoFrameSize, canvasBufferSize));
    }

    m_canvasSurface->unmapBufferAndNotifyUpdatedRegion(
        frame->x().toInt(), frame->y().toInt(), m_canvasSurface->width(),
        m_canvasSurface->height());

    b->setNeedsComposite();
}

void MediaPlayerWebRtcTizen::onData(MediaStream::AudioTrackObserver* observer)
{
    if (!isMainThread()) {
        struct Params {
            MediaPlayerWebRtcTizen* self;
            MediaStream::AudioTrackObserver* observer;
        };

        Params* p = new Params{ this, observer };

        container()
            ->webView()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                container()->window(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;
                    p->self->onData(p->observer);
                    delete p;
                },
                p);
        return;
    }

    if (observer && observer->audioData()) {
        media_packet_h mediaPacket;

        {
            Locker<Mutex> lock(*observer->audioLock());
            checkStatusMediaPacket(media_packet_create_from_external_memory(
                                       m_audioFormat, observer->audioData(),
                                       observer->numberOfFrames(), nullptr,
                                       nullptr, &mediaPacket),
                                   "onData::createFromExternalMemory");
        }

        checkStatusPlayer(player_push_media_stream(m_player, mediaPacket),
                          "onData::pushMediaStream");
        checkStatusMediaPacket(media_packet_destroy(mediaPacket),
                               "onData::destroy");
    }
}

} // namespace Starfish

#endif
#endif
