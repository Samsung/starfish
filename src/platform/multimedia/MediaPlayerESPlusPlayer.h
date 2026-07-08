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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && defined(STARFISH_TIZEN) && \
    defined(STARFISH_USE_ESPLUSPLAYER) &&                             \
    !defined(__StarfishMediaPlayerESPlusPlayer__)
#define __StarfishMediaPlayerESPlusPlayer__

#include "platform/multimedia/MediaPlayer.h"

#include <array>
#include <condition_variable>
#include <mutex>

#include <esplusplayer_capi.h>
#include <tbm_surface.h>

namespace Starfish {

class HTMLMediaElement;
class MediaPlayerESPPMediaSourceClient;
class Mutex;
class ResourceURL;
class Thread;

// Per-stream feed state for the esplusplayer MSE path. Unlike
// MediaPlayerSourceStream (capi path) there is no media_format handle;
// stream configuration is a plain struct passed once to
// esplusplayer_set_*_stream_info.
class ESPPSourceStream : public gc {
public:
    ESPPSourceStream(StreamType type)
        : m_type(type)
        , m_lastSubmittedDTS(0)
        , m_maxBufferSize(0)
        , m_initSegmentIndex(0)
        , m_waitingDemuxer(false)
        , m_eosSubmitted(false)
        , m_shouldFeed(false)
    {
    }
    bool isAudio()
    {
        return m_type == StreamTypeAudio;
    }

    StreamType m_type;
    volatile uint64_t m_lastSubmittedDTS;
    volatile uint64_t m_maxBufferSize;
    volatile size_t m_initSegmentIndex;
    // No packet available from the demuxer at the current cursor; cleared
    // when the SourceBuffer gets new data (MediaSourceClient callback).
    volatile bool m_waitingDemuxer;
    volatile bool m_eosSubmitted;
    // Driven by esplusplayer: ready_to_prepare/ready_to_seek turn it on,
    // the byte-status callback toggles it around the low/high watermarks.
    volatile bool m_shouldFeed;
    // Video only: after a seek the submission cursor must snap back to
    // the closest keyframe at or before the target before anything is
    // submitted (the decoder cannot preroll from a non-IDR packet). The
    // matching keyframe may not be buffered yet when ready_to_seek fires
    // (e.g. a quality switch clears the buffer and re-appends), so the
    // alignment is retried at fill time until a keyframe is found.
    volatile bool m_needIdrAlign{ false };
    // monotonic ms of the last rejected submit (FULL etc.); 0 = none.
    // Set when the stream is parked after a rejection, cleared when the
    // byte-status callback reports room or the feed-thread backoff expires.
    volatile uint64_t m_lastSubmitRejectedMs{ 0 };
    volatile uint64_t m_lastBufferStatusLogMs{ 0 };
    // False until the first packet is submitted after the player is
    // (re)created. While false the skip-ahead is UNCAPPED: the cursor starts
    // at 0 but the freshly (re)attached source buffer can begin anywhere --
    // ~20s at a cold start, or the current playback position (100s+) when the
    // element rebuilt the player mid-playback -- and there is no stale data to
    // guard against yet, so the feed must jump to wherever the buffer is or
    // the player never prerolls. Once true, the steady skip-ahead cap applies
    // (wide enough for post-seek segment alignment / eviction gaps, far below
    // the stale scrub/pre-seek leftover it must reject). Set on the first
    // accepted packet.
    volatile bool m_everSubmitted{ false };
};

// MSE-only media player backed by esplusplayer (ES push player used by
// chromium-efl for MSE). URL/Blob playback stays on MediaPlayerTizen
// (capi-media-player); esplusplayer has no URI mode.
// Reference implementation: chromium-efl
// tizen_src/chromium_impl/media/filters/media_player_esplusplayer.cc.
class MediaPlayerESPlusPlayer : public MediaPlayer {
public:
    friend class MediaPlayerESPPMediaSourceClient;

    MediaPlayerESPlusPlayer(HTMLMediaElement* element);

    virtual void destroy() override;
    virtual void play() override;
    virtual void pause() override;
    virtual void seek(double time) override;
    virtual double currentTime() override;
    virtual double duration() override;
    virtual void setVolume(double volume) override;
    virtual void setMuted(bool muted) override;
    virtual void setPlaybackRate(double rate) override;
    virtual void willDrawVideo(Compositor* canvas,
                               const LayoutRect& videoRect) override;
    virtual void didDrawVideo(Compositor* canvas, const LayoutRect& videoRect,
                              const LayoutRect& absVideoRect) override;
    virtual void hideVideoOverlay() override;
    virtual void prepare(ResourceURL* url) override;
    virtual void prepareMediaSource() override;

    void fillBufferIfNeeded(StreamType type);
    // Called from a file-scope C callback trampoline, hence public.
    void handleBufferByteStatus(StreamType type,
                                esplusplayer_buffer_status status,
                                uint64_t bytes);

private:
    bool createAndOpenPlayer();
    bool initAudioStreamInfo();
    bool initVideoStreamInfo();
    // Overlay (video hole) mode: the decoded-frame callback is not
    // available on every profile (trackrenderer reports "Not supported on
    // Public" on e.g. Family Hub mobile images), so the HW overlay plane +
    // punch-hole is the primary rendering path, exactly like the capi
    // backend's overlay mode.
    bool videoOverlayEnabled();
    bool setupOverlayDisplay();
    void setOverlayPlaneVisible(bool visible);
    void punchHole(Compositor* canvas, const LayoutRect& videoRect,
                   const LayoutRect& absVideoRect);
    void startFeedThread();
    void startCurrentTimeUpdateTimer();
    void stopCurrentTimeUpdateTimer();
    void fillBuffer(ESPPSourceStream* stream);
    void fillBufferWithoutGuard(ESPPSourceStream* stream);
    void dispose();

    // esplusplayer callback handlers. The raw callbacks arrive on player
    // threads; handlers that touch the DOM hop to the main thread via
    // message-loop idlers.
    void handleReadyToPrepare(StreamType type);
    void handlePrepared(bool success);
    void handleReadyToSeek(StreamType type, uint64_t timeMs);
    void handleSeeked(bool success);
    void handleEnded();
    void handlePlayerError();
    void handleDecodedFrame(const esplusplayer_decoded_video_packet* packet);
    void requestCompositeForVideoFrame();

    ESPPSourceStream* currentStream(StreamType type)
    {
        return type == StreamTypeAudio ? m_audioStream : m_videoStream;
    }

    void wakeMseThread()
    {
        {
            std::lock_guard<std::mutex> lock(m_mseWakeMutex);
            m_mseWakePending = true;
        }
        m_mseWakeCv.notify_one();
    }

    static void* threadFillingBuffer(void* data);

    esplusplayer_handle m_player;
    ESPPSourceStream* m_audioStream;
    ESPPSourceStream* m_videoStream;
    MediaPlayerESPPMediaSourceClient* m_mseClient;

    bool m_inPrepare : 1;
    bool m_pendingPlay : 1;
    // esplusplayer_start was already called once; play() after that must
    // use esplusplayer_resume.
    bool m_started : 1;
    // Selected once in createAndOpenPlayer (same source of truth as the
    // capi path: STARFISH_VIDEO_OVERLAY env override, then the WebView
    // setting).
    bool m_overlayMode : 1;
    // Tracks the HW overlay plane visibility so scroll-driven show/hide is
    // only pushed to the player on a change.
    bool m_overlayPlaneVisible : 1;
    // Once esplusplayer_set_video_roi fails we stop attempting source crop.
    bool m_videoSourceROIUnsupported : 1;
    LayoutRect m_lastAbsoluteROIArea;
    std::array<double, 4> m_lastVideoSourceROI{ { 0.0, 0.0, 1.0, 1.0 } };
    volatile bool m_prepared;
    double m_pendingSeekTime; // NaN = none
    // Target of the seek currently in flight; reported back to the element
    // on completion (the element matches it against its pending target).
    double m_lastSeekTargetTime{ 0 };
    // Last position (ms) returned by esplusplayer_get_playing_time.
    // esplusplayer_get_playing_time must not be called before the player
    // is prepared (it can block), so currentTime() falls back to this.
    volatile uint64_t m_lastPositionMs;
    double m_volume;
    bool m_muted;
    // Applied on prepare completion when set earlier (the native API only
    // accepts a rate in READY/PAUSED/PLAYING).
    double m_playbackRate;

    Mutex* m_fillBufferMutex;

    Mutex* m_decodedVideoFrameMutex;
    // Owned by esplusplayer; released with esplusplayer_decoded_buffer_destroy
    // when replaced by the next frame or on dispose. The buffer type is COPY,
    // so holding it does not pin the HW decoder's output pool.
    esplusplayer_decoded_video_packet* m_lastDecodedPacket;

    Mutex* m_setNeedsCompositeEventIdlerHandleMutex;
    volatile size_t m_setNeedsCompositeEventIdlerHandle;

    // Rejected-submit counter, only for throttled logging.
    unsigned m_submitRejectLogCount{ 0 };
    Thread* m_mseThread;
    volatile bool* m_playerDeadFlag;
    std::mutex m_mseWakeMutex;
    std::condition_variable m_mseWakeCv;
    bool m_mseWakePending{ false };
};

} // namespace Starfish

#endif
