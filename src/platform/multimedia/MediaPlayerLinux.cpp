/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/multimedia/MediaPlayerLinux.h"

#include <chrono>
#include <dlfcn.h>
#include <time.h>

namespace {

// PulseAudio Simple API loaded at runtime via dlopen so we do not have to add
// a build-time dependency on libpulse-dev. See pulse/simple.h / pulse/sample.h.
typedef enum {
    PA_SAMPLE_S16LE = 3,
} pa_sample_format_t;

typedef enum {
    PA_STREAM_PLAYBACK = 1,
} pa_stream_direction_t;

struct pa_sample_spec {
    pa_sample_format_t format;
    uint32_t rate;
    uint8_t channels;
};

typedef struct pa_simple pa_simple;
typedef struct pa_channel_map pa_channel_map;

struct pa_buffer_attr {
    uint32_t maxlength;
    uint32_t tlength;
    uint32_t prebuf;
    uint32_t minreq;
    uint32_t fragsize;
};

typedef pa_simple* (*pa_simple_new_fn)(const char*, const char*,
                                       pa_stream_direction_t, const char*,
                                       const char*, const pa_sample_spec*,
                                       const pa_channel_map*,
                                       const pa_buffer_attr*, int*);
typedef int (*pa_simple_write_fn)(pa_simple*, const void*, size_t, int*);
typedef int (*pa_simple_drain_fn)(pa_simple*, int*);
typedef void (*pa_simple_free_fn)(pa_simple*);

struct PulseSimpleApi {
    pa_simple_new_fn pa_simple_new;
    pa_simple_write_fn pa_simple_write;
    pa_simple_drain_fn pa_simple_drain;
    pa_simple_free_fn pa_simple_free;
};

static PulseSimpleApi* loadPulseSimple(void*& handleOut)
{
    static PulseSimpleApi s_api;
    static void* s_handle = nullptr;
    static bool s_loaded = false;
    static bool s_failed = false;
    if (s_failed) {
        return nullptr;
    }
    if (s_loaded) {
        handleOut = s_handle;
        return &s_api;
    }
    s_handle = dlopen("libpulse-simple.so.0", RTLD_NOW | RTLD_GLOBAL);
    if (s_handle == nullptr) {
        s_failed = true;
        return nullptr;
    }
    s_api.pa_simple_new = (pa_simple_new_fn)dlsym(s_handle, "pa_simple_new");
    s_api.pa_simple_write =
        (pa_simple_write_fn)dlsym(s_handle, "pa_simple_write");
    s_api.pa_simple_drain =
        (pa_simple_drain_fn)dlsym(s_handle, "pa_simple_drain");
    s_api.pa_simple_free = (pa_simple_free_fn)dlsym(s_handle, "pa_simple_free");
    if (s_api.pa_simple_new == nullptr || s_api.pa_simple_write == nullptr ||
        s_api.pa_simple_free == nullptr) {
        dlclose(s_handle);
        s_handle = nullptr;
        s_failed = true;
        return nullptr;
    }
    s_loaded = true;
    handleOut = s_handle;
    return &s_api;
}

} // namespace

namespace Starfish {

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100
#define STARFISH_MSE_SUBMIT_BYTES_RATE 0.3
#define STARFISH_MSE_MIN_MARGIN_IN_MS 3000

// How far ahead of the playback clock the decode pipeline may run, in ms.
// Each queued frame is a full-resolution RGBA buffer (e.g. 1080x1920 ~= 8MB),
// so a wide window keeps that many decoded frames resident -> memory pressure
// and GC churn (the main cause of bursty mid-playback stalls at high
// resolutions). Default 400ms; override at runtime via the
// STARFISH_DECODE_LOOKAHEAD_MS env var for A/B tuning without a rebuild.
#define STARFISH_DECODE_LOOKAHEAD_MS_DEFAULT 400

static uint64_t decodeLookaheadMs()
{
    static const uint64_t value = []() -> uint64_t {
        const char* e = getenv("STARFISH_DECODE_LOOKAHEAD_MS");
        if (e != nullptr) {
            int v = atoi(e);
            if (v > 0) {
                return (uint64_t)v;
            }
        }
        return STARFISH_DECODE_LOOKAHEAD_MS_DEFAULT;
    }();
    return value;
}

#define RETURN_WHEN_PLAYER_ERROR(...) \
    if (ret != PLAYER_ERROR_NONE) {   \
        PLAYER_LOGE(__VA_ARGS__);     \
        printNativePlayerError(ret);  \
        handlePlayerError();          \
        return;                       \
    }

void MediaPlayerLinux::printNativePlayerError(int errorCode)
{
    PLAYER_LOGI("MediaPlayerLinux::printNativePlayerError\n");
    switch (errorCode) {
#define F(errorenum)                   \
    case errorenum:                    \
        PLAYER_LOGI("%s", #errorenum); \
        return;
#undef F
    default:
        PLAYER_LOGI("Unknown error");
        return;
    }
}

static void completeCallback(void* data)
{
    PLAYER_LOGI("completeCallback\n");
    MediaPlayerLinux* player = (MediaPlayerLinux*)data;
    player->handleEnded();
}

static void preparedCallback(void* data)
{
    PLAYER_LOGI("preparedCallback\n");
    MediaPlayerLinux* self = (MediaPlayerLinux*)data;
    self->handlePrepared();
}

static void seekedCallback(void* data)
{
    PLAYER_LOGI("player_set_play_position_cb");
    MediaPlayerLinux* self = (MediaPlayerLinux*)data;
    self->handleSeeked();
}
static void printMediaPacketError(int errorCode)
{
    PLAYER_LOGI("printMediaPacketError\n");
    switch (errorCode) {
#define F(errorenum)                   \
    case errorenum:                    \
        PLAYER_LOGI("%s", #errorenum); \
        return;
#undef F
    default:
        PLAYER_LOGI("Unknown error");
        return;
    }
}

static void printMediaFormatError(int errorCode)
{
    PLAYER_LOGI("printMediaFormatError\n");
    switch (errorCode) {
#define F(errorenum)                   \
    case errorenum:                    \
        PLAYER_LOGI("%s", #errorenum); \
        return;
#undef F
    default:
        PLAYER_LOGI("Unknown error");
        return;
    }
}

FfmpegWrapperPlayer::FfmpegWrapperPlayer()
    : m_url(nullptr)
    , m_fmtCtx(nullptr)
    , m_codecCtx(nullptr)
    , m_videoStreamIndex(-1)
    , m_framedecodedCallbackData(nullptr)
    , m_completeCallbackData(nullptr)
    , m_errorCallbackData(nullptr)
    , m_bufferingCallbackData(nullptr)
    , m_stopRequested(false)
    , m_state(State::STOPPED)
    , m_muted(false)
    , m_volume(1.0f)
    , m_currentPositionMs(0)
    , m_seekRequested(false)
    , m_seekTargetMs(0)
    , m_seekCompleteData(nullptr)
{
    // Allocated via `new (PointerFreeGC)` (GC_MALLOC_ATOMIC), which is
    // allowed to hand back non-zeroed memory — especially when the GC
    // recycles a slot freed by a prior MediaPlayerLinux destroy/dispose
    // cycle. Without these explicit initializers, `m_url` carried stale
    // pointer bits from the previous owner, `if (m_url)` was true in
    // prepare(), and the YouTube MSE path crashed in
    // String::toUTF8NonGCString on the second load() (release build:
    // SEGV at ResourceURL::urlString() returning a junk String*).
}

FfmpegWrapperPlayer::~FfmpegWrapperPlayer()
{
    stop();

    // Wait for decoding thread to finish if it's running
    if (m_decodingThread.joinable()) {
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_stopRequested = true;
            m_statecv.notify_all();
        }
        m_decodingThread.join();
    }
}

void FfmpegWrapperPlayer::setUrl(ResourceURL* url)
{
    PLAYER_LOGI("FfmpegWrapperPlayer::setUrl %s \n",
                url->urlString()->toUTF8NonGCString().c_str());
    m_url = url;
}

bool FfmpegWrapperPlayer::setVideoFrameDecodedCB(
    std::function<void(LinuxMediaPacket* buffer, void* data)>
        framedecodedCallback,
    void* data)
{
    m_framedecodedCallback = framedecodedCallback;
    m_framedecodedCallbackData = data;
    return true;
}

bool FfmpegWrapperPlayer::setcompleteCB(
    std::function<void(void* data)> completeCallback, void* data)
{
    m_completeCallback = completeCallback;
    m_completeCallbackData = data;
    return true;
}

bool FfmpegWrapperPlayer::setErrorCB(
    std::function<void(int errorCode, void* data)> errorCallback, void* data)
{
    m_errorCallback = errorCallback;
    m_errorCallbackData = data;
    return true;
}

bool FfmpegWrapperPlayer::setBufferingCB(
    std::function<void(int percent, void* data)> bufferingCallback, void* data)
{
    m_bufferingCallback = bufferingCallback;
    m_bufferingCallbackData = data;
    return true;
}

bool FfmpegWrapperPlayer::unsetVideoFrameDecodedCB()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    STARFISH_UNIMPLEMENTED();
    return false;
}

bool FfmpegWrapperPlayer::unsetcompleteCB()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    STARFISH_UNIMPLEMENTED();
    return false;
}

bool FfmpegWrapperPlayer::unsetErrorCB()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    STARFISH_UNIMPLEMENTED();
    return false;
}

bool FfmpegWrapperPlayer::unsetBufferingCB()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    STARFISH_UNIMPLEMENTED();
    return false;
}

bool FfmpegWrapperPlayer::setPlayPosition(
    int milliseconds, bool accurate,
    const std::function<void(void* data)>& seekCompleteCallback,
    void* user_data)
{
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_fmtCtx == nullptr || m_videoStreamIndex < 0) {
            return false;
        }
        m_seekTargetMs.store(milliseconds < 0 ? 0 : milliseconds);
        m_seekCompleteCallback = seekCompleteCallback;
        m_seekCompleteData = user_data;
        m_seekRequested.store(true);
    }
    // Wake the decoding thread so it picks up the seek promptly even if it is
    // currently paused.
    m_statecv.notify_all();
    return true;
}

bool FfmpegWrapperPlayer::unprepare()
{
    STARFISH_UNIMPLEMENTED();
    return false;
}

void FfmpegWrapperPlayer::destroy()
{
    STARFISH_UNIMPLEMENTED();
}

bool FfmpegWrapperPlayer::prepare(
    const std::function<void(void* data)>& preparedCallback, void* data)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (m_state != State::STOPPED) {
        PLAYER_LOGI(
            "[FfmpegWrapperPlayer] Error: Player is not in stopped state\n");
        return false;
    }

    if (m_url) {
        PLAYER_LOGI("[FfmpegWrapperPlayer] Opening url : %s\n",
                    m_url->urlString()->toUTF8NonGCString().c_str());

        if (avformat_open_input(&m_fmtCtx,
                                m_url->urlString()->toUTF8NonGCString().c_str(),
                                nullptr, nullptr) < 0) {
            PLAYER_LOGI(
                "[FfmpegWrapperPlayer] Error: Could not open source url: %s\n",
                m_url->urlString()->toUTF8NonGCString().c_str());
            return false;
        }
        PLAYER_LOGI("[FfmpegWrapperPlayer] url opened successfully\n");

        if (avformat_find_stream_info(m_fmtCtx, nullptr) < 0) {
            PLAYER_LOGI(
                "[FfmpegWrapperPlayer] Could not find stream information\n");
            return false;
        }

        m_videoStreamIndex = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO,
                                                 -1, -1, nullptr, 0);
        if (m_videoStreamIndex < 0) {
            PLAYER_LOGI(
                "[FfmpegWrapperPlayer] Error: Could not find video stream\n");
            return false;
        }
        PLAYER_LOGI("[FfmpegWrapperPlayer] Found video stream at index: %d \n",
                    m_videoStreamIndex);

        AVCodecParameters* codec_par =
            m_fmtCtx->streams[m_videoStreamIndex]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
        if (!codec) {
            PLAYER_LOGI("[FfmpegWrapperPlayer] Error: Codec not found\n");
        }
        PLAYER_LOGI("[FfmpegWrapperPlayer] Found codec: %s \n", codec->name);

        m_codecCtx = avcodec_alloc_context3(codec);
        if (!m_codecCtx) {
            PLAYER_LOGI(
                "[FfmpegWrapperPlayer] Could not allocate codec context\n");
        }

        if (avcodec_parameters_to_context(m_codecCtx, codec_par) < 0) {
            PLAYER_LOGI(
                "[FfmpegWrapperPlayer] Could not copy codec parameters to "
                "context\n");
        }

        if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
            PLAYER_LOGI("[FfmpegWrapperPlayer] Error: Could not open codec\n");
        }
    }

    m_state = State::PREPARED;
    preparedCallback(data);

    return true;
}

bool FfmpegWrapperPlayer::prepare()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    STARFISH_UNIMPLEMENTED();
    return false;
}

bool FfmpegWrapperPlayer::play()
{
    PLAYER_LOGI("FfmpegWrapperPlayer::play\n");

    // std::lock_guard<std::mutex> lock(m_stateMutex);

    if (m_state != State::PREPARED && m_state != State::PAUSED) {
        PLAYER_LOGI(
            "[FfmpegWrapperPlayer] Error: Player is not in prepared or paused "
            "state\n");
        return false;
    }

    // Start decoding thread if not already running. Skip when there is no
    // demuxer context (MSE mode) - the FFmpeg-based packet read loop has no
    // input in that case and would crash on av_read_frame(nullptr).
    if (m_fmtCtx != nullptr && m_state == State::PREPARED &&
        !m_decodingThread.joinable()) {
        m_stopRequested = false;
        m_decodingThread =
            std::thread(&FfmpegWrapperPlayer::decodingThread, this);
    }

    m_state = State::PLAYING;
    m_statecv.notify_all();

    return true;
}

bool FfmpegWrapperPlayer::pause()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (m_state == State::PAUSED) {
        PLAYER_LOGI("[FfmpegWrapperPlayer] Already paused\n");
        return true;
    }

    if (m_state != State::PLAYING) {
        PLAYER_LOGI(
            "[FfmpegWrapperPlayer] Error: Cannot pause - player is not in "
            "playing state (current state: %d)\n",
            (int)m_state);
        return false;
    }

    m_state = State::PAUSED;

    // Notify the decoding thread to stop processing
    m_statecv.notify_all();

    PLAYER_LOGI("[FfmpegWrapperPlayer] Player paused successfully\n");
    return true;
}

bool FfmpegWrapperPlayer::stop()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (m_state == State::STOPPED) {
        return true; // Already stopped
    }

    m_state = State::STOPPED;
    m_stopRequested = true;
    m_currentPositionMs.store(0);
    m_statecv.notify_all();

    return true;
}

bool FfmpegWrapperPlayer::mute(bool mute)
{
    m_muted = mute;
    return true;
}

bool FfmpegWrapperPlayer::setVolume(float volume)
{
    if (volume < 0.0f || volume > 1.0f) {
        PLAYER_LOGI(
            "[FfmpegWrapperPlayer] Error: Volume must be between 0.0 and "
            "1.0\n");
        return false;
    }

    m_volume = volume;
    PLAYER_LOGI("[FfmpegWrapperPlayer] Volume set to: %f\n", volume);
    return true;
}

void FfmpegWrapperPlayer::setLooping(bool looping)
{
}

void FfmpegWrapperPlayer::setMemoryBuffer(void* buffer, int size)
{
}

player_state_e FfmpegWrapperPlayer::getState()
{
    switch (m_state) {
    case State::STOPPED:
        return PLAYER_STATE_IDLE;
    case State::PREPARED:
        return PLAYER_STATE_READY;
    case State::PLAYING:
        return PLAYER_STATE_PLAYING;
    case State::PAUSED:
        return PLAYER_STATE_PAUSED;
    default:
        return PLAYER_STATE_NONE;
    }
    return PLAYER_STATE_NONE;
}

int FfmpegWrapperPlayer::getPlayPosition()
{
    return (int)m_currentPositionMs.load();
}

double FfmpegWrapperPlayer::getDuration()
{
    if (m_fmtCtx != nullptr && m_fmtCtx->duration != AV_NOPTS_VALUE &&
        m_fmtCtx->duration > 0) {
        return (double)m_fmtCtx->duration / (double)AV_TIME_BASE;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

void FfmpegWrapperPlayer::decodingThread()
{
    PLAYER_LOGI("[FfmpegWrapperPlayer] Decoding thread started\n");

    // Processing loop
    std::vector<uint8_t> packet_data;
    std::vector<AVFrame*> frames;
    int64_t pts, dts;
    bool is_video;
    struct SwsContext* swsCtx = nullptr;

    while (!m_stopRequested) {
        // Check if we're in playing state
        {
            std::unique_lock<std::mutex> lock(m_stateMutex);
            m_statecv.wait(lock, [this] {
                return m_state == State::PLAYING || m_stopRequested;
            });

            if (m_stopRequested) {
                break;
            }
        }

        // Perform a pending seek here so the demuxer is only ever touched from
        // this thread.
        if (m_seekRequested.exchange(false)) {
            int64_t targetMs = m_seekTargetMs.load();
            AVRational tb = m_fmtCtx->streams[m_videoStreamIndex]->time_base;
            int64_t ts = (int64_t)((targetMs / 1000.0) / av_q2d(tb));
            int seekRet = av_seek_frame(m_fmtCtx, m_videoStreamIndex, ts,
                                        AVSEEK_FLAG_BACKWARD);
            if (seekRet < 0) {
                // Seek failed: the demuxer position is unchanged, so do not
                // advertise the target as the current position or drop frames.
                // Still fire the completion callback below so the player does
                // not wait forever for a seek that will never complete.
                STARFISH_LOG_ERROR(
                    "av_seek_frame failed (%d) for target %ld ms\n", seekRet,
                    (long)targetMs);
            } else {
                avcodec_flush_buffers(m_codecCtx);
                m_currentPositionMs.store((uint64_t)targetMs);

                // Drop frames decoded before the seek.
                for (AVFrame* f : frames) {
                    av_frame_free(&f);
                }
                frames.clear();
            }

            std::function<void(void* data)> cb;
            void* cbData = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                cb = m_seekCompleteCallback;
                cbData = m_seekCompleteData;
            }
            if (cb) {
                cb(cbData);
            }
        }

        AVPacket* pkt = av_packet_alloc();
        if (!pkt) {
            return;
        }

        int ret = av_read_frame(m_fmtCtx, pkt);
        if (ret < 0) {
            PLAYER_LOGI(
                "[FfmpegWrapperPlayer] End of file or error reading frame\n");
            av_packet_free(&pkt);
            return;
        }

        is_video = (pkt->stream_index == m_videoStreamIndex);
        if (is_video) {
            packet_data.assign(pkt->data, pkt->data + pkt->size);
            pts = pkt->pts;
            dts = pkt->dts;
            PLAYER_LOGI("[FfmpegWrapperPlayer] Read video packet. Size: %d\n",
                        pkt->size);
        } else {
            PLAYER_LOGI("[FfmpegWrapperPlayer] Skipped non-video packet\n");
        }

        ret = avcodec_send_packet(m_codecCtx, pkt);
        av_packet_free(&pkt);

        while (ret >= 0) {
            AVFrame* frame = av_frame_alloc();
            ret = avcodec_receive_frame(m_codecCtx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_frame_free(&frame);
                break;
            } else if (ret < 0) {
                av_frame_free(&frame);
                break;
            }
            frames.push_back(frame);
        }

        int width_ = 0;
        int height_ = 0;

        if (is_video) {
            for (AVFrame* frame : frames) {
                if (!swsCtx || width_ != frame->width ||
                    height_ != frame->height) {
                    if (swsCtx) {
                        sws_freeContext(swsCtx);
                    }
                    width_ = frame->width;
                    height_ = frame->height;
                    swsCtx = sws_getContext(
                        width_, height_, (AVPixelFormat)frame->format, width_,
                        height_, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr,
                        nullptr, nullptr);
                }
                int stride = width_ * 4;
                uint8_t* buffer = (uint8_t*)malloc(stride * height_);
                LinuxMediaPacket* packet =
                    new LinuxMediaPacket(width_, height_, stride);

                uint8_t* dest[4] = { (uint8_t*)buffer, nullptr, nullptr,
                                     nullptr };
                int dest_linesize[4] = { stride, 0, 0, 0 };

                sws_scale(swsCtx, frame->data, frame->linesize, 0, height_,
                          dest, dest_linesize);
                packet->setBuffer(buffer);

                // Advance the playhead to this frame's presentation time so
                // getPlayPosition() (and thus HTMLMediaElement.currentTime)
                // tracks progressive playback.
                int64_t framePts = frame->best_effort_timestamp;
                if (framePts == AV_NOPTS_VALUE) {
                    framePts = frame->pts;
                }
                if (framePts != AV_NOPTS_VALUE && m_fmtCtx != nullptr &&
                    m_videoStreamIndex >= 0) {
                    AVRational tb =
                        m_fmtCtx->streams[m_videoStreamIndex]->time_base;
                    double sec = (double)framePts * av_q2d(tb);
                    if (sec >= 0) {
                        m_currentPositionMs.store((uint64_t)(sec * 1000.0));
                    }
                }

                if (m_framedecodedCallback != nullptr &&
                    m_framedecodedCallbackData != nullptr) {
                    m_framedecodedCallback(packet, m_framedecodedCallbackData);
                }

                // Simulate frame rendering time
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(33)); // ~30 FPS

                av_frame_free(&frame);

                if (m_stopRequested) {
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        frames.clear();
    }
}

class MediaPlayerLinuxMediaSourceClient : public MediaSourceClient {
public:
    MediaPlayerLinuxMediaSourceClient(MediaPlayerLinux* player)
        : MediaSourceClient()
        , m_player(player)
    {
#ifndef NDEBUG
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                PLAYER_LOGI(
                    "[TRACE_MSE_GC] "
                    "MediaPlayerLinuxMediaSourceClient::~"
                    "MediaPlayerLinuxMediaSourceClient (%p)",
                    obj);
            },
            NULL, NULL, NULL);
#endif
    }

    virtual void activeSourceComputed()
    {
        PLAYER_LOGI(
            "MediaPlayerLinuxMediaSourceClient::activeSourceComputed\n");
        if (m_player != nullptr && m_player->alive() == true) {
            m_player->prepareMediaSource();
        }
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
        PLAYER_LOGI(
            "MediaPlayerLinuxMediaSourceClient::"
            "activeVideoSourceBufferUpdated\n");
        PLAYER_LOGI("activeVideoSourceBufferUpdated")
        if (m_player != nullptr && m_player->alive() == true &&
            m_player->activeSourceBuffer(StreamTypeVideo) == s) {
            m_player->fillBufferIfNeeded(StreamTypeVideo);
        }
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
        PLAYER_LOGI(
            "MediaPlayerLinuxMediaSourceClient::"
            "activeAudioSourceBufferUpdated\n");
        if (m_player != nullptr && m_player->alive() == true &&
            m_player->activeSourceBuffer(StreamTypeAudio) == s) {
            m_player->fillBufferIfNeeded(StreamTypeAudio);
        }
    }

    MediaPlayerLinux* m_player;
};

void MediaPlayerSourceStream::initFormatExtraForAudio()
{
}

void MediaPlayerSourceStream::initFormatExtraForVideo()
{
}

void MediaPlayerSourceStream::createMediaFormatStreamType()
{
}

void MediaPlayerSourceStream::releaseMediaFormatStreamType()
{
}

MediaPlayerSourceStream::MediaPlayerSourceStream(StreamType type)
    : m_type(type)
    , m_bufferState(BUFFERSTATE_INITIAL)
    , m_mediaStreamMutex(new Mutex())
    // , m_mediaFormat(nullptr)
    , m_maxBufferSize(0)
    , m_lastSubmittedDTS(0)
    , m_initSegmentIndex(0)
    , m_lastBufferBytes(0)
    , m_waitingDemuxer(false)
    , m_codecCtx(nullptr)
    , m_isAnnexB(false)
{
    PLAYER_LOGI("MediaPlayerSourceStream::MediaPlayerSourceStream\n");
    if (m_type == StreamTypeAudio) {
        m_maxBufferSize = 320 * 1000 / 8 * 5;
        initFormatExtraForAudio();
    } else {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        initFormatExtraForVideo();
    }
}

bool MediaPlayerSourceStream::createMediaFormat()
{
    return true;
}

void MediaPlayerSourceStream::releaseMediaFormat()
{
}

uint64_t MediaPlayerSourceStream::maxBufferSize()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_maxBufferSize;
}

void MediaPlayerSourceStream::setMaxBufferSize(uint64_t value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_maxBufferSize = value;
}

bool MediaPlayerSourceStream::needPacket()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_bufferState == BUFFERSTATE_UNDER_RUN ||
           m_bufferState == BUFFERSTATE_NEED_PACKET;
}

bool MediaPlayerSourceStream::isBufferState(BufferState state)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_bufferState == state;
}

MediaPlayerSourceStream::BufferState MediaPlayerSourceStream::bufferState()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_bufferState;
}

#ifdef STARFISH_MEDIAPLAYER_DEBUG
static const char* bufferStateString(MediaPlayerSourceStream::BufferState value)
{
    if (value == MediaPlayerSourceStream::BUFFERSTATE_INITIAL) {
        return "INITIAL";
    } else if (value == MediaPlayerSourceStream::BUFFERSTATE_UNDER_RUN) {
        return "UNDERRUN";
    } else if (value == MediaPlayerSourceStream::BUFFERSTATE_NEED_PACKET) {
        return "NEED_PACKET";
    } else if (value == MediaPlayerSourceStream::BUFFERSTATE_NORMAL) {
        return "NORMAL";
    }
    return "EOS";
}
#endif

void MediaPlayerSourceStream::setBufferState(BufferState value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_bufferState = value;
}

bool MediaPlayerSourceStream::waitingDemuxer()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_waitingDemuxer;
}

void MediaPlayerSourceStream::setWaitingDemuxer(bool value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_waitingDemuxer = value;
}

uint64_t MediaPlayerSourceStream::lastBufferBytes()
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    return m_lastBufferBytes;
}

void MediaPlayerSourceStream::setLastBufferBytes(size_t value)
{
    Locker<Mutex> locker(*m_mediaStreamMutex);
    m_lastBufferBytes = value;
}

MediaPlayerLinux::MediaPlayerLinux(HTMLMediaElement* element)
    : MediaPlayer(element)
    , m_inPrepare(false)
    , m_pendingPlay(false)
    , m_underrunMode(false)
    , m_seekingTimer(TimerInvalidID)
    , m_mseClient(nullptr)
    , m_fillBufferMutex(new Mutex())
    , m_decodedVideoFrameMutex(new Mutex())
    , m_lastDecodedVideoPacket(nullptr)
    , m_currentURL(nullptr)
    , m_setNeedsCompositeEventIdlerHandleMutex(new Mutex())
    , m_setNeedsCompositeEventIdlerHandle(MessageLoopInvalidID)
#if defined(STARFISH_RUN_MSE_THREAD)
    , m_mseThread(nullptr)
#endif
    , m_playerDeadFlag(nullptr)
    , m_audioStream(nullptr)
    , m_videoStream(nullptr)
    , m_framePoolWidth(0)
    , m_framePoolHeight(0)
    , m_swsCtx(nullptr)
    , m_swsCtxWidth(0)
    , m_swsCtxHeight(0)
    , m_clockStartMs(0)
    , m_clockOffsetSec(0)
    , m_audioSinkLib(nullptr)
    , m_audioSinkHandle(nullptr)
    , m_audioSinkChannels(0)
    , m_audioSinkRate(0)
    , m_swrCtx(nullptr)
    , m_swrChannels(0)
    , m_swrRate(0)
    , m_swrSrcFmt(-1)
    , m_audioWriterThread(nullptr)
    , m_audioWriterStop(false)
    , m_audioWriteQueueBytes(0)
{
    PLAYER_LOGI("MediaPlayerLinux::MediaPlayerLinux\n");
    STARFISH_ASSERT(element != nullptr);

    m_nativePlayer = new (PointerFreeGC) FfmpegWrapperPlayer();
}

void MediaPlayerLinux::handlePlayerError()
{
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerLinux* player = (MediaPlayerLinux*)data;
                player->handlePlayerError();
            },
            this);
        return;
    }

    m_foundError = true;
    if (m_inPrepare == true) {
        handlePrepared();
    } else if (m_seekState == SEEKSTATE_SEEKING) {
        handleSeeked();
    }
    destroy();
}

void MediaPlayerLinux::fillBufferIfNeeded(StreamType type)
{
    MediaPlayerSourceStream* stream = currentStream(type);
    if (stream == nullptr) {
        return;
    }
#ifdef STARFISH_RUN_MSE_THREAD
    stream->setWaitingDemuxer(false);
    wakeMseThread();
#else
    if (stream->waitingDemuxer() == true) {
        stream->setWaitingDemuxer(false);
        fillBuffer(stream);
    }
#endif
}

void MediaPlayerLinux::seek(double time)
{
    PLAYER_LOGI("MediaPlayerLinux::seek\n");
    if (m_inPrepare == true) {
        PLAYER_LOGI("MediaPlayerLinux::seek -> seeking failed saving time %lf",
                    time);
        m_container->setDefaultPlaybackStartPosition(time);
        if (isMSE() == true) {
            int timeInMS = time * 1000;
            if (m_audioStream != nullptr) {
                Locker<Mutex> locker(*m_fillBufferMutex);
                m_audioStream->setLastSubmittedDTS(timeInMS);
            }
            if (m_videoStream != nullptr) {
                Locker<Mutex> locker(*m_fillBufferMutex);
                m_videoStream->setLastSubmittedDTS(timeInMS);
            }
            m_container->mediaPlayerNotifySeekedItsContainer(time);
        }
        return;
    }
    STARFISH_ASSERT(m_seekState == SEEKSTATE_NO_SEEK);
    STARFISH_ASSERT(m_nativePlayer && m_alive);
    if (playbackState() == PLAYBACK_STATE_END) {
        setPlaybackState(PLAYBACK_STATE_PAUSED);
        if (m_audioStream != nullptr) {
            m_audioStream->setBufferState(
                MediaPlayerSourceStream::BUFFERSTATE_INITIAL);
        }
        if (m_videoStream != nullptr) {
            m_videoStream->setBufferState(
                MediaPlayerSourceStream::BUFFERSTATE_INITIAL);
        }
        m_nativePlayer->play();
        m_nativePlayer->pause();
        m_nativePlayer->setcompleteCB(completeCallback, this);
    }
    // Check seek boundary
    STARFISH_ASSERT(!std::isnan(time));
    double dur = duration();
    if (time < 0) {
        time = 0;
    } else if (dur != 0 && !std::isnan(dur) && time >= dur) {
        // Note: Seeking to EOS is impossible!!! (player_set_position fault)
        PLAYER_LOGI("MediaPlayerLinux::seek() reaches EOS");
        m_container->mediaPlayerNotifySeekedItsContainer(duration());
        handleEnded();
        return;
    }

    m_seekState = SEEKSTATE_SEEKING;
    m_container->executionContext()->addPointerInRootSet(this);

    // Set timer
    // Note : To avoid too much waiting 'seek' callback,
    //        set timer that would help the player to remove rooted pointer
    //        and properly destroyed
    m_seekingTimer = m_container->window()->setTimeout(
        [](void* data) {
            MediaPlayerLinux* self = (MediaPlayerLinux*)data;
            PLAYER_LOGI("MediaPlayerLinux::seek() : timeout");
            self->handleSeekTimeout();
        },
        MAX_WAITING_SECONDS_FOR_SEEK_OPERATION, this);

    seekOperation((int)(time * 1000.0));
}

void MediaPlayerLinux::seekOperation(int timeInMS)
{
    PLAYER_LOGI("MediaPlayerLinux::seekOperation() (time: %d)", timeInMS);
    if (isMSE()) {
        // MSE seek is driven by the demuxer feed; not handled here.
        return;
    }
    if (!m_nativePlayer) {
        return;
    }
    // setPlayPosition's completion callback runs on the decoding thread; hop
    // back to the main thread before touching the container / timers.
    m_nativePlayer->setPlayPosition(
        timeInMS, true,
        [](void* data) {
            MediaPlayerLinux* self = (MediaPlayerLinux*)data;
            MessageLoop* msgLoop = self->container()->webView()->messageLoop();
            msgLoop->addIdlerWithNoGCRootingInOtherThread(
                self->container()->window(),
                [](size_t, void* d) { ((MediaPlayerLinux*)d)->handleSeeked(); },
                self);
        },
        this);
}

void MediaPlayerLinux::handleSeeked()
{
    PLAYER_LOGI("MediaPlayerLinux::handleSeeked\n");
    STARFISH_ASSERT(isMainThread());
    if (m_seekState == SEEKSTATE_NO_SEEK) {
        return;
    }
    if (m_seekingTimer != TimerInvalidID) {
        m_container->window()->clearTimeout(m_seekingTimer);
        m_seekingTimer = TimerInvalidID;
    }
    m_seekState = SEEKSTATE_NO_SEEK;
    m_container->mediaPlayerNotifySeekedItsContainer(currentTime());
}

void MediaPlayerLinux::handleSeekTimeout()
{
    PLAYER_LOGI("MediaPlayerLinux::handleSeekTimeout\n");
    // Treat a timed-out seek as completed so the element does not stay stuck in
    // the seeking state.
    handleSeeked();
}

void MediaPlayerLinux::handleEnded()
{
    PLAYER_LOGI("MediaPlayerLinux::handleEnded\n");
    STARFISH_UNIMPLEMENTED();
}

double MediaPlayerLinux::currentTime()
{
    if (isMSE()) {
        if (playbackState() != PLAYBACK_STATE_PLAYING || m_clockStartMs == 0) {
            return m_clockOffsetSec;
        }
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t nowMs = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        double elapsed = (nowMs - m_clockStartMs) / 1000.0;
        return m_clockOffsetSec + elapsed;
    }
    if (m_nativePlayer) {
        return m_nativePlayer->getPlayPosition() / 1000.0;
    }
    return 0;
}

void MediaPlayerLinux::destroy()
{
    STARFISH_ASSERT(isMainThread());
    if (m_alive == false) {
        return;
    }
    PLAYER_LOGI("MediaPlayerLinux::destroy()");
    if (m_inPrepare == true) {
        m_foundError = true;
        handlePrepared();
        STARFISH_ASSERT(!m_alive);
        return;
    }
    if (m_seekState != SEEKSTATE_NO_SEEK) {
        m_foundError = true;
        handleSeeked();
        STARFISH_ASSERT(m_alive);
        return;
    }

    m_alive = false;
    // Dispatch error event when found error
    if (m_foundError == true) {
        m_container->dispatchErrorEvent();
    }

    pause();
    dispose();
}

double MediaPlayerLinux::duration()
{
    PLAYER_LOGI("MediaPlayerLinux::duration\n");
    if (m_activeMediaSource != nullptr) {
        return m_activeMediaSource->duration();
    }
    if (m_nativePlayer != nullptr) {
        return m_nativePlayer->getDuration();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

static void updateTimeCallback(void* data)
{
    PLAYER_LOGI("updateTimeCallback\n");
    MediaPlayerLinux* self = (MediaPlayerLinux*)data;
    if (self->seeking() == true || self->alive() == false) {
        // Do not update time while seeking
        return;
    }
    double position = self->currentTime();
    if (self->isMSE() == true &&
        position == self->container()->officialPlaybackPosition() &&
        self->isMSEBufferEOS() == true) {
        self->handleEnded();
    } else {
        self->container()->setOfficialPlaybackPosition(position);
    }
    // Drive the compositor at the timer cadence so the MSE/FFmpeg video
    // pipeline stays animating even if the per-frame setTimeout chain or
    // decoder-driven setNeedsComposite stalls.
    if (self->isMSE() == true && self->container() != nullptr &&
        self->container()->frame() != nullptr) {
        self->container()->setNeedsComposite();
    }
}

void MediaPlayerLinux::play()
{
    PLAYER_LOGI("MediaPlayerLinux::play\n");
    STARFISH_RELEASE_ASSERT(isMainThread());
    if (playbackState() == PLAYBACK_STATE_PLAYING) {
        return;
    }

    player_state_e state = m_nativePlayer->getState();
    PLAYER_LOGI("MediaPlayerLinux::play() state : %d state2: %d ms: %p",
                (int)state, (int)playbackState(), m_activeMediaSource);
    if (state < PLAYER_STATE_READY) {
        m_pendingPlay = true;
        return;
    }
    m_pendingPlay = false;
    setPlaybackState(PLAYBACK_STATE_PLAYING);
    if (isMSE()) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        m_clockStartMs = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
    m_nativePlayer->play();
    m_container->executionContext()->addPointerInRootSet(this);
    m_currentTimeUpdateTimer = m_container->window()->setInterval(
        updateTimeCallback, isMSE() ? 16 : 250, this);
}

void MediaPlayerLinux::pause()
{
    PLAYER_LOGI("MediaPlayerLinux::pause\n");
    if (playbackState() != PLAYBACK_STATE_PLAYING) {
        return;
    }
    if (isMSE() && m_clockStartMs != 0) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t nowMs = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        m_clockOffsetSec += (nowMs - m_clockStartMs) / 1000.0;
        m_clockStartMs = 0;
    }
    PLAYER_LOGI("pause()");
    setPlaybackState(PLAYBACK_STATE_PAUSED);
    if (m_container != nullptr) {
        m_container->executionContext()->removePointerFromRootSet(this);
        m_container->window()->clearInterval(m_currentTimeUpdateTimer);

        Locker<Mutex> locker(*m_setNeedsCompositeEventIdlerHandleMutex);
        if (m_setNeedsCompositeEventIdlerHandle != MessageLoopInvalidID) {
            MessageLoop* msgLoop = m_container->webView()->messageLoop();
            msgLoop->removeIdlerWithNoGCRooting(
                m_setNeedsCompositeEventIdlerHandle);
            m_setNeedsCompositeEventIdlerHandle = MessageLoopInvalidID;
        }
    }

    m_nativePlayer->pause();
    m_currentTimeUpdateTimer = TimerInvalidID;
}

void MediaPlayerLinux::setNativePlayerDefaultOptions(ResourceURL* url)
{
    PLAYER_LOGI("MediaPlayerLinux::setNativePlayerDefaultOptions\n");
}

void MediaPlayerLinux::setNativePlayerDisplayModeWithGL()
{
    m_nativePlayer->setVideoFrameDecodedCB(
        [](LinuxMediaPacket* packet, void* data) {
            MediaPlayerLinux* player = (MediaPlayerLinux*)data;
            {
                Locker<Mutex> l(*player->m_decodedVideoFrameMutex);
                LinuxMediaPacket* oldPacket = player->m_lastDecodedVideoPacket;
                player->m_lastDecodedVideoPacket = packet;
                if (oldPacket != nullptr) {
                    free(oldPacket);
                }
            }
            player->window()
                ->webView()
                ->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    player->window(),
                    [](size_t, void* data) {
                        BrowsingContext* b = (BrowsingContext*)data;
                        b->setNeedsComposite();
                    },
                    player->window()->browsingContext());
        },
        this);
}

void MediaPlayerLinux::openPreparingMode()
{
    PLAYER_LOGI("MediaPlayerLinux::openPreparingMode\n");
    STARFISH_ASSERT(!m_inPrepare);
    m_inPrepare = true;
    m_container->executionContext()->addPointerInRootSet(this);
}

void MediaPlayerLinux::closePreparingMode()
{
    PLAYER_LOGI("MediaPlayerLinux::closePreparingMode\n");
    if (m_inPrepare == true) {
        m_container->executionContext()->removePointerFromRootSet(this);
        m_inPrepare = false;
    }
}

void MediaPlayerLinux::prepare(ResourceURL* url)
{
    PLAYER_LOGI("MediaPlayerLinux::prepare\n");
    m_currentURL = url;
    m_nativePlayer->setVolume(1.0);
    m_nativePlayer->mute(false);
    m_nativePlayer->setLooping(m_isLooping);
    m_nativePlayer->setErrorCB(
        [](int errorCode, void* data) {
            PLAYER_LOGI("MediaPlayerLinux::player_error_cb");
            MediaPlayerLinux* player = (MediaPlayerLinux*)data;
            player->printNativePlayerError(errorCode);
            player->handlePlayerError();
        },
        this);
    m_nativePlayer->setcompleteCB(completeCallback, this);
    m_nativePlayer->setBufferingCB(
        [](int percent, void* data) {
            PLAYER_LOGI("MediaPlayerLinux -> buffering state... %d", percent);
        },
        this);

    setNativePlayerDefaultOptions(url);
    if (url->isBlobURL() == true) {
        BlobURLStore store;
        if (WebBase::stringToBlobURLString(url->urlString(), store) == false) {
            PLAYER_LOGE(
                "MediaPlayerLinux::prepare, seturl, FAIL - INVALID BLOB URL");
            processNextOperationQueueInContainer();
            return;
        }
        if (m_container->webView()->isValidBlobURL(store) == true) {
            m_nativePlayer->setMemoryBuffer(((Blob*)store.m_blob)->data(),
                                            ((Blob*)store.m_blob)->size());
        } else if (m_container->webView()->isValidMediaSourceBlobURL(store) ==
                   true) {
            BlobURLStore store;
            WebBase::stringToBlobURLString(url->urlString(), store);
            MediaSource* ms = (MediaSource*)store.m_blob;
            m_activeMediaSource = ms;
            m_mseClient = new MediaPlayerLinuxMediaSourceClient(this);
            m_activeMediaSource->addClient(m_mseClient);
            m_activeMediaSource->attach(m_container);
            if (m_container != nullptr) {
                // Note: In MSE case, ignore defaultPlaybackPosition
                m_container->setDefaultPlaybackStartPosition(0);
            }
            processNextOperationQueueInContainer();
            return;
        } else {
            // fire eror
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else {
        // Play with URL
        m_nativePlayer->setUrl(url);
    }

    openPreparingMode();

    if (!m_nativePlayer->prepare(preparedCallback, this)) {
        PLAYER_LOGE("MediaPlayerLinux::player_prepare_async return error !!!");
        m_foundError = true;
        handlePrepared();
        return;
    }
    setNativePlayerDisplayModeWithGL();
}

void MediaPlayerLinux::handlePrepared()
{
    PLAYER_LOGI("MediaPlayerLinux::handlePrepared\n");
    if (isMainThread() == false) {
        PLAYER_LOGI("MediaPlayerLinux::handlePrepared in non-MainThread");
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* user_data) {
                MediaPlayerLinux* self = (MediaPlayerLinux*)user_data;
                self->handlePrepared();
            },
            this);
        return;
    }
    PLAYER_LOGI("MediaPlayerLinux::handlePrepared in MainThread");
    closePreparingMode();
    if (m_foundError == true) {
        if (m_container != nullptr) {
            m_container->giveupFetchingResource();
        }
        destroy();
        return;
    }

    char* videoCodec = nullptr;
    char* audioCodec = nullptr;

    if (isMSE() == false && m_nativePlayer->m_fmtCtx != nullptr &&
        m_nativePlayer->m_videoStreamIndex >= 0) {
        AVCodecParameters* codecpar =
            m_nativePlayer->m_fmtCtx
                ->streams[m_nativePlayer->m_videoStreamIndex]
                ->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if (codec) {
            PLAYER_LOGI("Codec name: %s\n", codec->name);
            PLAYER_LOGI("Codec long name: %s\n", codec->long_name);
            PLAYER_LOGI("Width: %d\n", codecpar->width);
            PLAYER_LOGI("Height: %d\n", codecpar->height);

            videoCodec = const_cast<char*>(codec->name);
        }

        if (videoCodec != nullptr) {
            m_hasVideo = true;
            int width = codecpar->width;
            int height = codecpar->height;

            STARFISH_ASSERT(width > 0);
            STARFISH_ASSERT(height > 0);

            if ((width == 0 || height == 0) &&
                m_lastDecodedVideoPacket == nullptr) {
                STARFISH_LOG_INFO(
                    "player_get_video_size function tell us video has 0x0 "
                    "size && m_lastDecodedVideoPacket is not null");
                STARFISH_LOG_INFO(
                    "assume video size from m_lastDecodedVideoPacket");
                // TODO : Need to impl.
            }
            m_videoWidth = (unsigned long)width;
            m_videoHeight = (unsigned long)height;
        }
    }

    PLAYER_LOGI("MediaPlayerLinux::prepare ok %s %s %d %d", videoCodec,
                audioCodec, (int)m_videoWidth, (int)m_videoHeight);

    // free(videoCodec);
    // free(audioCodec);

    if (isMSE() == false) {
        processNextOperationQueueInContainer();
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_METADATA);
    } else if (m_container->defaultPlaybackStartPosition() != 0) {
        int defaultTimeInMS =
            m_container->defaultPlaybackStartPosition() * 1000;

        if (m_nativePlayer->setPlayPosition(defaultTimeInMS, true,
                                            seekedCallback, this)) {
            PLAYER_LOGI(
                "MediaPlayerLinux::handlePrepared failed to set default start "
                "pos");
            // printNativePlayerError(ret);
            m_foundError = true;
            m_container->giveupFetchingResource();
            destroy();
            return;
        }
    }
    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_ENOUGH_DATA);
    if (m_container->isHTMLVideoElement() == true &&
        m_container->frame() != nullptr) {
        m_container->setNeedsComposite();
    }
    // if (m_pendingPlay == true) {
    m_pendingPlay = false;
    play();
    // }
}

void MediaPlayerLinux::dispose()
{
    PLAYER_LOGI("MediaPlayerLinux::dispose");

    if (m_playerDeadFlag != nullptr) {
        *m_playerDeadFlag = true;
#if defined(STARFISH_RUN_MSE_THREAD)
        wakeMseThread();
        m_mseThread->joinIfNeeds();
        m_mseThread = nullptr;
#endif
        free((void*)m_playerDeadFlag);
        m_playerDeadFlag = nullptr;
    }

    if (m_nativePlayer != nullptr) {
        pause();
        m_nativePlayer->unprepare();
        m_nativePlayer->unsetcompleteCB();
        m_nativePlayer->unsetErrorCB();
        m_nativePlayer->unsetBufferingCB();
        m_nativePlayer->destroy();
        m_nativePlayer = nullptr;
    }
    {
        Locker<Mutex> l(*m_decodedVideoFrameMutex);
        if (m_lastDecodedVideoPacket != nullptr) {
            freeFramePacketLocked(m_lastDecodedVideoPacket);
            m_lastDecodedVideoPacket = nullptr;
        }
        for (auto& entry : m_decodedVideoQueue) {
            if (entry.packet != nullptr) {
                freeFramePacketLocked(entry.packet);
            }
        }
        m_decodedVideoQueue.clear();
        flushFramePoolLocked(0, 0);
    }
    if (m_swsCtx != nullptr) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    teardownAudioSink();
    if (m_activeMediaSource != nullptr) {
        m_activeMediaSource->removeClient(m_mseClient);
        m_activeMediaSource = nullptr;
    }
    if (m_mseClient != nullptr) {
        m_mseClient = nullptr;
    }
    if (m_audioStream != nullptr) {
        destroyDecoderForStream(m_audioStream);
        m_audioStream->releaseMediaFormat();
        m_audioStream = nullptr;
    }
    if (m_videoStream != nullptr) {
        destroyDecoderForStream(m_videoStream);
        m_videoStream->releaseMediaFormat();
        m_videoStream = nullptr;
    }
    if (m_canvasSurface != nullptr) {
        m_canvasSurface->detachNativeBuffer();
        m_canvasSurface = nullptr;
    }
    m_container = nullptr;
}

void MediaPlayerLinux::setVolume(double volume)
{
    PLAYER_LOGI("MediaPlayerLinux::setVolume(%f)", volume);
    if (m_nativePlayer == nullptr) {
        return;
    }
    player_state_e state = m_nativePlayer->getState();
    if (state > PLAYER_STATE_IDLE) {
        if (volume == 0.0) {
            setMuted(true);
            return;
        }
        setMuted(false);
        if (m_nativePlayer->setVolume(volume)) {
            PLAYER_LOGE("**ERROR: player_set_volume ");
        }
    }
}

void MediaPlayerLinux::setMuted(bool muted)
{
    PLAYER_LOGI("MediaPlayerLinux::setMuted(%s)", muted ? "true" : "false");
    if (m_nativePlayer == nullptr) {
        return;
    }

    player_state_e state = m_nativePlayer->getState();
    if (state > PLAYER_STATE_IDLE) {
        if (!m_nativePlayer->mute(muted)) {
            PLAYER_LOGE("**ERROR: player_set_mute ");
        }
    }
}

void MediaPlayerLinux::willDrawVideo(Compositor* canvas,
                                     const LayoutRect& videoRect)
{
    STARFISH_ASSERT(canvas != nullptr);
    canvas->setFillColor(Unit::Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
    promoteVideoFrameForCurrentTime();

    // If more frames remain queued, ask the message loop to recomposite
    // when the next frame's PTS becomes due. This drives smooth playback
    // even when no new packet has arrived (e.g. mid-burst decode).
    uint64_t nextPtsMs = 0;
    bool haveNext = false;
    {
        Locker<Mutex> l(*m_decodedVideoFrameMutex);
        if (m_lastDecodedVideoPacket != nullptr) {
            m_canvasSurface->attachPlatformExternalBuffer(
                m_lastDecodedVideoPacket);
        }
        if (!m_decodedVideoQueue.empty()) {
            nextPtsMs = m_decodedVideoQueue.front().ptsMs;
            haveNext = true;
        }
    }

    if (haveNext && playbackState() == PLAYBACK_STATE_PLAYING &&
        m_container != nullptr && m_container->window() != nullptr) {
        uint64_t nowMs = (uint64_t)(currentTime() * 1000.0);
        int32_t delay = 0;
        if (nextPtsMs > nowMs) {
            uint64_t d = nextPtsMs - nowMs;
            delay = d > 250 ? 250 : (int32_t)d;
        }
        m_container->window()->setTimeout(
            [](void* data) {
                MediaPlayerLinux* self = (MediaPlayerLinux*)data;
                if (self->alive() && self->container() != nullptr &&
                    self->container()->frame() != nullptr) {
                    self->container()->setNeedsComposite();
                }
            },
            delay, this);
    }
}

void MediaPlayerLinux::didDrawVideo(Compositor* canvas,
                                    const LayoutRect& videoRect,
                                    const LayoutRect& absVideoRect)
{
    PLAYER_LOGI("MediaPlayerLinux::didDrawVideo\n");
}

#ifdef STARFISH_RUN_MSE_THREAD
static void* threadFillingBuffer(void* data)
{
    MediaPlayerLinux* self = (MediaPlayerLinux*)data;
    volatile bool* playerDeadFlag = self->m_playerDeadFlag;
    while (!(*playerDeadFlag)) {
        MediaPlayer::PlaybackState state = self->playbackState();
        if (state != MediaPlayer::PLAYBACK_STATE_END) {
            MediaPlayerSourceStream* audioStream =
                self->currentStream(StreamTypeAudio);
            MediaPlayerSourceStream* videoStream =
                self->currentStream(StreamTypeVideo);
            // FFmpeg-based player must keep pulling packets from the MSE
            // source buffer as long as the decoder isn't already 1s ahead
            // of currentTime (lookahead gated inside fillBufferWithoutGuard).
            // The bufferState NORMAL only means the source buffer is full
            // enough that JS doesn't need to fetch more — it does NOT mean
            // the decoder should stop. Skipping fillBuffer here was causing
            // the decoded video queue to drain to empty after ~10s while
            // audio kept playing (audio used the same path but its source
            // buffer cycled through NEED_PACKET more often due to its
            // smaller maxBufferSize).
            // Always attempt to feed each active stream. waitingDemuxer is a
            // hint that the source buffer had no packet at the submission
            // pointer on the previous pass, NOT a hard gate: it was only ever
            // cleared by activeSourceBufferUpdated() (the append-notify path,
            // further gated by activeSourceBuffer(type) == s). If that notify
            // is missed or targets a non-active SourceBuffer, the latch never
            // clears and the feed thread -- though it keeps waking every 25ms
            // -- skips the stream forever, freezing playback mid-stream.
            // fillBufferWithoutGuard re-arms waitingDemuxer when the buffer is
            // still empty, and the lookahead throttle (lastDTS > currentMs +
            // decodeLookaheadMs()) prevents over-feeding, so re-checking every
            // wake is cheap
            // (one access-cached findProperMediaPacket) and self-limiting.
            if (audioStream != nullptr) {
                self->fillBuffer(audioStream);
            }
            if (videoStream != nullptr) {
                self->fillBuffer(videoStream);
            }
        }
#ifndef STARFISH_RUN_MSE_THREAD_WAIT_TIME
#define STARFISH_RUN_MSE_THREAD_WAIT_TIME 1000 * 25 // 25ms
#endif
        {
            std::unique_lock<std::mutex> lock(self->m_mseWakeMutex);
            self->m_mseWakeCv.wait_for(
                lock,
                std::chrono::microseconds(STARFISH_RUN_MSE_THREAD_WAIT_TIME),
                [&]() { return *playerDeadFlag || self->m_mseWakePending; });
            self->m_mseWakePending = false;
        }
    }
    PLAYER_LOGI("Close fillingBuffer thread");
    return nullptr;
}
#endif

void MediaPlayerLinux::prepareMediaSource()
{
    PLAYER_LOGI("MediaPlayerLinux::prepareMediaSource\n");
    initAudioStreamInfo();
    if (m_foundError == true) {
        return;
    }

    initVideoStreamInfo();
    if (m_foundError == true) {
        return;
    }

    if (m_audioStream == nullptr && m_videoStream == nullptr) {
        return;
    }

    // Re-entry guard: prepareMediaSource fires once per appendBuffer that
    // produces a new active source, but the native prepare / mseThread
    // bring-up should only run once.
    if (m_mseThread != nullptr) {
        return;
    }

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);
    openPreparingMode();

    if (!m_nativePlayer->prepare(preparedCallback, this)) {
        m_foundError = true;
        handlePrepared();
#ifdef STARFISH_RUN_MSE_THREAD
    } else {
        m_playerDeadFlag = (bool*)malloc(sizeof(bool));
        if (m_playerDeadFlag == NULL) {
            handlePlayerError();
            return;
        }
        *m_playerDeadFlag = false;
        STARFISH_ASSERT(m_mseThread == nullptr);
        m_mseThread = new Thread(NullOption, "MediaPlayerLinux thread");
        m_mseThread->run(m_container->webView()->messageLoop(),
                         threadFillingBuffer, this);
#endif
    }
}

void MediaPlayerLinux::fillBuffer(MediaPlayerSourceStream* stream)
{
    PLAYER_LOGI("MediaPlayerLinux::fillBuffer\n");
    Locker<Mutex> locker(*m_fillBufferMutex);
    fillBufferWithoutGuard(stream);
}

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#define DEBUG_STREAMBUFFER_LOG(STR, ...) \
    PLAYER_LOGI(                         \
        "[%s] "                          \
        "" STR,                          \
        (stream->isAudio() ? "AUDIO" : "VIDEO"), ##__VA_ARGS__);
#else
#define DEBUG_STREAMBUFFER_LOG(...)
#endif

void MediaPlayerLinux::enterUnderrunState()
{
    PLAYER_LOGI("MediaPlayerLinux::enterUnderrunState\n");
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerLinux* player = (MediaPlayerLinux*)data;
                player->enterUnderrunState();
            },
            this);
    } else {
        if (alive() == false || m_underrunMode == true ||
            playbackState() != PLAYBACK_STATE_PLAYING) {
            return;
        }
        bool needToNoti = false;
        int current = m_nativePlayer->getPlayPosition();
        uint64_t currentTimeInMS = (uint64_t)current;
        if (m_audioStream != nullptr) {
            needToNoti = needToNoti ||
                         (m_audioStream->lastSubmittedDTS() < currentTimeInMS ||
                          m_audioStream->lastSubmittedDTS() - currentTimeInMS <
                              STARFISH_MSE_MIN_MARGIN_IN_MS);
        }
        if (m_videoStream != nullptr) {
            needToNoti = needToNoti ||
                         (m_videoStream->lastSubmittedDTS() < currentTimeInMS ||
                          m_videoStream->lastSubmittedDTS() - currentTimeInMS <
                              STARFISH_MSE_MIN_MARGIN_IN_MS);
        }
        if (needToNoti == false) {
            return;
        }
        PLAYER_LOGI("MediaPlayerLinux::enterUnderrunState");
        m_underrunMode = true;
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_CURRENT_DATA);
    }
}

void MediaPlayerLinux::exitUnderrunState()
{
    PLAYER_LOGI("MediaPlayerLinux::exitUnderrunState\n");
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerLinux* player = (MediaPlayerLinux*)data;
                player->exitUnderrunState();
            },
            this);
    } else {
        if (alive() == false || m_underrunMode == false) {
            return;
        }
        bool allOut = true;
        if (m_audioStream != nullptr) {
            allOut &= !m_audioStream->isBufferState(
                MediaPlayerSourceStream::BUFFERSTATE_UNDER_RUN);
        }
        if (m_videoStream != nullptr) {
            allOut &= !m_videoStream->isBufferState(
                MediaPlayerSourceStream::BUFFERSTATE_UNDER_RUN);
        }
        if (allOut == false) {
            return;
        }
        PLAYER_LOGI("MediaPlayerLinux::exitUnderrunState");
        m_underrunMode = false;
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_ENOUGH_DATA);
    }
}

void MediaPlayerLinux::handlePlayerBuffer(StreamType type,
                                          uint64_t currentBytes)
{
    PLAYER_LOGI("MediaPlayerLinux::handlePlayerBuffer\n");
// Other thread
#ifdef STARFISH_RUN_MSE_THREAD
    MediaPlayerSourceStream* stream = currentStream(type);
    if (alive() == false || stream == nullptr) {
        return;
    }
    MediaPlayerSourceStream::BufferState prevState = stream->bufferState();
    if (prevState == MediaPlayerSourceStream::BUFFERSTATE_EOS) {
        return;
    }
    uint64_t maxSize = stream->maxBufferSize();
    uint64_t rate = currentBytes * 100 / maxSize;
    MediaPlayerSourceStream::BufferState state =
        MediaPlayerSourceStream::BUFFERSTATE_NORMAL;
    if (rate < 1) {
        state = MediaPlayerSourceStream::BUFFERSTATE_UNDER_RUN;
    } else if (rate < 30) {
        state = MediaPlayerSourceStream::BUFFERSTATE_NEED_PACKET;
    }
    if (prevState != state) {
        DEBUG_STREAMBUFFER_LOG("Buffer state: %s > %s",
                               bufferStateString(prevState),
                               bufferStateString(state));
        stream->setBufferState(state);
        if (prevState == MediaPlayerSourceStream::BUFFERSTATE_UNDER_RUN) {
            exitUnderrunState();
        } else if (prevState > MediaPlayerSourceStream::BUFFERSTATE_UNDER_RUN &&
                   state == MediaPlayerSourceStream::BUFFERSTATE_UNDER_RUN) {
            enterUnderrunState();
        }
    }
#else
    MediaPlayerSourceStream* stream = currentStream(type);
    uint64_t lastBytes;
    uint64_t maxSize;
    {
        Locker<Mutex> locker(*m_fillBufferMutex);
        lastBytes = stream->lastBufferBytes();
        if (lastBytes != 0 && lastBytes == currentBytes) {
            return;
        }
        stream->setLastBufferBytes(currentBytes);
        maxSize = stream->maxBufferSize();
    }
    if (stream->waitingDemuxer() == false && currentBytes <= lastBytes &&
        currentBytes < (maxSize * 0.1)) {
        DEBUG_STREAMBUFFER_LOG("Player need data (%llu/%llu)",
                               (long long unsigned int)currentBytes,
                               (long long unsigned int)maxSize);
        fillBuffer(stream);
    }
#endif
}

void MediaPlayerLinux::fillBufferWithoutGuard(MediaPlayerSourceStream* stream)
{
    if (stream == nullptr) {
        return;
    }
    SourceBuffer* sb = activeSourceBuffer(stream->type());
    if (sb == nullptr) {
        stream->setWaitingDemuxer(true);
        return;
    }

    uint64_t streamIdx = activeStreamIndex(stream->type());
    uint64_t lastDTS = stream->lastSubmittedDTS();
    uint64_t sizeUpTo =
        stream->maxBufferSize() * STARFISH_MSE_SUBMIT_BYTES_RATE;
    if (sizeUpTo == 0) {
        sizeUpTo = 256 * 1024;
    }

    // Throttle: do not let the decode pipeline race more than
    // decodeLookaheadMs() ahead of the current playback clock, otherwise we
    // eat memory storing pre-decoded RGBA frames. The check runs per packet
    // so a single fillBuffer call cannot dump ~sizeUpTo (often >1MB / >1s of
    // video) worth of frames in one shot.
    // The playback position is sampled once per call: within one call the
    // clock advances only by the loop duration (ms) against the lookahead
    // window, and a stale (smaller) value only trips the gate earlier
    // (under-fill, never over-fill); the feed thread re-runs within ~25ms.
    // The >500ms gap-skip below refreshes currentMs explicitly when it
    // rewrites the soft clock.
    const uint64_t lookaheadMs = decodeLookaheadMs();
    uint64_t currentMs = (uint64_t)(currentTime() * 1000.0);
    if (lastDTS > currentMs + lookaheadMs) {
        return;
    }

    size_t submitBytes = 0;
    while (submitBytes < sizeUpTo) {
        if (lastDTS > currentMs + lookaheadMs) {
            break;
        }
        std::pair<MediaPacket*, size_t> packet =
            sb->findProperMediaPacket(streamIdx, lastDTS);
        if (packet.first == nullptr) {
            uint64_t endTime = m_activeMediaSource->duration() * 1000;
            if (std::isinf(m_activeMediaSource->duration())) {
                endTime = std::numeric_limits<uint64_t>::max();
            }
            uint64_t lastBufferedTime = sb->lastBufferedTimestamp(streamIdx);
            if ((endTime > lastDTS && (endTime - lastDTS) < 10) ||
                (lastDTS == lastBufferedTime && endTime == lastBufferedTime)) {
                stream->setBufferState(
                    MediaPlayerSourceStream::BUFFERSTATE_EOS);
                break;
            }
            stream->setWaitingDemuxer(true);
            break;
        }
        if (packet.first->m_dts < lastDTS) {
            // Already-consumed packet; defer to next demuxer event.
            sb->clearPacketAccessCache();
            stream->setWaitingDemuxer(true);
            break;
        }
        if (packet.first->m_dts - lastDTS > 500) {
            // The MSE source has a >500ms gap ahead of our submission
            // pointer (typical when the player evicted old segments while
            // we were paused / falling behind). Jump lastDTS forward to
            // the next available packet, and on the video stream, also
            // advance our wall-clock so currentTime() catches up. Without
            // the wall-clock advance, the decode-lookahead gate prevents
            // further decoding (lastDTS already > currentMs+lookahead) and the
            // queued frames sit unpresented until wall-clock organically
            // reaches packet.dts — manifesting as multi-second freezes
            // every time the source buffer evicts. (MediaPlayerTizen does
            // the lastDTS jump but not the clock fixup — its currentTime()
            // reads from player_get_play_position() so there is no soft
            // clock to advance.)
            if (stream->isVideo() && isMSE() &&
                packet.first->m_dts > currentMs) {
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                uint64_t nowMonoMs =
                    (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
                m_clockOffsetSec = packet.first->m_dts / 1000.0;
                m_clockStartMs = nowMonoMs;
                currentMs = packet.first->m_dts;
            }
            lastDTS = packet.first->m_dts;
        }
        decodeAndDeliverPacket(stream, packet.first);
        // Data is flowing again; clear the wait latch so it reflects reality.
        stream->setWaitingDemuxer(false);
        submitBytes += packet.first->m_dataSize;
        lastDTS = packet.first->m_dts + packet.first->m_duration;
    }
    stream->setLastSubmittedDTS(lastDTS);
    if (stream->bufferState() != MediaPlayerSourceStream::BUFFERSTATE_EOS) {
        stream->setBufferState(
            MediaPlayerSourceStream::BUFFERSTATE_NEED_PACKET);
    }
}

void MediaPlayerLinux::setLoop(bool loop)
{
    PLAYER_LOGI("MediaPlayerLinux::setLoop\n");
    m_isLooping = loop;
    if (m_nativePlayer != nullptr) {
        m_nativePlayer->setLooping(loop);
    }
}

bool MediaPlayerLinux::isMSEBufferEOS()
{
    bool isEOS = true;
    if (m_audioStream != nullptr) {
        isEOS &= m_audioStream->isBufferState(
            MediaPlayerSourceStream::BUFFERSTATE_EOS);
    }
    if (m_videoStream != nullptr) {
        isEOS &= m_videoStream->isBufferState(
            MediaPlayerSourceStream::BUFFERSTATE_EOS);
    }
    return isEOS;
}

bool MediaPlayerLinux::createDecoderForStream(MediaPlayerSourceStream* stream,
                                              StreamInfo* info)
{
    if (stream == nullptr || info == nullptr) {
        return false;
    }

    AVCodecID codecId = AV_CODEC_ID_NONE;
    if (info->isCodec(MediaCodecVideoH264)) {
        codecId = AV_CODEC_ID_H264;
    } else if (info->isCodec(MediaCodecVideoHEVC)) {
        codecId = AV_CODEC_ID_HEVC;
    } else if (info->isCodec(MediaCodecVideoVP9)) {
        codecId = AV_CODEC_ID_VP9;
    } else if (info->isCodec(MediaCodecVideoAV1)) {
        codecId = AV_CODEC_ID_AV1;
    } else if (info->isCodec(MediaCodecAudioAAC)) {
        codecId = AV_CODEC_ID_AAC;
    } else if (info->isCodec(MediaCodecAudioMP3)) {
        codecId = AV_CODEC_ID_MP3;
    } else if (info->isCodec(MediaCodecAudioVorbis)) {
        codecId = AV_CODEC_ID_VORBIS;
    } else if (info->isCodec(MediaCodecAudioOpus)) {
        codecId = AV_CODEC_ID_OPUS;
    } else {
        STARFISH_LOG_INFO(
            "MediaPlayerLinux::createDecoderForStream: no AVCodecID "
            "mapping for codec '%s'",
            info->codecString());
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(codecId);
    if (codec == nullptr) {
        STARFISH_LOG_INFO(
            "MediaPlayerLinux::createDecoderForStream: libavcodec lacks a "
            "decoder for '%s' (AVCodecID=%d) — build without that decoder?",
            info->codecString(), (int)codecId);
        return false;
    }

    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (ctx == nullptr) {
        STARFISH_LOG_INFO(
            "MediaPlayerLinux::createDecoderForStream: "
            "avcodec_alloc_context3 returned null for '%s'",
            info->codecString());
        return false;
    }

    if (info->isVideo()) {
        ctx->width = info->videoWidth();
        ctx->height = info->videoHeight();
        // For H.264 in CMAF/MP4 the parsed packets are AVCC length-prefixed
        // with extradata containing avcC; we keep them in that form. WebM/VP9
        // and Annex-B sources would set isAnnexB true.
        if (codecId == AV_CODEC_ID_H264 && !info->m_extraData.empty() &&
            info->m_extraData[0] == 0x01) {
            stream->setIsAnnexB(false);
        } else {
            stream->setIsAnnexB(true);
        }
    } else {
        ctx->sample_rate = info->audioSampleRate();
        AVChannelLayout layout;
        av_channel_layout_default(&layout, info->audioChannels());
        av_channel_layout_copy(&ctx->ch_layout, &layout);
        av_channel_layout_uninit(&layout);
    }

    if (!info->m_extraData.empty()) {
        ctx->extradata_size = info->m_extraData.size();
        ctx->extradata = (uint8_t*)av_mallocz(ctx->extradata_size +
                                              AV_INPUT_BUFFER_PADDING_SIZE);
        if (ctx->extradata == nullptr) {
            avcodec_free_context(&ctx);
            return false;
        }
        memcpy(ctx->extradata, info->m_extraData.data(), ctx->extradata_size);
    }

    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx);
        return false;
    }

    destroyDecoderForStream(stream);
    stream->setCodecContext(ctx);
    return true;
}

void MediaPlayerLinux::destroyDecoderForStream(MediaPlayerSourceStream* stream)
{
    if (stream == nullptr) {
        return;
    }
    AVCodecContext* ctx = stream->codecContext();
    if (ctx != nullptr) {
        avcodec_free_context(&ctx);
        stream->setCodecContext(nullptr);
    }
}

LinuxMediaPacket* MediaPlayerLinux::takePooledFramePacketLocked(int width,
                                                                int height)
{
    if (width == m_framePoolWidth && height == m_framePoolHeight &&
        !m_framePool.empty()) {
        LinuxMediaPacket* p = m_framePool.back();
        m_framePool.pop_back();
        return p;
    }
    return nullptr;
}

void MediaPlayerLinux::freeFramePacketLocked(LinuxMediaPacket* packet)
{
    free(packet->buffer());
    delete packet;
}

void MediaPlayerLinux::releaseFramePacketLocked(LinuxMediaPacket* packet)
{
    if (packet->width() == m_framePoolWidth &&
        packet->height() == m_framePoolHeight &&
        m_framePool.size() < kMaxPooledFramePackets) {
        m_framePool.push_back(packet);
    } else {
        freeFramePacketLocked(packet);
    }
}

void MediaPlayerLinux::flushFramePoolLocked(int newWidth, int newHeight)
{
    for (size_t i = 0; i < m_framePool.size(); i++) {
        freeFramePacketLocked(m_framePool[i]);
    }
    m_framePool.clear();
    m_framePoolWidth = newWidth;
    m_framePoolHeight = newHeight;
}

void MediaPlayerLinux::publishDecodedFrame(AVFrame* frame)
{
    if (frame == nullptr || frame->width <= 0 || frame->height <= 0) {
        return;
    }
    int width = frame->width;
    int height = frame->height;
    int stride = width * 4;

    if (m_swsCtx == nullptr || m_swsCtxWidth != width ||
        m_swsCtxHeight != height) {
        if (m_swsCtx != nullptr) {
            sws_freeContext(m_swsCtx);
        }
        m_swsCtxWidth = width;
        m_swsCtxHeight = height;
        m_swsCtx = sws_getContext(width, height, (AVPixelFormat)frame->format,
                                  width, height, AV_PIX_FMT_RGBA, SWS_BILINEAR,
                                  nullptr, nullptr, nullptr);
        if (m_swsCtx == nullptr) {
            STARFISH_LOG_INFO(
                "MediaPlayerLinux::publishDecodedFrame sws_getContext failed");
            return;
        }
        {
            Locker<Mutex> l(*m_decodedVideoFrameMutex);
            flushFramePoolLocked(width, height);
        }
    }

    LinuxMediaPacket* mediaPacket = nullptr;
    {
        Locker<Mutex> l(*m_decodedVideoFrameMutex);
        mediaPacket = takePooledFramePacketLocked(width, height);
    }
    if (mediaPacket == nullptr) {
        uint8_t* buffer = (uint8_t*)malloc((size_t)stride * height);
        if (buffer == nullptr) {
            return;
        }
        mediaPacket = new LinuxMediaPacket(width, height, stride);
        mediaPacket->setBuffer(buffer);
    }
    uint8_t* dest[4] = { mediaPacket->buffer(), nullptr, nullptr, nullptr };
    int destLinesize[4] = { stride, 0, 0, 0 };
    sws_scale(m_swsCtx, frame->data, frame->linesize, 0, height, dest,
              destLinesize);

    int64_t pts = frame->best_effort_timestamp;
    if (pts == AV_NOPTS_VALUE) {
        pts = frame->pts;
    }
    uint64_t ptsMs = (pts == AV_NOPTS_VALUE || pts < 0) ? 0 : (uint64_t)pts;

    {
        Locker<Mutex> l(*m_decodedVideoFrameMutex);
        DecodedVideoFrame entry;
        entry.ptsMs = ptsMs;
        entry.packet = mediaPacket;
        m_decodedVideoQueue.push_back(entry);
    }

    if (m_container != nullptr && m_container->isHTMLVideoElement()) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        Locker<Mutex> locker(*m_setNeedsCompositeEventIdlerHandleMutex);
        if (m_setNeedsCompositeEventIdlerHandle == MessageLoopInvalidID) {
            m_setNeedsCompositeEventIdlerHandle =
                msgLoop->addIdlerWithNoGCRootingInOtherThread(
                    m_container->window(),
                    [](size_t, void* data) {
                        MediaPlayerLinux* self = (MediaPlayerLinux*)data;
                        if (self->alive() && self->container() != nullptr &&
                            self->container()->frame() != nullptr) {
                            self->container()->setNeedsComposite();
                        }
                        Locker<Mutex> locker(
                            *self->m_setNeedsCompositeEventIdlerHandleMutex);
                        self->m_setNeedsCompositeEventIdlerHandle =
                            MessageLoopInvalidID;
                    },
                    this);
        }
    }
}

void MediaPlayerLinux::promoteVideoFrameForCurrentTime()
{
    uint64_t nowMs = (uint64_t)(currentTime() * 1000.0);

    Locker<Mutex> l(*m_decodedVideoFrameMutex);
    if (m_decodedVideoQueue.empty()) {
        return;
    }

    LinuxMediaPacket* picked = nullptr;
    while (!m_decodedVideoQueue.empty()) {
        DecodedVideoFrame& head = m_decodedVideoQueue.front();
        // Future frame: stop here and present whatever we already picked.
        if (head.ptsMs > nowMs) {
            break;
        }
        if (picked != nullptr) {
            releaseFramePacketLocked(picked);
        }
        picked = head.packet;
        m_decodedVideoQueue.pop_front();
    }

    // If we have nothing displayed yet, force-present the head so the
    // <video> element shows the first decoded frame even before play()
    // (poster / first-frame behavior). Once playback starts, currentTime
    // advances and the PTS-gated path takes over.
    if (picked == nullptr && m_lastDecodedVideoPacket == nullptr &&
        !m_decodedVideoQueue.empty()) {
        picked = m_decodedVideoQueue.front().packet;
        m_decodedVideoQueue.pop_front();
    }

    if (picked != nullptr) {
        if (m_lastDecodedVideoPacket != nullptr) {
            releaseFramePacketLocked(m_lastDecodedVideoPacket);
        }
        m_lastDecodedVideoPacket = picked;
    }
}

void MediaPlayerLinux::ensureAudioSink(int channels, int sampleRate)
{
    if (channels <= 0 || sampleRate <= 0) {
        return;
    }
    if (m_audioSinkHandle != nullptr && m_audioSinkChannels == channels &&
        m_audioSinkRate == sampleRate) {
        return;
    }
    if (m_audioSinkHandle != nullptr) {
        teardownAudioSink();
    }

    void* handle = nullptr;
    PulseSimpleApi* api = loadPulseSimple(handle);
    if (api == nullptr) {
        return;
    }
    m_audioSinkLib = handle;

    pa_sample_spec spec;
    spec.format = PA_SAMPLE_S16LE;
    spec.rate = (uint32_t)sampleRate;
    spec.channels = (uint8_t)(channels > 8 ? 8 : channels);

    int err = 0;
    // Use PulseAudio defaults (large server-side buffer). Backpressure is
    // absorbed by the dedicated writer thread, not by the MSE driver.
    pa_simple* s =
        api->pa_simple_new(nullptr, "Starfish", PA_STREAM_PLAYBACK, nullptr,
                           "media", &spec, nullptr, nullptr, &err);
    if (s == nullptr) {
        STARFISH_LOG_INFO(
            "MediaPlayerLinux::ensureAudioSink pa_simple_new failed (%d)", err);
        return;
    }
    m_audioSinkHandle = s;
    m_audioSinkChannels = (int)spec.channels;
    m_audioSinkRate = sampleRate;

    m_audioWriterStop = false;
    m_audioWriteQueueBytes = 0;
    m_audioWriterThread = new std::thread([this]() {
        void* h = nullptr;
        PulseSimpleApi* api = loadPulseSimple(h);
        if (api == nullptr) {
            return;
        }
        while (true) {
            std::pair<uint8_t*, size_t> item(nullptr, 0);
            {
                std::unique_lock<std::mutex> lk(m_audioWriteMutex);
                m_audioWriteCv.wait(lk, [this]() {
                    return m_audioWriterStop || !m_audioWriteQueue.empty();
                });
                if (m_audioWriteQueue.empty()) {
                    if (m_audioWriterStop) {
                        return;
                    }
                    continue;
                }
                item = m_audioWriteQueue.front();
                m_audioWriteQueue.pop_front();
                if (m_audioWriteQueueBytes >= item.second) {
                    m_audioWriteQueueBytes -= item.second;
                } else {
                    m_audioWriteQueueBytes = 0;
                }
            }
            if (m_audioSinkHandle != nullptr && item.first != nullptr) {
                int err = 0;
                api->pa_simple_write((pa_simple*)m_audioSinkHandle, item.first,
                                     item.second, &err);
            }
            if (item.first != nullptr) {
                av_free(item.first);
            }
        }
    });
}

void MediaPlayerLinux::teardownAudioSink()
{
    if (m_audioWriterThread != nullptr) {
        {
            std::lock_guard<std::mutex> lk(m_audioWriteMutex);
            m_audioWriterStop = true;
        }
        m_audioWriteCv.notify_all();
        m_audioWriterThread->join();
        delete m_audioWriterThread;
        m_audioWriterThread = nullptr;
    }
    {
        std::lock_guard<std::mutex> lk(m_audioWriteMutex);
        for (auto& it : m_audioWriteQueue) {
            if (it.first != nullptr) {
                av_free(it.first);
            }
        }
        m_audioWriteQueue.clear();
        m_audioWriteQueueBytes = 0;
        m_audioWriterStop = false;
    }

    if (m_audioSinkHandle != nullptr && m_audioSinkLib != nullptr) {
        void* handle = nullptr;
        PulseSimpleApi* api = loadPulseSimple(handle);
        if (api != nullptr) {
            api->pa_simple_free((pa_simple*)m_audioSinkHandle);
        }
    }
    m_audioSinkHandle = nullptr;
    m_audioSinkLib = nullptr;
    m_audioSinkChannels = 0;
    m_audioSinkRate = 0;

    if (m_swrCtx != nullptr) {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }
    m_swrChannels = 0;
    m_swrRate = 0;
    m_swrSrcFmt = -1;
}

void MediaPlayerLinux::publishDecodedAudioFrame(AVFrame* frame)
{
    if (frame == nullptr || frame->nb_samples <= 0) {
        return;
    }
    int channels = frame->ch_layout.nb_channels;
    int sampleRate = frame->sample_rate;
    int srcFmt = frame->format;
    if (channels <= 0 || sampleRate <= 0) {
        return;
    }

    ensureAudioSink(channels, sampleRate);
    if (m_audioSinkHandle == nullptr) {
        return;
    }

    if (m_swrCtx == nullptr || m_swrChannels != channels ||
        m_swrRate != sampleRate || m_swrSrcFmt != srcFmt) {
        if (m_swrCtx != nullptr) {
            swr_free(&m_swrCtx);
            m_swrCtx = nullptr;
        }
        SwrContext* swr = nullptr;
        int rc = swr_alloc_set_opts2(
            &swr, &frame->ch_layout, AV_SAMPLE_FMT_S16, sampleRate,
            &frame->ch_layout, (AVSampleFormat)srcFmt, sampleRate, 0, nullptr);
        if (rc < 0 || swr == nullptr) {
            STARFISH_LOG_INFO(
                "MediaPlayerLinux::publishDecodedAudioFrame "
                "swr_alloc_set_opts2 failed (%d)",
                rc);
            return;
        }
        if (swr_init(swr) < 0) {
            swr_free(&swr);
            return;
        }
        m_swrCtx = swr;
        m_swrChannels = channels;
        m_swrRate = sampleRate;
        m_swrSrcFmt = srcFmt;
    }

    int outSamples = frame->nb_samples;
    int bytesPerSample = 2; // S16
    int outBufSize = outSamples * channels * bytesPerSample;
    uint8_t* outBuf = (uint8_t*)av_malloc((size_t)outBufSize);
    if (outBuf == nullptr) {
        return;
    }
    uint8_t* outPlanes[1] = { outBuf };
    int converted =
        swr_convert(m_swrCtx, outPlanes, outSamples,
                    (const uint8_t**)frame->extended_data, frame->nb_samples);
    if (converted <= 0) {
        av_free(outBuf);
        return;
    }

    size_t writeBytes = (size_t)(converted * channels * bytesPerSample);
    {
        std::lock_guard<std::mutex> lk(m_audioWriteMutex);
        // Cap pending audio at ~5 seconds worth (sampleRate * channels * 2 *
        // 5). Guards against runaway memory if the writer thread can't keep
        // up; in steady state the queue stays small.
        size_t maxBytes = (size_t)sampleRate * (size_t)channels * 2u * 5u;
        if (m_audioWriteQueueBytes + writeBytes > maxBytes &&
            !m_audioWriteQueue.empty()) {
            auto& victim = m_audioWriteQueue.front();
            if (victim.first != nullptr) {
                av_free(victim.first);
            }
            if (m_audioWriteQueueBytes >= victim.second) {
                m_audioWriteQueueBytes -= victim.second;
            } else {
                m_audioWriteQueueBytes = 0;
            }
            m_audioWriteQueue.pop_front();
        }
        m_audioWriteQueue.push_back(std::make_pair(outBuf, writeBytes));
        m_audioWriteQueueBytes += writeBytes;
    }
    m_audioWriteCv.notify_one();
}

void MediaPlayerLinux::decodeAndDeliverPacket(MediaPlayerSourceStream* stream,
                                              MediaPacket* packet)
{
    if (stream == nullptr || packet == nullptr ||
        stream->codecContext() == nullptr) {
        return;
    }
    AVCodecContext* ctx = stream->codecContext();

    AVPacket* avPkt = av_packet_alloc();
    if (avPkt == nullptr) {
        return;
    }
    avPkt->data = packet->m_data;
    avPkt->size = (int)packet->m_dataSize;
    avPkt->pts = (int64_t)packet->m_pts;
    avPkt->dts = (int64_t)packet->m_dts;
    avPkt->duration = (int64_t)packet->m_duration;
    if (packet->m_hasIdr) {
        avPkt->flags |= AV_PKT_FLAG_KEY;
    }

    int ret = avcodec_send_packet(ctx, avPkt);
    av_packet_free(&avPkt);
    if (ret < 0) {
        return;
    }

    while (true) {
        AVFrame* frame = av_frame_alloc();
        if (frame == nullptr) {
            break;
        }
        ret = avcodec_receive_frame(ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            break;
        }
        if (ret < 0) {
            av_frame_free(&frame);
            break;
        }
        if (stream->isVideo()) {
            publishDecodedFrame(frame);
        } else if (stream->isAudio()) {
            publishDecodedAudioFrame(frame);
        }
        av_frame_free(&frame);
    }
}

void MediaPlayerLinux::initVideoStreamInfo(size_t initSegmentIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeVideoSourceBuffer();
    if (sb == nullptr) {
        return;
    }

    StreamInfo* info = sb->streamInfo(
        initSegmentIndex, m_activeMediaSource->activeVideoStreamIndex());
    if (info == nullptr) {
        return;
    }

    m_videoStream = new MediaPlayerSourceStream(StreamTypeVideo);
    if (m_container != nullptr) {
        m_videoStream->setLastSubmittedDTS(
            m_container->defaultPlaybackStartPosition() * 1000);
    }

    m_videoWidth = info->videoWidth();
    m_videoHeight = info->videoHeight();
    m_hasVideo = true;
    m_videoStream->setMaxBufferSize(
        (m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 / 8 * 5);
    m_videoStream->setInitSegmentIndex(initSegmentIndex);
    m_videoStream->setBufferState(
        MediaPlayerSourceStream::BUFFERSTATE_NEED_PACKET);

    createDecoderForStream(m_videoStream, info);
}

void MediaPlayerLinux::initAudioStreamInfo(size_t initSegmentIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeAudioSourceBuffer();
    if (sb == nullptr) {
        return;
    }

    StreamInfo* info = sb->streamInfo(
        initSegmentIndex, m_activeMediaSource->activeAudioStreamIndex());
    if (info == nullptr) {
        return;
    }

    m_audioStream = new MediaPlayerSourceStream(StreamTypeAudio);
    if (m_container != nullptr) {
        m_audioStream->setLastSubmittedDTS(
            m_container->defaultPlaybackStartPosition() * 1000);
    }
    m_audioStream->setInitSegmentIndex(initSegmentIndex);
    m_audioStream->setBufferState(
        MediaPlayerSourceStream::BUFFERSTATE_NEED_PACKET);

    createDecoderForStream(m_audioStream, info);
}

void MediaPlayerLinux::updateStreamInfo(MediaPlayerSourceStream* stream,
                                        size_t pastInitIndex,
                                        size_t newInitIndex)
{
    if (stream->isVideo() == true) {
        updateVideoStreamInfo(stream, pastInitIndex, newInitIndex);
    } else {
        updateAudioStreamInfo(stream, pastInitIndex, newInitIndex);
    }
}

void MediaPlayerLinux::updateVideoStreamInfo(MediaPlayerSourceStream* stream,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeVideoSourceBuffer();
    if (sb == nullptr) {
        return;
    }
    StreamInfo* info = sb->streamInfo(
        newInitIndex, m_activeMediaSource->activeVideoStreamIndex());
    if (info == nullptr) {
        return;
    }
    m_videoWidth = info->videoWidth();
    m_videoHeight = info->videoHeight();
    stream->setMaxBufferSize((m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 /
                             8 * 5);
    createDecoderForStream(stream, info);
    PLAYER_LOGI("MediaPlayerLinux::updateVideoStreamInfo %dx%d",
                (int)m_videoWidth, (int)m_videoHeight);
}

void MediaPlayerLinux::updateAudioStreamInfo(MediaPlayerSourceStream* stream,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeAudioSourceBuffer();
    if (sb == nullptr) {
        return;
    }
    StreamInfo* info = sb->streamInfo(
        newInitIndex, m_activeMediaSource->activeAudioStreamIndex());
    if (info == nullptr) {
        return;
    }
    createDecoderForStream(stream, info);
    PLAYER_LOGI("MediaPlayerLinux::updateAudioStreamInfo channels=%d rate=%d",
                (int)info->audioChannels(), (int)info->audioSampleRate());
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerLinux(element);
}

bool MediaPlayer::isSupport(MediaCodec codec)
{
    if (codec == MediaCodec::MediaCodecUnknown) {
        return false;
    }
    return true;
}

} // namespace Starfish

#endif
#endif /* STARFISH_ENABLE_MULTIMEDIA */
