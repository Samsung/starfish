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
#include "MockMediaPlayer.h"
#include "util/URL.h"
#include "platform/message_loop/MessageLoop.h"
#include "extra/MediaSource.h"

namespace StarFish {

#if !defined(STARFISH_TIZEN)
MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MockMediaPlayer(element);
}
#endif

class MediaPlayerMediaSourceClient : public MediaSourceClient {
public:
    MediaPlayerMediaSourceClient(MediaPlayer* player)
        : MediaSourceClient(), m_player(player)
    {
#ifndef NDEBUG
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           STARFISH_LOG_INFO(
                                               "[TRACE_MSE_GC] "
                                               "MediaPlayerMediaSourceClient::~"
                                               "MediaPlayerMediaSourceClient "
                                               "(%p)\n",
                                               obj);
                                       },
                                       NULL, NULL, NULL);
#endif
    }

    virtual void activeSourceComputed()
    {
        m_player->prepareMediaSource();
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
    }

    MediaPlayer* m_player;
};

void MockMediaPlayer::prepareMediaSource()
{
    if (activeMediaSource()->activeVideoSourceBuffer()) {
        m_hasVideo = true;
    }
    VideoStreamInfo* v =
        (VideoStreamInfo*)activeMediaSource()
            ->activeVideoSourceBuffer()
            ->streamInfo(0, activeMediaSource()->activeVideoStreamIndex());
    m_videoWidth = v->m_width;
    m_videoHeight = v->m_height;
    processNextOperationQueueInContainer();

    if (container()->isHTMLVideoElement() && container()->frame()) {
        container()->setNeedsLayout();
    }

    container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);
    container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_FUTURE_DATA);
}

void MockMediaPlayer::prepare(URL* url)
{
    if (url->isBlobURL()) {
        BlobURLStore store;
        if (!StarFish::stringToBlobURLString(url->urlString(), store)) {
            return;
        }
        if (m_starFish->isValidMediaSourceBlobURL(store)) {
            m_activeMediaSource = ((MediaSource*)store.m_blob);
            m_activeMediaSource->addClient(
                new MediaPlayerMediaSourceClient(this));
            m_activeMediaSource->attach(m_container);
            return;
        }
    }

    if (container()->isHTMLVideoElement()) {
        m_hasVideo = true;
        if (container()->frame()) {
            container()->setNeedsLayout();
        }
    }
    m_videoWidth = STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
    m_videoHeight = STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
    container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);
    container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_FUTURE_DATA);
}
}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
