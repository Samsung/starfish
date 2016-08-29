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

#include "platform/canvas/Canvas.h"

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
#include <player.h>
#endif

namespace StarFish {

class Document;
class URL;
class PlayerWindowData;
class HTMLElement;

class MediaPlayer : public gc {
public:
    enum Request {
        REQUEST_NONE,
        REQUEST_PLAY,
        REQUEST_STOP,
        REQUEST_PAUSE,
    };

    MediaPlayer();
    virtual bool isVideoPlayer() { return false; }

    virtual void prepareAsync(Document* document, String* path) = 0;
    virtual void destroy() { m_lastRequest = REQUEST_NONE; }
    virtual void play() { m_lastRequest = REQUEST_PLAY; }
    virtual void pause() { m_lastRequest = REQUEST_PAUSE; }
    virtual void stop() { m_lastRequest = REQUEST_STOP; }
    virtual bool isReady() = 0;
    virtual bool isPlaying() = 0;
    virtual bool isPaused() = 0;

    virtual void onPrepared(bool hasError) { }
    virtual void onPlayFinished() { }

    virtual void setLoop(bool loop) = 0;
    bool lastRequestIs(Request r)
    {
        return r == m_lastRequest;
    }

protected:
    URL* m_url;
    Request m_lastRequest;
};

class VideoPlayer : public MediaPlayer {
public:
    virtual bool isVideoPlayer() { return true; }

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
    VideoPlayer(HTMLVideoElement* videoElement);

    virtual void prepareAsync(Document* document, String* path);
    virtual void destroy();
    virtual void play();
    virtual void pause();
    virtual void stop();
    virtual void setLoop(bool loop);
    virtual bool isReady();
    virtual bool isPlaying();
    virtual bool isPaused();
    int width();
    int height();

    void destroyInternal();
    void playInternal();
    void pauseInternal();
    void stopInternal();
    void postPrepare();
    void postPlayFinished();
#ifdef STARFISH_TIZEN_TV
    void setDisplayArea(int x, int y, int width, int height);
#elif STARFISH_TIZEN_MOBILE
    void setVideoSurface(CanvasSurface* surface);
#endif
#else  /* STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) */
    VideoPlayer(HTMLVideoElement* videoElement)
        : MediaPlayer() { }

    virtual void prepareAsync(Document* document, String* path) { onPrepared(true); }
    virtual void destroy() { }
    virtual void play() { }
    virtual void pause() { }
    virtual void stop() { }
    virtual void setLoop(bool loop) { }
    virtual bool isReady() { return false; }
    virtual bool isPlaying() { return false; }
    virtual bool isPaused() { return false; }
    int width() { return 0; }
    int height() { return 0; }
#endif /* STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) */

    virtual void onPrepared(bool hasError);
    virtual void onPlayFinished();

    HTMLVideoElement* videoElement()
    {
        return m_videoElement;
    }

protected:
    HTMLVideoElement* m_videoElement;
#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
    player_h m_player;
#ifdef STARFISH_TIZEN_TV
    Rect m_displayArea;
    int m_videoMixerHandle;
#elif STARFISH_TIZEN_MOBILE
    CanvasSurface* m_surface;
#endif
#endif
};

}

#endif
