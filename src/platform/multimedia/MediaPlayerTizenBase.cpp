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

//#include <media/player.h>

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

void MediaPlayerTizen::disposePlayer()
{
    dispose();
}

void MediaPlayerTizen::initCanvasSurface()
{
    m_canvasSurface =
        CanvasSurface::create(m_container->starFish()->platformWindow(), 1, 1);
    m_canvasSurface->clear();
}

void MediaPlayerTizen::setNativePlayerDisplayMode()
{
    player_set_display_mode(m_nativePlayer,
                            PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER);
    player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_EVAS,
                       m_canvasSurface->unwrap());
}

void MediaPlayerTizen::setPlayerDisplayVideoAtPausedState(int& ret)
{
}
void MediaPlayerTizen::punchHole(Compositor* canvas,
                                 const LayoutRect& videoRect,
                                 const LayoutRect& absVideoRect)
{
    canvas->setColor(Unit::Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
    canvas->drawSurface(m_canvasSurface,
                        Unit::Rect(videoRect.x(), videoRect.y(),
                                   videoRect.width(), videoRect.height()));
}

void MediaPlayerTizen::updateStreamInfo(MediaStream* stream,
                                        size_t pastInitIndex,
                                        size_t newInitIndex)
{
}

void MediaPlayerTizen::updateAudioStreamInfo(MediaStream* audio,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
}

void MediaPlayerTizen::updateVideoStreamInfo(MediaStream* video,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
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
