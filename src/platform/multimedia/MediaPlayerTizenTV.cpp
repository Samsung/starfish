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
    if (m_container->isHTMLVideoElement() && m_container->frame()) {
        player_display_h displayHandle = GET_DISPLAY(elm_win_xwindow_get((Evas_Object*) m_container->document()->window()->unwrap()));
        player_display_type_e displayType = PLAYER_DISPLAY_TYPE_X11;
        player_display_mode_e displayMode = PLAYER_DISPLAY_MODE_DST_ROI;
        player_display_roi_mode_e roiMode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;

        player_set_display(m_nativePlayer, displayType, displayHandle);
        player_set_display_mode(m_nativePlayer, displayMode);
        player_set_x11_display_roi_mode(m_nativePlayer, roiMode);
        player_display_video_at_paused_state(m_nativePlayer, TRUE);

        if (url->isNetworkURL()) {
            player_set_streaming_type(m_nativePlayer, const_cast<char*>("FFMPEG_HTTP"));
        }

        player_set_buffer_time_for_play(m_nativePlayer, 1);
        player_set_timeout_for_buffering_for_play(m_nativePlayer, INT_MAX);
    }
}

void MediaPlayerTizenTV::drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
{
    canvas->punchHole(Rect(videoRect.x(), videoRect.y(), videoRect.width(), videoRect.height()));
    player_set_x11_display_dst_roi(m_nativePlayer, absVideoRect.x(), absVideoRect.y(), absVideoRect.width(), absVideoRect.height());
}

double MediaPlayerTizenTV::currentTime()
{
    int s;
    int ret = player_get_position(m_nativePlayer, &s);
    if (ret)
        return 0;
    return s / 1000.0;
}

void MediaPlayerTizenTV::fillVideoBuffer()
{
    Locker<Mutex> locker(*m_videoBufferMutex);
    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer\n");

    uint64_t ptsStart = m_lastVideoPts;
    uint64_t ptsNow = m_lastVideoPts;
    while (ptsNow - ptsStart < 1000) {
        MediaPacket* packet = m_activeMediaSource->activeVideoSourceBuffer()->findProperMediaPacket(m_activeMediaSource->activeVideoStreamIndex(), m_lastVideoPts);
        if (!packet) {
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillVideoBuffer runs into video buffer under run state\n");
            m_isVideoBufferUnderrunState = true;
            break;
        }
        ptsNow = packet->m_pts;
        m_lastVideoPts = packet->m_pts + 1;
        int ret = player_submit_packet(m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_VIDEO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isVideoBufferUnderrunState = false;
        // printf("push packet %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
    }
}

void MediaPlayerTizenTV::fillAudioBuffer()
{
    Locker<Mutex> locker(*m_audioBufferMutex);
    STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer\n");

    uint64_t ptsStart = m_lastAudioPts;
    uint64_t ptsNow = m_lastAudioPts;
    while (ptsNow - ptsStart < 1000) {
        MediaPacket* packet = m_activeMediaSource->activeAudioSourceBuffer()->findProperMediaPacket(m_activeMediaSource->activeAudioStreamIndex(), m_lastAudioPts);
        if (!packet) {
            STARFISH_LOG_INFO("MediaPlayerTizenTV::fillAudioBuffer runs into audio buffer under run state\n");
            m_isAudioBufferUnderrunState = true;
            break;
        }
        ptsNow = packet->m_pts;
        m_lastAudioPts = packet->m_pts + 1;
        int ret = player_submit_packet(m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_AUDIO);

        if (ret != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("**ERROR: player_submit_packet %x", ret);
        }
        m_isAudioBufferUnderrunState = false;
        // printf("push packet %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
    }
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}

}

#endif
#endif
