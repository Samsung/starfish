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

#include <atomic>
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
#define PLAYER_LOGE(...) STARFISH_LOG_ERROR("[PLAYER_LOG] " __VA_ARGS__);

namespace Starfish {

class Canvas;
class CanvasSurface;
class Compositor;
class GL;
class MediaSource;
class Mutex;
class Resource;
class URL;
class WebOrigin;
class Window;
class MediaPlayerWebRtc;
#if defined(STARFISH_ENABLE_WEBAUDIO)
class MediaAudioPlaybackState;
#endif

class MediaPlayer : public gc {
    friend class ResponseURLProbeClient;

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
    // Called instead of seek() when the element already has a seek in
    // flight (m_isSeeking). A backend that can retarget the in-flight
    // native seek should do so and return true; the default keeps the
    // legacy behavior (the element parks the target in m_pendingSeek and
    // chains it after the current seek completes). Without retargeting, a
    // pure push-model backend can deadlock: it waits for data at the OLD
    // target while the page only fetches data for the NEW one.
    virtual bool supersedeSeek(double)
    {
        return false;
    }

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

    virtual bool isWebRtcPlayer()
    {
        return false;
    }

#if defined(STARFISH_ENABLE_WEBAUDIO)
    virtual MediaAudioPlaybackState* audioPlaybackState()
    {
        return nullptr;
    }
#endif

    MediaPlayerWebRtc* asMediaPlayerWebRtc()
    {
        STARFISH_ASSERT(isWebRtcPlayer());
        return (MediaPlayerWebRtc*)this;
    }

    virtual void prepare(ResourceURL* url) {};
    // The URL the media data came from, so the element can decide whether
    // its media data is CORS-same-origin
    // (https://html.spec.whatwg.org/multipage/media.html#media-data-is-cors-same-origin).
    // Empty until a backend vouches for the requested URL, and empty again
    // once it saw the request redirected anywhere (it can't report the hops
    // Fetch judges one by one): the element then treats the media data as
    // cross-origin, the safe side. Backends differ only in their evidence:
    // the FFmpeg player reads libavformat's response location, the Tizen
    // player relies on probeResponseURL().
    Optional<ResourceURL*> responseURL();
    // Whether responseURL() is same-origin with `origin`. Judged once per
    // response: the texture upload path asks for every frame.
    bool responseIsSameOrigin(WebOrigin* origin);
    // Called by a backend (or its native wrapper) with what its network
    // stack tells it. Vouching happens on the main thread once the resource
    // is prepared; revoking may come from a decoding thread.
    void vouchResponseURL(ResourceURL* url);
    void revokeResponseURL();
    bool responseURLVouched() const
    {
        return m_responseURLVouched.load();
    }
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

    // Binds the decoded frame on screen to `texture` of gl's current context,
    // for WebGL uploads: as GL_TEXTURE_EXTERNAL_OES over the decoder's buffer
    // (Tizen; `image` must then be released with gl->xglDestroyImage()) or as
    // an RGBA GL_TEXTURE_2D copy (FFmpeg). `width`/`height` come in as the
    // size of the texture's current 2D storage (0 if none) and go out as the
    // frame size. Returns the target bound, or 0 when no frame is available,
    // e.g. while the video is on the HW overlay plane.
    virtual unsigned bindCurrentVideoFrame(GL* gl, unsigned texture,
                                           size_t& width, size_t& height,
                                           void*& image)
    {
        return 0;
    }

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
    void notifyMediaSourceFailure();
    // For a backend whose player fetches the media itself and can't report
    // where it came from: a HEAD request through the engine's loader, shaped
    // like the player's own (no CORS, no credentials), vouches for `url`
    // when the response came back from it unredirected, with a success
    // status (a server rejecting HEAD leaves the media tainted) and not as
    // a playlist. The player's request is a separate one, so a server
    // answering the two differently defeats the check; platform players in
    // Chromium carry the same trade-off. didProbeResponseURL() follows on
    // the main thread whether or not the probe vouched, unless the player
    // died meanwhile.
    void probeResponseURL(ResourceURL* url);
    virtual void didProbeResponseURL(ResourceURL* url)
    {
    }
    void cancelResponseURLProbe();
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
    ResourceURL* m_judgedResponseURL{ nullptr };
    bool m_judgedSameOrigin{ false };
    Optional<ResourceURL*> m_vouchedResponseURL;
    std::atomic<bool> m_responseURLVouched{ false };
    Optional<Resource*> m_responseURLProbe;
    size_t m_currentTimeUpdateTimer;
    Mutex* m_playerStateMutex;
    CanvasSurface* m_canvasSurface;
};
} // namespace Starfish
#endif
#endif
