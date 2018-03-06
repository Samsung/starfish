/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifdef STARFISH_ENABLE_MULTIMEDIA
#ifndef __StarFishMediaPlayer__
#define __StarFishMediaPlayer__

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

#include "core/dom/HTMLMediaElement.h"
#include "platform/multimedia/StreamInfo.h"

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
        if (!m_hasVideo) {
            return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
        } else {
            return m_videoWidth;
        }
    }

    virtual unsigned long videoHeight()
    {
        if (!m_hasVideo) {
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

    bool isMSE();

    virtual void prepareMediaSource() = 0;

    virtual Window* window();

protected:
    MediaPlayer(HTMLMediaElement* element);
    void updateElementReadyState(HTMLMediaElement::ReadyState state);
    void processNextOperationQueueInContainer();
    void appendToOperationQueueInContainer(MediaOperationQueueData* data);
    SourceBuffer* activeSourceBuffer(StreamType type);
    uint64_t activeStreamIndex(StreamType type);
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
#endif
