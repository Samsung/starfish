/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLMediaElement.h"
#include "platform/multimedia/MediaPlayer.h"
#include "platform/window/PlatformWindow.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/page/Window.h"

namespace StarFish {

MediaPlayer::MediaPlayer(HTMLMediaElement* element)
    : m_alive(true)
    , m_foundError(false)
    , m_isLooping(false)
    , m_hasVideo(false)
    , m_seekState(SEEKSTATE_NO_SEEK)
    , m_playbackState(PLAYBACK_STATE_NONE)
    , m_container(element)
    , m_activeMediaSource(nullptr)
    , m_videoWidth(STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS)
    , m_videoHeight(STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS)
    , m_currentTimeUpdateTimer(SIZE_MAX)
{
}

void MediaPlayer::processNextOperationQueueInContainer()
{
    if (m_container) {
        m_container->processNextOperationQueue();
    }
}

void MediaPlayer::appendToOperationQueueInContainer(
    MediaOperationQueueData* data)
{
    if (m_container) {
        m_container->appendToOperationQueue(data);
    }
}

Window* MediaPlayer::window()
{
    return m_container->window();
}
CanvasSurface* MediaPlayer::createGraphicsBuffer(size_t visibleWidth,
                                                 size_t visibleHeight)
{
    return CanvasSurface::create(window()->starFish()->platformWindow(), 1, 1);
}

SourceBuffer* MediaPlayer::activeSourceBuffer(StreamType type)
{
    if (m_activeMediaSource) {
        if (type == StreamTypeAudio) {
            return m_activeMediaSource->activeAudioSourceBuffer();
        } else {
            return m_activeMediaSource->activeVideoSourceBuffer();
        }
    }
    return nullptr;
}

uint64_t MediaPlayer::activeStreamIndex(StreamType type)
{
    STARFISH_ASSERT(m_activeMediaSource);
    if (type == StreamTypeAudio) {
        return m_activeMediaSource->activeAudioStreamIndex();
    }
    STARFISH_ASSERT(type == StreamTypeVideo);
    return m_activeMediaSource->activeVideoStreamIndex();
}

bool MediaPlayer::isMSE()
{
    return m_activeMediaSource;
}
}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
