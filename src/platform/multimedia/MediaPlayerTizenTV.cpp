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
#ifdef STARFISH_TIZEN_TV
#include "StarFishConfig.h"
#include "MediaPlayer.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/canvas/Canvas.h"
#include "platform/threading/Thread.h"
#include "extra/MediaSource.h"

#include <player_product.h>

#include <media/player.h>
#include <Elementary.h>

namespace StarFish {

static void mediaPlayerErrorCallback(int errorCode, void *user_data)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum) \
    case errorenum: \
        STARFISH_LOG_INFO("mediaPlayerErrorCallback() : %s\n", #errorenum); \
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
        STARFISH_LOG_INFO("mediaPlayerErrorCallback() : Unknown error\n");
        return;
    }
}

class MediaPlayerTizenTV : public MediaPlayer {
public:
    MediaPlayerTizenTV(HTMLMediaElement* element)
        : MediaPlayer(element)
        , m_isURISetted(false)
    {
        player_create(&m_nativePlayer);
        player_set_error_cb(m_nativePlayer, mediaPlayerErrorCallback, this);
        player_set_completed_cb(m_nativePlayer, [](void* data) {
            MediaPlayerTizenTV* player = (MediaPlayerTizenTV*)data;
            player->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
                MediaPlayerTizenTV* player = (MediaPlayerTizenTV*)data;
                player->m_starFish->removePointerFromRootSet(player);
                player_stop(player->m_nativePlayer);
            }, data);
        }, this);

        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            MediaPlayerTizenTV* player = (MediaPlayerTizenTV*)obj;
            player_destroy(player->m_nativePlayer);
        }, NULL, NULL, NULL);
    }

    virtual void processOperationQueue(MediaPlayerOperationQueueData* data)
    {
        auto type = data->eventType();
        STARFISH_LOG_INFO("MediaPlayerTizenTV::processOperationQueue %d\n", (int)type);
        player_state_e state;
        player_get_state(m_nativePlayer, &state);
        if (type == MediaPlayerOperationQueueData::SetURLEventType) {
            URL* url = ((MediaPlayerOperationQueueDataSetURL*)data)->m_url;
            if (state == PLAYER_STATE_PLAYING) {
                pauseOperation();
            }

            if (state != PLAYER_STATE_IDLE) {
                player_unprepare(m_nativePlayer);
                if (m_container->isHTMLVideoElement() && m_container->frame()) {
                    m_container->setNeedsLayout();
                }
            }

            if (m_container->isHTMLVideoElement() && m_container->frame()) {
                player_display_h displayHandle = GET_DISPLAY(elm_win_xwindow_get((Evas_Object*) m_container->document()->window()->unwrap()));
                player_display_type_e displayType = PLAYER_DISPLAY_TYPE_X11;
                player_display_mode_e displayMode = PLAYER_DISPLAY_MODE_DST_ROI;
                player_display_roi_mode_e roiMode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;

                player_set_display(m_nativePlayer, displayType, displayHandle);
                player_set_display_mode(m_nativePlayer, displayMode);
                player_set_x11_display_roi_mode(m_nativePlayer, roiMode);
                player_display_video_at_paused_state(m_nativePlayer, TRUE);

                if (url->isNetworkURL())
                    player_set_streaming_type(m_nativePlayer, const_cast<char*>("FFMPEG_HTTP"));
            }

            m_hasVideo = false;
            if (url) {
                player_set_uri(m_nativePlayer, url->urlString()->utf8Data());
                m_isURISetted = true;
            } else {
                m_isURISetted = false;
            }
            processNextOperationQueue();
        } else if (type == MediaPlayerOperationQueueData::RequestPrepareEventType) {
            if (state == PLAYER_STATE_IDLE) {
                if (m_isURISetted) {
                    m_starFish->addPointerInRootSet(this);
                    player_prepare_async(m_nativePlayer, [](void *user_data) {
                        MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
                        STARFISH_ASSERT(!isMainThread());
                        self->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* user_data) {
                            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
                            self->m_starFish->removePointerFromRootSet(self);

                            char* videoCodec = nullptr;
                            char* audioCodec = nullptr;
                            player_get_codec_info(self->m_nativePlayer, &videoCodec, &audioCodec);

                            if (videoCodec) {
                                self->m_hasVideo = true;
                                int width = 1;
                                int height = 1;
                                player_get_video_size(self->m_nativePlayer, &width, &height);
                                STARFISH_ASSERT(width > 0);
                                STARFISH_ASSERT(height > 0);
                                self->m_videoWidth = (unsigned long)width;
                                self->m_videoHeight = (unsigned long)height;
                                if (self->m_container->isHTMLVideoElement() && self->m_container->frame()) {
                                    self->m_container->setNeedsLayout();
                                }
                            }

                            STARFISH_LOG_INFO("MediaPlayerTizenTV::processOperationQueue::player_prepare_async ok %s %s %d %d\n", videoCodec, audioCodec, (int)self->m_videoWidth, (int)self->m_videoHeight);

                            free(videoCodec);
                            free(audioCodec);

                            self->processNextOperationQueue();
                        }, user_data);
                    }, this);
                    return;
                } else {
                    // ignore command
                }
            }
            processNextOperationQueue();
        } else if (type == MediaPlayerOperationQueueData::RequestPlayEventType) {
            if (state == PLAYER_STATE_READY || state == PLAYER_STATE_PAUSED) {
                m_starFish->addPointerInRootSet(this);
                player_start(m_nativePlayer);
                STARFISH_LOG_INFO("MediaPlayerTizenTV::processOperationQueue::player_start\n");
            } else if (state == PLAYER_STATE_IDLE) {
                prependToOperationQueue(new MediaPlayerOperationQueueDataRequestPlay(this));
                prependToOperationQueue(new MediaPlayerOperationQueueDataRequestPrepare(this));
            } else if (state == PLAYER_STATE_PLAYING) {
                // ignore command
            } else {
                STARFISH_LOG_INFO("invalid state %d\n", (int)state);
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            processNextOperationQueue();
        } else if (type == MediaPlayerOperationQueueData::RequestPauseEventType) {
            if (state == PLAYER_STATE_PLAYING) {
                pauseOperation();
            }
            processNextOperationQueue();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void pauseOperation()
    {
        m_starFish->removePointerFromRootSet(this);
        player_pause(m_nativePlayer);
    }

    virtual unsigned long videoWidth()
    {
        if (m_hasVideo) {
            return m_videoWidth;
        } else
            return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual unsigned long videoHeight()
    {
        if (m_hasVideo) {
            return m_videoHeight;
        } else
            return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual double currentTime()
    {
        int s;
        int ret = player_get_play_position(m_nativePlayer, &s);
        if (ret)
            return 0;
        return s / 1000.0;
    }

    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
    {
        canvas->punchHole(Rect(videoRect.x(), videoRect.y(), videoRect.width(), videoRect.height()));
        player_set_x11_display_dst_roi(m_nativePlayer, absVideoRect.x(), absVideoRect.y(), absVideoRect.width(), absVideoRect.height());
    }

    bool m_isURISetted;
    player_h m_nativePlayer;
    unsigned long m_videoWidth, m_videoHeight;
};

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}

}

#if 0
#include <limits.h>
#include <stdlib.h>

#include "StarFishConfig.h"
#include "MediaPlayer.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "platform/message_loop/MessageLoop.h"
#include "extra/MediaSource.h"

#include <efl_extension.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <player_product.h>

#define DEFAULT_VIDEO_STREAM_INFO_MIME "video/x-vp9"
#define DEFAULT_VIDEO_STREAM_INFO_WIDTH 512
#define DEFAULT_VIDEO_STREAM_INFO_HEIGHT 288
#define DEFAULT_VIDEO_STREAM_INFO_FRAMERATE_DEN 1
#define DEFAULT_VIDEO_STREAM_INFO_FRAMERATE_NUM 30

#define PLAYER_DEBUG

#ifdef PLAYER_DEBUG
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define PLAYER_LOGI(...) \
    STARFISH_LOG_ERROR("[MediaPlayer|%ld] ", syscall(SYS_gettid)); \
    STARFISH_LOG_ERROR(__VA_ARGS__);

#define PLAYER_LOGE(errorcode, ...) \
    STARFISH_LOG_ERROR("[MediaPlayer|%ld] ERROR(%u)| ", syscall(SYS_gettid), errorcode); \
    STARFISH_LOG_ERROR(__VA_ARGS__);
#else
#define PLAYER_LOGI(...)
#define PLAYER_LOGE(...)
#endif

namespace StarFish {

VideoPlayer::VideoPlayer(HTMLVideoElement* videoElement)
    : MediaPlayer()
    , m_videoElement(videoElement)
    , m_currentMediaSource(nullptr)
    , m_cplayer(NULL)
    , m_lastRequest(VideoPlayer::REQUEST_NONE)
    , m_displayArea(Rect(0, 0, 0, 0))
    , m_isElementPointerLocked(false)
    , m_hasPendingUrl(false)
{
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        PLAYER_LOGI("VideoPlayer::~VideoPlayer\n");
        VideoPlayer* player = (VideoPlayer*)obj;
        STARFISH_ASSERT(!player->isPublicState(MediaPlayer::STATE_PREPARING | MediaPlayer::STATE_PLAYING));
        player->destroyCPlayer();
    }, NULL, NULL, NULL);

    STARFISH_ASSERT(videoElement);
    clearVideoStreamInfo();
    m_currentUrl = nullptr;
}

static void __videoPlayerPrepareCB(void *user_data)
{
    // NOTE : this callback will not be invoked when using unprepare player api
    PLAYER_LOGI("__videoPlayerPrepareCB()\n");
    VideoPlayer* player = (VideoPlayer*)user_data;
    player->videoElement()->document()->window()->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
        ((VideoPlayer*)data)->prepareCBShouldBeExecutedInMainThread();
    }, player);
}

static void __videoPlayerCompleteCB(void *user_data)
{
    PLAYER_LOGI("__videoPlayerCompleteCB()\n");
    VideoPlayer* player = (VideoPlayer*)user_data;
    player->videoElement()->document()->window()->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
        ((VideoPlayer*)data)->playFinishedCBShouldBeExecutedInMainThread();
    }, player);
}

static void __videoPlayerBufferingCB(int percent, void *user_data)
{
    PLAYER_LOGI("__videoPlayerBufferingCB() %d percent\n", percent);
}

static void __videoPlayerBufferNeedVideoDataCB(unsigned int size, void *user_data)
{
    PLAYER_LOGI("__videoPlayerBufferNeedVideoDataCB()\n");
    // TODO : element and MediaPlayer should be rooted
    // TODO : member of MediaPlayer should be locked
    VideoPlayer* player = (VideoPlayer*)user_data;
    if (player->currentMediaSource()) {
        player->onBufferNeedVideoData(player->currentMediaSource());
    }
}

static void __videoPlayerBufferNeedAudioDataCB(unsigned int size, void *user_data)
{
    PLAYER_LOGI("__videoPlayerBufferNeedAudioDataCB()\n");
    // TODO : element and MediaPlayer should be rooted
    // TODO : member of MediaPlayer should be locked
    VideoPlayer* player = (VideoPlayer*)user_data;
    if (player->currentMediaSource()) {
        player->onBufferNeedAudioData(player->currentMediaSource());
    }
}

static void __videoPlayerBufferEnoughDataCB(void *user_data)
{
    PLAYER_LOGI("__videoPlayerBufferEnoughDataCB()\n");
}

static void __videoPlayerErrorCB(int errorCode, void *user_data)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum) \
    case errorenum: \
        PLAYER_LOGI("__videoPlayerErrorCB() : %s\n", #errorenum); \
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
        GEN_ERROR_PRINTS(PLAYER_ERROR_STREAMING_PLAYER)
        GEN_ERROR_PRINTS(PLAYER_ERROR_AUDIO_CODEC_NOT_SUPPORTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_VIDEO_CODEC_NOT_SUPPORTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NO_AUTH)
        GEN_ERROR_PRINTS(PLAYER_ERROR_GENEREIC)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_INFO)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SYNC_PLAY_NETWORK_EXCEPTION)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SYNC_PLAY_SERVER_DOWN)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NOT_SUPPORTED_FORMAT)
        GEN_ERROR_PRINTS(PLAYER_ERROR_PERMISSION_DENIED)
#undef GEN_ERROR_PRINTS
    default:
        PLAYER_LOGI("__videoPlayerErrorCB() : Unknown error\n");
        return;
    }
}

bool VideoPlayer::assureCPlayer()
{
    // If there was an error, destroy cplayer
    if (m_cplayer && isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        destroyCPlayer();
    }
    // Create if cplayer is null
    if (!m_cplayer) {
        setPublicState(MediaPlayer::STATE_NONE);

        int errorCode = player_create(&m_cplayer);
        if (errorCode != PLAYER_ERROR_NONE) {
            PLAYER_LOGE(errorCode, "player_create()\n");
            m_cplayer = NULL;
            return false;
        }
        // Set looping
        setLoop(m_videoElement->loop());

        // Set callbacks
        player_set_completed_cb(m_cplayer, __videoPlayerCompleteCB, (void*)this);
        player_set_error_cb(m_cplayer, __videoPlayerErrorCB, (void*)this);
        player_set_buffering_cb(m_cplayer, __videoPlayerBufferingCB, (void*)this);
        player_set_buffer_need_video_data_cb(m_cplayer, __videoPlayerBufferNeedVideoDataCB, (void*)this);
        player_set_buffer_need_audio_data_cb(m_cplayer, __videoPlayerBufferNeedAudioDataCB, (void*)this);
        player_set_buffer_enough_video_data_cb(m_cplayer, __videoPlayerBufferEnoughDataCB, (void*)this);
    }
    return true;
}

void VideoPlayer::lockElementPointer()
{
    STARFISH_ASSERT(m_videoElement);
    if (!m_isElementPointerLocked) {
        m_videoElement->document()->window()->starFish()->addPointerInRootSet(m_videoElement);
        m_isElementPointerLocked = true;
    }
}

void VideoPlayer::unlockElementPointer()
{
    STARFISH_ASSERT(m_videoElement);
    if (m_isElementPointerLocked) {
        m_videoElement->document()->window()->starFish()->removePointerFromRootSet(m_videoElement);
        m_isElementPointerLocked = false;
    }
}

bool VideoPlayer::hasPendingUrl()
{
    // NOTE: m_inputUrl(new request) can equals to m_currentUrl
    return m_hasPendingUrl;
}

void VideoPlayer::pushPendingUrl()
{
    m_hasPendingUrl = true;
}

void VideoPlayer::popPendingUrl()
{
    STARFISH_ASSERT(m_hasPendingUrl);
    // Only one pending
    m_hasPendingUrl = false;
    m_currentUrl = m_inputUrl;
}

void VideoPlayer::prepareCBShouldBeExecutedInMainThread()
{
    PLAYER_LOGI("prepareCBShouldBeExecutedInMainThread()\n");
    unlockElementPointer();
    // If public state changed during PREPARING, exit.
    if (!isPublicState(MediaPlayer::STATE_PREPARING) || !m_cplayer) {
        PLAYER_LOGI("prepareCBShouldBeExecutedInMainThread() - public state changed during PREPARING, exit.\n");
        return;
    }
    // If cplayer's state is not ready -> load fail
    player_state_e state;
    if (player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE || state != PLAYER_STATE_READY) {
        PLAYER_LOGI("prepareCBShouldBeExecutedInMainThread() - unknown error exit.\n");
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        onPrepared(nullptr);
        return;
    }

    setPublicState(MediaPlayer::STATE_READY);
    // If there was another prepare() request during PREPARING,
    // forget current request and start to load new request.
    if (hasPendingUrl()) {
        PLAYER_LOGI("-- Player has pending request for loading another URL.\n");
        PLAYER_LOGI("-- Unprepare current and then start to prepare new one.\n");
        unprepareCPlayer();
        popPendingUrl();
        prepareCPlayer();
        return;
    }
    onPrepared(m_currentUrl);

    // If there was a play() request during PREPARING, start playing
    if (lastRequestIs(VideoPlayer::REQUEST_PLAY)) {
        PLAYER_LOGI("prepareCBShouldBeExecutedInMainThread() - player has been accepted playing request. start to play!\n");
        playCPlayer();
    }

#if 0 // Debug code
    if (m_cplayer) {
        player_video_track_info info;
        int errorCode = player_get_video_info(m_cplayer, &info);
        if (errorCode != PLAYER_ERROR_NONE) {
            PLAYER_LOGI("player_get_video_info() is failed(%d)\n", errorCode);
        }
        PLAYER_LOGI("video info----------------------------------------\n");
        PLAYER_LOGI("- resolution : %u x %u\n", info.width, info.height);
        PLAYER_LOGI("- bitrate : %u bps\n", info.bitrate);
        PLAYER_LOGI("--------------------------------------------------\n");
    }
#endif
}

void VideoPlayer::playFinishedCBShouldBeExecutedInMainThread()
{
    PLAYER_LOGI("playFinishedCBShouldBeExecutedInMainThread()\n");
    unlockElementPointer();
    // If public state changed during PREPARING, exit.
    if (!isPublicState(MediaPlayer::STATE_PLAYING) || !m_cplayer) {
        return;
    }
    // Normal case,
    onPlayFinished(m_currentUrl);
}

void VideoPlayer::destroyCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    PLAYER_LOGI("destroyCPlayer()\n");
    player_stop(m_cplayer);
    player_unprepare(m_cplayer);
    onUnprepared();

    player_destroy(m_cplayer);
    m_currentUrl = nullptr;
    m_cplayer = NULL;
    clearVideoStreamInfo();

    // Set public state
    setPublicState(MediaPlayer::STATE_NONE);
    unlockElementPointer();
}

void VideoPlayer::prepareCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    if (!m_currentUrl) {
        return;
    }
    PLAYER_LOGI("prepareCPlayer()\n");

    // Set display options (should reset after unprepare)
    Evas_Object* win = (Evas_Object*) m_videoElement->document()->window()->unwrap();
    player_display_h display_handle = GET_DISPLAY(elm_win_xwindow_get(win));
    player_display_type_e display_type = PLAYER_DISPLAY_TYPE_X11;
    player_display_mode_e display_mode = PLAYER_DISPLAY_MODE_DST_ROI;
    player_display_roi_mode_e roi_mode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;

    player_set_display(m_cplayer, (player_display_type_e) display_type, display_handle);
    player_set_display_mode(m_cplayer, display_mode);
    player_set_x11_display_roi_mode(m_cplayer, roi_mode);
    player_display_video_at_paused_state(m_cplayer, TRUE);

    if (m_currentUrl->isFileURL() || m_currentUrl->isNetworkURL()) {
        PLAYER_LOGI("prepare() for non-blob resource\n");
        PLAYER_LOGI("url : %s\n", m_currentUrl->urlString()->utf8Data());
        player_set_uri(m_cplayer, const_cast<char*>(m_currentUrl->urlString()->utf8Data()));

        // FIXME
        if (m_currentUrl->isNetworkURL())
            player_set_streaming_type(m_cplayer, const_cast<char*>("FFMPEG_HTTP"));
        startLoadingCPlayer();

    } else if (m_currentUrl->isBlobURL()) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        STARFISH_ASSERT(m_videoElement);
        PLAYER_LOGI("prepare() for blob resource\n");
        // Validate Blob url
        BlobURLStore store;
        if (!StarFish::stringToBlobURLString(m_currentUrl->urlString(), store)) {
            PLAYER_LOGI("prepare() FAIL - INVALID BLOB URL\n");
            return;
        }
        if (m_videoElement->document()->window()->starFish()->isValidMediaSourceBlobURL(store)) {
            MediaSource* mediaSource = (MediaSource*)store.m_blob;
            m_currentMediaSource = mediaSource;
            mediaSource->registerMediaPlayer(this);
            player_set_uri(m_cplayer, "external_demuxer://aaaa");
            player_set_video_stream_info(m_cplayer, &m_videoInfo);
            setPublicState(MediaPlayer::STATE_WAITING_FOR_MEDIASOURCE_READY);
            PLAYER_LOGI("prepare() is waiting ready signal from MediaSource\n");
        } else if (m_videoElement->document()->window()->starFish()->isValidBlobURL(store)) {
            PLAYER_LOGI("prepare() FAIL - blob type of non-MediaSource NOT SUPPORTED yet\n");
            return;
        }

    } else {
        return;
    }


}

void VideoPlayer::startLoadingCPlayer()
{
    PLAYER_LOGI("prepare() - startLoadingCPlayer()\n");
    setPublicState(MediaPlayer::STATE_PREPARING);
    lockElementPointer();
    int errorCode = player_prepare_async(m_cplayer, __videoPlayerPrepareCB, (void*)this);
    if (errorCode != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(errorCode, "player_prepare_async()\n");
        unlockElementPointer();
        setPublicState(MediaPlayer::STATE_NONE);
        return;
    }
}

void VideoPlayer::unprepareCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    PLAYER_LOGI("unprepareCPlayer()\n");
    player_unprepare(m_cplayer);
    // If cplayer's state is not [IDLE] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE || state != PLAYER_STATE_IDLE)) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    setPublicState(MediaPlayer::STATE_NONE);
    unlockElementPointer();
    onUnprepared();
    m_currentUrl = nullptr;
}

void VideoPlayer::playCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    PLAYER_LOGI("playCPlayer()\n");
    // DEBUG
    player_set_x11_display_dst_roi(m_cplayer, m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());
    PLAYER_LOGI("video display area : %f %f %f %f\n", m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());

    if (player_start(m_cplayer) != PLAYER_ERROR_NONE) {
        // NOTE: wrong url can cause this
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // If cplayer's state is not [IDLE|PLAYING] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE
        || !(state == PLAYER_STATE_IDLE || state == PLAYER_STATE_PLAYING))) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    if (state == PLAYER_STATE_IDLE) {
        PLAYER_LOGI("nothing to play\n");
        setPublicState(MediaPlayer::STATE_NONE);
    } else {
        PLAYER_LOGI("play start!!!\n");
        STARFISH_ASSERT(state == PLAYER_STATE_PLAYING);
        setPublicState(MediaPlayer::STATE_PLAYING);
        lockElementPointer();
    }
}

void VideoPlayer::pauseCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    PLAYER_LOGI("pauseCPlayer()\n");
    player_pause(m_cplayer);
    // TODO : confirm state
    // If cplayer's state is not [IDLE|PAUSED] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE
        || !(state == PLAYER_STATE_IDLE || state == PLAYER_STATE_PAUSED))) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    if (state == PLAYER_STATE_IDLE) {
        setPublicState(MediaPlayer::STATE_NONE);
    } else {
        STARFISH_ASSERT(state == PLAYER_STATE_PAUSED);
        setPublicState(MediaPlayer::STATE_PAUSED);
    }
    unlockElementPointer();
}

void VideoPlayer::stopCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    PLAYER_LOGI("stopCPlayer()\n");
    player_stop(m_cplayer);
    // If cplayer's state is not [IDLE|READY] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE
        || !(state == PLAYER_STATE_IDLE || state == PLAYER_STATE_READY))) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    if (state == PLAYER_STATE_IDLE) {
        setPublicState(MediaPlayer::STATE_NONE);
    } else {
        STARFISH_ASSERT(state == PLAYER_STATE_READY);
        setPublicState(MediaPlayer::STATE_READY);
    }
    unlockElementPointer();
}

void VideoPlayer::prepare()
{
    PLAYER_LOGI("prepare()---------------------------------------\n");
    m_lastRequest = VideoPlayer::REQUEST_NONE;
    if (!assureCPlayer()) {
        PLAYER_LOGI("prepare() FAIL : could not create player\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_PREPARING)) {
        PLAYER_LOGI("prepare() WAIT : player is processing prevous prepare() request.\n");
        pushPendingUrl();
        return;
    }
    if (isPublicState(MediaPlayer::STATE_PLAYING | MediaPlayer::STATE_PAUSED)) {
        stopCPlayer();
    }
    if (isPublicState(MediaPlayer::STATE_READY)) {
        unprepareCPlayer();
    }
    if (isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        PLAYER_LOGI("prepare() FAIL : unknown error\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_WAITING_FOR_MEDIASOURCE_READY)) {
        PLAYER_LOGI("prepare() : player was waiting MediaSource to be ready, but has gotten new prepare request.\n");
    }
    m_currentUrl = m_inputUrl;
    prepareCPlayer();
}

void VideoPlayer::pushVideoPacket(uint8_t *buf, uint32_t len, uint64_t pts)
{
    if (!m_cplayer) {
        PLAYER_LOGI("pushVideoPacket() FAIL : player does not exist (something wrong)\n");
        return;
    }
    int ret = player_submit_packet(m_cplayer, buf, len, pts, PLAYER_TRACK_TYPE_VIDEO);
    if (ret != PLAYER_ERROR_NONE) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        PLAYER_LOGI("pushVideoPacket() FAIL : unknown error\n");
        return;
    }
    PLAYER_LOGI("pushVideoPacket() len(%u) pts(%llu)\n", len, pts);
}

void VideoPlayer::pushAudioPacket(uint8_t *buf, uint32_t len, uint64_t pts)
{
    if (!m_cplayer) {
        PLAYER_LOGI("pushAudioPacket() FAIL : player does not exist (something wrong)\n");
        return;
    }
    int ret = player_submit_packet(m_cplayer, buf, len, pts, PLAYER_TRACK_TYPE_AUDIO);
    if (ret != PLAYER_ERROR_NONE) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        PLAYER_LOGI("pushAudioPacket() FAIL : unknown error\n");
        return;
    }
    // PLAYER_LOGI("pushAudioPacket() len(%u) pts(%llu)\n", len, pts);
}


void VideoPlayer::setDisplayArea(int x, int y, int width, int height)
{
    m_displayArea = Rect(x, y, width, height);
    if (m_cplayer && isPublicState(MediaPlayer::STATE_PLAYING)) {
        player_set_x11_display_dst_roi(m_cplayer, m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());
    }
}

void VideoPlayer::notifyInitialPacketReady(MediaSource* ms)
{
    PLAYER_LOGI("notifyInitialPacketReady()\n");
    if (!(currentURL() && currentURL()->isBlobURL()))
        return;
    if (!isPublicState(MediaPlayer::STATE_WAITING_FOR_MEDIASOURCE_READY))
        return;
    STARFISH_ASSERT(ms);
    BlobURLStore store;
    if (!StarFish::stringToBlobURLString(m_currentUrl->urlString(), store)) {
        PLAYER_LOGI("notifyInitialPacketReady() - Unknown error\n");
        STARFISH_ASSERT(false);
        return;
    }
    if (((MediaSource*)store.m_blob) != ms)
        return;
    PLAYER_LOGI("notifyInitialPacketReady() - media source ready, now start to prepare\n");
    startLoadingCPlayer();
}

void VideoPlayer::play()
{
    PLAYER_LOGI("play()------------------------------------------\n");
    m_lastRequest = VideoPlayer::REQUEST_PLAY;
    if (!assureCPlayer()) {
        PLAYER_LOGI("play() FAIL : could not create player\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_PREPARING | MediaPlayer::STATE_PLAYING)) {
        PLAYER_LOGI("play() IGNORED (already playing) or WAIT (preparing) \n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        PLAYER_LOGI("play() FAIL : unknown error\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_NONE)) {
        if (m_inputUrl) {
            PLAYER_LOGI("play() FAIL : prepare() first\n");
        } else {
            PLAYER_LOGI("play() FAIL : setURL() first\n");
        }
        return;
    }
    if (isPublicState(MediaPlayer::STATE_WAITING_FOR_MEDIASOURCE_READY)) {
        PLAYER_LOGI("play() WAIT : current processing url is Blob, and player is waiting initialPacket be ready\n");
        return;
    }
    playCPlayer();
}

void VideoPlayer::pause()
{
    PLAYER_LOGI("pause()-----------------------------------------\n");
    m_lastRequest = REQUEST_PAUSE;
    if (!assureCPlayer()) {
        PLAYER_LOGI("pause() FAIL : could not create player\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        PLAYER_LOGI("pause() FAIL : unknown error\n");
        return;
    }
    if (!isPublicState(MediaPlayer::STATE_PLAYING)) {
        PLAYER_LOGI("pause() IGNORE : nothing to pause\n");
        return;
    }
    pauseCPlayer();
}

void VideoPlayer::setLoop(bool loop)
{
    if (!m_cplayer) {
        return;
    }
    int errorCode = player_set_looping(m_cplayer, loop);
    if (PLAYER_ERROR_NONE != errorCode) {
        PLAYER_LOGE(errorCode, "player_set_looping()\n");
        return;
    }
}

void VideoPlayer::setVideoStreamInfo(String* type, int width, int height, int den, int num)
{
    m_videoInfo.mime = type->utf8Data();
    m_videoInfo.width = width;
    m_videoInfo.height = height;
    m_videoInfo.framerate_den = den;
    m_videoInfo.framerate_num = num;

    if (!m_cplayer) {
        return;
    }
    if (m_currentUrl && m_currentUrl->isBlobURL())
        player_set_video_stream_info(m_cplayer, &m_videoInfo);
    // TODO: maybe should call this after player_create
    // player_set_video_stream_info(m_cplayer, m_videoInfo);
}

void VideoPlayer::clearVideoStreamInfo()
{
    // Set default streaming info
    m_videoInfo.mime = DEFAULT_VIDEO_STREAM_INFO_MIME;
    m_videoInfo.width = DEFAULT_VIDEO_STREAM_INFO_WIDTH;
    m_videoInfo.height = DEFAULT_VIDEO_STREAM_INFO_HEIGHT;
    m_videoInfo.framerate_den = DEFAULT_VIDEO_STREAM_INFO_FRAMERATE_DEN;
    m_videoInfo.framerate_num = DEFAULT_VIDEO_STREAM_INFO_FRAMERATE_NUM;
}

int VideoPlayer::width()
{
    if (!m_cplayer)
        return 0;

    if (m_currentUrl && m_currentUrl->isBlobURL()) {
        return m_videoInfo.width;
    }
    player_video_track_info info;
    int errorCode = player_get_video_info(m_cplayer, &info);
    if (errorCode != PLAYER_ERROR_NONE) {
        PLAYER_LOGI("player_get_video_info() is failed(%u)\n", errorCode);
        return 0;
    }
    return info.width;
}

int VideoPlayer::height()
{
    if (!m_cplayer)
        return 0;
    if (m_currentUrl && m_currentUrl->isBlobURL()) {
        return m_videoInfo.height;
    }
    player_video_track_info info;
    int errorCode = player_get_video_info(m_cplayer, &info);
    if (errorCode != PLAYER_ERROR_NONE) {
        PLAYER_LOGI("player_get_video_info() is failed(%u)\n", errorCode);
        return 0;
    }
    return info.height;
}

#undef PLAYER_DEBUG
#undef PLAYER_LOGI
#undef PLAYER_LOGE
}
#endif
#endif
#endif /* STARFISH_ENABLE_MULTIMEDIA */
