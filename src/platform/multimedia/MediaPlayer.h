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
    enum PublicState {
        STATE_NONE = 1 << 0,
        STATE_PREPARING = 1 << 1,
        STATE_READY = 1 << 2,
        STATE_PLAYING = 1 << 3,
        STATE_PAUSED = 1 << 4,
        STATE_UNKNOWN_ERROR = 1 << 5,
    };

    MediaPlayer();
    virtual bool isVideoPlayer() { return false; }

    virtual void prepare() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;

    virtual void setLoop(bool loop) = 0;
    virtual void setURL(URL* url) { m_inputUrl = url; }

    virtual void onPrepared(bool hasError) { }
    virtual void onPlayFinished() { }

    bool isPublicState(unsigned state) { return ((m_state & state) > 0); }
    void setPublicState(PublicState state) { m_state = state; }

protected:
    URL* m_inputUrl;
    PublicState m_state;
};


class VideoPlayer : public MediaPlayer {
public:
    enum Request {
        REQUEST_NONE,
        REQUEST_PLAY,
        REQUEST_PAUSE,
    };

    virtual bool isVideoPlayer() { return true; }

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
public:
    VideoPlayer(HTMLVideoElement* videoElement);
    virtual void prepare();
    virtual void play();
    virtual void pause();

    virtual void setLoop(bool loop);
    virtual void setURL(URL* url)
    {
        if (!url) {
            unprepareCPlayer();
        }
        m_inputUrl = url;
    }

    int width();
    int height();
    void postLoaded();
    void postPlayFinished();

    bool assureCPlayer();
    void destroyCPlayer();

#ifdef STARFISH_TIZEN_TV
    void setDisplayArea(int x, int y, int width, int height);
#elif STARFISH_TIZEN_MOBILE
    void setDisplayArea(CanvasSurface* surface);
#endif
private:
    void prepareCPlayer();
    void unprepareCPlayer();
    void playCPlayer();
    void pauseCPlayer();
    void stopCPlayer();

    void lockElementPointer();
    void unlockElementPointer();

    bool hasPendingUrl();
    void popPendingUrl();
    void pushPendingUrl();
    bool lastRequestIs(Request r) { return r == m_lastRequest; }
#else  /* STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) */
public:
    VideoPlayer(HTMLVideoElement* videoElement)
        : MediaPlayer() { }

    virtual void prepare() { }
    virtual void play() { }
    virtual void pause() { }
    virtual void setLoop(bool loop) { }
    int width() { return 0; }
    int height() { return 0; }
#endif /* STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) */

public:
    virtual void onPrepared(bool hasError);
    virtual void onPlayFinished();
    HTMLVideoElement* videoElement() { return m_videoElement; }

protected:
    HTMLVideoElement* m_videoElement;
    URL* m_currentUrl;
#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
    player_h m_cplayer;
    Request m_lastRequest;
#ifdef STARFISH_TIZEN_TV
    Rect m_displayArea;
#elif STARFISH_TIZEN_MOBILE
    CanvasSurface* m_surface;
#endif
    bool m_isElementPointerLocked;
    bool m_hasPendingUrl;
#endif
};

}

#endif
