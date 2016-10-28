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
class MediaPlayerTizenMediaSourceClient;

class MediaPlayerTizen : public MediaPlayer {
public:
    friend class MediaPlayerTizenMediaSourceClient;
    MediaPlayerTizen(HTMLMediaElement* element);

    virtual void close();
    virtual void play();
    virtual void pause();
    void seekIfNeeded();

    void setLoop(bool loop) { m_isLooping = true; }
    bool loop()
    {
        return m_isLooping;
    }

    virtual void prepare(URL* url);
    virtual void initDisplay();
    virtual void setNativePlayerDefaultOptions(URL* url);
    virtual void prepareMediaSource();
    virtual void printNativePlayerError(int errorCode);
    virtual void fillVideoBuffer(bool useLock = true);
    virtual void fillAudioBuffer(bool useLock = true);
    virtual void mediaEndOperation()
    {
        player_stop(m_nativePlayer);
    }
    void fillVideoBufferIfNeeded();
    void fillAudioBufferIfNeeded();
    void unprepareOperation();

    void openPreparingMode();
    void closePreparingMode();
    void compleatePrepare();
    void endOfStream();

    void startPlaying();
    void stopPlaying();

    void handleEnded();
    void handleSeekend();
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

    virtual double duration();
    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect);

    bool m_inPrepare;
    bool m_inPlaying;
    bool m_alive;
    bool m_isVideoBufferUnderrunState;
    bool m_isAudioBufferUnderrunState;
    bool m_needsPlayAfterPrepare;
    double m_seekTimeAfterPrepare;
    MediaPlayerTizenMediaSourceClient* m_mseClient;
    Mutex* m_videoBufferMutex;
    Mutex* m_audioBufferMutex;
    URL* m_currentURL;
    void (*m_preparedCallback)(void*);
    CanvasSurface* m_canvasSurface;
    player_h m_nativePlayer;
    size_t m_currentTimeUpdateTimer;
    unsigned long m_videoWidth, m_videoHeight;
};
}

#endif
