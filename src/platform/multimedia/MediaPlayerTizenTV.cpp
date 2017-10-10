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
#include "MediaPlayerTizenTV.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"

#include <Elementary.h>

#include <media/player.h>
#include <media/player_product.h>

namespace StarFish {

void MediaPlayerTizenTV::printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum)           \
    case errorenum:                           \
        PLAYER_LOGE("ERR: %s\n", #errorenum); \
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
        GEN_ERROR_PRINTS(PLAYER_ERROR_STREAMING_PLAYER)
        GEN_ERROR_PRINTS(PLAYER_ERROR_AUDIO_CODEC_NOT_SUPPORTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_VIDEO_CODEC_NOT_SUPPORTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NO_AUTH)
        GEN_ERROR_PRINTS(PLAYER_ERROR_GENEREIC)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_INFO)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SYNC_PLAY_NETWORK_EXCEPTION)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SYNC_PLAY_SERVER_DOWN)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NOT_SUPPORTED_FORMAT)
#undef GEN_ERROR_PRINTS
    default:
        PLAYER_LOGI("Unknown error\n");
        return;
    }
}

void MediaPlayerTizenTV::setNativePlayerDefaultOptions(ResourceURL* url)
{
    if (m_container->isHTMLVideoElement()) {
        player_display_video_at_paused_state(m_nativePlayer, true);
        player_display_h displayHandle = GET_DISPLAY(
            (Evas_Object*)m_container->starFish()->platformWindow()->unwrap());
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_OVERLAY,
                           displayHandle);
        player_set_display_mode(m_nativePlayer, PLAYER_DISPLAY_MODE_DST_ROI);
        // NOTE: Do not edit `player_set_display_roi_area` parameter
        player_set_display_roi_area(m_nativePlayer, 0, 0, 1, 1);
    } else {
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_NONE, nullptr);
    }
}

void MediaPlayerTizenTV::drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                                   const LayoutRect& absVideoRect)
{
    canvas->punchHole(Unit::Rect(videoRect.x(), videoRect.y(),
                                 videoRect.width(), videoRect.height()));
    player_set_display_roi_area(
        m_nativePlayer, absVideoRect.x().toInt(), absVideoRect.y().toInt(),
        absVideoRect.width().toInt(), absVideoRect.height().toInt());
}

void MediaPlayerTizenTV::mediaEndOperation()
{
    player_stop(m_nativePlayer);
    if (m_activeMediaSource) {
        Locker<Mutex> locker(*m_bufferMutex);
        m_lastAudioDTS = m_lastVideoDTS = 0;
    }
}

void MediaPlayerTizenTV::seekOperation(int timeInMS)
{
    PLAYER_LOGI("MediaPlayerTizenTV::seekOperation() (time: %d)\n", timeInMS);
    if (m_activeMediaSource) {
        Locker<Mutex> locker(*m_bufferMutex);
        if (m_activeMediaSource->activeVideoSourceBuffer()) {
            m_activeMediaSource->activeVideoSourceBuffer()
                ->clearPacketAccessCache();
        }
        if (m_activeMediaSource->activeAudioSourceBuffer()) {
            m_activeMediaSource->activeAudioSourceBuffer()
                ->clearPacketAccessCache();
        }
    }

    int ret =
        player_set_play_position(m_nativePlayer, timeInMS, false,
                                 [](void* data) {
                                     PLAYER_LOGI(
                                         "MediaPlayerTizenTV::seekOperation() "
                                         "player_set_play_position_cb\n");
                                     MediaPlayerTizen* self =
                                         (MediaPlayerTizen*)data;
                                     self->handleSeeked();
                                 },
                                 this);

    if (ret != PLAYER_ERROR_NONE) {
        // Failed immediately
        PLAYER_LOGI(
            "MediaPlayerTizenTV::seekOperation() player_set_position_async "
            "failed immediately (IGNORE) : ");
        printNativePlayerError(ret);
        m_foundError = true;
        handleSeeked();
        return;
    }
    if (m_activeMediaSource) {
        m_bufferMutex->lock();
        m_lastVideoDTS = m_lastAudioDTS = timeInMS;
        m_bufferMutex->unlock();

        if (m_activeMediaSource->activeVideoSourceBuffer()) {
            fillVideoBufferIfNeeded();
        }
        if (m_activeMediaSource->activeAudioSourceBuffer()) {
            fillAudioBufferIfNeeded();
        }
    }
}

void MediaPlayerTizenTV::prepareMediaSource()
{
    PLAYER_LOGI("MediaPlayerTizenTV::prepareMediaSource\n");

    setVideoStreamInfoWithGuard();
    setAudioStreamInfoWithGuard();
    if (m_foundError) {
        return;
    }

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);

#if 0
    // Tizen 2.4
    player_set_buffer_need_video_data_cb(
        m_nativePlayer,
        [](unsigned int size, void* user_data) {
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->fillVideoBufferWithGuard();
        },
        this);
    player_set_buffer_need_audio_data_cb(
        m_nativePlayer,
        [](unsigned int size, void* user_data) {
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->fillAudioBufferWithGuard();
        },
        this);
#endif

    m_preparedCallback = [](void* user_data) {
        PLAYER_LOGI("MediaPlayerTizenTV:: MSE Prepare ok\n");
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
        self->handlePrepared();
    };

    openPreparingMode();
    int nativeResult =
        player_prepare_async(m_nativePlayer, m_preparedCallback, this);

    if (nativeResult != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(
            "MediaPlayerTizenTV:: player_prepare_async return error !!!\n");
        m_foundError = true;
        handlePrepared();
    }

    PLAYER_LOGI("MediaPlayerTizenTV::prepareMediaSource end\n");
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}
}

#endif
#endif
