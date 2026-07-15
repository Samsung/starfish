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
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN) && \
    defined(STARFISH_USE_ESPLUSPLAYER)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/renderer/Renderer.h"
#include "platform/multimedia/MediaPlayerESPlusPlayer.h"

// esplusplayer_set_ecore_display takes an Ecore_Wl2_Window*; the extended
// display APIs live in the internal header (same includes as chromium-efl's
// media_player_esplusplayer.cc).
#include <esplusplayer_internal.h>
#include <Ecore_Wl2.h>

namespace Starfish {

// Feed budget per fillBuffer call, as a fraction of the stream's native
// buffer budget (same policy as the capi path).
#define STARFISH_ESPP_SUBMIT_BYTES_RATE 0.3
// Never let one stream's submission pointer run further than this ahead
// of the other (same value as capi path / chromium-efl kMaxDiff).
#define STARFISH_ESPP_MAX_AV_DIFF_IN_MS 1500
// Feed thread poll period (microseconds), condvar-woken earlier.
#define STARFISH_ESPP_FEED_WAIT_US (1000 * 25)
// Native stream buffer budgets, mirroring chromium-efl
// kPlayerTotalBufferSize / kPlayerAudioBufferSize.
#define STARFISH_ESPP_TOTAL_BUFFER_SIZE (64 * 1024 * 1024)
#define STARFISH_ESPP_AUDIO_BUFFER_SIZE (768 * 1024)
// Underrun threshold registered with esplusplayer for status callbacks.
// Despite the *_MIN_BYTE_THRESHOLD option name, buffer.h documents this
// value as a PERCENT of MAX_BYTE_SIZE, not an absolute byte count: the
// player emits UNDERRUN whenever the queued bytes drop below this percent
// of the max. The old value of 100 was meant as "100 bytes" but the API
// read it as 100%, so the player treated the buffer as underrun unless it
// was completely full -- a perpetual-UNDERRUN storm that kept the audio
// renderer stuck in rebuffering and made playback cut in and out. 10%
// leaves a healthy cushion (~4.8s of the 768KB audio budget) before a
// refill is requested.
#define STARFISH_ESPP_MIN_BYTE_THRESHOLD 10
// Max distance a post-seek keyframe may sit behind the seek target. Must
// exceed one video GOP: YouTube VP9 keyframes are ~13.8s apart (device-
// measured), so a target lands up to a full GOP after the nearest preceding
// keyframe. An earlier 12000 cut below the GOP and rejected the legitimate
// GOP-start align, wedging the seek. A match farther back than this means
// the target segment is not buffered yet, so wait rather than feed the wrong
// spot; the hundreds-of-seconds stale-prefetch gap stays well outside.
#define STARFISH_ESPP_SEEK_IDR_MAX_LOOKBACK_MS 20000
// Same bound for the forward direction (true forward seek where the page
// appends the target segment starting at a keyframe after the target) and
// for the mid-stream skip-ahead. Without a bound the feeder submits
// whatever stale pre-seek prefetch is still buffered (observed: seek(15s)
// feeding video from 233.7s / audio from 222.3s), which poisons the
// esplusplayer preroll and turns a 1s rebuffer into a 10s+ stall or a
// full wedge. Anything farther than this from the cursor means the target
// data has not been appended yet: wait for the demuxer instead. Sized to a
// full GOP like the lookback: device logs showed seek(120s) landing in a
// buffer hole whose next keyframe was 133.8s (13.8s ahead), just past the
// old 12000 cap -> the align never fired and seek_done never came.
#define STARFISH_ESPP_SEEK_IDR_MAX_LOOKAHEAD_MS 20000
// Steady-state skip-ahead cap: how far past the cursor the feed may jump to
// the next buffered packet. Wide enough for the ~20s segment alignment of a
// freshly appended post-seek range and for normal eviction gaps, yet far
// below the hundreds-of-seconds gap that stale scrub/pre-seek prefetch leaves
// (287->860s), which must stay blocked so the feed waits for the real target
// data. The initial/rebuild preroll (m_everSubmitted == false) ignores this
// cap entirely -- see fillBufferWithoutGuard.
#define STARFISH_ESPP_MAX_SKIP_AHEAD_MS 30000
// After a rejected submit (FULL etc.) stop feeding the stream and retry
// only after this backoff, unless the byte-status callback re-enables it
// sooner. During preroll (prepare/seek) the pipeline consumes packets a
// few at a time and quickly, so retry fast; in steady-state playback a
// rejected submit just means the buffer is healthy and full, so retry
// lazily to keep the esplusplayer error-log noise down.
#define STARFISH_ESPP_SUBMIT_RETRY_BACKOFF_PREROLL_MS 50
#define STARFISH_ESPP_SUBMIT_RETRY_BACKOFF_STEADY_MS 500
// Default framerate when the demuxer does not know one (29.97fps), same
// as the capi path and chromium-efl kVideoFramerateNum/Den.
#define STARFISH_ESPP_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_ESPP_DEFAULT_FRAMERATE_DEN 100

class MediaPlayerESPPMediaSourceClient : public MediaSourceClient {
public:
    MediaPlayerESPPMediaSourceClient(MediaPlayerESPlusPlayer* player)
        : m_player(player)
    {
    }

    virtual void activeSourceComputed()
    {
        if (m_player != nullptr && m_player->alive() == true) {
            m_player->prepareMediaSource();
        }
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player != nullptr && m_player->alive() == true &&
            m_player->activeSourceBuffer(StreamTypeVideo) == s) {
            m_player->fillBufferIfNeeded(StreamTypeVideo);
        }
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
        if (m_player != nullptr && m_player->alive() == true &&
            m_player->activeSourceBuffer(StreamTypeAudio) == s) {
            m_player->fillBufferIfNeeded(StreamTypeAudio);
        }
    }

    MediaPlayerESPlusPlayer* m_player;
};

static uint64_t monotonicMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

static StreamType toStarfishStreamType(esplusplayer_stream_type type)
{
    return type == ESPLUSPLAYER_STREAM_TYPE_AUDIO ? StreamTypeAudio
                                                  : StreamTypeVideo;
}

// The exact callback typedef may vary between esplusplayer versions; a
// C-style cast at the registration site papers over const/int-width
// differences the same way chromium-efl does with its observer.
static void esppBufferByteStatusCb(const esplusplayer_stream_type type,
                                   const esplusplayer_buffer_status status,
                                   uint64_t bytes, void* data);

static esplusplayer_stream_type toESPPStreamType(StreamType type)
{
    return type == StreamTypeAudio ? ESPLUSPLAYER_STREAM_TYPE_AUDIO
                                   : ESPLUSPLAYER_STREAM_TYPE_VIDEO;
}

MediaPlayerESPlusPlayer::MediaPlayerESPlusPlayer(HTMLMediaElement* element)
    : MediaPlayer(element)
    , m_player(nullptr)
    , m_audioStream(nullptr)
    , m_videoStream(nullptr)
    , m_mseClient(nullptr)
    , m_inPrepare(false)
    , m_pendingPlay(false)
    , m_started(false)
    , m_overlayMode(false)
    , m_overlayPlaneVisible(false)
    , m_videoSourceROIUnsupported(false)
    , m_lastAbsoluteROIArea(0, 0, 1, 1)
    , m_prepared(false)
    , m_pendingSeekTime(std::numeric_limits<double>::quiet_NaN())
    , m_lastPositionMs(0)
    , m_volume(1.0)
    , m_muted(false)
    , m_playbackRate(1.0)
    , m_fillBufferMutex(new Mutex())
    , m_decodedVideoFrameMutex(new Mutex())
    , m_lastDecodedPacket(nullptr)
    , m_setNeedsCompositeEventIdlerHandleMutex(new Mutex())
    , m_setNeedsCompositeEventIdlerHandle(MessageLoopInvalidID)
    , m_mseThread(nullptr)
    , m_playerDeadFlag(nullptr)
{
    STARFISH_ASSERT(element != nullptr);
    PLAYER_LOGI("MediaPlayerESPlusPlayer::MediaPlayerESPlusPlayer (%p)", this);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            PLAYER_LOGI("MediaPlayerESPlusPlayer::~MediaPlayerESPlusPlayer");
            MediaPlayerESPlusPlayer* player = (MediaPlayerESPlusPlayer*)obj;
            player->destroy();
        },
        NULL, NULL, NULL);
}

// ---------------------------------------------------------------------------
// prepare: MSE blob attach only. esplusplayer has no URI mode; a non-MSE
// URL reaching this player is a factory-branching bug.
// ---------------------------------------------------------------------------

void MediaPlayerESPlusPlayer::prepare(ResourceURL* url)
{
    PLAYER_LOGI("MediaPlayerESPlusPlayer::prepare");
    if (url->isBlobURL() == true) {
        BlobURLStore store;
        if (WebBase::stringToBlobURLString(url->urlString(), store) == true &&
            m_container->webView()->isValidMediaSourceBlobURL(store) == true) {
            MediaSource* ms = (MediaSource*)store.m_blob;
            m_activeMediaSource = ms;
            m_mseClient = new MediaPlayerESPPMediaSourceClient(this);
            m_activeMediaSource->addClient(m_mseClient);
            m_activeMediaSource->attach(m_container);
            if (m_container != nullptr) {
                // In the MSE case, ignore defaultPlaybackPosition (same as
                // the capi path).
                m_container->setDefaultPlaybackStartPosition(0);
            }
            STARFISH_LOG_INFO(
                "MediaPlayerESPlusPlayer::prepare: attached MSE blob; waiting "
                "for first appendBuffer");
            processNextOperationQueueInContainer();
            return;
        }
    }
    PLAYER_LOGE(
        "MediaPlayerESPlusPlayer::prepare: non-MSE source reached the "
        "esplusplayer backend (factory branching bug) — cannot play");
    m_foundError = true;
    m_container->giveupFetchingResource();
    processNextOperationQueueInContainer();
    destroy();
}

// ---------------------------------------------------------------------------
// player creation + callbacks
// ---------------------------------------------------------------------------

bool MediaPlayerESPlusPlayer::videoOverlayEnabled()
{
    // Same policy as the capi path (MediaPlayerTizen::videoOverlayEnabled):
    // STARFISH_VIDEO_OVERLAY env overrides the app setting.
    static int envOverride = []() {
        const char* v = getenv("STARFISH_VIDEO_OVERLAY");
        return v && *v ? (atoi(v) != 0 ? 1 : 0) : -1;
    }();
    if (envOverride >= 0) {
        return envOverride == 1;
    }
    return m_container && m_container->webView() &&
           m_container->webView()->videoOverlayEnabled();
}

bool MediaPlayerESPlusPlayer::createAndOpenPlayer()
{
    if (m_player != nullptr) {
        return true;
    }

    m_player = esplusplayer_create();
    if (m_player == nullptr) {
        PLAYER_LOGE("ESPP: esplusplayer_create failed");
        return false;
    }
    m_overlayMode = videoOverlayEnabled();
    STARFISH_LOG_INFO("ESPP: created player %p (overlay:%d)", m_player,
                      (int)m_overlayMode);

    // All callbacks are registered before open (chromium-efl Initialize()
    // order). Capture-less lambdas decay to the C function pointers the
    // API expects.
    esplusplayer_set_ready_to_prepare_cb(
        m_player,
        [](const esplusplayer_stream_type type, void* data) {
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            self->handleReadyToPrepare(toStarfishStreamType(type));
        },
        this);
    esplusplayer_set_prepare_async_done_cb(
        m_player,
        [](bool result, void* data) {
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            self->handlePrepared(result);
        },
        this);
    esplusplayer_set_eos_cb(
        m_player,
        [](void* data) {
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            self->handleEnded();
        },
        this);
    if (!m_overlayMode) {
        // Texture mode only: on overlay the video goes straight to the HW
        // plane and this callback is not supported on every profile
        // anyway (trackrenderer "Not supported on Public").
        esplusplayer_set_media_packet_video_decoded_cb(
            m_player,
            [](const esplusplayer_decoded_video_packet* packet, void* data) {
                MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
                self->handleDecodedFrame(packet);
            },
            this);
    }
    esplusplayer_set_ready_to_seek_cb(
        m_player,
        [](const esplusplayer_stream_type type, const uint64_t timeMs,
           void* data) {
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            self->handleReadyToSeek(toStarfishStreamType(type), timeMs);
        },
        this);
    esplusplayer_set_seek_done_cb(
        m_player,
        [](void* data) {
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            self->handleSeeked(true);
        },
        this);
    esplusplayer_set_error_cb(
        m_player,
        [](const esplusplayer_error_type errorType, void* data) {
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            PLAYER_LOGE("ESPP: error callback: %s",
                        esplusplayer_get_error_string(errorType));
            self->handlePlayerError();
        },
        this);
    esplusplayer_set_resource_conflicted_cb(
        m_player,
        [](void* data) {
            // P0: treat a resource conflict as a fatal error. P3 upgrades
            // this to suspend/resume.
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            PLAYER_LOGE("ESPP: resource conflicted");
            self->handlePlayerError();
        },
        this);
    esplusplayer_set_buffer_byte_status_cb(
        m_player, (esplusplayer_buffer_byte_status_cb)esppBufferByteStatusCb,
        this);

    int ret = esplusplayer_open(m_player);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: esplusplayer_open failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return false;
    }

    // Texture mode: decoded frames are copied out of the decoder's output
    // pool by the player itself, so holding a frame for compositing never
    // starves the HW decoder (the chromium-efl non-TV policy). Overlay
    // mode: no decoded-frame delivery at all (chromium-efl TV policy).
    // Must be set in IDLE.
    ret = esplusplayer_set_video_frame_buffer_type(
        m_player, m_overlayMode
                      ? ESPLUSPLAYER_DECODED_VIDEO_FRAME_BUFFER_TYPE_NONE
                      : ESPLUSPLAYER_DECODED_VIDEO_FRAME_BUFFER_TYPE_COPY);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: set_video_frame_buffer_type failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// overlay (video hole) display — mirrors MediaPlayerTizenBase's
// setNativePlayerDisplayMode/punchHole with the esplusplayer display APIs.
// Must run while the player is IDLE (before prepare_async).
// ---------------------------------------------------------------------------

bool MediaPlayerESPlusPlayer::setupOverlayDisplay()
{
    int ret = esplusplayer_set_display_mode(m_player,
                                            ESPLUSPLAYER_DISPLAY_MODE_DST_ROI);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: set_display_mode(DST_ROI) failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return false;
    }
    m_lastAbsoluteROIArea = LayoutRect(0, 0, 1, 1);
    m_lastVideoSourceROI = { { 0.0, 0.0, 1.0, 1.0 } };
    esplusplayer_set_display_roi(m_player, 0, 0, 1, 1);

    auto width = m_container->webView()->renderer()->width();
    auto height = m_container->webView()->renderer()->height();
    void* ecoreWindowHandle = m_container->webView()->publicLayerUserDataMap()
                                  ["__internalLWEWebViewEFLEcoreWaylandHandle"];
    if (ecoreWindowHandle != nullptr) {
        ret = esplusplayer_set_ecore_display(
            m_player, ESPLUSPLAYER_DISPLAY_TYPE_OVERLAY,
            static_cast<Ecore_Wl2_Window*>(ecoreWindowHandle), 0, 0, width,
            height);
        STARFISH_LOG_INFO("ESPP: set_ecore_display(OVERLAY, %p, %dx%d) -> %d",
                          ecoreWindowHandle, (int)width, (int)height, ret);
    } else {
        // Fall back to the plain display handle (tcore builds).
        void* windowHandle = m_container->webView()->publicLayerUserDataMap()
                                 ["__internalLWEWebViewTcoreWaylandHandle"];
        ret = esplusplayer_set_display(
            m_player, ESPLUSPLAYER_DISPLAY_TYPE_OVERLAY, windowHandle);
        STARFISH_LOG_INFO("ESPP: set_display(OVERLAY, %p) -> %d", windowHandle,
                          ret);
    }
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: overlay display connect failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return false;
    }

    ret = esplusplayer_set_display_visible(m_player, true);
    if (ret == ESPLUSPLAYER_ERROR_TYPE_NONE) {
        m_overlayPlaneVisible = true;
    }
    return true;
}

void MediaPlayerESPlusPlayer::setOverlayPlaneVisible(bool visible)
{
    if (m_overlayPlaneVisible == visible || m_player == nullptr) {
        return;
    }
    int ret = esplusplayer_set_display_visible(m_player, visible);
    if (ret == ESPLUSPLAYER_ERROR_TYPE_NONE) {
        m_overlayPlaneVisible = visible;
    } else {
        PLAYER_LOGE("ESPP: set_display_visible(%d) failed: %s", (int)visible,
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
    }
}

void MediaPlayerESPlusPlayer::punchHole(Compositor* canvas,
                                        const LayoutRect& videoRect,
                                        const LayoutRect& absVideoRect)
{
    // Confine the HW plane to the visible part of the video (see
    // MediaPlayerTizenBase::punchHole for the full rationale: the plane is
    // not clipped by the page, so it must track the compositor's clip).
    Optional<Unit::Rect> clipRect = canvas->currentClipRect();
    LayoutUnit clipX(0);
    LayoutUnit clipY(0);
    LayoutUnit clipMaxX(m_container->webView()->renderer()->width());
    LayoutUnit clipMaxY(m_container->webView()->renderer()->height());
    if (clipRect) {
        clipX = LayoutUnit(clipRect.value().x());
        clipY = LayoutUnit(clipRect.value().y());
        clipMaxX = LayoutUnit(clipRect.value().maxX());
        clipMaxY = LayoutUnit(clipRect.value().maxY());
    }
    LayoutUnit visibleX = std::max(absVideoRect.x(), clipX);
    LayoutUnit visibleY = std::max(absVideoRect.y(), clipY);
    LayoutUnit visibleMaxX = std::min(absVideoRect.maxX(), clipMaxX);
    LayoutUnit visibleMaxY = std::min(absVideoRect.maxY(), clipMaxY);
    if (absVideoRect.width() <= 0 || absVideoRect.height() <= 0 ||
        visibleMaxX <= visibleX || visibleMaxY <= visibleY) {
        setOverlayPlaneVisible(false);
        return;
    }
    setOverlayPlaneVisible(true);
    canvas->punchHole(Unit::Rect(videoRect.x(), videoRect.y(),
                                 videoRect.width(), videoRect.height()));

    LayoutRect roiArea(visibleX, visibleY, visibleMaxX - visibleX,
                       visibleMaxY - visibleY);
    double videoWidth = absVideoRect.width().toDouble();
    double videoHeight = absVideoRect.height().toDouble();
    std::array<double, 4> sourceROI{
        { (visibleX - absVideoRect.x()).toDouble() / videoWidth,
          (visibleY - absVideoRect.y()).toDouble() / videoHeight,
          (visibleMaxX - visibleX).toDouble() / videoWidth,
          (visibleMaxY - visibleY).toDouble() / videoHeight }
    };

    if (!m_videoSourceROIUnsupported && sourceROI != m_lastVideoSourceROI) {
        int ret = esplusplayer_set_video_roi(
            m_player, sourceROI[0], sourceROI[1], sourceROI[2], sourceROI[3]);
        if (ret == ESPLUSPLAYER_ERROR_TYPE_NONE) {
            m_lastVideoSourceROI = sourceROI;
        } else {
            PLAYER_LOGE("ESPP: set_video_roi failed: %s",
                        esplusplayer_get_error_string(
                            static_cast<esplusplayer_error_type>(ret)));
            m_videoSourceROIUnsupported = true;
        }
    }

    if (m_lastAbsoluteROIArea != roiArea) {
        int ret = esplusplayer_set_display_roi(
            m_player, roiArea.x().toInt(), roiArea.y().toInt(),
            roiArea.width().toInt(), roiArea.height().toInt());
        if (ret == ESPLUSPLAYER_ERROR_TYPE_NONE) {
            m_lastAbsoluteROIArea = roiArea;
        } else {
            PLAYER_LOGE("ESPP: set_display_roi failed: %s",
                        esplusplayer_get_error_string(
                            static_cast<esplusplayer_error_type>(ret)));
        }
    }
}

// ---------------------------------------------------------------------------
// stream info from the Starfish demuxer's StreamInfo
// ---------------------------------------------------------------------------

bool MediaPlayerESPlusPlayer::initAudioStreamInfo()
{
    SourceBuffer* sb = m_activeMediaSource->activeAudioSourceBuffer();
    if (sb == nullptr) {
        return true; // video-only source
    }
    StreamInfo* info =
        sb->streamInfo(0, m_activeMediaSource->activeAudioStreamIndex());

    esplusplayer_audio_stream_info streamInfo;
    memset(&streamInfo, 0, sizeof(esplusplayer_audio_stream_info));

    if (info->isCodec(MediaCodecAudioAAC) == true) {
        streamInfo.mime_type = ESPLUSPLAYER_AUDIO_MIME_TYPE_AAC;
    } else if (info->isCodec(MediaCodecAudioMP3) == true) {
        streamInfo.mime_type = ESPLUSPLAYER_AUDIO_MIME_TYPE_MP3;
    } else if (info->isCodec(MediaCodecAudioVorbis) == true) {
        streamInfo.mime_type = ESPLUSPLAYER_AUDIO_MIME_TYPE_VORBIS;
    } else if (info->isCodec(MediaCodecAudioOpus) == true) {
        streamInfo.mime_type = ESPLUSPLAYER_AUDIO_MIME_TYPE_OPUS;
    } else {
        PLAYER_LOGE("ESPP: unsupported audio codec %s", info->codecString());
        return false;
    }
    streamInfo.sample_rate = info->audioSampleRate();
    streamInfo.channels = info->audioChannels();
    if (info->m_extraData.size() > 0) {
        streamInfo.codec_data = (char*)info->m_extraData.data();
        streamInfo.codec_data_length = info->m_extraData.size();
    }

    int ret = esplusplayer_set_audio_stream_info(m_player, &streamInfo);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: set_audio_stream_info failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return false;
    }

    m_audioStream = new ESPPSourceStream(StreamTypeAudio);
    m_audioStream->m_maxBufferSize = STARFISH_ESPP_AUDIO_BUFFER_SIZE;
    esplusplayer_set_buffer_size(m_player,
                                 ESPLUSPLAYER_BUFFER_AUDIO_MAX_BYTE_SIZE,
                                 STARFISH_ESPP_AUDIO_BUFFER_SIZE);
    esplusplayer_set_buffer_size(m_player,
                                 ESPLUSPLAYER_BUFFER_AUDIO_MIN_BYTE_THRESHOLD,
                                 STARFISH_ESPP_MIN_BYTE_THRESHOLD);
    PLAYER_LOGI("ESPP: audio stream info set (rate:%u ch:%u extra:%u)",
                (unsigned)info->audioSampleRate(),
                (unsigned)info->audioChannels(),
                (unsigned)info->m_extraData.size());
    return true;
}

bool MediaPlayerESPlusPlayer::initVideoStreamInfo()
{
    SourceBuffer* sb = m_activeMediaSource->activeVideoSourceBuffer();
    if (sb == nullptr) {
        return true; // audio-only source
    }
    StreamInfo* info =
        sb->streamInfo(0, m_activeMediaSource->activeVideoStreamIndex());

    esplusplayer_video_stream_info streamInfo;
    memset(&streamInfo, 0, sizeof(esplusplayer_video_stream_info));

    if (info->isCodec(MediaCodecVideoH264) == true) {
        streamInfo.mime_type = ESPLUSPLAYER_VIDEO_MIME_TYPE_H264;
    } else if (info->isCodec(MediaCodecVideoVP9) == true) {
        streamInfo.mime_type = ESPLUSPLAYER_VIDEO_MIME_TYPE_VP9;
    } else {
        PLAYER_LOGE("ESPP: unsupported video codec %s", info->codecString());
        return false;
    }
    streamInfo.width = info->videoWidth();
    streamInfo.height = info->videoHeight();
    // esplusplayer_set_video_stream_info can only be called once per
    // player, so resolution switches (init-segment changes) later must be
    // handled in-band by the decoder. Advertise a max resolution at least
    // FHD so upswitches within that budget are acceptable to the player.
    streamInfo.max_width =
        info->videoWidth() > 1920 ? info->videoWidth() : 1920;
    streamInfo.max_height =
        info->videoHeight() > 1080 ? info->videoHeight() : 1080;
    Framerate framerate = info->videoFramerate();
    if (framerate.isValid()) {
        streamInfo.framerate_num = framerate.m_num;
        streamInfo.framerate_den = framerate.m_den;
    } else {
        streamInfo.framerate_num = STARFISH_ESPP_DEFAULT_FRAMERATE_NUM;
        streamInfo.framerate_den = STARFISH_ESPP_DEFAULT_FRAMERATE_DEN;
    }
    if (info->m_extraData.size() > 0) {
        streamInfo.codec_data = (char*)info->m_extraData.data();
        streamInfo.codec_data_length = info->m_extraData.size();
    }

    int ret = esplusplayer_set_video_stream_info(m_player, &streamInfo);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: set_video_stream_info failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return false;
    }

    m_videoStream = new ESPPSourceStream(StreamTypeVideo);
    uint64_t videoBufferSize =
        STARFISH_ESPP_TOTAL_BUFFER_SIZE - STARFISH_ESPP_AUDIO_BUFFER_SIZE;
    m_videoStream->m_maxBufferSize = videoBufferSize;
    esplusplayer_set_buffer_size(
        m_player, ESPLUSPLAYER_BUFFER_VIDEO_MAX_BYTE_SIZE, videoBufferSize);
    esplusplayer_set_buffer_size(m_player,
                                 ESPLUSPLAYER_BUFFER_VIDEO_MIN_BYTE_THRESHOLD,
                                 STARFISH_ESPP_MIN_BYTE_THRESHOLD);

    m_hasVideo = true;
    m_videoWidth = info->videoWidth();
    m_videoHeight = info->videoHeight();
    PLAYER_LOGI("ESPP: video stream info set (%ux%u fr:%d/%d extra:%u)",
                (unsigned)info->videoWidth(), (unsigned)info->videoHeight(),
                streamInfo.framerate_num, streamInfo.framerate_den,
                (unsigned)info->m_extraData.size());
    return true;
}

// ---------------------------------------------------------------------------
// prepareMediaSource: called on the main thread once the demuxer produced
// StreamInfo (MediaSourceClient::activeSourceComputed).
// ---------------------------------------------------------------------------

void MediaPlayerESPlusPlayer::prepareMediaSource()
{
    STARFISH_LOG_INFO("MediaPlayerESPlusPlayer::prepareMediaSource entry");
    if (m_foundError == true || m_inPrepare == true) {
        return;
    }

    if (createAndOpenPlayer() == false) {
        m_foundError = true;
        handlePlayerError();
        return;
    }

    if (initAudioStreamInfo() == false || initVideoStreamInfo() == false) {
        m_foundError = true;
        handlePlayerError();
        return;
    }
    if (m_audioStream == nullptr && m_videoStream == nullptr) {
        PLAYER_LOGE(
            "ESPP: prepareMediaSource: both audio and video streams are null "
            "after init — bailing");
        return;
    }

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);

    startFeedThread();

    if (m_overlayMode && m_videoStream != nullptr) {
        if (setupOverlayDisplay() == false) {
            m_foundError = true;
            handlePlayerError();
            return;
        }
    }

    m_inPrepare = true;
    m_container->executionContext()->addPointerInRootSet(this);

    esplusplayer_set_display_visible(m_player, true);
    int ret = esplusplayer_prepare_async(m_player);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: prepare_async failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        m_foundError = true;
        handlePrepared(false);
        return;
    }
    STARFISH_LOG_INFO("ESPP: prepare_async issued");
}

// ---------------------------------------------------------------------------
// feed thread
// ---------------------------------------------------------------------------

void MediaPlayerESPlusPlayer::startFeedThread()
{
    if (m_mseThread != nullptr) {
        return;
    }
    m_playerDeadFlag = (bool*)malloc(sizeof(bool));
    STARFISH_RELEASE_ASSERT(m_playerDeadFlag != nullptr);
    *m_playerDeadFlag = false;
    m_mseThread = new Thread(m_container->webView()->threadPool());
    m_mseThread->run(m_container->webView()->messageLoop(), threadFillingBuffer,
                     this);
}

void* MediaPlayerESPlusPlayer::threadFillingBuffer(void* data)
{
    MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
    volatile bool* playerDeadFlag = self->m_playerDeadFlag;
    while (!(*playerDeadFlag)) {
        if (self->playbackState() != MediaPlayer::PLAYBACK_STATE_END) {
            ESPPSourceStream* audio = self->m_audioStream;
            ESPPSourceStream* video = self->m_videoStream;
            // Safety net: a stream parked by a rejected submit normally
            // resumes via the byte-status callback; if that never comes,
            // retry after a fixed backoff instead of stalling forever.
            uint64_t now = monotonicMs();
            uint64_t backoffMs =
                (self->m_prepared && !self->seeking())
                    ? STARFISH_ESPP_SUBMIT_RETRY_BACKOFF_STEADY_MS
                    : STARFISH_ESPP_SUBMIT_RETRY_BACKOFF_PREROLL_MS;
            ESPPSourceStream* streams[2] = { audio, video };
            for (int i = 0; i < 2; i++) {
                ESPPSourceStream* st = streams[i];
                if (st != nullptr && !st->m_shouldFeed &&
                    st->m_lastSubmitRejectedMs != 0 &&
                    now - st->m_lastSubmitRejectedMs > backoffMs) {
                    st->m_lastSubmitRejectedMs = 0;
                    st->m_shouldFeed = true;
                }
            }
            if (audio != nullptr && audio->m_shouldFeed &&
                !audio->m_waitingDemuxer && !audio->m_eosSubmitted) {
                self->fillBuffer(audio);
            }
            if (video != nullptr && video->m_shouldFeed &&
                !video->m_waitingDemuxer && !video->m_eosSubmitted) {
                self->fillBuffer(video);
            }
        }
        {
            std::unique_lock<std::mutex> lock(self->m_mseWakeMutex);
            self->m_mseWakeCv.wait_for(
                lock, std::chrono::microseconds(STARFISH_ESPP_FEED_WAIT_US),
                [&]() { return *playerDeadFlag || self->m_mseWakePending; });
            self->m_mseWakePending = false;
        }
    }
    PLAYER_LOGI("ESPP: close fillingBuffer thread");
    return nullptr;
}

void MediaPlayerESPlusPlayer::fillBufferIfNeeded(StreamType type)
{
    ESPPSourceStream* stream = currentStream(type);
    if (stream == nullptr) {
        return;
    }
    stream->m_waitingDemuxer = false;
    wakeMseThread();
}

void MediaPlayerESPlusPlayer::fillBuffer(ESPPSourceStream* stream)
{
    Locker<Mutex> locker(*m_fillBufferMutex);
    fillBufferWithoutGuard(stream);
}

void MediaPlayerESPlusPlayer::fillBufferWithoutGuard(ESPPSourceStream* stream)
{
    STARFISH_ASSERT(stream);
    if (alive() == false || m_player == nullptr || stream->m_eosSubmitted) {
        return;
    }
    if (stream->m_seekCursorStale) {
        // Between seek() and this stream's ready_to_seek the cursor still
        // points at the pre-seek position; feeding from it would submit
        // stale packets into the freshly flushed native buffer (the
        // byte-status UNDERRUN callback can unpark the stream inside that
        // window). ready_to_seek repositions the cursor and clears the flag.
        return;
    }
    SourceBuffer* sb = activeSourceBuffer(stream->m_type);
    if (sb == nullptr) {
        return;
    }

    // A/V balance gate, same policy as the capi path: only ever skip the
    // stream that is ahead, so the behind stream always progresses.
    {
        ESPPSourceStream* other = currentStream(
            stream->isAudio() ? StreamTypeVideo : StreamTypeAudio);
        if (other != nullptr && !other->m_eosSubmitted) {
            uint64_t myDTS = stream->m_lastSubmittedDTS;
            uint64_t otherDTS = other->m_lastSubmittedDTS;
            if (myDTS > otherDTS &&
                (myDTS - otherDTS) > STARFISH_ESPP_MAX_AV_DIFF_IN_MS) {
                return;
            }
        }
    }

    uint64_t streamIdx = activeStreamIndex(stream->m_type);
    size_t currentInitIndex = stream->m_initSegmentIndex;
    uint64_t lastDTS = stream->m_lastSubmittedDTS;
    if (stream->m_needIdrAlign) {
        // Seek: esplusplayer is a pure push model -- it renders from the
        // seek target and discards decoded output before it, but only if it
        // is fed a frame at or before the target. So snap back to the nearest
        // keyframe at-or-before the target (chromium's GetFirstKeyframeAtOr-
        // Before). Video keyframes are sparse -> the GOP start; audio frames
        // are all keyframes -> the frame covering the target. Bound the
        // snap-back to a GOP-sized window: a match far behind means the
        // target segment is not buffered yet (seek(90) aligning to 63700
        // because 88-90s was not appended).
        //
        // Only when nothing at-or-before the target is buffered within that
        // window -- a true forward seek where the page re-appends the segment
        // starting at its own keyframe after the target (observed: audio at
        // exactly 90s, video at 94.9s) -- start at the first keyframe
        // at-or-after the target instead.
        uint64_t alignedDTS = 0;
        bool haveBack =
            sb->findNearestIdrDTSBefore(streamIdx, lastDTS, alignedDTS);
        if (haveBack &&
            lastDTS - alignedDTS <= STARFISH_ESPP_SEEK_IDR_MAX_LOOKBACK_MS) {
            STARFISH_LOG_INFO("ESPP: IDR align %llu -> %llu (%s)",
                              (unsigned long long)lastDTS,
                              (unsigned long long)alignedDTS,
                              stream->isAudio() ? "AUDIO" : "VIDEO");
            lastDTS = alignedDTS;
            stream->m_needIdrAlign = false;
        } else {
            // Metadata-only peek via the snapshotting accessor: the raw
            // findProperMediaPacket hands back a MediaPacket* that a concurrent
            // main-thread remove()/eviction can free the instant the source-
            // buffer lock is dropped, so reading m_hasIdr/m_dts off it races.
            SourceBuffer::MediaPacketView peek =
                sb->peekProperMediaPacket(streamIdx, lastDTS);
            if (peek.m_found && peek.m_hasIdr && peek.m_dts >= lastDTS &&
                peek.m_dts - lastDTS <=
                    STARFISH_ESPP_SEEK_IDR_MAX_LOOKAHEAD_MS) {
                STARFISH_LOG_INFO("ESPP: IDR align (forward) %llu -> %llu (%s)",
                                  (unsigned long long)lastDTS,
                                  (unsigned long long)peek.m_dts,
                                  stream->isAudio() ? "AUDIO" : "VIDEO");
                lastDTS = peek.m_dts;
                stream->m_needIdrAlign = false;
            } else {
                // Nothing decodable near the target yet (a keyframe far
                // ahead is stale pre-seek prefetch, not the target
                // segment). Wait for the next append. Throttled: the demuxer
                // append callback re-probes this path many times per second
                // while the page is still fetching the target segment.
                uint64_t nowMs = monotonicMs();
                if (nowMs - stream->m_lastIdrWaitLogMs > 1000) {
                    stream->m_lastIdrWaitLogMs = nowMs;
                    STARFISH_LOG_INFO(
                        "ESPP: IDR align waiting target data (%s) target:%llu "
                        "nearestBackDTS:%llu nextIdrDTS:%llu lastBuffered:%llu",
                        stream->isAudio() ? "AUDIO" : "VIDEO",
                        (unsigned long long)lastDTS,
                        haveBack ? (unsigned long long)alignedDTS : 0ULL,
                        peek.m_found ? (unsigned long long)peek.m_dts : 0ULL,
                        (unsigned long long)sb->lastBufferedTimestamp(
                            streamIdx));
                }
                stream->m_waitingDemuxer = true;
                return;
            }
        }
    }
    size_t submitBytes = 0;
    uint64_t sizeUpTo =
        stream->m_maxBufferSize * STARFISH_ESPP_SUBMIT_BYTES_RATE;
    uint64_t fillStartDTS = lastDTS;
    size_t submitCount = 0;

    // Reused across iterations. copyProperMediaPacket snapshots the packet's
    // encoded bytes here under the source-buffer lock, so the memory handed to
    // esplusplayer_submit_packet is a caller-owned copy that a concurrent
    // main-thread remove()/eviction (YouTube appends+evicts segments
    // continuously) cannot free out from under the push. The raw
    // findProperMediaPacket used before returned a MediaPacket* that became a
    // use-after-free the moment the lock dropped -> random crash under MSE.
    std::vector<uint8_t> packetData;
    while (submitBytes < sizeUpTo) {
        SourceBuffer::MediaPacketView packet =
            sb->copyProperMediaPacket(streamIdx, lastDTS, packetData);

        if (!packet.m_found) {
            STARFISH_LOG_INFO(
                "ESPP: no packet at DTS %llu (%s); pushed %u this pass "
                "(%llu -> %llu), lastBuffered %llu",
                (unsigned long long)lastDTS,
                stream->isAudio() ? "AUDIO" : "VIDEO", (unsigned)submitCount,
                (unsigned long long)fillStartDTS, (unsigned long long)lastDTS,
                (unsigned long long)sb->lastBufferedTimestamp(streamIdx));
            // End-of-stream detection, same policy as the capi path.
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            uint64_t lastBufferedTime = sb->lastBufferedTimestamp(streamIdx);
            uint64_t elapsedTime = endTime < lastBufferedTime
                                       ? lastBufferedTime - endTime
                                       : endTime - lastBufferedTime;
            if ((endTime - lastDTS) < 10 ||
                ((lastDTS == lastBufferedTime) && (elapsedTime < 1000))) {
                int ret = esplusplayer_submit_eos_packet(
                    m_player, toESPPStreamType(stream->m_type));
                STARFISH_LOG_INFO("ESPP: submit eos (%s) ret:%d",
                                  stream->isAudio() ? "AUDIO" : "VIDEO", ret);
                stream->m_eosSubmitted = true;
                break;
            }
            stream->m_waitingDemuxer = true;
            break;
        }
        if (packet.m_dts < lastDTS) {
            // Already-consumed packet; defer to next demuxer event.
            sb->clearPacketAccessCache();
            stream->m_waitingDemuxer = true;
            break;
        }
        // How far ahead of the cursor the feed may jump to the next buffered
        // packet. The initial / mid-playback-rebuild preroll (!m_everSubmitted)
        // is uncapped: the source buffer was just reattached, so whatever it
        // holds is the current content -- jump to it wherever it is (0->20s
        // cold, 0->110s when rebuilt) or the player never prerolls, and there
        // is no stale data to guard against yet. Once anything has been
        // submitted the steady cap applies: wide enough for post-seek segment
        // alignment and eviction gaps, far below the stale scrub/pre-seek
        // leftover it must reject.
        uint64_t skipAheadCap = stream->m_everSubmitted
                                    ? STARFISH_ESPP_MAX_SKIP_AHEAD_MS
                                    : std::numeric_limits<uint64_t>::max();
        if (packet.m_dts - lastDTS > 500) {
            if (packet.m_dts - lastDTS > skipAheadCap) {
                // The only buffered data ahead is far from the cursor:
                // stale pre-seek/scrub prefetch, or the segment covering the
                // cursor has not been appended yet. Feeding it would hand
                // the player data unrelated to the current position
                // (observed post-seek: 15s cursor fed 222s audio), so wait
                // for the demuxer instead.
                STARFISH_LOG_INFO(
                    "ESPP: skip-ahead blocked %llu -> %llu (%s); waiting "
                    "demuxer",
                    (unsigned long long)lastDTS,
                    (unsigned long long)packet.m_dts,
                    stream->isAudio() ? "AUDIO" : "VIDEO");
                stream->m_waitingDemuxer = true;
                break;
            }
            // >500ms gap ahead of the submission pointer (eviction, a
            // cluster-boundary undershoot, or a resolution/segment change
            // where the demuxer has not produced the in-between packets);
            // jump instead of stalling, same as the capi backend's long-
            // running skip-ahead. Video keeps decoding because the next
            // buffered range on a byte-stream boundary starts at a
            // keyframe.
            STARFISH_LOG_INFO("ESPP: skip-ahead %llu -> %llu (%s)",
                              (unsigned long long)lastDTS,
                              (unsigned long long)packet.m_dts,
                              stream->isAudio() ? "AUDIO" : "VIDEO");
            lastDTS = packet.m_dts;
        }

        if (packet.m_initSegmentIndex != currentInitIndex) {
            if (!packet.m_hasIdr) {
                lastDTS = packet.m_dts + packet.m_duration;
                continue;
            }
            // Unlike the capi path there is no stream-info re-registration:
            // esplusplayer_set_video_stream_info is one-shot, so config
            // changes ride in-band (SPS/PPS in the ES) from the IDR on.
            STARFISH_LOG_INFO(
                "ESPP: config change detected (initIdx %u -> %u) at "
                "DTS %llu (%s)",
                (unsigned)currentInitIndex, (unsigned)packet.m_initSegmentIndex,
                (unsigned long long)packet.m_dts,
                stream->isAudio() ? "AUDIO" : "VIDEO");
            stream->m_initSegmentIndex = packet.m_initSegmentIndex;
            currentInitIndex = packet.m_initSegmentIndex;
        }

        esplusplayer_es_packet esPacket;
        memset(&esPacket, 0, sizeof(esplusplayer_es_packet));
        esPacket.type = toESPPStreamType(stream->m_type);
        esPacket.buffer = (char*)packetData.data();
        esPacket.buffer_size = packet.m_dataSize;
        esPacket.pts = packet.m_pts;           // ms
        esPacket.duration = packet.m_duration; // ms

        esplusplayer_submit_status status =
            esplusplayer_submit_packet(m_player, &esPacket);
        if (status == ESPLUSPLAYER_SUBMIT_STATUS_FULL ||
            status == ESPLUSPLAYER_SUBMIT_STATUS_NOT_PREPARED ||
            status == ESPLUSPLAYER_SUBMIT_STATUS_OUT_OF_MEMORY) {
            // Packet not consumed: rewind the demuxer access cache so the
            // same packet is returned on the next attempt, and keep
            // lastDTS unchanged.
            sb->revertLastCacheIfPossible(streamIdx);
            // Stop feeding this stream until either the byte-status
            // callback reports room again or the backoff below expires.
            // Retrying every feed-thread tick amplifies into a storm:
            // esplusplayer emits a buffer-status callback for every
            // rejected submit, and the capi layer logs an error line for
            // each one (observed at ~14k submits/s).
            stream->m_shouldFeed = false;
            stream->m_lastSubmitRejectedMs = monotonicMs();
            if ((m_submitRejectLogCount++ % 64) == 0) {
                STARFISH_LOG_INFO(
                    "ESPP: submit rejected (%s) status:%d lastDTS:%llu "
                    "(rejects so far:%u)",
                    stream->isAudio() ? "AUDIO" : "VIDEO", (int)status,
                    (unsigned long long)lastDTS,
                    (unsigned)m_submitRejectLogCount);
            }
            break;
        } else if (status != ESPLUSPLAYER_SUBMIT_STATUS_SUCCESS) {
            PLAYER_LOGE("ESPP: submit_packet failed with status %d",
                        (int)status);
            handlePlayerError();
            return;
        }
        submitBytes += packet.m_dataSize;
        lastDTS = packet.m_dts + packet.m_duration;
        submitCount++;
        // First packet accepted: the uncapped preroll ends, steady cap
        // applies from here on.
        stream->m_everSubmitted = true;
    }
    stream->m_lastSubmittedDTS = lastDTS;
}

// ---------------------------------------------------------------------------
// esplusplayer callback handlers
// ---------------------------------------------------------------------------

void MediaPlayerESPlusPlayer::handleReadyToPrepare(StreamType type)
{
    STARFISH_LOG_INFO("ESPP: ready_to_prepare (%s)",
                      type == StreamTypeAudio ? "AUDIO" : "VIDEO");
    ESPPSourceStream* stream = currentStream(type);
    if (stream != nullptr) {
        stream->m_shouldFeed = true;
        wakeMseThread();
    }
}

void MediaPlayerESPlusPlayer::handlePrepared(bool success)
{
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        // Pass success via the pointer-tag-free route: retry on main thread
        // by re-reading m_foundError (set below before the hop on failure).
        if (!success) {
            m_foundError = true;
        }
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
                self->handlePrepared(self->m_foundError == false);
            },
            this);
        return;
    }

    STARFISH_LOG_INFO("ESPP: handlePrepared success:%d foundError:%d",
                      (int)success, (int)m_foundError);
    if (m_inPrepare == true) {
        m_container->executionContext()->removePointerFromRootSet(this);
        m_inPrepare = false;
    }

    if (!success || m_foundError == true || alive() == false) {
        m_foundError = true;
        if (m_container != nullptr) {
            m_container->giveupFetchingResource();
        }
        destroy();
        return;
    }

    m_prepared = true;
    if (m_container->playbackRate() != 1.0) {
        m_playbackRate = m_container->playbackRate();
    }
    if (m_playbackRate != 1.0) {
        setPlaybackRate(m_playbackRate);
    }
    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_ENOUGH_DATA);
    if (m_container->isHTMLVideoElement() == true &&
        m_container->frame() != nullptr) {
        m_container->setNeedsComposite();
    }
    if (!std::isnan(m_pendingSeekTime)) {
        double t = m_pendingSeekTime;
        m_pendingSeekTime = std::numeric_limits<double>::quiet_NaN();
        seek(t);
    }
    if (m_pendingPlay == true) {
        m_pendingPlay = false;
        play();
    }
}

void MediaPlayerESPlusPlayer::handleReadyToSeek(StreamType type,
                                                uint64_t timeMs)
{
    ESPPSourceStream* stream = currentStream(type);
    STARFISH_LOG_INFO("ESPP: ready_to_seek (%s) time:%llu",
                      type == StreamTypeAudio ? "AUDIO" : "VIDEO",
                      (unsigned long long)timeMs);
    if (stream != nullptr) {
        // Take the feed lock: a fillBufferWithoutGuard already in flight on
        // the feed thread writes stream->m_lastSubmittedDTS at its end, and
        // that write would clobber the seek reposition below, leaving the
        // feeder pushing pre-seek data (observed: after seek(90) the feeder
        // kept submitting 30-52s audio, so the native buffer filled with
        // stale packets and seek_done never fired).
        Locker<Mutex> locker(*m_fillBufferMutex);
        if (type == StreamTypeVideo) {
            // Snap back to a keyframe before submitting; retried in
            // fillBufferWithoutGuard because the keyframe may not be
            // buffered yet (quality switches clear and re-append).
            stream->m_needIdrAlign = true;
        }
        stream->m_lastSubmittedDTS = timeMs;
        stream->m_eosSubmitted = false;
        stream->m_waitingDemuxer = false;
        stream->m_shouldFeed = true;
        stream->m_lastSubmitRejectedMs = 0;
        // Cursor repositioned to the seek target: feeding may resume.
        stream->m_seekCursorStale = false;
    }
    wakeMseThread();
}

void MediaPlayerESPlusPlayer::handleSeeked(bool success)
{
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        // Same success-passing route as handlePrepared: the idler carries only
        // `this`, so a failure is recorded in m_foundError before the hop and
        // success is re-derived from it on the main thread. Without the store
        // a cross-thread handleSeeked(false) would arrive as success=true.
        if (!success) {
            m_foundError = true;
        }
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
                self->handleSeeked(self->m_foundError == false);
            },
            this);
        return;
    }
    STARFISH_LOG_INFO("ESPP: handleSeeked success:%d target:%lf pending:%lf",
                      (int)success, m_lastSeekTargetTime, m_pendingSeekTime);
    if (m_seekingTimer != TimerInvalidID && m_container != nullptr) {
        m_container->window()->clearTimeout(m_seekingTimer);
        m_seekingTimer = TimerInvalidID;
    }
    if (m_seekState == SEEKSTATE_NO_SEEK) {
        return;
    }
    m_seekState = SEEKSTATE_NO_SEEK;
    if (success && m_foundError == false) {
        if (!std::isnan(m_pendingSeekTime)) {
            // Another target superseded this seek while it was in flight.
            // Chain to it without notifying the element: from the page's
            // point of view the seek is still running, and the element's
            // own pending bookkeeping stays consistent because it only
            // ever sees the final target's completion.
            double t = m_pendingSeekTime;
            m_pendingSeekTime = std::numeric_limits<double>::quiet_NaN();
            seek(t);
            return;
        }
        // Report the requested target, not the measured position: the
        // element compares this value against its own pending-seek target
        // (mediaPlayerNotifySeekedItsContainer), and the measured clock
        // is usually a frame or a keyframe away from the request. Same
        // contract as the capi backend.
        m_container->mediaPlayerNotifySeekedItsContainer(m_lastSeekTargetTime);
    } else {
        m_container->mediaPlayerNotifySeekFailureItsContainer();
        destroy();
    }
}

void MediaPlayerESPlusPlayer::handleEnded()
{
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
                self->handleEnded();
            },
            this);
        return;
    }
    STARFISH_LOG_INFO("ESPP: handleEnded (loop:%d)", (int)m_isLooping);
    if (alive() == false) {
        return;
    }
    if (m_isLooping == true) {
        // esplusplayer has no native looping: restart via seek(0).
        seek(0);
        return;
    }
    stopCurrentTimeUpdateTimer();
    setPlaybackState(MediaPlayer::PLAYBACK_STATE_END);
    m_container->mediaPlayerNotifyEndedItsContainer();
}

void MediaPlayerESPlusPlayer::handlePlayerError()
{
    m_foundError = true;
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
                self->handlePlayerError();
            },
            this);
        return;
    }
    PLAYER_LOGE("ESPP: handlePlayerError");
    destroy();
}

void MediaPlayerESPlusPlayer::handleBufferByteStatus(
    StreamType type, esplusplayer_buffer_status status, uint64_t bytes)
{
    ESPPSourceStream* stream = currentStream(type);
    if (stream == nullptr) {
        return;
    }
    // Feed control follows the status enum only. The bytes argument is not
    // trustworthy on every platform (observed garbage above 2^60 for the
    // video stream on a Tizen 10.0 mobile image); it is logged for
    // diagnosis but never used for decisions.
    uint64_t now = monotonicMs();
    if (now - stream->m_lastBufferStatusLogMs > 2000) {
        stream->m_lastBufferStatusLogMs = now;
        STARFISH_LOG_INFO(
            "ESPP: buffer status (%s) status:%d bytes:%llu "
            "feed:%d",
            stream->isAudio() ? "AUDIO" : "VIDEO", (int)status,
            (unsigned long long)bytes, (int)stream->m_shouldFeed);
    }
    if (status == ESPLUSPLAYER_BUFFER_STATUS_UNDERRUN) {
        stream->m_lastSubmitRejectedMs = 0;
        if (stream->m_shouldFeed == false) {
            stream->m_shouldFeed = true;
        }
        // The player wants data. If the feed had parked this stream on
        // m_waitingDemuxer (no packet at the cursor at the time), clear it so
        // the feed thread re-probes the source buffer: the poll loop skips
        // waitingDemuxer streams and the only other clears are the demuxer
        // append callback and ready_to_seek, so a missed/raced append event
        // would otherwise leave the stream stalled until the next seek.
        stream->m_waitingDemuxer = false;
        wakeMseThread();
    } else {
        // OVERRUN: park the stream and stamp the retry-backoff deadline, but
        // ONLY on the transition into the parked state. The player emits an
        // OVERRUN callback ~once/second while the buffer stays full; stamping
        // m_lastSubmitRejectedMs on every one pushed the "now - rej >
        // backoff" deadline forever, so the safety-net re-arm never fired and
        // a paused-full player (e.g. after a scrub that ended with the buffer
        // full and playback not resumed) deadlocked the feed permanently.
        // Stamping once lets the backoff eventually re-arm and retry.
        if (stream->m_shouldFeed) {
            stream->m_shouldFeed = false;
            stream->m_lastSubmitRejectedMs = now;
        }
    }
}

static void esppBufferByteStatusCb(const esplusplayer_stream_type type,
                                   const esplusplayer_buffer_status status,
                                   uint64_t bytes, void* data)
{
    MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
    self->handleBufferByteStatus(toStarfishStreamType(type), status, bytes);
}

// ---------------------------------------------------------------------------
// decoded frame (texture mode; buffer type COPY)
// ---------------------------------------------------------------------------

void MediaPlayerESPlusPlayer::handleDecodedFrame(
    const esplusplayer_decoded_video_packet* packet)
{
    {
        Locker<Mutex> l(*m_decodedVideoFrameMutex);
        esplusplayer_decoded_video_packet* old = m_lastDecodedPacket;
        m_lastDecodedPacket =
            const_cast<esplusplayer_decoded_video_packet*>(packet);
        if (old != nullptr && m_player != nullptr) {
            esplusplayer_decoded_buffer_destroy(m_player, old);
        }
    }
    requestCompositeForVideoFrame();
}

void MediaPlayerESPlusPlayer::requestCompositeForVideoFrame()
{
    // Coalesce per-frame composite idlers; at most one pending (same as
    // the capi path).
    Locker<Mutex> locker(*m_setNeedsCompositeEventIdlerHandleMutex);
    if (m_setNeedsCompositeEventIdlerHandle == MessageLoopInvalidID) {
        m_setNeedsCompositeEventIdlerHandle =
            window()
                ->webView()
                ->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    window(),
                    [](size_t, void* data) {
                        MediaPlayerESPlusPlayer* self =
                            (MediaPlayerESPlusPlayer*)data;
                        {
                            Locker<Mutex> locker(
                                *self->m_setNeedsCompositeEventIdlerHandleMutex);
                            self->m_setNeedsCompositeEventIdlerHandle =
                                MessageLoopInvalidID;
                        }
                        if (self->alive() == false) {
                            return;
                        }
                        if (self->container() != nullptr &&
                            self->container()->frame() != nullptr) {
                            self->container()->setNeedsCompositeForVideoFrame();
                        }
                    },
                    this);
    }
}

void MediaPlayerESPlusPlayer::willDrawVideo(Compositor* canvas,
                                            const LayoutRect& videoRect)
{
    STARFISH_ASSERT(canvas != nullptr);
    canvas->setFillColor(Unit::Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
    if (m_overlayMode) {
        // Video is on the HW plane; didDrawVideo punches the hole.
        return;
    }
    Locker<Mutex> l(*m_decodedVideoFrameMutex);
    if (m_lastDecodedPacket != nullptr &&
        m_lastDecodedPacket->surface_data != nullptr) {
        m_canvasSurface->attachPlatformExternalBuffer(
            (tbm_surface_h)m_lastDecodedPacket->surface_data);
    }
}

void MediaPlayerESPlusPlayer::didDrawVideo(Compositor* canvas,
                                           const LayoutRect& videoRect,
                                           const LayoutRect& absVideoRect)
{
    if (!m_overlayMode || alive() == false || m_player == nullptr ||
        !m_prepared) {
        return;
    }
    if (playbackState() == MediaPlayer::PLAYBACK_STATE_END) {
        return;
    }
    punchHole(canvas, videoRect, absVideoRect);
}

void MediaPlayerESPlusPlayer::hideVideoOverlay()
{
    if (!m_overlayMode) {
        return;
    }
    setOverlayPlaneVisible(false);
}

// ---------------------------------------------------------------------------
// playback control
// ---------------------------------------------------------------------------

// Drives HTMLMediaElement's official playback position (and thus JS
// timeupdate/currentTime) while playing, same as the capi path's
// updateTimeCallback. Without this the page sees currentTime stuck at the
// last seek target and e.g. the YouTube player stays in BUFFERING forever.
static void esppUpdateTimeCallback(void* data)
{
    MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
    if (self->seeking() == true || self->alive() == false) {
        return;
    }
    self->container()->setOfficialPlaybackPosition(self->currentTime());
}

void MediaPlayerESPlusPlayer::startCurrentTimeUpdateTimer()
{
    if (m_currentTimeUpdateTimer != TimerInvalidID) {
        return;
    }
    m_container->executionContext()->addPointerInRootSet(this);
    m_currentTimeUpdateTimer =
        m_container->window()->setInterval(esppUpdateTimeCallback, 250, this);
}

void MediaPlayerESPlusPlayer::stopCurrentTimeUpdateTimer()
{
    if (m_currentTimeUpdateTimer == TimerInvalidID) {
        return;
    }
    if (m_container != nullptr) {
        m_container->executionContext()->removePointerFromRootSet(this);
        m_container->window()->clearInterval(m_currentTimeUpdateTimer);
    }
    m_currentTimeUpdateTimer = TimerInvalidID;
}

void MediaPlayerESPlusPlayer::play()
{
    STARFISH_LOG_INFO("ESPP: play (prepared:%d started:%d)", (int)m_prepared,
                      (int)m_started);
    if (alive() == false || m_foundError == true) {
        return;
    }
    if (!m_prepared) {
        m_pendingPlay = true;
        return;
    }
    if (playbackState() == MediaPlayer::PLAYBACK_STATE_PLAYING) {
        return;
    }
    int ret;
    if (!m_started) {
        ret = esplusplayer_start(m_player);
        if (ret == ESPLUSPLAYER_ERROR_TYPE_NONE) {
            m_started = true;
        }
    } else {
        ret = esplusplayer_resume(m_player);
    }
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: start/resume failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return;
    }
    setPlaybackState(MediaPlayer::PLAYBACK_STATE_PLAYING);
    startCurrentTimeUpdateTimer();
    // Volume/mute may have been requested before prepare completed.
    setVolume(m_volume);
    if (m_muted) {
        setMuted(true);
    }
    wakeMseThread();
}

void MediaPlayerESPlusPlayer::pause()
{
    STARFISH_LOG_INFO("ESPP: pause");
    if (m_player == nullptr || !m_started) {
        m_pendingPlay = false;
        return;
    }
    if (playbackState() != MediaPlayer::PLAYBACK_STATE_PLAYING) {
        return;
    }
    stopCurrentTimeUpdateTimer();
    int ret = esplusplayer_pause(m_player);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: pause failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
    }
    setPlaybackState(MediaPlayer::PLAYBACK_STATE_PAUSED);
}

void MediaPlayerESPlusPlayer::seek(double time)
{
    STARFISH_LOG_INFO("ESPP: seek(%lf)", time);
    if (alive() == false || m_foundError == true) {
        return;
    }
    if (!m_prepared || m_seekState != SEEKSTATE_NO_SEEK) {
        // Not prepared yet, or a seek is in flight: remember the latest
        // target and chain it from handlePrepared/handleSeeked.
        m_pendingSeekTime = time;
        return;
    }
    m_seekState = SEEKSTATE_SEEKING;
    m_lastSeekTargetTime = time;
    prepareStreamsForNativeSeek();
    int ret = esplusplayer_seek(m_player, (uint64_t)(time * 1000));
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: seek failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        m_foundError = true;
        handleSeeked(false);
        return;
    }
    // Watchdog: if seek_done never arrives (wedged pipeline / bad segment /
    // resource conflict), unwedge m_seekState instead of buffering forever.
    armSeekWatchdog();
}

void MediaPlayerESPlusPlayer::prepareStreamsForNativeSeek()
{
    {
        // Match the capi-media-player seek: only reset the packet access
        // cache and let the native player drive the seek. Do NOT reset the
        // demuxer parser state (as an earlier version did via resetForSeek):
        // recreating the demuxer drops the parsed stream info (EBML/Tracks),
        // and the media-only segment YouTube appends after a seek -- which
        // carries no init data -- then fails findStreamInfo, errors the
        // media element and triggers a reload loop. The persistent demuxer
        // resyncs to the new cluster boundary on the discontinuous append.
        // Guarded by the feed lock so it cannot race a fillBuffer already
        // reading the source buffers.
        Locker<Mutex> locker(*m_fillBufferMutex);
        SourceBuffer* asb = activeSourceBuffer(StreamTypeAudio);
        SourceBuffer* vsb = activeSourceBuffer(StreamTypeVideo);
        if (asb != nullptr) {
            asb->clearPacketAccessCache();
        }
        if (vsb != nullptr) {
            vsb->clearPacketAccessCache();
        }
    }
    if (m_audioStream != nullptr) {
        m_audioStream->m_shouldFeed = false;
        m_audioStream->m_seekCursorStale = true;
    }
    if (m_videoStream != nullptr) {
        m_videoStream->m_shouldFeed = false;
        m_videoStream->m_seekCursorStale = true;
    }
}

bool MediaPlayerESPlusPlayer::supersedeSeek(double time)
{
    if (alive() == false || m_foundError == true || m_player == nullptr ||
        !m_prepared || m_seekState == SEEKSTATE_NO_SEEK) {
        return false;
    }
    STARFISH_LOG_INFO("ESPP: supersedeSeek(%lf) over in-flight target %lf",
                      time, m_lastSeekTargetTime);
    // Retarget the in-flight native seek. Without this the pipeline
    // deadlocks on rapid consecutive seeks: the native player waits to
    // preroll at the OLD target while the page fetches and appends data
    // only for the NEW one, so seek_done never comes and the watchdog
    // eventually tears the player down (observed with YouTube: seekTo A,
    // then seekTo B 400ms later, froze playback until the teardown).
    prepareStreamsForNativeSeek();
    int ret = esplusplayer_seek(m_player, (uint64_t)(time * 1000));
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        // Could not retarget (backend rejected the mid-seek seek). Fall
        // back to the legacy chain: finish the old seek, then seek again
        // from handleSeeked.
        PLAYER_LOGE("ESPP: supersede seek failed: %s",
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        m_pendingSeekTime = time;
        return true;
    }
    m_lastSeekTargetTime = time;
    m_pendingSeekTime = std::numeric_limits<double>::quiet_NaN();
    // Fresh watchdog for the new target (armSeekWatchdog no-ops while
    // armed, so drop the old one first).
    if (m_seekingTimer != TimerInvalidID && m_container != nullptr) {
        m_container->window()->clearTimeout(m_seekingTimer);
        m_seekingTimer = TimerInvalidID;
    }
    armSeekWatchdog();
    return true;
}

void MediaPlayerESPlusPlayer::armSeekWatchdog()
{
    if (m_seekingTimer != TimerInvalidID || m_container == nullptr) {
        return;
    }
    m_seekWatchdogStamp = seekProgressStamp();
    m_seekingTimer = m_container->window()->setTimeout(
        [](void* data) {
            MediaPlayerESPlusPlayer* self = (MediaPlayerESPlusPlayer*)data;
            self->handleSeekTimeout();
        },
        STARFISH_ESPP_SEEK_WATCHDOG_MS, this);
}

uint64_t MediaPlayerESPlusPlayer::seekProgressStamp()
{
    uint64_t stamp = 0;
    ESPPSourceStream* streams[2] = { m_audioStream, m_videoStream };
    for (int i = 0; i < 2; i++) {
        ESPPSourceStream* st = streams[i];
        if (st == nullptr) {
            continue;
        }
        stamp = stamp * 1000003 + st->m_lastSubmittedDTS;
        SourceBuffer* sb = activeSourceBuffer(st->m_type);
        if (sb != nullptr) {
            stamp = stamp * 1000003 +
                    sb->lastBufferedTimestamp(activeStreamIndex(st->m_type));
        }
    }
    return stamp;
}

void MediaPlayerESPlusPlayer::handleSeekTimeout()
{
    STARFISH_ASSERT(isMainThread());
    m_seekingTimer = TimerInvalidID;
    if (m_seekState == SEEKSTATE_NO_SEEK || alive() == false) {
        return;
    }
    uint64_t stamp = seekProgressStamp();
    if (stamp != m_seekWatchdogStamp) {
        // Something moved during the period -- the page is still appending
        // (slow network fetching the target segment) or the feeder is still
        // submitting. Not a wedge: keep waiting, and fail only after a full
        // period with no movement at all.
        STARFISH_LOG_INFO(
            "ESPP: seek watchdog re-armed (target:%lf), still progressing",
            m_lastSeekTargetTime);
        armSeekWatchdog();
        return;
    }
    PLAYER_LOGE("ESPP: seek watchdog fired (target:%lf) — seek_done never came",
                m_lastSeekTargetTime);
    // Treat as a failed seek: same recovery contract as the capi backend
    // (mediaPlayerNotifySeekFailure + teardown, letting the element rebuild).
    m_foundError = true;
    handleSeeked(false);
}

double MediaPlayerESPlusPlayer::currentTime()
{
    // esplusplayer_get_playing_time must not be called before the player
    // is prepared (it can block the caller).
    if (m_player != nullptr && m_prepared) {
        uint64_t ms = 0;
        if (esplusplayer_get_playing_time(m_player, &ms) ==
            ESPLUSPLAYER_ERROR_TYPE_NONE) {
            m_lastPositionMs = ms;
        }
    }
    return m_lastPositionMs / 1000.0;
}

double MediaPlayerESPlusPlayer::duration()
{
    if (m_activeMediaSource != nullptr) {
        return m_activeMediaSource->duration();
    }
    return 0;
}

void MediaPlayerESPlusPlayer::setVolume(double volume)
{
    m_volume = volume;
    if (m_player == nullptr || !m_prepared || m_muted) {
        return;
    }
    int v = (int)(volume * 100);
    if (v < 0) {
        v = 0;
    } else if (v > 100) {
        v = 100;
    }
    int ret = esplusplayer_set_volume(m_player, v);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: set_volume(%d) failed: %s", v,
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
    }
}

void MediaPlayerESPlusPlayer::setPlaybackRate(double rate)
{
    STARFISH_LOG_INFO("ESPP: setPlaybackRate(%lf)", rate);
    if (rate <= 0) {
        // esplusplayer has no negative/zero rate; pause semantics are
        // handled by the element.
        return;
    }
    // Device-verified on Tizen 10.0 mobile: the trackrenderer accepts a
    // non-1.0 rate but then stops consuming; even a full flush/preroll
    // cycle completes without playback ever progressing at the new rate.
    // Keep real-time playback (the element still reflects the requested
    // rate) unless native rate application is explicitly opted in for
    // platforms where it works.
    static bool nativeRateEnabled = []() {
        const char* v = getenv("STARFISH_ESPP_NATIVE_RATE");
        return v && *v && atoi(v) != 0;
    }();
    if (!nativeRateEnabled) {
        STARFISH_LOG_INFO(
            "ESPP: native rate application disabled on this platform; "
            "keeping 1.0");
        return;
    }
    m_playbackRate = rate;
    if (m_player == nullptr || !m_prepared) {
        return;
    }
    int ret = esplusplayer_set_playback_rate(m_player, rate,
                                             /* audio_mute = */ false);
    if (ret != ESPLUSPLAYER_ERROR_TYPE_NONE) {
        PLAYER_LOGE("ESPP: set_playback_rate(%lf) failed: %s", rate,
                    esplusplayer_get_error_string(
                        static_cast<esplusplayer_error_type>(ret)));
        return;
    }
    if (m_started && playbackState() == MediaPlayer::PLAYBACK_STATE_PLAYING) {
        // Observed on Tizen 10.0 mobile: after a live rate change the
        // trackrenderer stops consuming and does not recover even when
        // the rate is restored to 1.0. Re-arm the pipeline through the
        // proven seek path (flush + IDR-aligned refeed + preroll) at the
        // current position.
        seek(currentTime());
    }
}

void MediaPlayerESPlusPlayer::setMuted(bool muted)
{
    PLAYER_LOGI("MediaPlayerESPlusPlayer::setMuted(%d)", (int)muted);
    m_muted = muted;
    if (m_player == nullptr || !m_prepared) {
        return;
    }
    // No dedicated mute API confirmed on esplusplayer; emulate with
    // volume 0 / restore.
    if (muted) {
        esplusplayer_set_volume(m_player, 0);
    } else {
        setVolume(m_volume);
    }
}

// ---------------------------------------------------------------------------
// teardown
// ---------------------------------------------------------------------------

void MediaPlayerESPlusPlayer::destroy()
{
    STARFISH_ASSERT(isMainThread());
    if (m_alive == false) {
        return;
    }
    STARFISH_LOG_INFO("ESPP: destroy()");
    if (m_inPrepare == true) {
        // Unwind through handlePrepared so the root-set entry is removed
        // and pending operations resolve.
        m_foundError = true;
        handlePrepared(false);
        return;
    }
    if (m_seekState != SEEKSTATE_NO_SEEK) {
        if (m_foundError == true) {
            // Genuine pipeline error while a seek is in flight: report it
            // as a seek failure (unwinds through handleSeeked -> notify ->
            // destroy() again with the state cleared).
            handleSeeked(false);
            return;
        }
        // Element-driven teardown (JS load()/src change): the element has
        // already cleared its own seeking state, so abort the in-flight
        // seek SILENTLY and continue. Routing this through
        // handleSeeked(false) fired mediaPlayerNotifySeekFailure ->
        // dedicatedMediaSourceFailure, i.e. a spurious MEDIA_ERR + error
        // event in the middle of the page's own load() -- observed wedging
        // YouTube into a permanent spinner.
        m_seekState = SEEKSTATE_NO_SEEK;
        m_pendingSeekTime = std::numeric_limits<double>::quiet_NaN();
        if (m_seekingTimer != TimerInvalidID && m_container != nullptr) {
            m_container->window()->clearTimeout(m_seekingTimer);
            m_seekingTimer = TimerInvalidID;
        }
    }

    m_alive = false;
    if (m_foundError == true && m_container != nullptr) {
        m_container->dispatchErrorEvent();
    }

    dispose();

    if (m_canvasSurface != nullptr) {
        m_canvasSurface->detachNativeBuffer();
        m_canvasSurface = nullptr;
    }
}

void MediaPlayerESPlusPlayer::dispose()
{
    STARFISH_LOG_INFO("ESPP: dispose (%p)", this);
    stopCurrentTimeUpdateTimer();
    if (m_seekingTimer != TimerInvalidID && m_container != nullptr) {
        m_container->window()->clearTimeout(m_seekingTimer);
        m_seekingTimer = TimerInvalidID;
    }
    if (m_playerDeadFlag != nullptr) {
        *m_playerDeadFlag = true;
        wakeMseThread();
        // Join before tearing the player down (same as the capi path):
        // without it the feed thread can still be inside
        // esplusplayer_submit_packet / m_activeMediaSource when the
        // handle is destroyed and the source detached below.
        if (m_mseThread != nullptr) {
            m_mseThread->joinIfNeeds();
        }
        free((void*)m_playerDeadFlag);
        m_playerDeadFlag = nullptr;
    }
    if (m_player != nullptr) {
        esplusplayer_stop(m_player);
        {
            Locker<Mutex> l(*m_decodedVideoFrameMutex);
            if (m_lastDecodedPacket != nullptr) {
                esplusplayer_decoded_buffer_destroy(m_player,
                                                    m_lastDecodedPacket);
                m_lastDecodedPacket = nullptr;
            }
        }
        esplusplayer_set_ready_to_prepare_cb(m_player, nullptr, nullptr);
        esplusplayer_set_prepare_async_done_cb(m_player, nullptr, nullptr);
        esplusplayer_set_eos_cb(m_player, nullptr, nullptr);
        esplusplayer_set_media_packet_video_decoded_cb(m_player, nullptr,
                                                       nullptr);
        esplusplayer_set_ready_to_seek_cb(m_player, nullptr, nullptr);
        esplusplayer_set_seek_done_cb(m_player, nullptr, nullptr);
        esplusplayer_set_error_cb(m_player, nullptr, nullptr);
        esplusplayer_set_resource_conflicted_cb(m_player, nullptr, nullptr);
        esplusplayer_set_buffer_byte_status_cb(m_player, nullptr, nullptr);
        esplusplayer_close(m_player);
        esplusplayer_destroy(m_player);
        m_player = nullptr;
    }
    if (m_activeMediaSource != nullptr) {
        m_activeMediaSource->removeClient(m_mseClient);
        m_activeMediaSource->detach();
        m_activeMediaSource = nullptr;
    }
    m_mseClient = nullptr;
    m_audioStream = nullptr;
    m_videoStream = nullptr;
    m_mseThread = nullptr;
    m_prepared = false;
    m_started = false;
}

} // namespace Starfish

#endif
#endif
