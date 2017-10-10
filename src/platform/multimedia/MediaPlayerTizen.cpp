/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#ifdef STARFISH_TIZEN_TV

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/multimedia/MediaPlayerTizen.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

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

void MediaPlayerTizen::printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum)      \
    case errorenum:                      \
        PLAYER_LOGI("%s\n", #errorenum); \
        return;
        GEN_ERROR_PRINTS(PLAYER_ERROR_OUT_OF_MEMORY)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_PARAMETER)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NO_SUCH_FILE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_OPERATION)
        GEN_ERROR_PRINTS(PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SEEK_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_STATE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NOT_SUPPORTED_FILE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_URI)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SOUND_POLICY)
        GEN_ERROR_PRINTS(PLAYER_ERROR_CONNECTION_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_VIDEO_CAPTURE_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_EXPIRED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_NO_LICENSE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_FUTURE_USE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_NOT_PERMITTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_RESOURCE_LIMIT)
        GEN_ERROR_PRINTS(PLAYER_ERROR_PERMISSION_DENIED)
#undef GEN_ERROR_PRINTS
    default:
        PLAYER_LOGI("Unknown error\n");
        return;
    }
}

void MediaPlayerTizen::printMediaPacketError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum)      \
    case errorenum:                      \
        PLAYER_LOGI("%s\n", #errorenum); \
        return;
        GEN_ERROR_PRINTS(MEDIA_PACKET_ERROR_OUT_OF_MEMORY)
        GEN_ERROR_PRINTS(MEDIA_PACKET_ERROR_INVALID_PARAMETER)
        GEN_ERROR_PRINTS(MEDIA_PACKET_ERROR_INVALID_OPERATION)
        GEN_ERROR_PRINTS(MEDIA_PACKET_ERROR_FILE_NO_SPACE_ON_DEVICE)
        GEN_ERROR_PRINTS(MEDIA_PACKET_ERROR_NO_AVAILABLE_PACKET)
#undef GEN_ERROR_PRINTS
    default:
        PLAYER_LOGI("Unknown error\n");
        return;
    }
}

void MediaPlayerTizen::printMediaFormatError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum)      \
    case errorenum:                      \
        PLAYER_LOGI("%s\n", #errorenum); \
        return;
        GEN_ERROR_PRINTS(MEDIA_FORMAT_ERROR_OUT_OF_MEMORY)
        GEN_ERROR_PRINTS(MEDIA_FORMAT_ERROR_INVALID_PARAMETER)
        GEN_ERROR_PRINTS(MEDIA_FORMAT_ERROR_INVALID_OPERATION)
        GEN_ERROR_PRINTS(MEDIA_FORMAT_ERROR_FILE_NO_SPACE_ON_DEVICE)
#undef GEN_ERROR_PRINTS
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
        if (m_player && m_player->alive() && m_player->m_activeMediaSource &&
            m_player->m_activeMediaSource->activeVideoSourceBuffer() == s) {
            m_player->fillVideoBufferIfNeeded();
        }
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player && m_player->alive() && m_player->m_activeMediaSource &&
            m_player->m_activeMediaSource->activeAudioSourceBuffer() == s) {
            m_player->fillAudioBufferIfNeeded();
        }
    }

    MediaPlayerTizen* m_player;
};

MediaPlayerTizen::MediaPlayerTizen(HTMLMediaElement* element)
    : MediaPlayer(element)
    , m_inPrepare(false)
    , m_needsPlayAfterPrepare(false)
    , m_seekingTimer(SIZE_MAX)
    , m_mseClient(nullptr)
    , m_bufferMutex(new Mutex())
    , m_mediaFormatMutex(new Mutex())
    , m_preparedCallback(nullptr)
    , m_completeCallback(nullptr)
    , m_canvasSurface(nullptr)
    , m_audioFormat(nullptr)
    , m_videoFormat(nullptr)
    , m_isAudioBufferUnderrunState(false)
    , m_isVideoBufferUnderrunState(false)
    , m_audioMaxBufferSize(320 * 1000 / 8 * 5)
    , m_videoMaxBufferSize(0)
    , m_audioInitSegmentIndex(0)
    , m_videoInitSegmentIndex(0)
    , m_audioLastBufferBytes(SIZE_MAX)
    , m_videoLastBufferBytes(SIZE_MAX)
{
    m_lastVideoDTS = m_lastAudioDTS =
        element->defaultPlaybackStartPosition() * 1000;
    m_videoFormatExtra.codec_extradata = nullptr;
    m_videoFormatExtra.extradata_size = 0;
    m_audioFormatExtra.codec_extradata = nullptr;
    m_audioFormatExtra.extradata_size = 0;
    player_create(&m_nativePlayer);
    initDisplay();

    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       MediaPlayerTizen* player =
                                           (MediaPlayerTizen*)obj;
                                       player->unprepareOperation();
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

void MediaPlayerTizen::fillVideoBufferIfNeeded()
{
    Locker<Mutex> locker(*m_bufferMutex);
    if (m_isVideoBufferUnderrunState) {
        fillBuffer(StreamTypeVideo);
    }
}

void MediaPlayerTizen::fillAudioBufferIfNeeded()
{
    Locker<Mutex> locker(*m_bufferMutex);
    if (m_isAudioBufferUnderrunState) {
        fillBuffer(StreamTypeAudio);
    }
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
        player_unset_completed_cb(m_nativePlayer);
        player_start(m_nativePlayer);
        player_pause(m_nativePlayer);
        player_set_completed_cb(m_nativePlayer, m_completeCallback, this);
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
        endOfStream();
        return;
    }

    m_seekState = SEEKSTATE_SEEKING;
    m_container->document()->browsingContext()->addPointerInRootSet(this);

    // Set timer
    // Note : Sometimes player_set_position_async() does not invoke its
    // callback.
    //        in that case, the timer will help the player to remove rooted
    //        pointer and properly destroyed
    m_seekingTimer = m_container->window()->setTimeout(
        [](Window* window, void* data) {
            MediaPlayerTizen* self = (MediaPlayerTizen*)data;
            PLAYER_LOGI("MediaPlayerTizen::seek() : timeout\n");
            self->handleSeekTimeout();
        },
        MAX_WAITING_SECONDS_FOR_SEEK_OPERATION, this);

    seekOperation((int)(time * 1000.0));
}

void MediaPlayerTizen::seekOperation(int timeInMS)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    handleSeeked();
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
        pause();
        if (m_container) {
            m_container->mediaPlayerNotifyEndedItsContainer();
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

void MediaPlayerTizen::startPlaying()
{
    if (!m_inPlaying) {
        m_inPlaying = true;
        m_isEnded = false;
        player_start(m_nativePlayer);
        m_container->document()->browsingContext()->addPointerInRootSet(this);
        m_currentTimeUpdateTimer = m_container->window()->setInterval(
            [](Window* window, void* data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)data;
                if (self->m_seekState == SEEKSTATE_NO_SEEK) {
                    if (self->currentTime() ==
                            self->m_container->officialPlaybackPosition() &&
                        self->m_isEnded && !self->m_isLooping) {
                        self->endOfStream();
                    } else {
                        self->m_container->setOfficialPlaybackPosition(
                            self->currentTime());
                    }
                }
            },
            250, this);
    }
}

void MediaPlayerTizen::stopPlaying()
{
    if (m_inPlaying) {
        m_inPlaying = false;
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);
        m_container->window()->clearInterval(m_currentTimeUpdateTimer);
        m_currentTimeUpdateTimer = SIZE_MAX;
    }
}

void MediaPlayerTizen::close()
{
    if (!m_alive) {
        return;
    }
    if (m_inPrepare || m_seekState != SEEKSTATE_NO_SEEK) {
        // Wait prepare/seek callback
        m_foundError = true;
        return;
    }

    m_alive = false;
    // Dispatch error event when found error
    if (m_foundError) {
        m_container->dispatchErrorEvent();
    }
    if (m_seekingTimer != SIZE_MAX) {
        m_container->window()->clearTimeout(m_seekingTimer);
        m_seekingTimer = SIZE_MAX;
    }

    stopPlaying();

    unprepareOperation();
    if (m_nativePlayer) {
        player_unset_media_stream_buffer_status_cb_ex(m_nativePlayer,
                                                      PLAYER_STREAM_TYPE_AUDIO);
        player_unset_media_stream_buffer_status_cb_ex(m_nativePlayer,
                                                      PLAYER_STREAM_TYPE_VIDEO);
        player_unset_completed_cb(m_nativePlayer);
        player_unset_error_cb(m_nativePlayer);
        player_destroy(m_nativePlayer);
    }
    if (m_audioFormat) {
        media_format_unref(m_audioFormat);
        m_audioFormat = nullptr;
    }
    if (m_videoFormat) {
        media_format_unref(m_videoFormat);
        m_videoFormat = nullptr;
    }
    if (m_videoFormatExtra.extradata_size > 0) {
        free(m_videoFormatExtra.codec_extradata);
        m_videoFormatExtra.codec_extradata = nullptr;
        m_videoFormatExtra.extradata_size = 0;
    }
    if (m_audioFormatExtra.extradata_size > 0) {
        free(m_audioFormatExtra.codec_extradata);
        m_audioFormatExtra.codec_extradata = nullptr;
        m_audioFormatExtra.extradata_size = 0;
    }

    m_nativePlayer = nullptr;
    m_container = nullptr;

#ifndef NDEBUG
    PLAYER_LOGI(
        "[TRACE_MSE_GC] MediaPlayerTizen::close() : Rooting count of "
        "player(%p) is %d\n",
        this, (int)m_container->starFish()->countPointersInRootSet(this));
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

void MediaPlayerTizen::play()
{
    player_state_e state;
    player_get_state(m_nativePlayer, &state);
    PLAYER_LOGI("MediaPlayerTizen::play() state : %d state2: %d ms: %p\n",
                (int)state, (int)m_playbackState, m_activeMediaSource);
    if (m_activeMediaSource &&
        m_playbackState == PlaybackState::PLAYBACK_STATE_END) {
        PLAYER_LOGI("MediaPlayerTizen::play() meets mse && playback end\n");
        int ret;
        unprepareOperation();
        PLAYER_LOGI("MediaPlayerTizen::play() unprepare end %d\n", (int)ret);
        ret = player_destroy(m_nativePlayer);
        PLAYER_LOGI("MediaPlayerTizen::play() destory end %d\n", (int)ret);
        ret = player_create(&m_nativePlayer);
        PLAYER_LOGI("MediaPlayerTizen::play() create end %d\n", (int)ret);
        m_playbackState = PLAYBACK_STATE_NONE;
        m_needsPlayAfterPrepare = true;
        appendToOperationQueueInContainer(
            new MediaOperationQueueDataRequestPrepare(m_container,
                                                      m_currentURL));
    } else {
        startPlaying();
    }
}

void MediaPlayerTizen::pause()
{
    if (m_inPlaying) {
        m_inPlaying = false;
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);
        m_container->window()->clearInterval(m_currentTimeUpdateTimer);
        player_pause(m_nativePlayer);
        m_currentTimeUpdateTimer = SIZE_MAX;
    }
}

void MediaPlayerTizen::initDisplay()
{
    m_canvasSurface =
        CanvasSurface::create(m_container->starFish()->platformWindow(), 1, 1);
    m_canvasSurface->clear();
}

void MediaPlayerTizen::setNativePlayerDefaultOptions(ResourceURL* url)
{
    if (m_container->isHTMLVideoElement()) {
        player_display_h displayHandle = GET_DISPLAY(m_canvasSurface->unwrap());
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_EVAS,
                           displayHandle);
        player_set_display_mode(m_nativePlayer,
                                PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER);
    } else {
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_NONE, nullptr);
    }
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

void MediaPlayerTizen::endOfStream()
{
    PLAYER_LOGI("MediaPlayerTizen::endOfStream\n");
    if (!m_nativePlayer) {
        return;
    }
    mediaEndOperation();
    stopPlaying();
    m_playbackState = MediaPlayer::PLAYBACK_STATE_END;
    handleEnded();
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
    m_completeCallback = [](void* data) {
        MediaPlayerTizen* player = (MediaPlayerTizen*)data;
        player->m_container->starFish()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                player->m_container->document()->browsingContext(),
                [](size_t, void* data) {
                    MediaPlayerTizen* player = (MediaPlayerTizen*)data;
                    player->endOfStream();
                },
                data);
    };
    ret = player_set_completed_cb(m_nativePlayer, m_completeCallback, this);
    STARFISH_ASSERT(ret == 0);
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
    m_preparedCallback = [](void* user_data) {
        PLAYER_LOGI("MediaPlayerTizen::player_prepare_async_cb\n");
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
        self->handlePrepared();
    };
    int nativeResult =
        player_prepare_async(m_nativePlayer, m_preparedCallback, this);
    if (nativeResult != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(
            "MediaPlayerTizen::player_prepare_async return error !!!\n");
        printNativePlayerError(nativeResult);
        closePreparingMode();
        m_foundError = true;
        handlePrepared();
    }
    return;
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
            if (m_canvasSurface) {
                m_canvasSurface->resize(m_videoWidth, m_videoHeight);
            }
        }

        PLAYER_LOGI("MediaPlayerTizen::prepare ok %s %s %d %d\n", videoCodec,
                    audioCodec, (int)m_videoWidth, (int)m_videoHeight);

        free(videoCodec);
        free(audioCodec);

        if (m_needsPlayAfterPrepare) {
            startPlaying();
            m_needsPlayAfterPrepare = false;
        }

        if (!m_activeMediaSource) {
            processNextOperationQueueInContainer();
            m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_METADATA);
        }
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_ENOUGH_DATA);
    }
}

void MediaPlayerTizen::unprepareOperation()
{
    if (m_nativePlayer) {
        PLAYER_LOGI(
            "[TRACE_MSE_GC] MediaPlayerTizen::unprepareOperation (%p)\n", this);
        stopPlaying();

        player_unprepare(m_nativePlayer);
        if (m_container->isHTMLVideoElement() && m_container->frame()) {
            m_container->setNeedsLayout();
        }

        if (m_activeMediaSource) {
            m_activeMediaSource->removeClient(m_mseClient);
            m_activeMediaSource->detach();
            m_activeMediaSource = nullptr;
        }
        if (m_mseClient) {
            m_mseClient = nullptr;
        }
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_NOTHING);
    }
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

void MediaPlayerTizen::drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                                 const LayoutRect& absVideoRect)
{
    canvas->setColor(Unit::Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
    canvas->drawImage(m_canvasSurface,
                      Unit::Rect(videoRect.x(), videoRect.y(),
                                 videoRect.width(), videoRect.height()));
}

void MediaPlayerTizen::prepareMediaSource()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void MediaPlayerTizen::fillBufferWithGuard(StreamType type)
{
    Locker<Mutex> locker(*m_bufferMutex);
    fillBuffer(type);
}

#define DEBUG_STREAMBUFFER_LOG(...)                                    \
    PLAYER_LOGI("[%s] ", type == StreamTypeAudio ? "AUDIO" : "VIDEO"); \
    STARFISH_LOG_INFO(__VA_ARGS__);

void MediaPlayerTizen::handlePlayerBuffer(StreamType type,
                                          uint64_t currentBytes)
{
    // Other thread
    uint64_t lastBytes;
    uint64_t maxSize;
    {
        Locker<Mutex> locker(*m_bufferMutex);
        lastBytes = lastBufferBytes(type);
        if (lastBytes != SIZE_MAX && lastBytes == currentBytes) {
            return;
        }
        updateLastBufferBytes(type, currentBytes);
        maxSize = maxBufferSize(type);
    }
    if (!bufferUnderrunState(type) && currentBytes < lastBytes &&
        currentBytes < (maxSize * 0.1)) {
        DEBUG_STREAMBUFFER_LOG("Player need data (%llu/%llu)\n", currentBytes,
                               maxSize);
        fillBufferWithGuard(type);
    }
}

void MediaPlayerTizen::fillBuffer(StreamType type)
{
    if (!alive() || !streamFormat(type)) {
        return;
    }
    media_format_h format = streamFormat(type);

#define RETURN_WHEN_MEDIA_PACKET_ERROR(...)  \
    if (ret != MEDIA_PACKET_ERROR_NONE) {    \
        DEBUG_STREAMBUFFER_LOG(__VA_ARGS__); \
        printMediaPacketError(ret);          \
        handlePlayerError();                 \
        return;                              \
    }

    SourceBuffer* sb = activeSourceBuffer(type);
    if (!sb) {
        return;
    }
    uint64_t streamIdx = activeStreamIndex(type);
    size_t currentInitIndex = initSegmentIndex(type);

    DEBUG_STREAMBUFFER_LOG("fillBuffer start %llums\n", lastSubmitDTS(type));
#ifdef PLAYER_DEBUG
    uint64_t submitMS = 0;
    size_t submitCount = 0;
    StreamInfo* info = sb->streamInfo(currentInitIndex, streamIdx);
#endif
    size_t submitBytes = 0;
    uint64_t sizeUpTo = maxBufferSize(type) * 0.3;
    while (submitBytes < sizeUpTo) {
        uint64_t lastDTS = lastSubmitDTS(type);
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
                m_isEnded = true;
                media_packet_create(format, NULL, NULL, &mediaPacket);
                media_packet_set_flags(mediaPacket, MEDIA_PACKET_END_OF_STREAM);
                player_push_media_stream(m_nativePlayer, mediaPacket);
                DEBUG_STREAMBUFFER_LOG("fillBuffer detect EOS\n");
                break;
            }
            DEBUG_STREAMBUFFER_LOG("fillBuffer runs into under run state[1]\n");
            updateBufferUnderrunState(type, true);
            break;
        }
        if (packet.first->m_dts > lastDTS &&
            packet.first->m_dts - lastDTS > 500) {
            DEBUG_STREAMBUFFER_LOG(
                "fillBuffer runs into under run state[2] - requested(%lld) but "
                "returned(%lld)\n",
                lastDTS, packet.first->m_dts);
            updateBufferUnderrunState(type, true);
            break;
        }

        if (packet.first->m_hasIdr && submitCount != 0) {
            sb->revertLastCacheIfPossible(streamIdx);
            DEBUG_STREAMBUFFER_LOG("Meet IDR in middle -> stop\n");
            break;
        }

        if (packet.second != currentInitIndex) {
            if (!packet.first->m_hasIdr) {
                updateLastSubmitDTS(type, packet.first->m_dts +
                                              packet.first->m_duration);
                DEBUG_STREAMBUFFER_LOG(
                    "fillBuffer drops non-idr packet (config changed)\n");
                continue;
            } else {
                DEBUG_STREAMBUFFER_LOG(
                    "fillBuffer detect changed config (and will submit packet "
                    "including idr)\n");
                updateStreamInfoWithGuard(type, currentInitIndex,
                                          packet.second);
                updateInitSegmentIndex(type, packet.second);
                currentInitIndex = packet.second;
#ifdef PLAYER_DEBUG
                info = sb->streamInfo(currentInitIndex, streamIdx);
#endif
                sizeUpTo = maxBufferSize(type) * 0.9;
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

        updateBufferUnderrunState(type, false);
        ret = player_push_media_stream(m_nativePlayer, mediaPacket);
        // media_format_unref(format);
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
#ifdef PLAYER_DEBUG
        submitCount++;
        submitMS += packet.first->m_duration;
#endif
        submitBytes += packet.first->m_dataSize;
        updateLastSubmitDTS(type,
                            packet.first->m_dts + packet.first->m_duration);
    }
    DEBUG_STREAMBUFFER_LOG("fillBuffer end %llums (count:%d, size:%d)\n\n",
                           lastSubmitDTS(type), (int)submitCount,
                           (int)submitBytes);

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

media_format_h MediaPlayerTizen::streamFormat(StreamType type)
{
    return type == StreamTypeAudio ? m_audioFormat : m_videoFormat;
}

SourceBuffer* MediaPlayerTizen::activeSourceBuffer(StreamType type)
{
    if (m_activeMediaSource) {
        if (type == StreamTypeAudio) {
            return m_activeMediaSource->activeAudioSourceBuffer();
        } else {
            return m_activeMediaSource->activeVideoSourceBuffer();
        }
    }
    return nullptr;
}

uint64_t MediaPlayerTizen::activeStreamIndex(StreamType type)
{
    STARFISH_ASSERT(m_activeMediaSource);
    if (type == StreamTypeAudio) {
        return m_activeMediaSource->activeAudioStreamIndex();
    }
    STARFISH_ASSERT(type == StreamTypeVideo);
    return m_activeMediaSource->activeVideoStreamIndex();
}

uint64_t MediaPlayerTizen::maxBufferSize(StreamType type)
{
    return type == StreamTypeAudio ? m_audioMaxBufferSize
                                   : m_videoMaxBufferSize;
}

bool MediaPlayerTizen::bufferUnderrunState(StreamType type)
{
    return type == StreamTypeAudio ? m_isAudioBufferUnderrunState
                                   : m_isVideoBufferUnderrunState;
}

void MediaPlayerTizen::updateBufferUnderrunState(StreamType type, bool value)
{
    if (type == StreamTypeAudio) {
        m_isAudioBufferUnderrunState = value;
    } else {
        STARFISH_ASSERT(type == StreamTypeVideo);
        m_isVideoBufferUnderrunState = value;
    }
}

uint64_t MediaPlayerTizen::lastSubmitDTS(StreamType type)
{
    return type == StreamTypeAudio ? m_lastAudioDTS : m_lastVideoDTS;
}

void MediaPlayerTizen::updateLastSubmitDTS(StreamType type, uint64_t value)
{
    if (type == StreamTypeAudio) {
        m_lastAudioDTS = value;
    } else {
        STARFISH_ASSERT(type == StreamTypeVideo);
        m_lastVideoDTS = value;
    }
}

size_t MediaPlayerTizen::initSegmentIndex(StreamType type)
{
    return type == StreamTypeAudio ? m_audioInitSegmentIndex
                                   : m_videoInitSegmentIndex;
}

void MediaPlayerTizen::updateInitSegmentIndex(StreamType type, size_t value)
{
    if (type == StreamTypeAudio) {
        m_audioInitSegmentIndex = value;
    } else {
        STARFISH_ASSERT(type == StreamTypeVideo);
        m_videoInitSegmentIndex = value;
    }
}

uint64_t MediaPlayerTizen::lastBufferBytes(StreamType type)
{
    return type == StreamTypeAudio ? m_audioLastBufferBytes
                                   : m_videoLastBufferBytes;
}

void MediaPlayerTizen::updateLastBufferBytes(StreamType type, size_t value)
{
    if (type == StreamTypeAudio) {
        m_audioLastBufferBytes = value;
    } else {
        STARFISH_ASSERT(type == StreamTypeVideo);
        m_videoLastBufferBytes = value;
    }
}

void MediaPlayerTizen::updateStreamInfoWithGuard(StreamType type,
                                                 size_t pastInitIndex,
                                                 size_t newInitIndex)
{
    if (type == StreamTypeVideo) {
        updateVideoStreamInfoWithGuard(pastInitIndex, newInitIndex);
    }
}

void MediaPlayerTizen::updateAudioStreamInfoWithGuard(size_t pastInitIndex,
                                                      size_t newInitIndex)
{
    Locker<Mutex> locker(*m_mediaFormatMutex);
    PLAYER_LOGI("New audio -----------------------------\n");
    PLAYER_LOGI("> index     : %d\n", (int)newInitIndex);
    PLAYER_LOGI("---------------------------------------\n");
}

void MediaPlayerTizen::updateVideoStreamInfoWithGuard(size_t pastInitIndex,
                                                      size_t newInitIndex)
{
    Locker<Mutex> locker(*m_mediaFormatMutex);
    if (!m_videoFormat) {
        return;
    }

    StreamInfo* pastInfo =
        m_activeMediaSource->activeVideoSourceBuffer()->streamInfo(
            pastInitIndex, m_activeMediaSource->activeVideoStreamIndex());
    StreamInfo* newInfo =
        m_activeMediaSource->activeVideoSourceBuffer()->streamInfo(
            newInitIndex, m_activeMediaSource->activeVideoStreamIndex());
    m_videoWidth = newInfo->videoWidth();
    m_videoHeight = newInfo->videoHeight();
    // media_format_set_video_width(m_videoFormat, m_videoWidth);
    // media_format_set_video_height(m_videoFormat, m_videoHeight);
    m_videoMaxBufferSize =
        (m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 / 8 * 5;
    bool isFrameRateChanged = false;
    if (newInfo->videoHasFramerate() && newInfo->videoFramerate().isValid()) {
        if (!pastInfo->videoHasFramerate() ||
            !pastInfo->videoFramerate().isValid() ||
            (pastInfo->videoHasFramerate() &&
             isFramerateChanged(pastInfo->videoFramerate(),
                                newInfo->videoFramerate()))) {
            isFrameRateChanged = true;
        }
    }
    if (isFrameRateChanged) {
        m_videoFormatExtra.is_framerate_changed = isFrameRateChanged;
        m_videoFormatExtra.framerate_num = newInfo->videoFramerate().m_num;
        m_videoFormatExtra.framerate_den = newInfo->videoFramerate().m_den;
        media_format_set_extra(m_videoFormat, &m_videoFormatExtra);
        int ret = player_set_media_stream_info(
            m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO, m_videoFormat);
        if (ret != PLAYER_ERROR_NONE) {
            PLAYER_LOGI("ERROR: player_set_media_stream_info\n");
            printNativePlayerError(ret);
        }
    }
    int ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO, m_videoMaxBufferSize);
    if (ret != PLAYER_ERROR_NONE) {
        PLAYER_LOGI("ERROR: player_set_media_stream_buffer_max_size\n");
        printNativePlayerError(ret);
    }
    PLAYER_LOGI("New video -----------------------------\n");
    PLAYER_LOGI("> index     : %d\n", (int)newInitIndex);
    PLAYER_LOGI("> size      : %lu x %lu\n", m_videoWidth, m_videoHeight);
    PLAYER_LOGI("> max_buffer: %llu\n", m_videoMaxBufferSize);
    PLAYER_LOGI("> framerate_changed: %s\n",
                isFrameRateChanged ? "true" : "false");
    PLAYER_LOGI("> avg_frame_rate: %d/%d\n", newInfo->videoFramerate().m_num,
                newInfo->videoFramerate().m_den);
    PLAYER_LOGI("---------------------------------------\n");
}

#if !defined(STARFISH_TIZEN_TV)
MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizen(element);
}
#endif
}

#endif
#endif /* STARFISH_ENABLE_MULTIMEDIA */
