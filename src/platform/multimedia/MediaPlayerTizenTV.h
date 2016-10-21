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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishMediaPlayerTizen__)
#define __StarFishMediaPlayerTizenTV__

#include "MediaPlayerTizen.h"

#include <media/player.h>

namespace StarFish {

class CanvasSurface;
class MediaSource;
class MediaPlayerTizenMediaSourceClient;

class MediaPlayerTizenTV : public MediaPlayerTizen {
public:
    MediaPlayerTizenTV(HTMLMediaElement* element)
        : MediaPlayerTizen(element)
    {
        m_lastVideoPts = m_lastAudioPts = element->defaultPlaybackStartPosition() * 1000;
    }

    virtual void initDisplay()
    {
    }

    virtual void setNativePlayerDefaultOptions(URL* url);
    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect);
    virtual double currentTime();
    virtual void fillVideoBuffer(bool useLock = true);
    virtual void fillAudioBuffer(bool useLock = true);
    virtual void prepareMediaSource();
    virtual void mediaEndOperation()
    {
        m_lastAudioPts = m_lastVideoPts = 0;
        player_set_position(m_nativePlayer, 0, 0, 0);
        // player_set_play_position(m_nativePlayer, 0, 0, 0);
    }
    virtual void seek(double time);

    uint64_t m_lastVideoPts;
    uint64_t m_lastAudioPts;
};

}

#endif
