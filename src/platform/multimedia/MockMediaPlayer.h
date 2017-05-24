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

#include "StarFishConfig.h"
#include "platform/multimedia/MediaPlayer.h"

namespace StarFish {

class Canvas;
class HTMLMediaElement;
class URL;

class MockMediaPlayer : public MediaPlayer {
    friend class MediaPlayer;

public:
    virtual void close()
    {
        pause();
    }

    virtual void play();

    virtual void pause();

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
                           const LayoutRect& absVideoRect);

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
