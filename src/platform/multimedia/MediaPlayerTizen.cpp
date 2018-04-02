/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/multimedia/MediaPlayerTizen.h"
#include "platform/window/PlatformWindow.h"

#include <media/player.h>
#if defined(STARFISH_TIZEN_TV)
#include <media/player_product.h>
#include <Elementary.h>
#endif

namespace StarFish {

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100
#define STARFISH_MSE_SUBMIT_BYTES_RATE 0.3

// chromium-efl (media_source_delegate_efl.cc)
static bool isFramerateChanged(const Framerate& before, const Framerate& after)
{
    constexpr const double kDetectedThreshold = 0.01;
    if (before.toDouble() > 0 &&
        std::abs(before.toDouble() - after.toDouble()) > kDetectedThreshold) {
        const double kFramerateDetectionTable[13] = { 23.9, 24,   24.1, 24.9,
                                                      25.1, 29.9, 30,   30.1,
                                                      49.9, 50.1, 59.9, 60,
                                                      60.1 };

        int detectIndex = 0;
        int previousFrameDetectIndex = 0;
        int newFrameDetectIndex = 0;

        for (detectIndex = 0; detectIndex < 12; ++detectIndex) {
            if (!newFrameDetectIndex &&
                after.toDouble() >= kFramerateDetectionTable[detectIndex] &&
                after.toDouble() < kFramerateDetectionTable[detectIndex + 1])
                newFrameDetectIndex = detectIndex;
            if (!previousFrameDetectIndex &&
                before.toDouble() >= kFramerateDetectionTable[detectIndex] &&
                before.toDouble() < kFramerateDetectionTable[detectIndex + 1])
                previousFrameDetectIndex = detectIndex;

            if (newFrameDetectIndex && previousFrameDetectIndex)
                break;
        }
        if (newFrameDetectIndex != previousFrameDetectIndex) {
            return true;
        }
    }
    return false;
}

#define _PLAYER_ERROR_LIST_COMMON(F)                \
    F(PLAYER_ERROR_OUT_OF_MEMORY)                   \
    F(PLAYER_ERROR_INVALID_PARAMETER)               \
    F(PLAYER_ERROR_NO_SUCH_FILE)                    \
    F(PLAYER_ERROR_INVALID_OPERATION)               \
    F(PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE)         \
    F(PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE) \
    F(PLAYER_ERROR_SEEK_FAILED)                     \
    F(PLAYER_ERROR_INVALID_STATE)                   \
    F(PLAYER_ERROR_NOT_SUPPORTED_FILE)              \
    F(PLAYER_ERROR_INVALID_URI)                     \
    F(PLAYER_ERROR_SOUND_POLICY)                    \
    F(PLAYER_ERROR_CONNECTION_FAILED)               \
    F(PLAYER_ERROR_VIDEO_CAPTURE_FAILED)            \
    F(PLAYER_ERROR_DRM_EXPIRED)                     \
    F(PLAYER_ERROR_DRM_NO_LICENSE)                  \
    F(PLAYER_ERROR_DRM_FUTURE_USE)                  \
    F(PLAYER_ERROR_DRM_NOT_PERMITTED)               \
    F(PLAYER_ERROR_RESOURCE_LIMIT)                  \
    F(PLAYER_ERROR_PERMISSION_DENIED)

#if defined(STARFISH_TIZEN_TV)
#define PLAYER_ERROR_LIST(F)                    \
    _PLAYER_ERROR_LIST_COMMON(F)                \
    F(PLAYER_ERROR_STREAMING_PLAYER)            \
    F(PLAYER_ERROR_AUDIO_CODEC_NOT_SUPPORTED)   \
    F(PLAYER_ERROR_VIDEO_CODEC_NOT_SUPPORTED)   \
    F(PLAYER_ERROR_NO_AUTH)                     \
    F(PLAYER_ERROR_GENEREIC)                    \
    F(PLAYER_ERROR_DRM_INFO)                    \
    F(PLAYER_ERROR_SYNC_PLAY_NETWORK_EXCEPTION) \
    F(PLAYER_ERROR_SYNC_PLAY_SERVER_DOWN)       \
    F(PLAYER_ERROR_NOT_SUPPORTED_FORMAT)
#else
#define PLAYER_ERROR_LIST(F)                  \
    _PLAYER_ERROR_LIST_COMMON(F)              \
    F(PLAYER_ERROR_SERVICE_DISCONNECTED)      \
    F(PLAYER_ERROR_BUFFER_SPACE)              \
    F(PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC) \
    F(PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC) \
    F(PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE)
#endif

void MediaPlayerTizen::printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define F(errorenum)                     \
    case errorenum:                      \
        PLAYER_LOGI("%s\n", #errorenum); \
        return;
        PLAYER_ERROR_LIST(F)
#undef F
    default:
        PLAYER_LOGI("Unknown error\n");
        return;
    }
}

static void completeCallback(void* data)
{
    MediaPlayerTizen* player = (MediaPlayerTizen*)data;
    player->handleEnded();
}

static void preparedCallback(void* data)
{
    PLAYER_LOGI("MediaPlayerTizen::player_prepare_async_cb\n");
    MediaPlayerTizen* self = (MediaPlayerTizen*)data;
    self->handlePrepared();
}

static void printMediaPacketError(int errorCode)
{
    switch (errorCode) {
#define F(errorenum)                     \
    case errorenum:                      \
        PLAYER_LOGI("%s\n", #errorenum); \
        return;
        F(MEDIA_PACKET_ERROR_OUT_OF_MEMORY)
        F(MEDIA_PACKET_ERROR_INVALID_PARAMETER)
        F(MEDIA_PACKET_ERROR_INVALID_OPERATION)
        F(MEDIA_PACKET_ERROR_FILE_NO_SPACE_ON_DEVICE)
        F(MEDIA_PACKET_ERROR_NO_AVAILABLE_PACKET)
#undef F
    default:
        PLAYER_LOGI("Unknown error\n");
        return;
    }
}

static void printMediaFormatError(int errorCode)
{
    switch (errorCode) {
#define F(errorenum)                     \
    case errorenum:                      \
        PLAYER_LOGI("%s\n", #errorenum); \
        return;
        F(MEDIA_FORMAT_ERROR_OUT_OF_MEMORY)
        F(MEDIA_FORMAT_ERROR_INVALID_PARAMETER)
        F(MEDIA_FORMAT_ERROR_INVALID_OPERATION)
        F(MEDIA_FORMAT_ERROR_FILE_NO_SPACE_ON_DEVICE)
#undef F
    default:
        PLAYER_LOGI("Unknown error\n");
        return;
    }
}

class MediaPlayerTizenMediaSourceClient : public MediaSourceClient {
public:
    MediaPlayerTizenMediaSourceClient(MediaPlayerTizen* player)
        : MediaSourceClient()
        , m_player(player)
    {
#ifndef NDEBUG
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                PLAYER_LOGI(
                    "[TRACE_MSE_GC] "
                    "MediaPlayerTizenMediaSourceClient::~"
                    "MediaPlayerTizenMediaSourceClient (%p)\n",
                    obj);
            },
            NULL, NULL, NULL);
#endif
    }

    virtual void activeSourceComputed()
    {
        if (m_player && m_player->alive()) {
            m_player->prepareMediaSource();
        }
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player && m_player->alive() &&
            m_player->activeSourceBuffer(StreamTypeVideo) == s) {
            m_player->fillBufferIfNeeded(StreamTypeVideo);
        }
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player && m_player->alive() &&
            m_player->activeSourceBuffer(StreamTypeAudio) == s) {
            m_player->fillBufferIfNeeded(StreamTypeAudio);
        }
    }

    MediaPlayerTizen* m_player;
};

MediaStream::MediaStream(StreamType type)
    : m_type(type)
    , m_bufferState(BUFFERSTATE_INITIAL)
    , m_mediaStreamMutex(new Mutex())
    , m_mediaFormat(nullptr)
    , m_maxBufferSize(0)
    , m_lastSubmittedDTS(0)
    , m_initSegmentIndex(0)
    , m_lastBufferBytes(0)
    , m_waitingDemuxer(false)
{
    if (m_type == StreamTypeAudio) {
        m_maxBufferSize = 320 * 1000 / 8 * 5;
#if defined(STARFISH_TIZEN_TV)
        m_formatExtra.m_audioFormatExtra.codec_extradata = nullptr;
        m_formatExtra.m_audioFormatExtra.extradata_size = 0;
#endif
    } else {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
#if defined(STARFISH_TIZEN_TV)
        m_formatExtra.m_videoFormatExtra.codec_extradata = nullptr;
        m_formatExtra.m_videoFormatExtra.extradata_size = 0;
#endif
    }
}

bool MediaStream::createMediaFormat()
{
    STARFISH_ASSERT(m_mediaFormat == nullptr);
    int ret = media_format_create(&m_mediaFormat);
    if (ret != MEDIA_FORMAT_ERROR_NONE) {
        printMediaFormatError(ret);
        return false;
    }
#if defined(STARFISH_TIZEN_TV)
    if (m_type == StreamTypeAudio) {
        player_media_stream_audio_extra_info_s& extra =
            m_formatExtra.m_audioFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
        }
        memset(&extra, 0, sizeof(player_media_stream_audio_extra_info_s));
    } else {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        player_media_stream_video_extra_info_s& extra =
            m_formatExtra.m_videoFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
        }
        memset(&extra, 0, sizeof(player_media_stream_video_extra_info_s));
    }
#endif
    return true;
}

void MediaStream::releaseMediaFormat()
{
    if (m_mediaFormat) {
        media_format_unref(m_mediaFormat);
        m_mediaFormat = nullptr;
    }
#if defined(STARFISH_TIZEN_TV)
    if (m_type == StreamTypeAudio) {
        player_media_stream_audio_extra_info_s& extra =
            m_formatExtra.m_audioFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
            extra.codec_extradata = nullptr;
            extra.extradata_size = 0;
        }
    } else {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        player_media_stream_video_extra_info_s& extra =
            m_formatExtra.m_videoFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
            extra.codec_extradata = nullptr;
            extra.extradata_size = 0;
        }
    }
#endif
}

uint64_t MediaStream::maxBufferSize()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_maxBufferSize;
}

void MediaStream::setMaxBufferSize(uint64_t value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_maxBufferSize = value;
}

bool MediaStream::needPacket()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_bufferState == BUFFERSTATE_UNDER_RUN ||
           m_bufferState == BUFFERSTATE_NEED_PACKET;
}

bool MediaStream::isBufferState(BufferState state)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_bufferState == state;
}

MediaStream::BufferState MediaStream::bufferState()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_bufferState;
}

#ifdef STARFISH_MEDIAPLAYER_DEBUG
static const char* bufferStateString(MediaStream::BufferState value)
{
    if (value == MediaStream::BUFFERSTATE_INITIAL) {
        return "INITIAL";
    } else if (value == MediaStream::BUFFERSTATE_UNDER_RUN) {
        return "UNDERRUN";
    } else if (value == MediaStream::BUFFERSTATE_NEED_PACKET) {
        return "NEED_PACKET";
    } else if (value == MediaStream::BUFFERSTATE_NORMAL) {
        return "NORMAL";
    }
    return "EOS";
}
#endif

void MediaStream::setBufferState(BufferState value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_bufferState = value;
}

bool MediaStream::waitingDemuxer()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_waitingDemuxer;
}

void MediaStream::setWaitingDemuxer(bool value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_waitingDemuxer = value;
}

uint64_t MediaStream::lastBufferBytes()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_lastBufferBytes;
}

void MediaStream::setLastBufferBytes(size_t value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_lastBufferBytes = value;
}

MediaPlayerTizen::MediaPlayerTizen(HTMLMediaElement* element)
    : MediaPlayer(element)
    , m_inPrepare(false)
    , m_pendingPlay(false)
    , m_underrunMode(false)
    , m_seekingTimer(SIZE_MAX)
    , m_mseClient(nullptr)
    , m_fillBufferMutex(new Mutex())
    , m_canvasSurface(nullptr)
    , m_playerDeadFlag(nullptr)
    , m_audioStream(nullptr)
    , m_videoStream(nullptr)
{
    player_create(&m_nativePlayer);
    initDisplay();

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            PLAYER_LOGI("MediaPlayerTizen::~MediaPlayerTizen\n");
            MediaPlayerTizen* player = (MediaPlayerTizen*)obj;
            player->close();
        },
        NULL, NULL, NULL);
}

void MediaPlayerTizen::handlePlayerError()
{
    if (!isMainThread()) {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data) {
                MediaPlayerTizen* player = (MediaPlayerTizen*)data;
                player->handlePlayerError();
            },
            this);
        return;
    }

    m_foundError = true;
    if (m_inPrepare) {
        handlePrepared();
    } else if (m_seekState == SEEKSTATE_SEEKING) {
        handleSeeked();
    }
    close();
}

void MediaPlayerTizen::fillBufferIfNeeded(StreamType type)
{
    MediaStream* stream = currentStream(type);
    if (!stream) {
        return;
    }
#ifdef STARFISH_RUN_MSE_THREAD
    stream->setWaitingDemuxer(false);
#else
    if (stream->waitingDemuxer()) {
        stream->setWaitingDemuxer(false);
        fillBuffer(stream);
    }
#endif
}

void MediaPlayerTizen::seek(double time)
{
    if (m_inPrepare) {
        PLAYER_LOGI(
            "MediaPlayerTizen::seek -> seeking failed saving time %lf\n", time);
        m_container->setDefaultPlaybackStartPosition(time);
        return;
    }
    STARFISH_ASSERT(m_seekState == SEEKSTATE_NO_SEEK);
    STARFISH_ASSERT(m_nativePlayer && m_alive);
    if (m_playbackState == PLAYBACK_STATE_END) {
        m_playbackState = PLAYBACK_STATE_PAUSED;
        if (m_audioStream) {
            m_audioStream->setBufferState(MediaStream::BUFFERSTATE_INITIAL);
        }
        if (m_videoStream) {
            m_videoStream->setBufferState(MediaStream::BUFFERSTATE_INITIAL);
        }
        player_start(m_nativePlayer);
        player_pause(m_nativePlayer);
        player_set_completed_cb(m_nativePlayer, completeCallback, this);
    }

    // Check seek boundary
    STARFISH_ASSERT(!std::isnan(time));
    double dur = duration();
    if (time < 0) {
        time = 0;
    } else if (dur != 0 && !std::isnan(dur) && time >= dur) {
        // Note: Seeking to EOS is impossible!!! (player_set_position fault)
        PLAYER_LOGI("MediaPlayerTizen::seek() reaches EOS\n");
        m_container->mediaPlayerNotifySeekedItsContainer(duration());
        handleEnded();
        return;
    }

    m_seekState = SEEKSTATE_SEEKING;
    m_container->document()->browsingContext()->addPointerInRootSet(this);

    // Set timer
    // Note : To avoid too much waiting 'seek' callback,
    //        set timer that would help the player to remove rooted pointer
    //        and properly destroyed
    m_seekingTimer = m_container->window()->setTimeout(
        [](Window* window, void* data) {
            MediaPlayerTizen* self = (MediaPlayerTizen*)data;
            PLAYER_LOGI("MediaPlayerTizen::seek() : timeout\n");
            self->handleSeekTimeout();
        },
        MAX_WAITING_SECONDS_FOR_SEEK_OPERATION, this);

    seekOperation((int)(time * 1000.0));
}

static void seekedCallback(void* data)
{
    PLAYER_LOGI("player_set_play_position_cb\n");
    MediaPlayerTizen* self = (MediaPlayerTizen*)data;
    self->handleSeeked();
}

void MediaPlayerTizen::seekOperation(int timeInMS)
{
    PLAYER_LOGI("MediaPlayerTizen::seekOperation() (time: %d)\n", timeInMS);
#if defined(STARFISH_TIZEN_TV)
    int ret = player_set_play_position_ex(m_nativePlayer, timeInMS, true,
                                          seekedCallback, this);
#else
    int ret = player_set_play_position(m_nativePlayer, timeInMS, true,
                                       seekedCallback, this);
#endif
    if (ret != PLAYER_ERROR_NONE) {
        // Failed immediately
        PLAYER_LOGI(
            "MediaPlayerTizen::seekOperation() player_set_position_async "
            "failed immediately (IGNORE) : ");
        printNativePlayerError(ret);
        m_foundError = true;
        handleSeeked();
        return;
    }
    if (m_audioStream) {
        Locker<Mutex> locker(*m_fillBufferMutex);
        m_audioStream->setLastSubmittedDTS(timeInMS);
        m_audioStream->setWaitingDemuxer(true);
    }
    if (m_videoStream) {
        Locker<Mutex> locker(*m_fillBufferMutex);
        m_videoStream->setLastSubmittedDTS(timeInMS);
        m_videoStream->setWaitingDemuxer(true);
    }
    if (activeSourceBuffer(StreamTypeAudio)) {
        activeSourceBuffer(StreamTypeAudio)->clearPacketAccessCache();
        fillBufferIfNeeded(StreamTypeAudio);
    }
    if (activeSourceBuffer(StreamTypeVideo)) {
        activeSourceBuffer(StreamTypeVideo)->clearPacketAccessCache();
        fillBufferIfNeeded(StreamTypeVideo);
    }
}

void MediaPlayerTizen::handleSeeked()
{
    if (isMainThread()) {
        PLAYER_LOGI("MediaPlayerTizen::handleSeeked\n");
        if (m_seekState == SEEKSTATE_NO_SEEK) {
            return;
        }
        STARFISH_ASSERT(m_seekState == SEEKSTATE_SEEKING);
        m_seekState = SEEKSTATE_NO_SEEK;

        // Remove timeout timer
        if (m_seekingTimer != SIZE_MAX) {
            m_container->window()->clearTimeout(m_seekingTimer);
            m_seekingTimer = SIZE_MAX;
        }
        // Remove rooted pointer
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);

        if (!m_foundError) {
            // Success
            // Notify "Seeked" to its container
            m_container->mediaPlayerNotifySeekedItsContainer(currentTime());
        } else {
            // Fail
            m_container->mediaPlayerNotifySeekFailureItsContainer();
            close();
        }
    } else {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)data;
                self->handleSeeked();
            },
            this);
    }
}

void MediaPlayerTizen::handleSeekTimeout()
{
    if (isMainThread()) {
        PLAYER_LOGI("MediaPlayerTizen::handleSeekTimeout\n");
        m_foundError = true;
        m_seekingTimer = SIZE_MAX;
        handleSeeked();
    } else {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)data;
                self->handleSeekTimeout();
            },
            this);
    }
}

void MediaPlayerTizen::handleEnded()

{
    if (isMainThread()) {
        PLAYER_LOGI("MediaPlayerTizen::handleEnded (EOS)\n");
        if (!m_nativePlayer ||
            m_playbackState == MediaPlayer::PLAYBACK_STATE_END) {
            return;
        }
        if (!isMSE() && loop()) {
            return;
        }
        pause();
        player_stop(m_nativePlayer);
        player_unset_completed_cb(m_nativePlayer);
        m_playbackState = MediaPlayer::PLAYBACK_STATE_END;
        if (isMSE() && m_container->isHTMLVideoElement() &&
            m_container->frame()) {
            m_container->setNeedsComposite();
        }
        if (m_audioStream) {
            Locker<Mutex> locker(*m_fillBufferMutex);
            m_audioStream->setLastSubmittedDTS(0);
        }
        if (m_videoStream) {
            Locker<Mutex> locker(*m_fillBufferMutex);
            m_videoStream->setLastSubmittedDTS(0);
        }
        if (activeSourceBuffer(StreamTypeAudio)) {
            activeSourceBuffer(StreamTypeAudio)->clearAll();
        }
        if (activeSourceBuffer(StreamTypeVideo)) {
            activeSourceBuffer(StreamTypeVideo)->clearAll();
        }
        if (m_container) {
            if (loop()) {
                // NOTE Only MSE reaches here
                STARFISH_ASSERT(isMSE());
                m_container->mediaPlayerRequestRestartItsContainer();
            } else {
                m_container->mediaPlayerNotifyEndedItsContainer();
            }
        }
    } else {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)data;
                self->handleEnded();
            },
            this);
    }
}

void MediaPlayerTizen::close()
{
    STARFISH_ASSERT(isMainThread());
    if (!m_alive) {
        return;
    }
    PLAYER_LOGI("MediaPlayerTizen::close()\n");
    if (m_inPrepare) {
        m_foundError = true;
        handlePrepared();
        STARFISH_ASSERT(!m_alive);
        return;
    }
    if (m_seekState != SEEKSTATE_NO_SEEK) {
        m_foundError = true;
        handleSeeked();
        STARFISH_ASSERT(!m_alive);
        return;
    }

    m_alive = false;
    // Dispatch error event when found error
    if (m_foundError) {
        m_container->dispatchErrorEvent();
    }

    pause();

#ifdef STARFISH_TIZEN_TV
    if (m_container->isHTMLVideoElement() && m_container->frame()) {
        m_container->setNeedsComposite();
        // NOTE Deplay dispose()
        //      Transparent hole can be exposed by disposal of player.
        m_container->starFish()->platformWindow()->rendering();
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        PLAYER_LOGI(
            "MediaPlayerTizen::close() - dispose player next idle time\n");
        msgLoop->addIdler(nullptr,
                          [](size_t, void* data) {
                              MediaPlayerTizen* player =
                                  (MediaPlayerTizen*)data;
                              player->dispose();
                          },
                          this);
    } else {
        PLAYER_LOGI("MediaPlayerTizen::close() - instant disposal \n");
        dispose();
    }
#else
    dispose();
#endif
}

double MediaPlayerTizen::duration()
{
    if (m_activeMediaSource) {
        return m_activeMediaSource->duration();
    }
    int duration = 0;
    player_get_duration(m_nativePlayer, &duration);
    return duration / 1000.0;
}

static void updateTimeCallback(Window* window, void* data)
{
    MediaPlayerTizen* self = (MediaPlayerTizen*)data;
    if (self->seeking() || !self->alive()) {
        // Do not update time while seeking
        return;
    }
    if (self->isMSE() &&
        self->currentTime() == self->container()->officialPlaybackPosition() &&
        self->isMSEBufferEOS()) {
        self->handleEnded();
    } else {
        self->container()->setOfficialPlaybackPosition(self->currentTime());
    }
}

void MediaPlayerTizen::play()
{
    if (m_playbackState == PLAYBACK_STATE_PLAYING) {
        return;
    }
    player_state_e state;
    player_get_state(m_nativePlayer, &state);
    PLAYER_LOGI("MediaPlayerTizen::play() state : %d state2: %d ms: %p\n",
                (int)state, (int)m_playbackState, m_activeMediaSource);
    if (state < PLAYER_STATE_READY) {
        m_pendingPlay = true;
        return;
    }
    m_playbackState = PLAYBACK_STATE_PLAYING;
    player_start(m_nativePlayer);
    m_container->document()->browsingContext()->addPointerInRootSet(this);
    m_currentTimeUpdateTimer =
        m_container->window()->setInterval(updateTimeCallback, 250, this);
}

void MediaPlayerTizen::pause()
{
    if (m_playbackState != PLAYBACK_STATE_PLAYING) {
        return;
    }
    m_playbackState = PLAYBACK_STATE_PAUSED;
    if (m_container) {
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);
        m_container->window()->clearInterval(m_currentTimeUpdateTimer);
    }
    player_pause(m_nativePlayer);
    m_currentTimeUpdateTimer = SIZE_MAX;
}

void MediaPlayerTizen::initDisplay()
{
#if !defined(STARFISH_TIZEN_TV)
    m_canvasSurface =
        CanvasSurface::create(m_container->starFish()->platformWindow(), 1, 1);
    m_canvasSurface->clear();
#endif
}

void MediaPlayerTizen::setNativePlayerDefaultOptions(ResourceURL* url)
{
#if defined(STARFISH_TIZEN_HEADLESS)
    player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_NONE, nullptr);
#else
    if (m_container->isHTMLVideoElement()) {
#if defined(STARFISH_TIZEN_TV)
        player_display_video_at_paused_state(m_nativePlayer, true);
        player_display_h displayHandle = GET_DISPLAY(
            (Evas_Object*)m_container->starFish()->platformWindow()->unwrap());
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_OVERLAY,
                           displayHandle);
        player_set_display_mode(m_nativePlayer, PLAYER_DISPLAY_MODE_DST_ROI);
        // NOTE: Do not edit `player_set_display_roi_area` parameter
        m_lastAbsoluteROIArea = LayoutRect(0, 0, 1, 1);
        player_set_display_roi_area(m_nativePlayer, 0, 0, 1, 1);
#else
        player_set_display_mode(m_nativePlayer,
                                PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER);
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_EVAS,
                           m_canvasSurface->unwrap());
#endif
    } else {
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_NONE, nullptr);
    }
#endif
}

void MediaPlayerTizen::openPreparingMode()
{
    STARFISH_ASSERT(!m_inPrepare);
    m_inPrepare = true;
    m_container->document()->browsingContext()->addPointerInRootSet(this);
}

void MediaPlayerTizen::closePreparingMode()
{
    if (m_inPrepare) {
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);
        m_inPrepare = false;
    }
}

void MediaPlayerTizen::prepare(ResourceURL* url)
{
    m_currentURL = url;
    PLAYER_LOGI("MediaPlayerTizen::prepare\n");
    player_set_volume(m_nativePlayer, 1, 1);
    player_set_mute(m_nativePlayer, false);
    player_set_looping(m_nativePlayer, m_isLooping);
    int ret;
    ret = player_set_error_cb(
        m_nativePlayer,
        [](int errorCode, void* data) {
            PLAYER_LOGI("MediaPlayerTizen::player_error_cb\n");
            MediaPlayerTizen* player = (MediaPlayerTizen*)data;
            player->printNativePlayerError(errorCode);
            player->handlePlayerError();
        },
        this);
    STARFISH_ASSERT(ret == 0);
    ret = player_set_completed_cb(m_nativePlayer, completeCallback, this);
#ifdef STARFISH_TIZEN_TV
    ret = player_display_video_at_paused_state(m_nativePlayer, true);
    STARFISH_ASSERT(ret == 0);
#endif
    ret = player_set_buffering_cb(
        m_nativePlayer,
        [](int percent, void* data) {
            PLAYER_LOGI("MediaPlayerTizen -> buffering state... %d\n", percent);
        },
        this);
    setNativePlayerDefaultOptions(url);
    if (url->isBlobURL()) {
        BlobURLStore store;
        if (!WebView::stringToBlobURLString(url->urlString(), store)) {
            PLAYER_LOGE(
                "MediaPlayerTizen::prepare, seturl, FAIL - INVALID BLOB URL\n");
            processNextOperationQueueInContainer();
            return;
        }
        if (m_container->webView()->isValidBlobURL(store)) {
            player_set_memory_buffer(m_nativePlayer,
                                     ((Blob*)store.m_blob)->data(),
                                     ((Blob*)store.m_blob)->size());
        } else if (m_container->webView()->isValidMediaSourceBlobURL(store)) {
            BlobURLStore store;
            WebView::stringToBlobURLString(url->urlString(), store);
            MediaSource* ms = (MediaSource*)store.m_blob;
            m_activeMediaSource = ms;
            m_mseClient = new MediaPlayerTizenMediaSourceClient(this);
            m_activeMediaSource->addClient(m_mseClient);
            m_activeMediaSource->attach(m_container);
            if (m_container) {
                // Note: In MSE case, ignore defaultPlaybackPosition
                m_container->setDefaultPlaybackStartPosition(0);
            }
            processNextOperationQueueInContainer();
            return;
        } else {
            // fire eror
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        auto s = url->urlString()->toUTF8NonGCString();
        player_set_uri(m_nativePlayer, s.data());
    }

    openPreparingMode();
    ret = player_prepare_async(m_nativePlayer, preparedCallback, this);
    if (ret != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(
            "MediaPlayerTizen::player_prepare_async return error !!!\n");
        printNativePlayerError(ret);
        m_foundError = true;
        handlePrepared();
    }
}

void MediaPlayerTizen::handlePrepared()
{
    if (!isMainThread()) {
        PLAYER_LOGI("MediaPlayerTizen::handlePrepared in non-MainThread\n");
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* user_data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
                self->handlePrepared();
            },
            this);
    } else {
        PLAYER_LOGI("MediaPlayerTizen::handlePrepared in MainThread\n");
        closePreparingMode();
        if (m_foundError) {
            if (m_container) {
                m_container->giveupFetchingResource();
            }
            close();
            return;
        }

        char* videoCodec = nullptr;
        char* audioCodec = nullptr;
        player_get_codec_info(m_nativePlayer, &audioCodec, &videoCodec);

        if (videoCodec) {
            m_hasVideo = true;
            int width = 1;
            int height = 1;
            player_get_video_size(m_nativePlayer, &width, &height);
            STARFISH_ASSERT(width > 0);
            STARFISH_ASSERT(height > 0);
            m_videoWidth = (unsigned long)width;
            m_videoHeight = (unsigned long)height;

            m_canvasSurface->attachNativeBuffer(m_videoWidth, m_videoHeight);
        }

        PLAYER_LOGI("MediaPlayerTizen::prepare ok %s %s %d %d\n", videoCodec,
                    audioCodec, (int)m_videoWidth, (int)m_videoHeight);

        free(videoCodec);
        free(audioCodec);

        if (!m_activeMediaSource) {
            processNextOperationQueueInContainer();
            m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_METADATA);
        }
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_ENOUGH_DATA);
        if (m_container->isHTMLVideoElement() && m_container->frame()) {
            m_container->setNeedsComposite();
        }
        if (m_pendingPlay) {
            m_pendingPlay = false;
            play();
        }
    }
}

void MediaPlayerTizen::dispose()
{
    PLAYER_LOGI("MediaPlayerTizen::dispose (%p)\n", this);
    if (m_nativePlayer) {
        pause();
        player_unprepare(m_nativePlayer);
        player_unset_media_stream_buffer_status_cb_ex(m_nativePlayer,
                                                      PLAYER_STREAM_TYPE_AUDIO);
        player_unset_media_stream_buffer_status_cb_ex(m_nativePlayer,
                                                      PLAYER_STREAM_TYPE_VIDEO);
        player_unset_completed_cb(m_nativePlayer);
        player_unset_error_cb(m_nativePlayer);
        player_unset_buffering_cb(m_nativePlayer);
        player_destroy(m_nativePlayer);
        m_nativePlayer = nullptr;
    }
    if (m_activeMediaSource) {
        m_activeMediaSource->removeClient(m_mseClient);
        m_activeMediaSource->detach();
        m_activeMediaSource = nullptr;
    }
    if (m_mseClient) {
        m_mseClient = nullptr;
    }
    if (m_audioStream) {
        m_audioStream->releaseMediaFormat();
        m_audioStream = nullptr;
    }
    if (m_videoStream) {
        m_videoStream->releaseMediaFormat();
        m_videoStream = nullptr;
    }
    if (m_container) {
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_NOTHING);
    }
    if (m_playerDeadFlag) {
        *m_playerDeadFlag = true;
    }
    m_container = nullptr;
}

void MediaPlayerTizen::setVolume(double volume)
{
    PLAYER_LOGI("MediaPlayerTizen::setVolume(%f)\n", volume);
    if (!m_nativePlayer) {
        return;
    }
    player_state_e state;
    player_get_state(m_nativePlayer, &state);
    if (state > PLAYER_STATE_IDLE) {
        if (volume == 0.0) {
            setMuted(true);
            return;
        }
        setMuted(false);

        int ret = player_set_volume(m_nativePlayer, volume, volume);
        if (ret != PLAYER_ERROR_NONE) {
            PLAYER_LOGE("**ERROR: player_set_volume %x -> ", ret);
        }
    }
}

void MediaPlayerTizen::setMuted(bool muted)
{
    PLAYER_LOGI("MediaPlayerTizen::setMuted(%s)\n", muted ? "true" : "false");
    if (!m_nativePlayer) {
        return;
    }
    player_state_e state;
    player_get_state(m_nativePlayer, &state);
    if (state > PLAYER_STATE_IDLE) {
        int ret = player_set_mute(m_nativePlayer, muted);
        if (ret != PLAYER_ERROR_NONE) {
            PLAYER_LOGE("**ERROR: player_set_mute %x -> ", ret);
        }
    }
}

void MediaPlayerTizen::drawVideo(Compositor* canvas,
                                 const LayoutRect& videoRect,
                                 const LayoutRect& absVideoRect)
{
    if (!alive()) {
        return;
    }
#if !defined(STARFISH_TIZEN_HEADLESS)
    player_state_e state = PLAYER_STATE_NONE;
    player_get_state(m_nativePlayer, &state);
    if (state < PLAYER_STATE_READY) {
        return;
    }
    if (isMSE() && m_playbackState == MediaPlayer::PLAYBACK_STATE_END) {
        return;
    }
#if defined(STARFISH_TIZEN_TV)
    canvas->punchHole(Unit::Rect(videoRect.x(), videoRect.y(),
                                 videoRect.width(), videoRect.height()));
    if (m_lastAbsoluteROIArea != absVideoRect) {
        player_set_display_roi_area(
            m_nativePlayer, absVideoRect.x().toInt(), absVideoRect.y().toInt(),
            absVideoRect.width().toInt(), absVideoRect.height().toInt());
        m_lastAbsoluteROIArea = absVideoRect;
    }
#else
    canvas->setColor(Unit::Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
    canvas->drawSurface(m_canvasSurface,
                        Unit::Rect(videoRect.x(), videoRect.y(),
                                   videoRect.width(), videoRect.height()));
#endif
#endif
}

#ifdef STARFISH_RUN_MSE_THREAD
static void* threadFillingBuffer(void* data)
{
    MediaPlayerTizen* self = (MediaPlayerTizen*)data;
    bool* playerDeadFlag = self->m_playerDeadFlag;
    while (!(*playerDeadFlag)) {
        if (self->playbackState() != MediaPlayer::PLAYBACK_STATE_END) {
            MediaStream* audioStream = self->currentStream(StreamTypeAudio);
            MediaStream* videoStream = self->currentStream(StreamTypeVideo);
            if (audioStream && !audioStream->waitingDemuxer() &&
                audioStream->needPacket()) {
                PLAYER_LOGI("[AUDIO] Player need data\n");
                self->fillBuffer(audioStream);
            }
            if (videoStream && !videoStream->waitingDemuxer() &&
                videoStream->needPacket()) {
                PLAYER_LOGI("[VIDEO] Player need data\n");
                self->fillBuffer(videoStream);
            }
        }
        sleep(1);
    }
    PLAYER_LOGI("Close fillingBuffer thread\n");
    free(playerDeadFlag);
    return nullptr;
}
#endif

void MediaPlayerTizen::prepareMediaSource()
{
    PLAYER_LOGI("MediaPlayerTizen::prepareMediaSource\n");
    initAudioStreamInfo();
    if (m_foundError) {
        return;
    }
#if !defined(STARFISH_TIZEN_HEADLESS)
    initVideoStreamInfo();
    if (m_foundError) {
        return;
    }
#endif
    if (!m_audioStream && !m_videoStream) {
        return;
    }

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);
    openPreparingMode();
    int ret = player_prepare_async(m_nativePlayer, preparedCallback, this);
    if (ret != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(
            "MediaPlayerTizen:: player_prepare_async return error !!!\n");
        printNativePlayerError(ret);
        m_foundError = true;
        handlePrepared();
    }
#ifdef STARFISH_RUN_MSE_THREAD
    else {
        m_playerDeadFlag = (bool*)malloc(sizeof(bool));
        *m_playerDeadFlag = false;
        Thread* t = new Thread(m_container->starFish());
        t->run(m_container->starFish()->messageLoop(), threadFillingBuffer,
               this);
    }
#endif
    PLAYER_LOGI("MediaPlayerTizen::prepareMediaSource end\n");
}

void MediaPlayerTizen::fillBuffer(MediaStream* stream)
{
    Locker<Mutex> locker(*m_fillBufferMutex);
    fillBufferWithoutGuard(stream);
}

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#define DEBUG_STREAMBUFFER_LOG(...)                              \
    PLAYER_LOGI("[%s] ", stream->isAudio() ? "AUDIO" : "VIDEO"); \
    STARFISH_LOG_INFO(__VA_ARGS__);
#else
#define DEBUG_STREAMBUFFER_LOG(...)
#endif

void MediaPlayerTizen::enterUnderrunState()
{
    if (!isMainThread()) {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data) {
                MediaPlayerTizen* player = (MediaPlayerTizen*)data;
                player->enterUnderrunState();
            },
            this);
    } else {
        if (!alive() || m_underrunMode) {
            return;
        }
        PLAYER_LOGI("MediaPlayerTizen::enterUnderrunState\n");
        m_underrunMode = true;
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_CURRENT_DATA);
        if (m_playbackState == PLAYBACK_STATE_PLAYING) {
            pause();
            m_container->play();
        }
    }
}

void MediaPlayerTizen::exitUnderrunState()
{
    if (!isMainThread()) {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data) {
                MediaPlayerTizen* player = (MediaPlayerTizen*)data;
                player->exitUnderrunState();
            },
            this);
    } else {
        if (!alive() || !m_underrunMode) {
            return;
        }
        bool allOut = true;
        if (m_audioStream) {
            allOut &= !m_audioStream->isBufferState(
                MediaStream::BUFFERSTATE_UNDER_RUN);
        }
        if (m_videoStream) {
            allOut &= !m_videoStream->isBufferState(
                MediaStream::BUFFERSTATE_UNDER_RUN);
        }
        if (!allOut) {
            return;
        }
        PLAYER_LOGI("MediaPlayerTizen::exitUnderrunState\n");
        m_underrunMode = false;
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_ENOUGH_DATA);
    }
}

void MediaPlayerTizen::handlePlayerBuffer(StreamType type,
                                          uint64_t currentBytes)
{
// Other thread
#ifdef STARFISH_RUN_MSE_THREAD
    MediaStream* stream = currentStream(type);
    if (!alive() || !stream) {
        return;
    }
    MediaStream::BufferState prevState = stream->bufferState();
    if (prevState == MediaStream::BUFFERSTATE_EOS) {
        return;
    }
    uint64_t maxSize = stream->maxBufferSize();
    uint64_t rate = currentBytes * 100 / maxSize;
    MediaStream::BufferState state = MediaStream::BUFFERSTATE_NORMAL;
    if (rate < 1) {
        state = MediaStream::BUFFERSTATE_UNDER_RUN;
    } else if (rate < 30) {
        state = MediaStream::BUFFERSTATE_NEED_PACKET;
    }
    if (prevState != state) {
        DEBUG_STREAMBUFFER_LOG("Buffer state: %s > %s\n",
                               bufferStateString(prevState),
                               bufferStateString(state));
        stream->setBufferState(state);
        if (prevState == MediaStream::BUFFERSTATE_UNDER_RUN) {
            exitUnderrunState();
        } else if (prevState > MediaStream::BUFFERSTATE_UNDER_RUN &&
                   state == MediaStream::BUFFERSTATE_UNDER_RUN) {
            enterUnderrunState();
        }
    }
#else
    MediaStream* stream = currentStream(type);
    uint64_t lastBytes;
    uint64_t maxSize;
    {
        Locker<Mutex> locker(*m_fillBufferMutex);
        lastBytes = stream->lastBufferBytes();
        if (lastBytes != 0 && lastBytes == currentBytes) {
            return;
        }
        stream->setLastBufferBytes(currentBytes);
        maxSize = stream->maxBufferSize();
    }
    if (!stream->waitingDemuxer() && currentBytes <= lastBytes &&
        currentBytes < (maxSize * 0.1)) {
        DEBUG_STREAMBUFFER_LOG("Player need data (%llu/%llu)\n", currentBytes,
                               maxSize);
        fillBuffer(stream);
    }
#endif
}

void MediaPlayerTizen::fillBufferWithoutGuard(MediaStream* stream)
{
    STARFISH_ASSERT(stream);
    if (!alive() || !stream->mediaFormat()) {
        return;
    }
    media_format_h format = stream->mediaFormat();

#define RETURN_WHEN_MEDIA_PACKET_ERROR(...)  \
    if (ret != MEDIA_PACKET_ERROR_NONE) {    \
        DEBUG_STREAMBUFFER_LOG(__VA_ARGS__); \
        printMediaPacketError(ret);          \
        handlePlayerError();                 \
        return;                              \
    }

    SourceBuffer* sb = activeSourceBuffer(stream->type());
    if (!sb) {
        return;
    }
    uint64_t streamIdx = activeStreamIndex(stream->type());
    size_t currentInitIndex = stream->initSegmentIndex();
    uint64_t lastDTS = stream->lastSubmittedDTS();
    DEBUG_STREAMBUFFER_LOG("fillBuffer start %llums\n", lastDTS);

#ifdef STARFISH_MEDIAPLAYER_DEBUG
    uint64_t submitMS = 0;
    size_t submitCount = 0;
#endif
    size_t submitBytes = 0;
    uint64_t sizeUpTo =
        stream->maxBufferSize() * STARFISH_MSE_SUBMIT_BYTES_RATE;
    while (submitBytes < sizeUpTo) {
        std::pair<MediaPacket*, size_t> packet =
            sb->findProperMediaPacket(streamIdx, lastDTS);
        media_packet_h mediaPacket = nullptr;

        if (!packet.first) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            DEBUG_STREAMBUFFER_LOG("fillBuffer try to detect end -> %d %d\n",
                                   (int)endTime, (int)lastDTS);
            uint64_t lastBufferedTime = sb->lastBufferedTimestamp(streamIdx);
            if ((endTime - lastDTS) < 10 ||
                ((lastDTS == lastBufferedTime) &&
                 (std::abs(endTime - lastBufferedTime) < 1000))) {
                stream->setBufferState(MediaStream::BUFFERSTATE_EOS);
                media_packet_create(format, NULL, NULL, &mediaPacket);
                media_packet_set_flags(mediaPacket, MEDIA_PACKET_END_OF_STREAM);
                player_push_media_stream(m_nativePlayer, mediaPacket);
                DEBUG_STREAMBUFFER_LOG("fillBuffer detect EOS\n");
                break;
            }
            DEBUG_STREAMBUFFER_LOG("fillBuffer runs into under run state[1]\n");
            stream->setWaitingDemuxer(true);
            break;
        }
        if (packet.first->m_dts < lastDTS ||
            packet.first->m_dts - lastDTS > 500) {
            sb->clearPacketAccessCache();
            DEBUG_STREAMBUFFER_LOG(
                "fillBuffer runs into under run state[2] - requested(%lld) but "
                "returned(%lld)\n",
                lastDTS, packet.first->m_dts);
            stream->setWaitingDemuxer(true);
            break;
        }

        if (packet.second != currentInitIndex) {
            if (!packet.first->m_hasIdr) {
                lastDTS = packet.first->m_dts + packet.first->m_duration;
                DEBUG_STREAMBUFFER_LOG(
                    "fillBuffer drops non-idr packet (config changed)\n");
                continue;
            } else {
                DEBUG_STREAMBUFFER_LOG(
                    "fillBuffer detect changed config (and will submit packet "
                    "including idr. DTS:%d)\n",
                    (int)packet.first->m_dts);
#if defined(STARFISH_TIZEN_TV)
                updateStreamInfo(stream, currentInitIndex, packet.second);
#endif
                stream->setInitSegmentIndex(packet.second);
                currentInitIndex = packet.second;
                sizeUpTo =
                    stream->maxBufferSize() * STARFISH_MSE_SUBMIT_BYTES_RATE;
            }
        }

        int ret = media_packet_create_from_external_memory(
            format, packet.first->m_data, packet.first->m_dataSize, nullptr,
            nullptr, &mediaPacket);
        RETURN_WHEN_MEDIA_PACKET_ERROR(
            "ERROR: media_packet_create_from_external_memory\n");

        ret = media_packet_set_pts(mediaPacket, packet.first->m_pts * 1e6);
        RETURN_WHEN_MEDIA_PACKET_ERROR("ERROR: media_packet_set_pts\n");

        ret = media_packet_set_duration(mediaPacket,
                                        packet.first->m_duration * 1e6);
        RETURN_WHEN_MEDIA_PACKET_ERROR("ERROR: media_packet_set_duration\n");

        // DEBUG_STREAMBUFFER_LOG("Submit packet pts:%llu dts:%llu dur:%llu\n",
        // packet.first->m_pts, packet.first->m_dts, packet.first->m_duration);

        ret = player_push_media_stream(m_nativePlayer, mediaPacket);
        media_packet_destroy(mediaPacket);

        if (ret == PLAYER_ERROR_BUFFER_SPACE) {
            sb->revertLastCacheIfPossible(streamIdx);
            DEBUG_STREAMBUFFER_LOG("fillBuffer stop: PLAYER BUFFER FULL\n");
            break;
        } else if (ret != PLAYER_ERROR_NONE) {
            DEBUG_STREAMBUFFER_LOG("ERROR: player_push_media_stream\n");
            printNativePlayerError(ret);
            handlePlayerError();
            return;
        }
#ifdef STARFISH_MEDIAPLAYER_DEBUG
        submitCount++;
        submitMS += packet.first->m_duration;
#endif
        submitBytes += packet.first->m_dataSize;
        lastDTS = packet.first->m_dts + packet.first->m_duration;
    }
    stream->setLastSubmittedDTS(lastDTS);
    DEBUG_STREAMBUFFER_LOG("fillBuffer end %llums (count:%d, size:%d)\n\n",
                           lastDTS, (int)submitCount, (int)submitBytes);

#undef DEBUG_STREAMBUFFER_LOG
#undef RETURN_WHEN_MEDIA_PACKET_ERROR
}

void MediaPlayerTizen::setLoop(bool loop)
{
    m_isLooping = loop;
    if (m_nativePlayer) {
        player_set_looping(m_nativePlayer, loop);
    }
}

bool MediaPlayerTizen::isMSEBufferEOS()
{
    bool isEOS = true;
    if (m_audioStream) {
        isEOS &= m_audioStream->isBufferState(MediaStream::BUFFERSTATE_EOS);
    }
    if (m_videoStream) {
        isEOS &= m_videoStream->isBufferState(MediaStream::BUFFERSTATE_EOS);
    }
    return isEOS;
}

#if defined(STARFISH_TIZEN_TV)
void MediaPlayerTizen::updateStreamInfo(MediaStream* stream,
                                        size_t pastInitIndex,
                                        size_t newInitIndex)
{
    if (stream->isVideo()) {
        updateVideoStreamInfo(stream, pastInitIndex, newInitIndex);
    } else {
        // TODO audio
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

void MediaPlayerTizen::updateAudioStreamInfo(MediaStream* stream,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
    // TODO audio
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void MediaPlayerTizen::updateVideoStreamInfo(MediaStream* stream,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
    media_format_h mediaFormat = stream->mediaFormat();
    if (!mediaFormat) {
        return;
    }
    auto mediaFormatExtra = stream->videoFormatExtra();
    SourceBuffer* sb = activeSourceBuffer(StreamTypeVideo);
    StreamInfo* pastInfo =
        sb->streamInfo(pastInitIndex, activeStreamIndex(StreamTypeVideo));
    StreamInfo* newInfo =
        sb->streamInfo(newInitIndex, activeStreamIndex(StreamTypeVideo));
    m_videoWidth = newInfo->videoWidth();
    m_videoHeight = newInfo->videoHeight();
    media_format_set_video_width(mediaFormat, m_videoWidth);
    media_format_set_video_height(mediaFormat, m_videoHeight);
    stream->setMaxBufferSize((m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 /
                             8 * 5);
    bool hasFramerateChanged = false;
    if (newInfo->videoHasFramerate() && newInfo->videoFramerate().isValid()) {
        if (newInfo->videoFramerate().toDouble() > 60.0) {
            newInfo->setVideoFramerate(Framerate(60, 1));
        }
        if (!pastInfo->videoHasFramerate() ||
            !pastInfo->videoFramerate().isValid() ||
            isFramerateChanged(pastInfo->videoFramerate(),
                               newInfo->videoFramerate())) {
            hasFramerateChanged = true;
        }
    }
    if (hasFramerateChanged) {
        mediaFormatExtra->is_framerate_changed = isFramerateChanged;
        mediaFormatExtra->framerate_num = newInfo->videoFramerate().m_num;
        mediaFormatExtra->framerate_den = newInfo->videoFramerate().m_den;
        media_format_set_extra(mediaFormat, mediaFormatExtra);
        int ret = player_set_media_stream_info(
            m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO, mediaFormat);
        if (ret != PLAYER_ERROR_NONE) {
            PLAYER_LOGI("ERROR: player_set_media_stream_info\n");
            printNativePlayerError(ret);
        }
    }
    int ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO, stream->maxBufferSize());
    if (ret != PLAYER_ERROR_NONE) {
        PLAYER_LOGI("ERROR: player_set_media_stream_buffer_max_size\n");
        printNativePlayerError(ret);
    }
    PLAYER_LOGI("New video -----------------------------\n");
    PLAYER_LOGI("> index     : %d\n", (int)newInitIndex);
    PLAYER_LOGI("> size      : %lu x %lu\n", m_videoWidth, m_videoHeight);
    PLAYER_LOGI("> max_buffer: %llu\n", stream->maxBufferSize());
    PLAYER_LOGI("> framerate_changed: %s\n",
                hasFramerateChanged ? "true" : "false");
    PLAYER_LOGI(
        "> avg_frame_rate: %d/%d(%f)\n", newInfo->videoFramerate().m_num,
        newInfo->videoFramerate().m_den, newInfo->videoFramerate().toDouble());
    PLAYER_LOGI("---------------------------------------\n");
}
#endif

#define RETURN_WHEN_PLAYER_ERROR(...) \
    if (ret != PLAYER_ERROR_NONE) {   \
        PLAYER_LOGE(__VA_ARGS__);     \
        printNativePlayerError(ret);  \
        handlePlayerError();          \
        return;                       \
    }

void MediaPlayerTizen::initVideoStreamInfo(size_t initSegmentIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeVideoSourceBuffer();
    if (!sb) {
        return;
    }

    // set video options
    PLAYER_LOGI("MediaPlayerTizen::initVideoStreamInfo\n");

    m_videoStream = new MediaStream(StreamTypeVideo);
    if (m_container) {
        m_videoStream->setLastSubmittedDTS(
            m_container->defaultPlaybackStartPosition() * 1000);
    }
    if (!m_videoStream->createMediaFormat()) {
        handlePlayerError();
        return;
    }
    media_format_h mediaFormat = m_videoStream->mediaFormat();
    // Get info from demuxer
    StreamInfo* info = sb->streamInfo(
        initSegmentIndex, m_activeMediaSource->activeVideoStreamIndex());
    // Get mimetype
    if (info->isCodec(MediaCodecVideoH264)) {
        media_format_set_video_mime(mediaFormat, MEDIA_FORMAT_H264_SP);
    } else if (info->isCodec(MediaCodecVideoVP9)) {
        media_format_set_video_mime(mediaFormat, MEDIA_FORMAT_VP9);
    } else {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    m_videoWidth = info->videoWidth();
    m_videoHeight = info->videoHeight();
    media_format_set_video_width(mediaFormat, m_videoWidth);
    media_format_set_video_height(mediaFormat, m_videoHeight);
    m_videoStream->setMaxBufferSize(
        (m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 / 8 * 5);

#if defined(STARFISH_TIZEN_TV)
    auto mediaFormatExtra = m_videoStream->videoFormatExtra();
    mediaFormatExtra->max_width = STARFISH_VIDEO_MAX_WIDTH;
    mediaFormatExtra->max_height = STARFISH_VIDEO_MAX_HEIGHT;
    m_videoStream->setMaxBufferSize(
        (m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 / 8 * 5);

    if (info->videoHasFramerate() && info->videoFramerate().isValid()) {
        if (info->videoFramerate().toDouble() > 60.0) {
            info->setVideoFramerate(Framerate(60, 1));
        }
        mediaFormatExtra->framerate_num = info->videoFramerate().m_num;
        mediaFormatExtra->framerate_den = info->videoFramerate().m_den;
    } else {
        mediaFormatExtra->framerate_num = STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM;
        mediaFormatExtra->framerate_den = STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN;
    }
    mediaFormatExtra->hdr_mode = MEDIA_STREAM_HDR_TYPE_MATROSKA;
    mediaFormatExtra->hdr_info = std::string().c_str();
    mediaFormatExtra->is_framerate_changed = false;
    // TODO PLAYER_DRM_TYPE_EME
    mediaFormatExtra->drm_type = PLAYER_DRM_TYPE_NONE;

    // Extra data
    mediaFormatExtra->extradata_size = info->m_extraData.size();
    if (mediaFormatExtra->extradata_size != 0) {
        mediaFormatExtra->codec_extradata =
            (unsigned char*)malloc(mediaFormatExtra->extradata_size);
        memcpy(mediaFormatExtra->codec_extradata, info->m_extraData.data(),
               mediaFormatExtra->extradata_size);
    }
    media_format_set_extra(mediaFormat, mediaFormatExtra);
#else
    if (info->duration() > 0) {
        media_format_set_video_frame_rate(mediaFormat,
                                          (1000 / info->duration()));
    }
#endif
    PLAYER_LOGI("Video Info-----------------------------\n");
    PLAYER_LOGI("> codec     : %s\n", info->codecString());
#if defined(STARFISH_TIZEN_TV)
    PLAYER_LOGI("> framerate : %d/%d\n", mediaFormatExtra->framerate_num,
                mediaFormatExtra->framerate_den);
#endif
    PLAYER_LOGI("> size      : %dx%d\n", info->videoWidth(),
                info->videoHeight());
    PLAYER_LOGI("> max_buffer: %llu\n", m_videoStream->maxBufferSize());
    PLAYER_LOGI("---------------------------------------\n");

    player_set_media_stream_buffer_min_threshold(m_nativePlayer,
                                                 PLAYER_STREAM_TYPE_VIDEO, 100);
    int ret = player_set_media_stream_buffer_status_cb_ex(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
        [](player_media_stream_buffer_status_e status, unsigned long long bytes,
           void* user_data) {
            MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
            self->handlePlayerBuffer(StreamTypeVideo, bytes);
        },
        this);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_status_cb_ex\n");

    ret = player_set_media_stream_info(m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
                                       mediaFormat);
    RETURN_WHEN_PLAYER_ERROR("ERROR: player_set_media_stream_info\n");

    // TODO Replace test value to real estimate value
    ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
        m_videoStream->maxBufferSize());
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_max_size\n");
    m_videoStream->setInitSegmentIndex(initSegmentIndex);
}

void MediaPlayerTizen::initAudioStreamInfo(size_t initSegmentIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeAudioSourceBuffer();
    if (!sb) {
        return;
    }
    // set audio options
    PLAYER_LOGI("MediaPlayerTizen::initAudioStreamInfo\n");

    m_audioStream = new MediaStream(StreamTypeAudio);
    if (m_container) {
        m_audioStream->setLastSubmittedDTS(
            m_container->defaultPlaybackStartPosition() * 1000);
    }
    if (!m_audioStream->createMediaFormat()) {
        handlePlayerError();
        return;
    }
    media_format_h mediaFormat = m_audioStream->mediaFormat();

    // Get info from demuxer
    StreamInfo* info = sb->streamInfo(
        initSegmentIndex, m_activeMediaSource->activeAudioStreamIndex());

    if (info->isCodec(MediaCodecAudioAAC)) {
        media_format_set_audio_mime(mediaFormat, MEDIA_FORMAT_AAC);
    } else if (info->isCodec(MediaCodecAudioVorbis)) {
        media_format_set_audio_mime(mediaFormat, MEDIA_FORMAT_VORBIS);
    } else {
        // TODO
        media_format_set_audio_mime(mediaFormat, MEDIA_FORMAT_MP3);
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    media_format_set_audio_channel(mediaFormat, (int)info->audioChannels());
    media_format_set_audio_samplerate(mediaFormat,
                                      (int)info->audioSampleRate());
// media_format_set_audio_avg_bps(m_audioFormat, audioCodecCtx->bit_rate);

#if defined(STARFISH_TIZEN_TV)
    auto mediaFormatExtra = m_audioStream->audioFormatExtra();
    mediaFormatExtra->extradata_size = info->m_extraData.size();
    if (mediaFormatExtra->extradata_size != 0) {
        mediaFormatExtra->codec_extradata =
            (unsigned char*)malloc(mediaFormatExtra->extradata_size);
        memcpy(mediaFormatExtra->codec_extradata, info->m_extraData.data(),
               mediaFormatExtra->extradata_size);
    }
    // TODO PLAYER_DRM_TYPE_EME
    mediaFormatExtra->drm_type = PLAYER_DRM_TYPE_NONE;
    media_format_set_extra(mediaFormat, mediaFormatExtra);
#endif
    PLAYER_LOGI("Audio Info-----------------------------\n");
    PLAYER_LOGI("> codec     : %s\n", info->codecString());
    PLAYER_LOGI("> channels  : %d\n", (int)info->audioChannels());
    PLAYER_LOGI("> sample_rate : %d\n", (int)info->audioSampleRate());
    PLAYER_LOGI("> extradata : %d\n", (int)info->m_extraData.size());
    PLAYER_LOGI("---------------------------------------\n");

    player_set_media_stream_buffer_min_threshold(m_nativePlayer,
                                                 PLAYER_STREAM_TYPE_AUDIO, 100);
    int ret = player_set_media_stream_buffer_status_cb_ex(
        m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
        [](player_media_stream_buffer_status_e status, unsigned long long bytes,
           void* user_data) {
            MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
            self->handlePlayerBuffer(StreamTypeAudio, bytes);
        },
        this);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_status_cb_ex\n");

    ret = player_set_media_stream_info(m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
                                       mediaFormat);
    RETURN_WHEN_PLAYER_ERROR("ERROR: player_set_media_stream_info\n");

    ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
        m_audioStream->maxBufferSize());
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_max_size\n");

    m_audioStream->setInitSegmentIndex(initSegmentIndex);
}
#undef RETURN_WHEN_PLAYER_ERROR

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizen(element);
}
}

#endif
#endif /* STARFISH_ENABLE_MULTIMEDIA */
