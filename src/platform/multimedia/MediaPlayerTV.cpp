/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifdef STARFISH_ENABLE_MULTIMEDIA
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN) && \
    !defined(STARFISH_TIZEN_USERAPP_SDK_API_ONLY) &&                      \
    defined(STARFISH_TIZEN_PROD_TV)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/multimedia/MediaPlayerTizen.h"
#include "core/modules/renderer/Renderer.h"

namespace Starfish {

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100
#define STARFISH_MSE_SUBMIT_BYTES_RATE 0.3

void MediaPlayerSourceStream::initFormatExtraForAudio()
{
    m_formatExtra.m_audioFormatExtra.codec_extradata = nullptr;
    m_formatExtra.m_audioFormatExtra.extradata_size = 0;
}

void MediaPlayerSourceStream::initFormatExtraForVideo()
{
    m_formatExtra.m_videoFormatExtra.codec_extradata = nullptr;
    m_formatExtra.m_videoFormatExtra.extradata_size = 0;
}

void MediaPlayerSourceStream::createMediaFormatStreamType()
{
    if (m_type == StreamTypeAudio) {
        player_media_stream_audio_extra_info_s& extra =
            m_formatExtra.m_audioFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
        }
        memset(&extra, 0, sizeof(player_media_stream_audio_extra_info_s));
    } else {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        player_media_stream_video_extra_info_s& extra =
            m_formatExtra.m_videoFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
        }
        memset(&extra, 0, sizeof(player_media_stream_video_extra_info_s));
    }
}

void MediaPlayerSourceStream::releaseMediaFormatStreamType()
{
    if (m_type == StreamTypeAudio) {
        player_media_stream_audio_extra_info_s& extra =
            m_formatExtra.m_audioFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
            extra.codec_extradata = nullptr;
            extra.extradata_size = 0;
        }
    } else {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        player_media_stream_video_extra_info_s& extra =
            m_formatExtra.m_videoFormatExtra;
        if (extra.codec_extradata != nullptr) {
            free(extra.codec_extradata);
            extra.codec_extradata = nullptr;
            extra.extradata_size = 0;
        }
    }
}

int MediaPlayerTizen::playerSetPlayPosition(int& timeInMS)
{
    return player_set_play_position_ex(m_nativePlayer, timeInMS, true,
                                       seekedCallback, this);
}

void MediaPlayerTizen::disposePlayer()
{
    if (m_container->isHTMLVideoElement() && m_container->frame()) {
        m_container->setNeedsComposite();
        // NOTE Deplay dispose()
        //      Transparent hole can be exposed by disposal of player.
        m_container->webView()->renderer()->rendering();
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        if (m_playerDeadFlag) {
            // give dead flag for killing MSE thread first.
            // because, when destroying we want to end threads first
            // but, thread is ended by idler
            // so if we don't give flag this time, we wait forever for MSE
            // thread
            *m_playerDeadFlag = true;
        }
        PLAYER_LOGI(
            "MediaPlayerTizen::close() - dispose player next idle time");
        msgLoop->addIdler(
            nullptr,
            [](size_t, void* data) {
                MediaPlayerTizen* player = (MediaPlayerTizen*)data;
                player->dispose();
            },
            this);
    } else {
        PLAYER_LOGI("MediaPlayerTizen::close() - instant disposal ");
        dispose();
    }
}

void MediaPlayerTizen::initCanvasSurface()
{
}

void MediaPlayerTizen::setNativePlayerDisplayMode()
{
    player_display_video_at_paused_state(m_nativePlayer, true);
#if defined(STARFISH_MM_OUTPUT_WITH_GL)
    setNativePlayerDisplayModeWithGL();
#else
    player_set_display_mode(m_nativePlayer, PLAYER_DISPLAY_MODE_DST_ROI);
    // NOTE: Do not edit `player_set_display_roi_area` parameter
    m_lastAbsoluteROIArea = LayoutRect(0, 0, 1, 1);
    player_set_display_roi_area(m_nativePlayer, 0, 0, 1, 1);

    void* ecoreWaylandHandle =
        m_container->webView()->publicLayerUserDataMap()
            ["__internalLWEWebViewEFLEcoreWaylandHandle"];

    // This is need for displaying video
    // ecore_wl2_window_alpha_set((Ecore_Wl2_Window*)ecoreWaylandHandle, false);

    auto width = m_container->webView()->renderer()->width();
    auto height = m_container->webView()->renderer()->height();

    player_set_ecore_wl_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_OVERLAY,
                                ecoreWaylandHandle, 0, 0, width, height);
    player_set_display_visible(m_nativePlayer, true);
#endif
}

void MediaPlayerTizen::setPlayerDisplayVideoAtPausedState(int& ret)
{
    ret = player_display_video_at_paused_state(m_nativePlayer, true);
    STARFISH_ASSERT(ret == 0);
}

void MediaPlayerTizen::punchHole(Compositor* canvas,
                                 const LayoutRect& videoRect,
                                 const LayoutRect& absVideoRect)
{
#if !defined(STARFISH_MM_OUTPUT_WITH_GL)
    canvas->punchHole(Unit::Rect(videoRect.x(), videoRect.y(),
                                 videoRect.width(), videoRect.height()));
    if (m_lastAbsoluteROIArea != absVideoRect) {
        // TODO consider LWE::WebView x, y
        player_set_display_roi_area(
            m_nativePlayer, absVideoRect.x().toInt(), absVideoRect.y().toInt(),
            absVideoRect.width().toInt(), absVideoRect.height().toInt());
        m_lastAbsoluteROIArea = absVideoRect;
    }
#endif
}

void MediaPlayerTizen::updateAudioStreamInfo(MediaPlayerSourceStream* stream,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
    // TODO audio
    STARFISH_UNIMPLEMENTED();
}

void MediaPlayerTizen::videoFramerateChanged(MediaPlayerSourceStream* stream,
                                             int num, int den)
{
    media_format_h mediaFormat = stream->mediaFormat();
    auto mediaFormatExtra = stream->videoFormatExtra();
    mediaFormatExtra->is_framerate_changed = true;
    mediaFormatExtra->framerate_num = num;
    mediaFormatExtra->framerate_den = den;
    media_format_set_extra(mediaFormat, mediaFormatExtra);
    int ret = player_set_media_stream_info(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO, mediaFormat);
    if (ret != PLAYER_ERROR_NONE) {
        PLAYER_LOGI("ERROR: player_set_media_stream_info");
        printNativePlayerError(ret);
    }
}

void MediaPlayerTizen::setMediaFormatExtraForVideo(media_format_h& mediaFormat,
                                                   StreamInfo* info)
{
    auto mediaFormatExtra = m_videoStream->videoFormatExtra();
    mediaFormatExtra->max_width = STARFISH_VIDEO_MAX_WIDTH;
    mediaFormatExtra->max_height = STARFISH_VIDEO_MAX_HEIGHT;
    m_videoStream->setMaxBufferSize(
        (m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 / 8 * 5);

    if (info->videoHasFramerate() && info->videoFramerate().isValid()) {
        if (info->videoFramerate().toDouble() > 60.0) {
            info->setVideoFramerate(Framerate(60, 1));
        }
        mediaFormatExtra->framerate_num = info->videoFramerate().m_num;
        mediaFormatExtra->framerate_den = info->videoFramerate().m_den;
    } else {
        mediaFormatExtra->framerate_num = STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM;
        mediaFormatExtra->framerate_den = STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN;
    }
    mediaFormatExtra->hdr_mode = MEDIA_STREAM_HDR_TYPE_MATROSKA;
    mediaFormatExtra->hdr_info = std::string().c_str();
    mediaFormatExtra->is_framerate_changed = false;
    // TODO PLAYER_DRM_TYPE_EME
    mediaFormatExtra->drm_type = PLAYER_DRM_TYPE_NONE;

    // Extra data
    mediaFormatExtra->extradata_size = info->m_extraData.size();
    if (mediaFormatExtra->extradata_size != 0) {
        mediaFormatExtra->codec_extradata =
            (unsigned char*)malloc(mediaFormatExtra->extradata_size);
        memcpy(mediaFormatExtra->codec_extradata, info->m_extraData.data(),
               mediaFormatExtra->extradata_size);
    }
    media_format_set_extra(mediaFormat, mediaFormatExtra);
    PLAYER_LOGI("> framerate : %d/%d", mediaFormatExtra->framerate_num,
                mediaFormatExtra->framerate_den);
}

void MediaPlayerTizen::setMediaFormatExtraForAudio(media_format_h& mediaFormat,
                                                   StreamInfo* info)
{
    auto mediaFormatExtra = m_audioStream->audioFormatExtra();
    mediaFormatExtra->extradata_size = info->m_extraData.size();
    if (mediaFormatExtra->extradata_size != 0) {
        mediaFormatExtra->codec_extradata =
            (unsigned char*)malloc(mediaFormatExtra->extradata_size);
        memcpy(mediaFormatExtra->codec_extradata, info->m_extraData.data(),
               mediaFormatExtra->extradata_size);
    }
    // TODO PLAYER_DRM_TYPE_EME
    mediaFormatExtra->drm_type = PLAYER_DRM_TYPE_NONE;
    media_format_set_extra(mediaFormat, mediaFormatExtra);
}
} // namespace Starfish

#endif
#endif
