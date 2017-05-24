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
#include "core/util/URL.h"
#include "core/dom/HTMLVideoElement.h"
#include "MediaPlayerTizenTV.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/extra/MediaSource.h"
#include "core/extra/SourceBuffer.h"

#include <Elementary.h>

#include <media/player.h>
#include <media/player_product.h>

namespace StarFish {

void MediaPlayerTizenTV::printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum)             \
    case errorenum:                             \
        STARFISH_LOG_ERROR("%s\n", #errorenum); \
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

void MediaPlayerTizenTV::setNativePlayerDefaultOptions(URL* url)
{
    if (url->isNetworkURL()) {
        player_set_streaming_type(m_nativePlayer,
                                  const_cast<char*>("FFMPEG_HTTP"));
    }

    if (m_container->isHTMLVideoElement() && m_container->frame()) {
        player_display_h displayHandle = GET_DISPLAY(
            elm_win_xwindow_get((Evas_Object*)m_container->window()->unwrap()));
        player_display_type_e displayType = PLAYER_DISPLAY_TYPE_X11;
        player_display_mode_e displayMode = PLAYER_DISPLAY_MODE_DST_ROI;
        player_display_roi_mode_e roiMode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;

        player_set_display(m_nativePlayer, displayType, displayHandle);
        player_set_display_mode(m_nativePlayer, displayMode);
        player_set_x11_display_roi_mode(m_nativePlayer, roiMode);
        player_display_video_at_paused_state(m_nativePlayer, TRUE);
    }

    player_set_buffer_size(m_nativePlayer, PLAYER_BUFFER_FOR_PLAY,
                           PLAYER_BUFFER_SIZE_IN_SECOND, 1);
    player_set_buffer_size(m_nativePlayer, PLAYER_BUFFER_FOR_RESUME,
                           PLAYER_BUFFER_SIZE_IN_SECOND, 1);
}

void MediaPlayerTizenTV::drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                                   const LayoutRect& absVideoRect)
{
    canvas->punchHole(Rect(videoRect.x(), videoRect.y(), videoRect.width(),
                           videoRect.height()));
    player_set_x11_display_dst_roi(m_nativePlayer, absVideoRect.x(),
                                   absVideoRect.y(), absVideoRect.width(),
                                   absVideoRect.height());
}

double MediaPlayerTizenTV::currentTime()
{
    if (m_playbackState == MediaPlayer::PLAYBACK_STATE_END) {
        return duration();
    }

    int s;
    int ret = player_get_position(m_nativePlayer, &s);
    if (ret) {
        return 0;
    }
    return s / 1000.0;
}

void MediaPlayerTizenTV::seekOperation(int timeInMS)
{
    STARFISH_LOG_INFO("MediaPlayerTizenTV::seekOperation() (time: %d)\n",
                      timeInMS);
    if (m_activeMediaSource) {
        Locker<Mutex> videoLock(*m_videoBufferMutex);
        Locker<Mutex> audioLock(*m_audioBufferMutex);
        if (m_activeMediaSource->activeVideoSourceBuffer()) {
            m_activeMediaSource->activeVideoSourceBuffer()
                ->clearPacketAccessCache();
        }
        if (m_activeMediaSource->activeAudioSourceBuffer()) {
            m_activeMediaSource->activeAudioSourceBuffer()
                ->clearPacketAccessCache();
        }
    }

    int ret = player_set_position_async(
        m_nativePlayer, timeInMS,
        [](void* data, int result) {
            // Result 0  : Succeed
            // Otherwise : Failed
            STARFISH_LOG_INFO(
                "MediaPlayerTizenTV::seekOperation() "
                "player_set_position_async_cb (%s)(success:%s)\n",
                isMainThread() ? "main-thread" : "other-thread",
                result == 0 ? "true" : "false");
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
        m_videoBufferMutex->lock();
        m_audioBufferMutex->lock();
        m_lastVideoPts = m_lastAudioPts = timeInMS;
        m_videoBufferMutex->unlock();
        m_audioBufferMutex->unlock();

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
        if (m_seekState == SEEKSTATE_SEEKING) {
            m_seekState = SEEKSTATE_WAITING;
            return;
        }
        STARFISH_ASSERT(m_seekState == SEEKSTATE_WAITING);
        m_seekState = SEEKSTATE_NO_SEEK;

        // Notify "Seeked" to its container
        if (m_container) {
            m_container->mediaPlayerNotifySeekedItsContainer(currentTime());
        }

        // Remove timeout timer
        if (m_seekingTimer != SIZE_MAX) {
            m_starFish->window()->clearTimeout(m_seekingTimer);
            m_seekingTimer = SIZE_MAX;
        }
        // Remove rooted pointer
        m_starFish->removePointerFromRootSet(this);

        if (!m_alive) {
            close();
            return;
        }
    } else {
        m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
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
        m_seekState = SEEKSTATE_WAITING;
        handleSeeked();
    } else {
        m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            [](size_t, void* data) {
                MediaPlayerTizen* self = (MediaPlayerTizen*)data;
                self->handleSeekFail();
            },
            this);
    }
}

void MediaPlayerTizenTV::prepareMediaSource()
{
    STARFISH_LOG_INFO("MediaPlayerTizenTV::prepareMediaSource\n");
    player_set_uri(m_nativePlayer, "external_demuxer://aaaa");

    setVideoStreamInfo();
    setAudioStreamInfo();

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);

    player_set_buffer_need_video_data_cb(
        m_nativePlayer,
        [](unsigned int size, void* user_data) {
            STARFISH_LOG_INFO("videoPlayerBufferNeedVideoDataCB called\n");
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
            STARFISH_LOG_INFO("videoPlayerBufferNeedAudioDataCB called\n");
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->fillAudioBuffer();
        },
        this);

    m_preparedCallback = [](void* user_data) {
        STARFISH_LOG_INFO("MediaPlayerTizenTV MSE Prepare ok");
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
        self->completePrepare();
    };

    openPreparingMode();
    int nativeResult =
        player_prepare_async(m_nativePlayer, m_preparedCallback, this);

    if (nativeResult != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("player_prepare_async return error !!!\n");
        STARFISH_ASSERT_NOT_REACHED();

        STARFISH_ASSERT(m_inPrepare);
        closePreparingMode();
    }

    STARFISH_LOG_INFO("prepareMediaSourceEnd\n");
}

void MediaPlayerTizenTV::fillVideoBuffer(bool useLock)
{
    if (useLock) {
        m_videoBufferMutex->lock();
    }

    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer -> %dms\n",
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
            STARFISH_LOG_INFO(
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
                player_submit_packet(m_nativePlayer, 0, 0, 0,
                                     PLAYER_TRACK_TYPE_VIDEO);
                STARFISH_LOG_INFO(
                    "MediaPlayerTizenTV::fillVideoBuffer detect end of "
                    "Video!\n");
                if (useLock) {
                    m_videoBufferMutex->unlock();
                }
                return;
            }

            STARFISH_LOG_INFO(
                "MediaPlayerTizenTV::fillVideoBuffer runs into video buffer "
                "under run state[1]\n");
            m_isVideoBufferUnderrunState = true;
            if (useLock) {
                m_videoBufferMutex->unlock();
            }
            return;
        }
        if (packet.first->m_pts > m_lastVideoPts &&
            packet.first->m_pts - m_lastVideoPts > 500) {
            STARFISH_LOG_INFO(
                "MediaPlayerTizenTV::fillVideoBuffer runs into video buffer "
                "under run state[2] - requested(%lld) but returned(%lld)\n",
                m_lastVideoPts, packet.first->m_pts);
            m_isVideoBufferUnderrunState = true;
            break;
        }

        m_lastVideoPts = packet.first->m_pts + packet.first->m_duration;
        if (packet.second != m_videoInitSegmentIndex) {
            if (!packet.first->m_hasIdr) {
                STARFISH_LOG_INFO(
                    "MediaPlayerTizenTV::fillVideoBuffer drops non-idr packet "
                    "when video type changed\n");
                continue;
            } else {
                m_videoInitSegmentIndex = packet.second;
                STARFISH_LOG_INFO(
                    "MediaPlayerTizenTV::fillVideoBuffer detect ohter type of "
                    "Video! (and will submit packet including idr)\n");
            }
        }
        submitted += packet.first->m_duration;
        int ret = player_submit_packet(
            m_nativePlayer, packet.first->m_data, packet.first->m_dataSize,
            packet.first->m_pts, PLAYER_TRACK_TYPE_VIDEO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isVideoBufferUnderrunState = false;
        // STARFISH_LOG_INFO("push packet(video) %d %p %d\n",
        // (int)packet.first->m_pts, packet.first->m_data,
        // (int)packet.first->m_dataSize);
    }

    if (useLock) {
        m_videoBufferMutex->unlock();
    }
}

void MediaPlayerTizenTV::fillAudioBuffer(bool useLock)
{
    if (useLock) {
        m_audioBufferMutex->lock();
    }

    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer -> %dms\n",
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
            STARFISH_LOG_INFO(
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
                player_submit_packet(m_nativePlayer, 0, 0, 0,
                                     PLAYER_TRACK_TYPE_AUDIO);
                STARFISH_LOG_INFO(
                    "MediaPlayerTizenTV::fillAudioBuffer detect end of "
                    "Audio!\n");
                if (useLock) {
                    m_audioBufferMutex->unlock();
                }
                return;
            }

            STARFISH_LOG_INFO(
                "MediaPlayerTizenTV::fillAudioBuffer runs into audio buffer "
                "under run state[1]\n");
            m_isAudioBufferUnderrunState = true;
            if (useLock) {
                m_audioBufferMutex->unlock();
            }
            return;
        }
        if (packet.first->m_pts > m_lastAudioPts &&
            packet.first->m_pts - m_lastAudioPts > 500) {
            STARFISH_LOG_INFO(
                "MediaPlayerTizenTV::fillAudioBuffer runs into audio buffer "
                "under run state[2]\n");
            m_isAudioBufferUnderrunState = true;
            break;
        }
        if (packet.second != m_audioInitSegmentIndex) {
            m_audioInitSegmentIndex = packet.second;
            STARFISH_LOG_INFO(
                "MediaPlayerTizenTV::fillAudioBuffer detect ohter type of "
                "Audio!\n");
        }
        m_lastAudioPts = packet.first->m_pts + packet.first->m_duration;
        int ret = player_submit_packet(
            m_nativePlayer, packet.first->m_data, packet.first->m_dataSize,
            packet.first->m_pts, PLAYER_TRACK_TYPE_AUDIO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isAudioBufferUnderrunState = false;
        // STARFISH_LOG_INFO("push packet(audio) %d %p %d\n",
        // (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
    }
    if (useLock) {
        m_audioBufferMutex->unlock();
    }
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}
}

#endif
#endif
