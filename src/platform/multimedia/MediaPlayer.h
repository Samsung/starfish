/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#ifndef __StarfishMediaPlayer__
#define __StarfishMediaPlayer__

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

#include "core/dom/HTMLMediaElement.h"
#include "platform/multimedia/StreamInfo.h"

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define PLAYER_LOGI(STR, ...) \
    STARFISH_LOG_INFO(        \
        "[PLAYER_LOG|%ld] "   \
        "" STR,               \
        syscall(SYS_gettid), ##__VA_ARGS__);
#else
#define PLAYER_LOGI(...)
#endif

// PLAYER_LOGE is always on (independent of STARFISH_MEDIAPLAYER_DEBUG).
// Error paths are rare; without this, production triage of media-pipeline
// failures (e.g. silent handlePlayerError → MediaSource::detach on Tizen)
// has no breadcrumbs in the device log.
#define PLAYER_LOGE(STR, ...) \
    STARFISH_LOG_ERROR("[PLAYER_LOG] " STR, ##__VA_ARGS__);

namespace Starfish {

class Canvas;
class CanvasSurface;
class Compositor;
class MediaSource;
class Mutex;
class URL;
class Window;
class MediaPlayerWebRtc;

class MediaPlayer : public gc {
public:
    virtual ~MediaPlayer()
    {
    }
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

    // `url` is the resource selected by HTMLMediaElement (null when not
    // yet known, e.g. srcObject flows); Tizen uses it to route MediaSource
    // playback to the esplusplayer backend.
    static MediaPlayer* create(HTMLMediaElement* element,
                               ResourceURL* url = nullptr);
    static bool isSupport(MediaCodec codec);
    virtual void destroy() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void seek(double time) = 0;

    bool alive()
    {
        return m_alive && !m_foundError;
    }
    void setLoop(bool loop)
    {
        m_isLooping = loop;
    }
    bool loop()
    {
        return m_isLooping;
    }

    virtual bool isWebRtcPlayer()
    {
        return false;
    }

    MediaPlayerWebRtc* asMediaPlayerWebRtc()
    {
        STARFISH_ASSERT(isWebRtcPlayer());
        return (MediaPlayerWebRtc*)this;
    }

    virtual void prepare(ResourceURL* url){};
    virtual double currentTime() = 0;
    virtual double duration() = 0;
    virtual void setVolume(double volume) = 0;
    virtual void setMuted(bool muted) = 0;
    // Optional: backends without native rate support ignore it (the
    // element still reflects the value and fires ratechange).
    virtual void setPlaybackRate(double rate)
    {
    }

    PlaybackState playbackState();
    void setPlaybackState(PlaybackState state);

    virtual void didDrawVideo(Compositor* canvas, const LayoutRect& videoRect,
                              const LayoutRect& absVideoRect) = 0;
    virtual void willDrawVideo(Compositor* canvas,
                               const LayoutRect& videoRect) = 0;
    // Called when the video's stacking context is not composited this frame
    // (scrolled fully off-screen). A HW overlay plane is not driven by the web
    // compositor, so it must be hidden explicitly here; otherwise it stays
    // painted at its last on-screen ROI. No-op unless the platform uses an
    // overlay plane.
    virtual void hideVideoOverlay()
    {
    }

    CanvasSurface* contentSurface();

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

    CanvasSurface* canvasSurface()
    {
        return m_canvasSurface;
    }

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
    Mutex* m_playerStateMutex;
    CanvasSurface* m_canvasSurface;
};
} // namespace Starfish
#endif
#endif
