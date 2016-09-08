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

#include "util/URL.h"
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
        STATE_WAITING_FOR_MEDIASOURCE_READY = 1 << 5,
        STATE_UNKNOWN_ERROR = 1 << 7,
    };

    enum Error {
        PLAYER_ERROR_UNKNOWN,
    };

    MediaPlayer();
    virtual bool isVideoPlayer() { return false; }

    virtual void prepare() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;

    virtual void setLoop(bool loop) = 0;
    virtual void setURL(URL* url) { m_inputUrl = url; }
    URL* currentURL() { return m_currentUrl; }

    virtual void onUnprepared() { }
    virtual void onPrepared(URL* url) { }
    virtual void onPlayFinished(URL* url) { }

    bool isPublicState(unsigned state) { return ((m_state & state) > 0); }
    void setPublicState(PublicState state) { m_state = state; }

protected:
    URL* m_inputUrl; // the URL has been accepted recently
    URL* m_currentUrl; // the URL is being processed currently
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

    void prepareCBShouldBeExecutedInMainThread();
    void playFinishedCBShouldBeExecutedInMainThread();

    bool assureCPlayer();
    void destroyCPlayer();

    void setVideoStreamInfo(String* type, int width, int height, int den, int num);
    void notifyInitialPacketReady(MediaSource* ms);
    void pushVideoPacket(uint8_t *buf, uint32_t len, uint64_t pts);
    void pushAudioPacket(uint8_t *buf, uint32_t len, uint64_t pts);

#ifdef STARFISH_TIZEN_TV
    void setDisplayArea(int x, int y, int width, int height);
#elif STARFISH_TIZEN_MOBILE
    void setDisplayArea(CanvasSurface* surface);
#endif
private:
    void prepareCPlayer();
    void startLoadingCPlayer();
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
    void clearVideoStreamInfo();

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

    void setVideoStreamInfo(String* type, int width, int height, int den, int num) { }
    void notifyInitialPacketReady(MediaSource* ms) { }
    void pushVideoPacket(uint8_t *buf, uint32_t len, uint64_t pts) { }
    void pushAudioPacket(uint8_t *buf, uint32_t len, uint64_t pts) { }
#endif /* STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) */

public:
    virtual void onUnprepared();
    virtual void onPrepared(URL* url);
    virtual void onPlayFinished(URL* url);
    void onBufferNeedVideoData(MediaSource* ms);
    void onBufferNeedAudioData(MediaSource* ms);
    HTMLVideoElement* videoElement() { return m_videoElement; }
    MediaSource* currentMediaSource() { return m_currentMediaSource; }

protected:
    HTMLVideoElement* m_videoElement;
    MediaSource* m_currentMediaSource;
#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
    player_h m_cplayer;
    Request m_lastRequest;
#ifdef STARFISH_TIZEN_TV
    Rect m_displayArea;
#elif STARFISH_TIZEN_MOBILE
    CanvasSurface* m_surface;
#endif
    player_video_stream_info_s m_videoInfo;

    bool m_isElementPointerLocked;
    bool m_hasPendingUrl;
    bool m_initialPacketReady;
#endif
};

}

#endif
