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
#define __StarFishMediaPlayer__

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

#include "core/dom/HTMLMediaElement.h"

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define PLAYER_LOGI(...)                                         \
    STARFISH_LOG_INFO("[PLAYER_LOG|%ld] ", syscall(SYS_gettid)); \
    STARFISH_LOG_INFO(__VA_ARGS__);
#define PLAYER_LOGE(...) PLAYER_LOGI(__VA_ARGS__)
#else
#define PLAYER_LOGI(...)
#define PLAYER_LOGE(...)
#endif

namespace StarFish {

class Canvas;
class CanvasSurface;
class Compositor;
class MediaSource;
class URL;
class Window;

class MediaPlayer : public gc {
public:
    enum PlaybackState {
        PLAYBACK_STATE_NONE = 1 << 0,
        PLAYBACK_STATE_PLAYING = 1 << 1,
        PLAYBACK_STATE_PAUSED = 1 << 2,
        PLAYBACK_STATE_END = 1 << 3 | PLAYBACK_STATE_PAUSED,
    };
    enum SeekState {
        SEEKSTATE_NO_SEEK,
        SEEKSTATE_SEEKING, // Waiting first callback
        SEEKSTATE_WAITING, // Waiting second callback (for Tizen2.4 TV)
    };

    static MediaPlayer* create(HTMLMediaElement* element);
    virtual void close() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void seek(double time) = 0;

    bool alive()
    {
        return m_alive && !m_foundError;
    }
    virtual void setLoop(bool loop)
    {
        m_isLooping = loop;
    }
    bool loop()
    {
        return m_isLooping;
    }

    virtual void prepare(ResourceURL* url) = 0;
    virtual double currentTime() = 0;
    virtual double duration() = 0;
    virtual void setVolume(double volume) = 0;
    virtual void setMuted(bool muted) = 0;

    PlaybackState playbackState()
    {
        return m_playbackState;
    }

    virtual void drawVideo(Compositor* canvas, const LayoutRect& videoRect,
                           const LayoutRect& absVideoRect) = 0;

    virtual CanvasSurface* createGraphicsBuffer(size_t visibleWidth,
                                                size_t visibleHeight);

    virtual unsigned long videoWidth()
    {
        if (m_hasVideo) {
            return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
        } else {
            return m_videoWidth;
        }
    }

    virtual unsigned long videoHeight()
    {
        if (m_hasVideo) {
            return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
        } else {
            return m_videoHeight;
        }
    }

    MediaSource* activeMediaSource()
    {
        return m_activeMediaSource;
    }

    HTMLMediaElement* container()
    {
        return m_container;
    }

    bool seeking()
    {
        return (m_seekState != SEEKSTATE_NO_SEEK);
    }

    virtual void prepareMediaSource() = 0;

    virtual Window* window();

protected:
    MediaPlayer(HTMLMediaElement* element);
    void updateElementReadyState(HTMLMediaElement::ReadyState state);
    void processNextOperationQueueInContainer();
    void appendToOperationQueueInContainer(MediaOperationQueueData* data);
    bool m_alive;
    bool m_foundError;
    bool m_isLooping;
    bool m_hasVideo;
    SeekState m_seekState;
    PlaybackState m_playbackState;
    HTMLMediaElement* m_container;
    MediaSource* m_activeMediaSource;
    volatile unsigned long m_videoWidth, m_videoHeight;
    size_t m_currentTimeUpdateTimer;
};
}

#endif
