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
    printf(" seek !!!!! %f \n", (float) time);
    time = time * 1000;
    if (m_activeMediaSource) {
        m_lastVideoPts = m_lastAudioPts = time;
    }
    // FIXME status: player_set_position_cb is NOT called and playing is stop. why?
    int ret = player_set_position(m_nativePlayer, time, [](void* data) {
         STARFISH_LOG_INFO("player_set_position_cb\n");
         MediaPlayerTizenTV* player = (MediaPlayerTizenTV*)data;
    }, this);
    if (ret != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("**ERROR: player_set_position %x", ret);
    }
}

void MediaPlayerTizenTV::fillVideoBuffer(bool useLock)
{
    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer\n");
    if (useLock)
        m_videoBufferMutex->lock();

    uint64_t ptsStart = m_lastVideoPts;
    uint64_t ptsNow = m_lastVideoPts;
    while (ptsNow - ptsStart < 1000) {
        MediaPacket* packet = m_activeMediaSource->activeVideoSourceBuffer()->findProperMediaPacket(m_activeMediaSource->activeVideoStreamIndex(), ptsNow);
        if (!packet) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer detect end of Video -> %d %d\n", (int)endTime, (int)ptsNow);
            if ((endTime - ptsNow) < 10) {
                player_submit_packet(m_nativePlayer, 0, 0, 0, PLAYER_TRACK_TYPE_VIDEO);
                STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer detect end of Video!!\n");
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
            m_isVideoBufferUnderrunState = true;
            break;
        }
        ptsNow = m_lastVideoPts = packet->m_pts + packet->m_duration;
        int ret = player_submit_packet(m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_VIDEO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isVideoBufferUnderrunState = false;
        // printf("push packet %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
    }

    if (useLock)
        m_videoBufferMutex->unlock();
}

void MediaPlayerTizenTV::fillAudioBuffer(bool useLock)
{
    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer\n");
    if (useLock)
        m_audioBufferMutex->lock();

    uint64_t ptsStart = m_lastAudioPts;
    uint64_t ptsNow = m_lastAudioPts;
    while (ptsNow - ptsStart < 1000) {
        MediaPacket* packet = m_activeMediaSource->activeAudioSourceBuffer()->findProperMediaPacket(m_activeMediaSource->activeAudioStreamIndex(), ptsNow);
        if (!packet) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer detect end of Audio -> %d %d\n", (int)endTime, (int)ptsNow);
            if ((endTime - ptsNow) < 10) {
                player_submit_packet(m_nativePlayer, 0, 0, 0, PLAYER_TRACK_TYPE_AUDIO);
                STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer detect end of Audio!!\n");
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
        ptsNow = m_lastAudioPts = packet->m_pts + packet->m_duration;
        int ret = player_submit_packet(m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_AUDIO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isAudioBufferUnderrunState = false;
        // printf("push packet %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
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
