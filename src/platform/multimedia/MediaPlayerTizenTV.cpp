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
        STARFISH_LOG_ERROR("Unknown error\n");
        return;
    }
}

void MediaPlayerTizenTV::setNativePlayerDefaultOptions(ResourceURL* url)
{
    if (m_container->isHTMLVideoElement()) {
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
        m_lastAudioPts = m_lastVideoPts = 0;
    }
}

void MediaPlayerTizenTV::seekOperation(int timeInMS)
{
    STARFISH_LOG_INFO("MediaPlayerTizenTV::seekOperation() (time: %d)\n",
                      timeInMS);
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

    int ret = player_set_play_position(
        m_nativePlayer, timeInMS, false,
        [](void* data) {
            STARFISH_LOG_INFO(
                "MediaPlayerTizenTV::seekOperation() "
                "player_set_position_async_cb (%s)\n",
                isMainThread() ? "main-thread" : "other-thread");
            MediaPlayerTizen* self = (MediaPlayerTizen*)data;
            self->handleSeeked();
        },
        this);

    if (ret != PLAYER_ERROR_NONE) {
        // Failed immediately
        STARFISH_LOG_INFO(
            "MediaPlayerTizenTV::seekOperation() player_set_position_async "
            "failed immediately (IGNORE) : ");
        printNativePlayerError(ret);
        handleSeekFail();
        return;
    }
    if (m_activeMediaSource) {
        m_bufferMutex->lock();
        m_lastVideoPts = m_lastAudioPts = timeInMS;
        m_bufferMutex->unlock();

        if (m_activeMediaSource->activeVideoSourceBuffer()) {
            fillVideoBufferIfNeeded();
        }
        if (m_activeMediaSource->activeAudioSourceBuffer()) {
            fillAudioBufferIfNeeded();
        }
    }
}

void MediaPlayerTizenTV::handleSeeked()
{
    if (isMainThread()) {
        STARFISH_LOG_INFO("MediaPlayerTizenTV::handleSeeked\n");
        if (m_seekState == SEEKSTATE_NO_SEEK) {
            return;
        }
        STARFISH_ASSERT(m_seekState == SEEKSTATE_SEEKING);
        m_seekState = SEEKSTATE_NO_SEEK;

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

void MediaPlayerTizenTV::handleSeekFail()
{
    if (isMainThread()) {
        STARFISH_LOG_INFO("MediaPlayerTizenTV::handleSeekFail\n");
        handleSeeked();
    } else {
        MessageLoop* msgLoop = m_container->starFish()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->document()->browsingContext(),
            [](size_t, void* data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)data;
                self->handleSeekFail();
            },
            this);
    }
}

void MediaPlayerTizenTV::prepareMediaSource()
{
    PLAYER_LOGI("MediaPlayerTizenTV::prepareMediaSource\n");
    player_set_uri(m_nativePlayer, "external_demuxer://MSE");

    setVideoStreamInfo();
    setAudioStreamInfo();

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);

    player_set_buffer_need_video_data_cb(
        m_nativePlayer,
        [](unsigned int size, void* user_data) {
            PLAYER_LOGI(
                "MediaPlayerTizenTV:: videoPlayerBufferNeedVideoDataCB\n");
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->fillVideoBuffer();
        },
        this);

    /*
    player_set_video_frame_decoded_cb(m_nativePlayer, [](unsigned char *data,
    int width, int height, unsigned int size, void *user_data)
    {
        STARFISH_LOG_INFO("player_set_video_frame_decoded_cb called %d %d\n",
    width, height);
    }, this); */

    player_set_buffer_need_audio_data_cb(
        m_nativePlayer,
        [](unsigned int size, void* user_data) {
            PLAYER_LOGI(
                "MediaPlayerTizenTV:: videoPlayerBufferNeedAudioDataCB\n");
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->fillAudioBuffer();
        },
        this);

    m_preparedCallback = [](void* user_data) {
        PLAYER_LOGI("MediaPlayerTizenTV:: MSE Prepare ok\n");
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
        self->completePrepare();
    };

    openPreparingMode();
    int nativeResult =
        player_prepare_async(m_nativePlayer, m_preparedCallback, this);

    if (nativeResult != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(
            "MediaPlayerTizenTV:: player_prepare_async return error !!!\n");
        STARFISH_ASSERT_NOT_REACHED();

        STARFISH_ASSERT(m_inPrepare);
        closePreparingMode();
    }

    PLAYER_LOGI("MediaPlayerTizenTV::prepareMediaSource end\n");
}

void MediaPlayerTizenTV::fillVideoBuffer(bool useLock)
{
    if (useLock) {
        m_bufferMutex->lock();
    }

    PLAYER_LOGI("MediaPlayerTizenTV::fillVideoBuffer start %dms\n",
                (int)m_lastVideoPts);
    uint64_t submitted = 0;
    uint64_t streamIdx = m_activeMediaSource->activeVideoStreamIndex();

    while (submitted < 500) {
        std::pair<MediaPacket*, size_t> packet =
            m_activeMediaSource->activeVideoSourceBuffer()
                ->findProperMediaPacket(streamIdx, m_lastVideoPts);

        if (!packet.first) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            PLAYER_LOGI(
                "MediaPlayerTizenTV::fillVideoBuffer try to detect end of "
                "Video -> %d %d\n",
                (int)endTime, (int)m_lastVideoPts);
            uint64_t lastBufferedTime =
                m_activeMediaSource->activeVideoSourceBuffer()
                    ->lastBufferedTimestamp(streamIdx);
            if ((endTime - m_lastVideoPts) < 10 ||
                ((m_lastVideoPts == lastBufferedTime) &&
                 (std::abs(endTime - lastBufferedTime) < 1000))) {
                m_isEnded = true;
                player_submit_es_packet(m_nativePlayer, 0, 0, 0,
                                        PLAYER_STREAM_TYPE_VIDEO, nullptr);
                PLAYER_LOGI(
                    "MediaPlayerTizenTV::fillVideoBuffer detect end of "
                    "Video!\n");
                if (useLock) {
                    m_bufferMutex->unlock();
                }
                return;
            }

            PLAYER_LOGI(
                "MediaPlayerTizenTV::fillVideoBuffer runs into video buffer "
                "under run state[1]\n");
            m_isVideoBufferUnderrunState = true;
            if (useLock) {
                m_bufferMutex->unlock();
            }
            return;
        }
        if (packet.first->m_pts > m_lastVideoPts &&
            packet.first->m_pts - m_lastVideoPts > 500) {
            PLAYER_LOGI(
                "MediaPlayerTizenTV::fillVideoBuffer runs into video buffer "
                "under run state[2] - requested(%lld) but returned(%lld)\n",
                m_lastVideoPts, packet.first->m_pts);
            m_isVideoBufferUnderrunState = true;
            break;
        }

        m_lastVideoPts = packet.first->m_pts + packet.first->m_duration;
        if (packet.second != m_videoInitSegmentIndex) {
            if (!packet.first->m_hasIdr) {
                PLAYER_LOGI(
                    "MediaPlayerTizenTV::fillVideoBuffer drops non-idr packet "
                    "when video type changed\n");
                continue;
            } else {
                m_videoInitSegmentIndex = packet.second;
                PLAYER_LOGI(
                    "MediaPlayerTizenTV::fillVideoBuffer detect ohter type of "
                    "Video! (and will submit packet including idr)\n");
            }
        }
        submitted += packet.first->m_duration;
        int ret = player_submit_es_packet(
            m_nativePlayer, packet.first->m_data, packet.first->m_dataSize,
            packet.first->m_pts, PLAYER_STREAM_TYPE_VIDEO, nullptr);
        // PLAYER_LOGI("> %dms (data: %d ... %d", (int)m_lastVideoPts,
        // (int)packet.first->m_data[0],
        // (int)packet.first->m_data[packet.first->m_dataSize - 1]);

        if (ret != PLAYER_ERROR_NONE) {
            PLAYER_LOGE("**ERROR: player_submit_es_packet\n");
            printNativePlayerError(ret);
        }
        m_isVideoBufferUnderrunState = false;
    }
    PLAYER_LOGI("MediaPlayerTizenTV::fillVideoBuffer end %dms\n\n",
                (int)m_lastVideoPts);

    if (useLock) {
        m_bufferMutex->unlock();
    }
}

void MediaPlayerTizenTV::fillAudioBuffer(bool useLock)
{
    if (useLock) {
        m_bufferMutex->lock();
    }

    PLAYER_LOGI("MediaPlayerTizenTV::fillAudioBuffer start %dms\n",
                (int)m_lastAudioPts);
    uint64_t ptsStart = m_lastAudioPts;
    uint64_t streamIdx = m_activeMediaSource->activeAudioStreamIndex();

    while (m_lastAudioPts - ptsStart < 500) {
        std::pair<MediaPacket*, size_t> packet =
            m_activeMediaSource->activeAudioSourceBuffer()
                ->findProperMediaPacket(streamIdx, m_lastAudioPts);

        if (!packet.first) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            PLAYER_LOGI(
                "MediaPlayerTizenTV::fillAudioBuffer try to detect end of "
                "Audio -> %d %d\n",
                (int)endTime, (int)m_lastAudioPts);
            uint64_t lastBufferedTime =
                m_activeMediaSource->activeAudioSourceBuffer()
                    ->lastBufferedTimestamp(streamIdx);
            if ((endTime - m_lastAudioPts) < 10 ||
                ((m_lastAudioPts == lastBufferedTime) &&
                 (std::abs(endTime - lastBufferedTime) < 1000))) {
                m_isEnded = true;
                player_submit_es_packet(m_nativePlayer, 0, 0, 0,
                                        PLAYER_STREAM_TYPE_AUDIO, nullptr);
                PLAYER_LOGI(
                    "MediaPlayerTizenTV::fillAudioBuffer detect end of "
                    "Audio!\n");
                if (useLock) {
                    m_bufferMutex->unlock();
                }
                return;
            }

            PLAYER_LOGI(
                "MediaPlayerTizenTV::fillAudioBuffer runs into audio buffer "
                "under run state[1]\n");
            m_isAudioBufferUnderrunState = true;
            if (useLock) {
                m_bufferMutex->unlock();
            }
            return;
        }
        if (packet.first->m_pts > m_lastAudioPts &&
            packet.first->m_pts - m_lastAudioPts > 500) {
            PLAYER_LOGI(
                "MediaPlayerTizenTV::fillAudioBuffer runs into audio buffer "
                "under run state[2]\n");
            m_isAudioBufferUnderrunState = true;
            break;
        }
        if (packet.second != m_audioInitSegmentIndex) {
            m_audioInitSegmentIndex = packet.second;
            PLAYER_LOGI(
                "MediaPlayerTizenTV::fillAudioBuffer detect ohter type of "
                "Audio!\n");
        }
        m_lastAudioPts = packet.first->m_pts + packet.first->m_duration;
        int ret = player_submit_es_packet(
            m_nativePlayer, packet.first->m_data, packet.first->m_dataSize,
            packet.first->m_pts, PLAYER_STREAM_TYPE_AUDIO, nullptr);
        // PLAYER_LOGI("> %dms\n", (int)m_lastAudioPts);

        if (ret != PLAYER_ERROR_NONE) {
            PLAYER_LOGE("**ERROR: player_submit_es_packet\n");
            printNativePlayerError(ret);
        }
        m_isAudioBufferUnderrunState = false;
    }
    PLAYER_LOGI("MediaPlayerTizenTV::fillAudioBuffer end %dms\n\n",
                (int)m_lastAudioPts);
    if (useLock) {
        m_bufferMutex->unlock();
    }
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}
}

#endif
#endif
