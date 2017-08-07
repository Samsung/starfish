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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource//SourceBuffer.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/MockMediaPlayer.h"
#include "platform/window/PlatformWindow.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/util/URL.h"

namespace StarFish {

#if !defined(STARFISH_TIZEN_TV)
MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MockMediaPlayer(element);
}
#endif

class MediaPlayerMediaSourceClient : public MediaSourceClient {
public:
    MediaPlayerMediaSourceClient(MediaPlayer* player)
        : MediaSourceClient()
        , m_player(player)
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

void MockMediaPlayer::play()
{
    if (!m_inPlaying) {
        m_inPlaying = true;
        m_container->document()->browsingContext()->addPointerInRootSet(this);
        m_currentTimeUpdateTimer = window()->setInterval(
            [](Window* window, void* data) {
                MockMediaPlayer* self = (MockMediaPlayer*)data;

                if (self->activeMediaSource()) {
                    uint64_t videoStart = self->m_currentTimestamp;
                    uint64_t audioStart = self->m_currentTimestamp;
                    while (self->m_currentTimestamp - videoStart < 250) {
                        std::pair<MediaPacket*, size_t> packet =
                            self->activeMediaSource()
                                ->activeVideoSourceBuffer()
                                ->findProperMediaPacket(
                                    self->activeMediaSource()
                                        ->activeVideoStreamIndex(),
                                    self->m_currentTimestamp);
                        if (!packet.first) {
                            break;
                        }
                        self->m_currentTimestamp =
                            packet.first->m_pts + packet.first->m_duration;
                    }

                    while (audioStart < self->m_currentTimestamp) {
                        std::pair<MediaPacket*, size_t> packet =
                            self->activeMediaSource()
                                ->activeAudioSourceBuffer()
                                ->findProperMediaPacket(
                                    self->activeMediaSource()
                                        ->activeAudioStreamIndex(),
                                    audioStart);
                        if (!packet.first) {
                            break;
                        }
                        audioStart =
                            packet.first->m_pts + packet.first->m_duration;
                    }
                } else {
                    self->m_currentTimestamp += 250;
                }

                if (self->m_currentTimestamp > self->duration() * 1000) {
                    self->m_currentTimestamp = self->duration() * 1000;
                }
                STARFISH_LOG_INFO("MockMediaPlayer currentTimeStamp %fs\n",
                                  self->m_currentTimestamp / 1000.f);

                if (self->duration() * 1000 - self->m_currentTimestamp < 1000) {
                    self->pause();
                    self->m_container->mediaPlayerNotifyEndedItsContainer();
                }

                self->m_container->setOfficialPlaybackPosition(
                    self->m_currentTimestamp / 1000.0);
            },
            250, this);
    }
}

void MockMediaPlayer::pause()
{
    if (m_inPlaying) {
        m_inPlaying = false;
        m_container->document()->browsingContext()->removePointerFromRootSet(
            this);
        window()->clearInterval(m_currentTimeUpdateTimer);
        m_currentTimeUpdateTimer = SIZE_MAX;
    }
}

void MockMediaPlayer::prepare(ResourceURL* url)
{
    if (url->isBlobURL()) {
        BlobURLStore store;
        if (!WebView::stringToBlobURLString(url->urlString(), store)) {
            return;
        }
        if (m_container->webView()->isValidMediaSourceBlobURL(store)) {
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

void MockMediaPlayer::drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                                const LayoutRect& absVideoRect)
{
    canvas->setColor(Unit::Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
}
}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
