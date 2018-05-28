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
#if defined(STARFISH_USE_MOCK_MEDIAPLAYER) || !defined(STARFISH_TIZEN)

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource//SourceBuffer.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/MockMediaPlayer.h"
#include "platform/window/PlatformWindow.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/util/URL.h"

#define STARFISH_FRAME_EVICTION_BACKWARD_DUR 500

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#define MOCKPLAYER_LOG(mk, ...)                \
    STARFISH_LOG_INFO("[MockPlayer|%p] ", mk); \
    STARFISH_LOG_INFO(__VA_ARGS__);
#else
#define MOCKPLAYER_LOG(mk, ...)
#endif

namespace StarFish {

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MockMediaPlayer(element);
}

class MediaPlayerMediaSourceClient : public MediaSourceClient {
public:
    MediaPlayerMediaSourceClient(MediaPlayer* player)
        : MediaSourceClient()
        , m_player(player)
    {
#ifndef NDEBUG
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           MOCKPLAYER_LOG(
                                               obj,
                                               "[TRACE_MSE_GC] "
                                               "MediaPlayerMediaSourceClient::~"
                                               "MediaPlayerMediaSourceClient "
                                               "\n");
                                       },
                                       NULL, NULL, NULL);
#endif
    }

    virtual void activeSourceComputed()
    {
        if (m_player && m_player->alive()) {
            m_player->prepareMediaSource();
        }
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
        m_videoStream = new MockMediaStream(StreamTypeVideo);
        StreamInfo* v =
            activeMediaSource()->activeVideoSourceBuffer()->streamInfo(
                0, activeMediaSource()->activeVideoStreamIndex());
        m_videoWidth = v->videoWidth();
        m_videoHeight = v->videoHeight();
        MOCKPLAYER_LOG(this, "Video Info-----------------------------\n");
        MOCKPLAYER_LOG(this, "> codec     : %s\n", v->codecString());
        MOCKPLAYER_LOG(this, "> size      : %dx%d\n", v->videoWidth(),
                       v->videoHeight());
        MOCKPLAYER_LOG(this, "---------------------------------------\n");
    }
    if (activeMediaSource()->activeAudioSourceBuffer()) {
        m_audioStream = new MockMediaStream(StreamTypeAudio);
    }
    if (container()->isHTMLVideoElement() && container()->frame()) {
        container()->setNeedsLayout();
    }

    container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);
    container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_ENOUGH_DATA);
}

void MockMediaPlayer::fillBuffer(MockMediaStream* stream)
{
    SourceBuffer* sb = activeSourceBuffer(stream->type());
    if (!sb) {
        return;
    }
    uint64_t streamIdx = activeStreamIndex(stream->type());
    size_t currentInitIndex = stream->initSegmentIndex();
    uint64_t lastDTS = stream->lastSubmittedDTS();
    MOCKPLAYER_LOG(this, "fillBuffer start %lums (%s)\n", lastDTS,
                   stream->isAudio() ? "AUDIO" : "VIDEO");

    uint64_t submitMS = 0;
    size_t submitCount = 0;
    size_t submitBytes = 0;
    while (submitCount < 100) {
        std::pair<MediaPacket*, size_t> packet =
            sb->findProperMediaPacket(streamIdx, lastDTS);
        if (!packet.first) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            MOCKPLAYER_LOG(this, "fillBuffer try to detect end -> %d %d\n",
                           (int)endTime, (int)lastDTS);
            uint64_t lastBufferedTime = sb->lastBufferedTimestamp(streamIdx);
            if ((endTime - lastDTS) < 10 ||
                ((lastDTS == lastBufferedTime) &&
                 (std::llabs(endTime - lastBufferedTime) < 1000))) {
                stream->setBufferState(MockMediaStream::BUFFERSTATE_EOS);
                MOCKPLAYER_LOG(this, "fillBuffer detect EOS\n");
                break;
            }
            MOCKPLAYER_LOG(this, "fillBuffer runs into under run state[1]\n");
            stream->setWaitingDemuxer(true);
            break;
        }
        if (packet.first->m_dts < lastDTS ||
            packet.first->m_dts - lastDTS > 500) {
            sb->clearPacketAccessCache();
            MOCKPLAYER_LOG(
                this,
                "fillBuffer runs into under run state[2] - requested(%lu) but "
                "returned(%lu)\n",
                lastDTS, packet.first->m_dts);
            stream->setWaitingDemuxer(true);
            break;
        }

        if (packet.second != currentInitIndex) {
            if (!packet.first->m_hasIdr) {
                lastDTS = packet.first->m_dts + packet.first->m_duration;
                MOCKPLAYER_LOG(
                    this, "fillBuffer drops non-idr packet (config changed)\n");
                continue;
            } else {
                MOCKPLAYER_LOG(
                    this,
                    "fillBuffer detect changed config (and will submit packet "
                    "including idr. DTS:%d)\n",
                    (int)packet.first->m_dts);
                if (stream->isVideo()) {
                    StreamInfo* info = sb->streamInfo(
                        packet.second, activeStreamIndex(StreamTypeVideo));
                    m_videoWidth = info->videoWidth();
                    m_videoHeight = info->videoHeight();
                    MOCKPLAYER_LOG(this,
                                   "New Video Info-------------------------\n");
                    MOCKPLAYER_LOG(this, "> size      : %dx%d\n",
                                   info->videoWidth(), info->videoHeight());
                    MOCKPLAYER_LOG(this,
                                   "---------------------------------------\n");
                }
                stream->setInitSegmentIndex(packet.second);
                currentInitIndex = packet.second;
            }
        }
        submitCount++;
        submitMS += packet.first->m_duration;
        submitBytes += packet.first->m_dataSize;
        lastDTS = packet.first->m_dts + packet.first->m_duration;
    }
    stream->setLastSubmittedDTS(lastDTS);
    MOCKPLAYER_LOG(this, "fillBuffer end %lums (count:%d, size:%d)\n\n",
                   lastDTS, (int)submitCount, (int)submitBytes);
}

const uint64_t forwardDuration = 1000;
const uint64_t timerInterval = 250;

void MockMediaPlayer::play()
{
    if (m_playbackState != PLAYBACK_STATE_PLAYING) {
        m_playbackState = PLAYBACK_STATE_PLAYING;
        m_container->document()->browsingContext()->addPointerInRootSet(this);
        m_currentTimeUpdateTimer = window()->setInterval(
            [](Window* window, void* data) {
                MockMediaPlayer* self = (MockMediaPlayer*)data;
                if (self->seeking()) {
                    return;
                }
                MockMediaStream* audioStream =
                    self->currentStream(StreamTypeAudio);
                MockMediaStream* videoStream =
                    self->currentStream(StreamTypeVideo);
                uint64_t currentTime = self->currentTimeInMS();
                uint64_t targetTime = currentTime + timerInterval;
                targetTime = targetTime > self->duration() ? self->duration()
                                                           : targetTime;
                if (audioStream &&
                    audioStream->lastSubmittedDTS() <
                        currentTime + forwardDuration) {
                    self->fillBuffer(audioStream);
                    if (audioStream->lastSubmittedDTS() < targetTime) {
                        MOCKPLAYER_LOG(self, "AUDIO underrun state\n");
                        return;
                    }
                }
                if (videoStream &&
                    videoStream->lastSubmittedDTS() <
                        currentTime + forwardDuration) {
                    self->fillBuffer(videoStream);
                    if (videoStream->lastSubmittedDTS() < targetTime) {
                        MOCKPLAYER_LOG(self, "VIDEO underrun state\n");
                        return;
                    }
                }
                uint64_t newCurrentTime = currentTime + timerInterval;
                self->setCurrentTimeInMS(newCurrentTime);
                if (self->duration() * 1000 - newCurrentTime < 1000) {
                    self->handleEnded();
                } else {
                    self->m_container->setOfficialPlaybackPosition(
                        newCurrentTime / 1000.0);
                }
            },
            timerInterval, this);
    }
}

void MockMediaPlayer::pause()
{
    if (m_playbackState == PLAYBACK_STATE_PLAYING) {
        m_playbackState = PLAYBACK_STATE_PAUSED;
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
            m_mseClient = new MediaPlayerMediaSourceClient(this);
            m_activeMediaSource->addClient(m_mseClient);
            m_activeMediaSource->attach(m_container);
            processNextOperationQueueInContainer();
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

    MessageLoop* msgLoop = m_container->starFish()->messageLoop();
    msgLoop->addIdler(
        m_container->document()->browsingContext(),
        [](size_t, void* data) {
            MockMediaPlayer* self = (MockMediaPlayer*)data;
            self->processNextOperationQueueInContainer();
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_METADATA);
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_ENOUGH_DATA);
        },
        this);
}

void MockMediaPlayer::drawVideo(Compositor* canvas, const LayoutRect& videoRect,
                                const LayoutRect& absVideoRect)
{
    canvas->setColor(Unit::Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
}

void MockMediaPlayer::seek(double time)
{
    MOCKPLAYER_LOG(this, "seek(%f)\n", time);
    STARFISH_ASSERT(!m_seeking);
    double dur = duration();
    if (time < 0) {
        time = 0;
    } else if (dur != 0 && !std::isnan(dur) && time >= dur) {
        MOCKPLAYER_LOG(this, "seek() reaches EOS\n");
        m_container->mediaPlayerNotifySeekedItsContainer(duration());
        handleEnded();
        return;
    }
    m_seeking = true;
    int timeInMS = (int)(time * 1000.0);
    if (m_audioStream) {
        m_audioStream->setLastSubmittedDTS(timeInMS);
        m_audioStream->setWaitingDemuxer(true);
        activeSourceBuffer(StreamTypeAudio)->clearPacketAccessCache();
    }
    if (m_videoStream) {
        m_videoStream->setLastSubmittedDTS(timeInMS);
        m_videoStream->setWaitingDemuxer(true);
        activeSourceBuffer(StreamTypeVideo)->clearPacketAccessCache();
    }

    setCurrentTimeInMS(timeInMS);
    STARFISH_ASSERT(m_seekingTimer == SIZE_MAX);

    m_seekingTimer = window()->setInterval(
        [](Window* window, void* data) {
            MockMediaPlayer* self = (MockMediaPlayer*)data;
            if (self->isMSE()) {
                MockMediaStream* audioStream =
                    self->currentStream(StreamTypeAudio);
                MockMediaStream* videoStream =
                    self->currentStream(StreamTypeVideo);
                uint64_t currentTime = self->currentTimeInMS();
                uint64_t targetTime = currentTime + forwardDuration;
                targetTime = targetTime > self->duration() ? self->duration()
                                                           : targetTime;
                if (audioStream) {
                    self->fillBuffer(audioStream);
                    if (audioStream->lastSubmittedDTS() < targetTime) {
                        return;
                    }
                }
                if (videoStream) {
                    self->fillBuffer(videoStream);
                    if (videoStream->lastSubmittedDTS() < targetTime) {
                        return;
                    }
                }
            }
            self->handleSeeked();
        },
        timerInterval, this);
}

void MockMediaPlayer::handleEnded()
{
    pause();
    if (m_audioStream) {
        m_audioStream->setLastSubmittedDTS(0);
    }
    if (m_videoStream) {
        m_videoStream->setLastSubmittedDTS(0);
    }
    if (activeSourceBuffer(StreamTypeAudio)) {
        activeSourceBuffer(StreamTypeAudio)->clearAll();
    }
    if (activeSourceBuffer(StreamTypeVideo)) {
        activeSourceBuffer(StreamTypeVideo)->clearAll();
    }
    if (m_container) {
        m_container->mediaPlayerNotifyEndedItsContainer();
    }
}
void MockMediaPlayer::handleSeeked()
{
    m_seeking = false;
    window()->clearInterval(m_seekingTimer);
    m_seekingTimer = SIZE_MAX;
    m_container->mediaPlayerNotifySeekedItsContainer(currentTimeInMS() / 1000);
}
void MockMediaPlayer::fillBufferIfNeeded(StreamType type)
{
    MockMediaStream* stream = currentStream(type);
    if (!stream) {
        return;
    }
    if (stream->waitingDemuxer()) {
        stream->setWaitingDemuxer(false);
        fillBuffer(stream);
    }
}
void MockMediaPlayer::close()
{
    m_alive = false;
    pause();
    if (m_activeMediaSource) {
        m_activeMediaSource->removeClient(m_mseClient);
        m_activeMediaSource->detach();
        m_activeMediaSource = nullptr;
    }
    if (m_mseClient) {
        m_mseClient = nullptr;
    }
    m_audioStream = nullptr;
    m_videoStream = nullptr;
    if (m_container) {
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_NOTHING);
    }
    m_container = nullptr;
}
}
#endif
#endif /* STARFISH_ENABLE_MULTIMEDIA */
