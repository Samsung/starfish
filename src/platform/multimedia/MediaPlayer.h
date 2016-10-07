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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishMediaPlayer__)
#define __StarFishMediaPlayer__

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

#include "util/URL.h"

namespace StarFish {

class Document;
class URL;
class PlayerWindowData;
class HTMLElement;
class HTMLMediaElement;
class MediaPlayer;
class Canvas;

class MediaPlayer : public gc {
public:
    enum PlaybackState {
        PLAYBACK_STATE_NONE = 1 << 0,
        PLAYBACK_STATE_PLAYING = 1 << 1,
        PLAYBACK_STATE_PAUSED = 1 << 2,
        PLAYBACK_STATE_END = 1 << 3 | PLAYBACK_STATE_PAUSED,
    };

    static MediaPlayer* create(HTMLMediaElement* element);
    virtual void close()
    {
    }

    virtual void play()
    {
    }

    virtual void pause()
    {
    }

    void setLoop(bool loop) { m_isLooping = true; }
    bool loop()
    {
        return m_isLooping;
    }

    virtual void prepare(URL* url)
    {
    }

    virtual double currentTime()
    {
        return 0;
    }

    PlaybackState playbackState()
    {
        return m_playbackState;
    }

    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
    {
    }

    virtual unsigned long videoWidth()
    {
        return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual unsigned long videoHeight()
    {
        return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
    }

protected:
    MediaPlayer(HTMLMediaElement* element);
    void updateElementReadyState(HTMLMediaElement::ReadyState state);
    void processNextOperationQueueInContainer();
    bool m_isLooping;
    bool m_hasVideo;
    PlaybackState m_playbackState;
    HTMLMediaElement* m_container;
    StarFish* m_starFish;
};

}

#endif
