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
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "MediaPlayerTizenTV.h"
#include "platform/canvas/Canvas.h"
#include "platform/message_loop/MessageLoop.h"
#include "extra/MediaSource.h"
#include "extra/SourceBuffer.h"

#include <Elementary.h>

#include <media/player.h>
#include <media/player_product.h>


namespace StarFish {

void MediaPlayerTizenTV::printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum) \
    case errorenum: \
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
        player_set_streaming_type(m_nativePlayer, const_cast<char*>("FFMPEG_HTTP"));
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
    }
}

void MediaPlayerTizenTV::drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
{
    canvas->punchHole(Rect(videoRect.x(), videoRect.y(), videoRect.width(), videoRect.height()));
    player_set_x11_display_dst_roi(m_nativePlayer, absVideoRect.x(), absVideoRect.y(), absVideoRect.width(), absVideoRect.height());
}

double MediaPlayerTizenTV::currentTime()
{
    if (m_playbackState == MediaPlayer::PLAYBACK_STATE_END)
        return duration();

    int s;
    int ret = player_get_position(m_nativePlayer, &s);
    if (ret)
        return 0;
    return s / 1000.0;
}

void MediaPlayerTizenTV::seek(double time)
{
    player_state_e state;
    player_get_state(m_nativePlayer, &state);
    STARFISH_LOG_INFO("MediaPlayerTizenTV::seek() time: %f state: %d \n", (float) time, (int)state);
    time = time * 1000;
    int ret;
    if (m_activeMediaSource) {
        Locker<Mutex> videoLock(*m_videoBufferMutex);
        Locker<Mutex> audioLock(*m_audioBufferMutex);
        m_activeMediaSource->activeVideoSourceBuffer()->clearPacketAccessCache();
        m_lastVideoPts = m_lastAudioPts = time;
        ret = player_set_position(m_nativePlayer, time, [](void* data) {
            STARFISH_LOG_INFO("player_set_position_cb\n");
        }, this);
    } else {
        ret = player_set_position(m_nativePlayer, time, [](void* data) {
            STARFISH_LOG_INFO("player_set_position_cb\n");
        }, this);
    }
    if (ret != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("**ERROR: player_set_position %x", ret);
    }
}

void MediaPlayerTizenTV::fillVideoBuffer(bool useLock)
{
    if (useLock)
        m_videoBufferMutex->lock();

    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer -> %dms\n", (int)m_lastVideoPts);
    uint64_t ptsStart = m_lastVideoPts;
    uint64_t streamIdx = m_activeMediaSource->activeVideoStreamIndex();

    while (m_lastVideoPts - ptsStart < 1000) {
        MediaPacket* packet = m_activeMediaSource->activeVideoSourceBuffer()->findProperMediaPacket(streamIdx, m_lastVideoPts);
        if (!packet) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer detect end of Video -> %d %d\n", (int)endTime, (int)m_lastVideoPts);
            uint64_t lastBufferedTime = m_activeMediaSource->activeVideoSourceBuffer()->lastBufferedTimestamp(streamIdx);
            if ((endTime - m_lastVideoPts) < 10 || ((m_lastVideoPts == lastBufferedTime) && (std::abs(endTime - lastBufferedTime) < 1000))) {
                player_submit_packet(m_nativePlayer, 0, 0, 0, PLAYER_TRACK_TYPE_VIDEO);
                STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer detect end of Video!\n");
                if (useLock)
                    m_videoBufferMutex->unlock();
                return;
            }

            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer runs into video buffer under run state\n");
            m_isVideoBufferUnderrunState = true;
            if (useLock)
                m_videoBufferMutex->unlock();
            return;
        }
        if (packet->m_pts > m_lastVideoPts && packet->m_pts - m_lastVideoPts > 500) {
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer runs into video buffer under run state\n");
            m_isVideoBufferUnderrunState = true;
            break;
        }
        m_lastVideoPts = packet->m_pts + packet->m_duration;
        int ret = player_submit_packet(m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_VIDEO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isVideoBufferUnderrunState = false;
        // printf("push packet(video) %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
    }

    if (useLock)
        m_videoBufferMutex->unlock();
}

void MediaPlayerTizenTV::fillAudioBuffer(bool useLock)
{
    if (useLock)
        m_audioBufferMutex->lock();

    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer -> %dms\n", (int)m_lastAudioPts);

    uint64_t ptsStart = m_lastAudioPts;
    uint64_t streamIdx = m_activeMediaSource->activeAudioStreamIndex();
    while (m_lastAudioPts - ptsStart < 1000) {
        MediaPacket* packet = m_activeMediaSource->activeAudioSourceBuffer()->findProperMediaPacket(streamIdx, m_lastAudioPts);
        if (!packet) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer detect end of Audio -> %d %d\n", (int)endTime, (int)m_lastAudioPts);
            uint64_t lastBufferedTime = m_activeMediaSource->activeVideoSourceBuffer()->lastBufferedTimestamp(streamIdx);
            if ((endTime - m_lastAudioPts) < 10 || ((m_lastAudioPts == lastBufferedTime) && (std::abs(endTime - lastBufferedTime) < 1000))) {
                player_submit_packet(m_nativePlayer, 0, 0, 0, PLAYER_TRACK_TYPE_AUDIO);
                STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer detect end of Audio!\n");
                if (useLock)
                    m_audioBufferMutex->unlock();
                return;
            }

            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer runs into audio buffer under run state\n");
            m_isAudioBufferUnderrunState = true;
            if (useLock)
                m_audioBufferMutex->unlock();
            return;
        }
        if (packet->m_pts > m_lastAudioPts && packet->m_pts - m_lastAudioPts > 500) {
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer runs into audio buffer under run state\n");
            m_isAudioBufferUnderrunState = true;
            break;
        }
        m_lastAudioPts = packet->m_pts + packet->m_duration;
        int ret = player_submit_packet(m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_AUDIO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isAudioBufferUnderrunState = false;
        // printf("push packet(audio) %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
    }

    if (useLock)
        m_audioBufferMutex->unlock();
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}

}

#endif
#endif
