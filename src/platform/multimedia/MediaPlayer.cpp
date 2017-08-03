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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLMediaElement.h"
#include "platform/multimedia/MediaPlayer.h"

namespace StarFish {

MediaPlayer::MediaPlayer(HTMLMediaElement* element)
    : m_isLooping(false)
    , m_hasVideo(false)
    , m_inPlaying(false)
    , m_seekState(SEEKSTATE_NO_SEEK)
    , m_playbackState(PLAYBACK_STATE_NONE)
    , m_container(element)
    , m_activeMediaSource(nullptr)
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
}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
