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
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "MediaPlayer.h"
#include "util/URL.h"
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
    , m_playbackState(PLAYBACK_STATE_NONE)
    , m_container(element)
    , m_activeMediaSource(nullptr)
    , m_starFish(element->document()->window()->starFish())
{
#ifndef NDEBUG
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        STARFISH_LOG_INFO("[TRACE_MSE_GC] MediaPlayer::~MediaPlayer (%p)\n", obj);
    }, NULL, NULL, NULL);
#endif
}

void MediaPlayer::processNextOperationQueueInContainer()
{
    m_container->processNextOperationQueue();
}

void MediaPlayer::prepare(URL* url)
{
    if (url->isBlobURL()) {
        BlobURLStore store;
        if (!StarFish::stringToBlobURLString(url->urlString(), store)) {
            return;
        }
        if (m_starFish->isValidMediaSourceBlobURL(store)) {
            ((MediaSource*)store.m_blob)->attach(m_container);
        }
    }
}

}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
