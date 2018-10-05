/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)

#include "StarFishConfig.h"
#include "StarFish.h"
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
#include "platform/window/PlatformWindow.h"

namespace StarFish {

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100
#define STARFISH_MSE_SUBMIT_BYTES_RATE 0.3

void MediaStream::initFormatExtraForAudio()
{
}

void MediaStream::initFormatExtraForVideo()
{
}

void MediaStream::createMediaFormatStreamType()
{
}

void MediaStream::releaseMediaFormatStreamType()
{
}

int MediaPlayerTizen::playerSetPlayPosition(int& timeInMS)
{
    return player_set_play_position(m_nativePlayer, timeInMS, true,
                                    seekedCallback, this);
}

void MediaPlayerTizen::initCanvasSurface()
{
}

void MediaPlayerTizen::disposePlayer()
{
    dispose();
}

void MediaPlayerTizen::setNativePlayerDisplayMode()
{
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

    // ecore_wl_window_alpha_set(ecoreWaylandHandle, false);

    auto width = m_container->webView()->platformWindow()->width();
    auto height = m_container->webView()->platformWindow()->height();

    player_set_ecore_wl_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_OVERLAY,
                                ecoreWaylandHandle, 0, 0, width, height);
    player_set_display_visible(m_nativePlayer, true);
#endif
}

void MediaPlayerTizen::setPlayerDisplayVideoAtPausedState(int& ret)
{
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

void MediaPlayerTizen::updateAudioStreamInfo(MediaStream* audio,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
}

void MediaPlayerTizen::videoFramerateChanged(MediaStream* stream, int num,
                                             int den)
{
}

void MediaPlayerTizen::setMediaFormatExtraForVideo(media_format_h& mediaFormat,
                                                   StreamInfo* info)
{
    if (info->duration() > 0) {
        media_format_set_video_frame_rate(mediaFormat,
                                          (1000 / info->duration()));
    }
}
void MediaPlayerTizen::setMediaFormatExtraForAudio(media_format_h& mediaFormat,
                                                   StreamInfo* info)
{
}
}

#endif
#endif
