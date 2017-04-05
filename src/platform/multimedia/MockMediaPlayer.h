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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishMediaPlayer__)
#define __StarFishMockMediaPlayer__

#include "platform/multimedia/MediaPlayer.h"

namespace StarFish {

class MockMediaPlayer : public MediaPlayer {
    friend class MediaPlayer;

public:
    virtual void close()
    {
        pause();
    }

    virtual void play()
    {
        if (!m_inPlaying) {
            m_inPlaying = true;
            m_starFish->addPointerInRootSet(this);
            m_currentTimeUpdateTimer = m_starFish->window()->setInterval(
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

                    if (self->duration() * 1000 - self->m_currentTimestamp <
                        1000) {
                        self->pause();
                        self->m_container->mediaPlayerNotifyEndedItsContainer();
                    }

                    self->m_container->setOfficialPlaybackPosition(
                        self->m_currentTimestamp / 1000.0);
                },
                250, this);
        }
    }

    virtual void pause()
    {
        if (m_inPlaying) {
            m_inPlaying = false;
            m_starFish->removePointerFromRootSet(this);
            m_starFish->window()->clearInterval(m_currentTimeUpdateTimer);
            m_currentTimeUpdateTimer = SIZE_MAX;
        }
    }

    virtual void seek(double time)
    {
        m_currentTimestamp = time * 1000;
    }

    virtual void prepare(URL* url);
    virtual double currentTime()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual double duration()
    {
        if (m_activeMediaSource) {
            return activeMediaSource()->duration();
        }

        // fake time to test
        return 60;
    }
    virtual void setVolume(double volume)
    {
    }
    virtual void setMuted(bool muted)
    {
    }
    virtual void prepareMediaSource();

    PlaybackState playbackState()
    {
        return m_playbackState;
    }

    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                           const LayoutRect& absVideoRect)
    {
        canvas->setColor(Unit::Color(0, 0, 0, 255));
        canvas->drawRect(videoRect);
    }

protected:
    MockMediaPlayer(HTMLMediaElement* element)
        : MediaPlayer(element)
        , m_currentTimestamp(0)
    {
    }
    uint64_t m_currentTimestamp;
};
}

#endif
