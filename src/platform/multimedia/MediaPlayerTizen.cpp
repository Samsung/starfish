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
#include "MediaPlayerTizen.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/threading/Thread.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

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
        if (m_player) {
            m_player->prepareMediaSource();
        }
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player && m_player->m_activeMediaSource &&
            m_player->m_activeMediaSource->activeVideoSourceBuffer() == s) {
            m_player->fillVideoBufferIfNeeded();
        }
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player && m_player->m_activeMediaSource &&
            m_player->m_activeMediaSource->activeAudioSourceBuffer() == s) {
            m_player->fillAudioBufferIfNeeded();
        }
    }

    MediaPlayerTizen* m_player;
};

MediaPlayerTizen::MediaPlayerTizen(HTMLMediaElement* element)
    : MediaPlayer(element)
    , m_inPrepare(false)
    , m_alive(true)
    , m_isVideoBufferUnderrunState(false)
    , m_isAudioBufferUnderrunState(false)
    , m_needsPlayAfterPrepare(false)
    , m_isEnded(false)
    , m_seekingTimer(SIZE_MAX)
    , m_mseClient(nullptr)
    , m_videoBufferMutex(new Mutex())
    , m_audioBufferMutex(new Mutex())
    , m_preparedCallback(nullptr)
    , m_completeCallback(nullptr)
    , m_canvasSurface(nullptr)
{
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

void MediaPlayerTizen::handlePlayerError(int error)
{
    if (!isMainThread()) {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data, void* data1) {
                MediaPlayerTizen* player = (MediaPlayerTizen*)data;
                int errorCode = (int)data1;
                player->handlePlayerError(errorCode);
            },
            this, (void*)error);
        return;
    }
    printNativePlayerError(error);
    if (m_inPrepare) {
        closePreparingMode();
        if (m_alive) {
            m_container->giveupFetchingResource();
        } else {
            close();
        }
    }
}

void MediaPlayerTizen::fillVideoBufferIfNeeded()
{
    Locker<Mutex> locker(*m_videoBufferMutex);
    if (m_isVideoBufferUnderrunState) {
        fillVideoBuffer(false);
    }
}

void MediaPlayerTizen::fillAudioBufferIfNeeded()
{
    Locker<Mutex> locker(*m_audioBufferMutex);
    if (m_isAudioBufferUnderrunState) {
        fillAudioBuffer(false);
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
        PLAYER_LOGI("MediaPlayerTizenTV::seek() reaches EOS\n");
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
            PLAYER_LOGI("MediaPlayerTizenTV::seek() : timeout\n");
            self->handleSeekTimeout();
        },
        MAX_WAITING_SECONDS_FOR_SEEK_OPERATION, this);

    seekOperation((int)(time * 1000.0));
}

void MediaPlayerTizen::seekOperation(int timeInMS)
{
    // TODO
    handleSeeked();
}

void MediaPlayerTizen::handleSeeked()
{
    if (isMainThread()) {
        PLAYER_LOGI("MediaPlayerTizen::handleSeeked\n");
        if (m_seekState == SEEKSTATE_NO_SEEK) {
            return;
        }

        // Notify "Seeked" to its container
        if (m_container) {
            m_container->mediaPlayerNotifySeekedItsContainer(currentTime());
        }

        // Remove timeout timer
        if (m_seekingTimer != SIZE_MAX) {
            m_container->window()->clearTimeout(m_seekingTimer);
            m_seekingTimer = SIZE_MAX;
        }
        // Remove rooted pointer
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);

        if (!m_alive) {
            close();
            return;
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

void MediaPlayerTizen::handleSeekFail()
{
    // TODO
}

void MediaPlayerTizen::handleSeekTimeout()
{
    if (isMainThread()) {
        PLAYER_LOGI("MediaPlayerTizen::handleSeekTimeout\n");
        if (m_seekState == SEEKSTATE_NO_SEEK) {
            return;
        }
        if (m_seekingTimer != SIZE_MAX) {
            m_container->window()->clearTimeout(m_seekingTimer);
            m_seekingTimer = SIZE_MAX;
        }
        m_seekState = SEEKSTATE_NO_SEEK;
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);
        if (m_container) {
            m_container->mediaPlayerNotifySeekFailureItsContainer();
        }
        // TODO
        close();
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
        if (m_nativePlayer) {
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
                        self->m_isEnded) {
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
    m_alive = false;
    if (m_inPrepare || m_seekState != SEEKSTATE_NO_SEEK) {
        return;
    }

    if (m_seekingTimer != SIZE_MAX) {
        m_container->window()->clearTimeout(m_seekingTimer);
        m_seekingTimer = SIZE_MAX;
    }

    stopPlaying();

    unprepareOperation();
    if (m_nativePlayer) {
        player_destroy(m_nativePlayer);
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
    if (m_container->isHTMLVideoElement() && m_container->frame()) {
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
    int ret;
    ret = player_set_error_cb(
        m_nativePlayer,
        [](int errorCode, void* data) {
            PLAYER_LOGI("MediaPlayerTizen::player_error_cb\n");
            MediaPlayerTizen* player = (MediaPlayerTizen*)data;
            player->handlePlayerError(errorCode);
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
        player_set_uri(m_nativePlayer,
                       url->urlString()->toUTF8NonGCString().data());
    }

    openPreparingMode();
    m_preparedCallback = [](void* user_data) {
        PLAYER_LOGI("MediaPlayerTizen::player_prepare_async_cb\n");
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
        self->completePrepare();
    };
    int nativeResult =
        player_prepare_async(m_nativePlayer, m_preparedCallback, this);

    if (nativeResult != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(
            "MediaPlayerTizen::player_prepare_async return error !!!\n");
        printNativePlayerError(nativeResult);
        STARFISH_ASSERT_NOT_REACHED();

        STARFISH_ASSERT(m_inPrepare);
        closePreparingMode();
        processNextOperationQueueInContainer();
    }
    return;
}

void MediaPlayerTizen::completePrepare()
{
    if (!isMainThread()) {
        PLAYER_LOGI("MediaPlayerTizen::completePrepare in non-MainThread\n");
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* user_data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
                self->completePrepare();
            },
            this);
    } else {
        PLAYER_LOGI("MediaPlayerTizen::completePrepare in MainThread\n");

        STARFISH_ASSERT(m_inPrepare);
        closePreparingMode();

        if (!m_alive) {
            close(); // unprepare() and destroy() to free data
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
            STARFISH_LOG_ERROR("**ERROR: player_set_mute %x -> ", ret);
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

void MediaPlayerTizen::fillVideoBuffer(bool useLock)
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void MediaPlayerTizen::fillAudioBuffer(bool useLock)
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
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
