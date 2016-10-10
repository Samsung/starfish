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
#define __StarFishMediaPlayerTizen__

#include "MediaPlayer.h"

#include <media/player.h>

namespace StarFish {

class CanvasSurface;
class MediaSource;

class MediaPlayerTizen : public MediaPlayer {
public:
    MediaPlayerTizen(HTMLMediaElement* element);

    virtual void close();

    virtual void play()
    {
        player_start(m_nativePlayer);
        processNextOperationQueueInContainer();
    }

    virtual void pause()
    {
        player_pause(m_nativePlayer);
        processNextOperationQueueInContainer();
    }

    void setLoop(bool loop) { m_isLooping = true; }
    bool loop()
    {
        return m_isLooping;
    }

    virtual void prepare(URL* url);

    virtual void initDisplay();

    virtual void setNativeOptions(URL* url);

    void pauseOperation();
    void unprepareOperation();

    void openPreparingMode();
    void closePreparingMode();

    void handlePlayerError(int error);

    virtual unsigned long videoWidth()
    {
        if (m_hasVideo) {
            return m_videoWidth;
        } else
            return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual unsigned long videoHeight()
    {
        if (m_hasVideo) {
            return m_videoHeight;
        } else
            return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual double currentTime()
    {
        int s;
        int ret = player_get_play_position(m_nativePlayer, &s);
        if (ret)
            return 0;
        return s / 1000.0;
    }

    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect);

    bool m_inPrepare;
    bool m_alive;
    MediaSource* m_activeMediaSource;
    CanvasSurface* m_canvasSurface;
    player_h m_nativePlayer;
    unsigned long m_videoWidth, m_videoHeight;
};
}

#endif
