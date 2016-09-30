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

#include "StarFishConfig.h"
#include "MediaPlayer.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "platform/message_loop/MessageLoop.h"
#include "extra/MediaSource.h"

namespace StarFish {

#if !defined(STARFISH_TIZEN)
MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayer(element);
}
#endif

MediaPlayer::MediaPlayer(HTMLMediaElement* element)
    : m_isLooping(false)
    , m_hasVideo(false)
    , m_state(State::STATE_NONE)
    , m_currentPendingOperationCount(0)
    , m_container(element)
    , m_starFish(element->document()->window()->starFish())
    , m_url(nullptr)
{
}

void MediaPlayer::processNextOperationQueue()
{
    if (m_operationQueue.size()) {
        STARFISH_ASSERT(m_currentPendingOperationCount == 0);
        m_currentPendingOperationCount++;
        m_starFish->messageLoop()->addIdler([](size_t, void* data) {
            MediaPlayerOperationQueueData* queueData = (MediaPlayerOperationQueueData*)data;
            queueData->m_mediaPlayer->m_currentPendingOperationCount--;
            queueData->m_mediaPlayer->processOperationQueue(queueData);
        }, m_operationQueue.front());
        m_operationQueue.pop_front();
    }
}

}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
