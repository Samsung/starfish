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

    virtual void play()
    {
        player_state_e state;
        player_get_state(m_nativePlayer, &state);
        STARFISH_LOG_INFO("MediaPlayerTizen::play() state : %d state2: %d ms: %p\n", (int)state, (int)m_playbackState, m_activeMediaSource);
        if (m_activeMediaSource && m_playbackState == PlaybackState::PLAYBACK_STATE_END) {
            STARFISH_LOG_INFO("MediaPlayerTizen::play() meets mse && playback end\n");
            int ret;
            unprepareOperation();
            STARFISH_LOG_INFO("MediaPlayerTizen::play() unprepare end %d\n", (int)ret);
            ret = player_destroy(m_nativePlayer);
            STARFISH_LOG_INFO("MediaPlayerTizen::play() destory end %d\n", (int)ret);
            ret = player_create(&m_nativePlayer);
            STARFISH_LOG_INFO("MediaPlayerTizen::play() create end %d\n", (int)ret);
            m_playbackState = PLAYBACK_STATE_NONE;
            m_needsPlayAfterPrepare = true;
            prepare(m_currentURL);
        } else {
            player_start(m_nativePlayer);
            seekIfNeeded();
        }
    }

    void seekIfNeeded();

    virtual void pause()
    {
        player_pause(m_nativePlayer);
    }

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
    void pauseOperation();
    void unprepareOperation();

    void openPreparingMode();
    void closePreparingMode();
    void compleatePrepare();
    void endOfStream();

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
    virtual TimeRanges* buffered();

    bool m_inPrepare;
    bool m_alive;
    bool m_isVideoBufferUnderrunState;
    bool m_isAudioBufferUnderrunState;
    bool m_needsPlayAfterPrepare;
    MediaSource* m_activeMediaSource;
    MediaPlayerTizenMediaSourceClient* m_mseClient;
    Mutex* m_videoBufferMutex;
    Mutex* m_audioBufferMutex;
    URL* m_currentURL;
    void (*m_preparedCallback)(void*);
    CanvasSurface* m_canvasSurface;
    player_h m_nativePlayer;
    unsigned long m_videoWidth, m_videoHeight;
};
}

#endif
