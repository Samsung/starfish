/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#ifdef STARFISH_TIZEN

#include "StarFishConfig.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "MediaPlayerTizen.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/canvas/Canvas.h"
#include "platform/threading/Thread.h"
#include "extra/MediaSource.h"
#include "extra/SourceBuffer.h"
#include "extra/Blob.h"
#include "extra/TimeRanges.h"

#define PLAYER_DEBUG
#ifdef PLAYER_DEBUG
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define PLAYER_LOGI(...) \
    STARFISH_LOG_INFO("[MediaPlayer|%ld] ", syscall(SYS_gettid)); \
    STARFISH_LOG_INFO(__VA_ARGS__); \
    STARFISH_LOG_INFO("\n");
#define PLAYER_LOGE(...) \
    STARFISH_LOG_ERROR("[MediaPlayer|Error|%ld] ", syscall(SYS_gettid)); \
    STARFISH_LOG_ERROR(__VA_ARGS__); \
    STARFISH_LOG_ERROR("\n");
#else
#define PLAYER_LOGI(...)
#define PLAYER_LOGE(...)
#endif


namespace StarFish {

void MediaPlayerTizen::printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum) \
    case errorenum: \
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
    MediaPlayerTizenMediaSourceClient(MediaSource* ms, MediaPlayerTizen* player)
        : MediaSourceClient(ms)
        , m_player(player)
    {

    }

    virtual void activeSourceComputed()
    {
        m_player->prepareMediaSource();
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player->m_activeMediaSource->activeVideoSourceBuffer() == s) {
            Locker<Mutex> locker(*m_player->m_videoBufferMutex);
            if (m_player->m_isVideoBufferUnderrunState) {
                m_player->fillVideoBuffer(false);
            }
        }
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player->m_activeMediaSource->activeAudioSourceBuffer() == s) {
            Locker<Mutex> locker(*m_player->m_audioBufferMutex);
            if (m_player->m_isAudioBufferUnderrunState) {
                m_player->fillAudioBuffer(false);
            }
        }
    }

    MediaPlayerTizen* m_player;
};

MediaPlayerTizen::MediaPlayerTizen(HTMLMediaElement* element)
    : MediaPlayer(element)
    , m_inPrepare(false)
    , m_inPlaying(false)
    , m_alive(true)
    , m_isVideoBufferUnderrunState(false)
    , m_isAudioBufferUnderrunState(false)
    , m_needsPlayAfterPrepare(false)
    , m_activeMediaSource(nullptr)
    , m_mseClient(nullptr)
    , m_videoBufferMutex(new Mutex())
    , m_audioBufferMutex(new Mutex())
    , m_preparedCallback(nullptr)
    , m_canvasSurface(nullptr)
{
    player_create(&m_nativePlayer);
    initDisplay();

    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        MediaPlayerTizen* player = (MediaPlayerTizen*)obj;
        player->unprepareOperation();
    }, NULL, NULL, NULL);
}

void MediaPlayerTizen::handlePlayerError(int error)
{
    if (!isMainThread()) {
        m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data, void* data1) {
            MediaPlayerTizen* player = (MediaPlayerTizen*)data;
            int errorCode = (int) data1;
            player->handlePlayerError(errorCode);
        }, this, (void*)error);
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

void MediaPlayerTizen::seekIfNeeded()
{
    if (!m_activeMediaSource)
        return;
    double seekTime = m_activeMediaSource->attachedMediaElement()->defaultPlaybackStartPosition();
    if (seekTime > 0) {
        seek(seekTime);
        m_activeMediaSource->attachedMediaElement()->setDefaultPlaybackStartPosition(0);
    }
}

void MediaPlayerTizen::close()
{
    m_alive = false;
    if (m_inPrepare)
        return;

    unprepareOperation();
    if (m_nativePlayer)
        player_destroy(m_nativePlayer);

    m_nativePlayer = nullptr;
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
    STARFISH_LOG_INFO("MediaPlayerTizen::play() state : %d state2: %d ms: %p\n", (int)state, (int)m_playbackState, m_activeMediaSource);
    if (m_activeMediaSource && m_playbackState == PlaybackState::PLAYBACK_STATE_END) {
        STARFISH_LOG_INFO("MediaPlayerTizen::play() meets mse && playback end\n");
        int ret;
        unprepareOperation();
        STARFISH_LOG_INFO("MediaPlayerTizen::play() unprepare end %d\n", (int)ret);
        ret = player_destroy(m_nativePlayer);
        STARFISH_LOG_INFO("MediaPlayerTizen::play() destory end %d\n", (int)ret);
        ret = player_create(&m_nativePlayer);
        STARFISH_LOG_INFO("MediaPlayerTizen::play() create end %d\n", (int)ret);
        m_playbackState = PLAYBACK_STATE_NONE;
        m_needsPlayAfterPrepare = true;
        m_container->addOperation(new MediaOperationQueueDataRequestPrepare(m_container, m_currentURL));
    } else {
        player_start(m_nativePlayer);
        seekIfNeeded();
    }
}

void MediaPlayerTizen::initDisplay()
{
    m_canvasSurface = CanvasSurface::create(m_container->document()->window(), 1, 1);
    m_canvasSurface->clear();
}

void MediaPlayerTizen::setNativePlayerDefaultOptions(URL* url)
{
    player_display_h displayHandle = GET_DISPLAY(m_canvasSurface->unwrap());
    player_display_type_e displayType = PLAYER_DISPLAY_TYPE_EVAS;
    player_display_mode_e displayMode = PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER;

    player_set_display(m_nativePlayer, displayType, displayHandle);
    player_set_display_mode(m_nativePlayer, displayMode);
}

void MediaPlayerTizen::openPreparingMode()
{
    STARFISH_ASSERT(!m_inPrepare);
    m_inPrepare = true;
    m_starFish->addPointerInRootSet(this);
}

void MediaPlayerTizen::closePreparingMode()
{
    if (m_inPrepare) {
        m_starFish->removePointerFromRootSet(this);
        m_inPrepare = false;
    }
}

void MediaPlayerTizen::endOfStream()
{
    mediaEndOperation();
    m_starFish->removePointerFromRootSet(this);
    m_playbackState = MediaPlayer::PLAYBACK_STATE_END;
    if (m_container) {
        m_container->dispatchPauseEventNow();
        m_container->dispatchEndedEventNow();
    }
}

void MediaPlayerTizen::prepare(URL* url)
{
    m_currentURL = url;
    STARFISH_LOG_INFO("MediaPlayerTizen::prepare\n");
    player_set_volume(m_nativePlayer, 1, 1);
    player_set_mute(m_nativePlayer, false);
    int ret;
    ret = player_set_error_cb(m_nativePlayer, [](int errorCode, void* data) {
        PLAYER_LOGI("player_error_cb");
        MediaPlayerTizen* player = (MediaPlayerTizen*)data;
        player->handlePlayerError(errorCode);
    }, this);
    STARFISH_ASSERT(ret == 0);
    ret = player_set_completed_cb(m_nativePlayer, [](void* data) {
        PLAYER_LOGI("player_completed_cb");
        MediaPlayerTizen* player = (MediaPlayerTizen*)data;
        player->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
            MediaPlayerTizen* player = (MediaPlayerTizen*)data;
            player->endOfStream();
        }, data);
    }, this);
    STARFISH_ASSERT(ret == 0);
    ret = player_set_buffering_cb(m_nativePlayer, [](int percent, void* data) {
        STARFISH_LOG_INFO("MediaPlayerTizen -> buffering state... %d\n", percent);
    }, this);
    setNativePlayerDefaultOptions(url);
    if (url->isBlobURL()) {
        BlobURLStore store;
        if (!StarFish::stringToBlobURLString(url->urlString(), store)) {
            PLAYER_LOGE("MediaPlayerTizen::prepare, seturl, FAIL - INVALID BLOB URL\n");
            return;
        }
        if (m_starFish->isValidBlobURL(store)) {
            player_set_memory_buffer(m_nativePlayer, ((Blob *)store.m_blob)->data(), ((Blob *)store.m_blob)->size());
        } else if (m_starFish->isValidMediaSourceBlobURL(store)) {
            openPreparingMode();

            BlobURLStore store;
            StarFish::stringToBlobURLString(url->urlString(), store);
            MediaSource* ms = (MediaSource*)store.m_blob;
            m_activeMediaSource = ms;
            m_mseClient = new MediaPlayerTizenMediaSourceClient(m_activeMediaSource, this);
            m_activeMediaSource->addClient(m_mseClient);
            m_activeMediaSource->attach(m_container);
            return;
        } else {
            // fire eror
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        player_set_uri(m_nativePlayer, url->urlString()->utf8Data());
    }

    openPreparingMode();
    m_preparedCallback = [](void *user_data)
    {
        PLAYER_LOGI("player_prepare_async_cb");
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
        self->compleatePrepare();
    };
    int nativeResult = player_prepare_async(m_nativePlayer, m_preparedCallback, this);

    if (nativeResult != PLAYER_ERROR_NONE) {
        PLAYER_LOGE("player_prepare_async return error !!!");
        printNativePlayerError(nativeResult);
        STARFISH_ASSERT_NOT_REACHED();

        STARFISH_ASSERT(m_inPrepare);
        closePreparingMode();
    }

    return;
}

void MediaPlayerTizen::compleatePrepare()
{
    STARFISH_ASSERT(!isMainThread());
    m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* user_data) {
        PLAYER_LOGI("MediaPlayerTizen::compleatePrepare in MainThread");
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;

        STARFISH_ASSERT(self->m_inPrepare);
        self->closePreparingMode();

        if (!self->m_alive) {
            self->close(); // unprepare() and destroy() to free data
            return;
        }

        char* videoCodec = nullptr;
        char* audioCodec = nullptr;
        player_get_codec_info(self->m_nativePlayer, &audioCodec, &videoCodec);

        if (videoCodec) {
            self->m_hasVideo = true;
            int width = 1;
            int height = 1;
            player_get_video_size(self->m_nativePlayer, &width, &height);
            STARFISH_ASSERT(width > 0);
            STARFISH_ASSERT(height > 0);
            self->m_videoWidth = (unsigned long)width;
            self->m_videoHeight = (unsigned long)height;
            if (self->m_canvasSurface) {
                self->m_canvasSurface->resize(self->m_videoWidth, self->m_videoHeight);
            }
        }

        PLAYER_LOGI("MediaPlayerTizen::prepare ok %s %s %d %d\n", videoCodec, audioCodec, (int)self->m_videoWidth, (int)self->m_videoHeight);

        free(videoCodec);
        free(audioCodec);

        if (self->m_needsPlayAfterPrepare) {
            player_start(self->m_nativePlayer);
            self->seekIfNeeded();
            self->m_needsPlayAfterPrepare = false;
        }

        self->processNextOperationQueueInContainer();

        self->m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(HTMLMediaElement::HAVE_METADATA);
        self->m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(HTMLMediaElement::HAVE_FUTURE_DATA);
    }, this);
}

void MediaPlayerTizen::pauseOperation()
{
    m_starFish->removePointerFromRootSet(this);
    player_pause(m_nativePlayer);
    if (m_container) {
        m_container->dispatchPauseEvent();
    }
}

void MediaPlayerTizen::unprepareOperation()
{
    STARFISH_LOG_INFO("MediaPlayerTizen::unprepareOperation\n");
    if (m_nativePlayer) {
        player_unprepare(m_nativePlayer);
        if (m_container->isHTMLVideoElement() && m_container->frame()) {
            m_container->setNeedsLayout();
        }

        if (m_activeMediaSource) {
            m_activeMediaSource->removeClient(m_mseClient);
            m_mseClient = nullptr;
            m_activeMediaSource->detach();
        }
        m_activeMediaSource = nullptr;

        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(HTMLMediaElement::HAVE_NOTHING);
    }
}

void MediaPlayerTizen::drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
{
    canvas->setColor(Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
    canvas->drawImage(m_canvasSurface, Rect(videoRect.x(), videoRect.y(), videoRect.width(), videoRect.height()));
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

TimeRanges* MediaPlayerTizen::buffered()
{
    if (m_activeMediaSource) {
        // CASE : MediaSource
        SourceBufferList* bufferList = m_activeMediaSource->activeSourceBuffers();
        unsigned nbuffer = bufferList->length();

        if (nbuffer == 0)
            return new TimeRanges();

        if (nbuffer == 1)
            return bufferList->at(0)->buffered();

        TimeRanges* result = bufferList->at(0)->buffered();
        for (unsigned i = 1; i < nbuffer; i++) {
            TimeRanges* buffered = bufferList->at(i)->buffered();
            unsigned bufferedSize = buffered->length();
            unsigned resultSize = result->length();
            TimeRanges* newResult = new TimeRanges();

            unsigned t = 0, j = 0;
            while (t != bufferedSize && j != resultSize) {
                if (buffered->end(t) < result->start(j)) {
                    t++;
                } else if (buffered->start(t) > result->end(j)) {
                    j++;
                } else {
                    newResult->push_back(std::max(buffered->start(t), result->start(j)), std::min(buffered->end(t), result->end(j)));
                    if (buffered->start(t) >= result->start(j) && buffered->end(t) <= result->end(j)) {
                        t++;
                    } else if (buffered->start(t) <= result->start(j) && buffered->end(t) >= result->end(j)) {
                        j++;
                    } else if (buffered->start(t) < result->start(j)) {
                        t++;
                    } else {
                        j++;
                    }
                }
            }
            result = newResult;
            // delete prev result?
        }
        return result;

    } else {
        // CASE : load from URL
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return nullptr;
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
