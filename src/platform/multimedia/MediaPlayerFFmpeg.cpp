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
#include "platform/multimedia/MediaPlayerFFmpeg.h"
#include "platform/multimedia/FFmpegAudioOutput.h"
#if defined(STARFISH_ENABLE_WEBAUDIO)
#include "core/modules/webaudio/MediaElementAudioSourceNode.h"
#include "core/modules/webaudio/render/AudioHandlers.h"
#endif

#include <chrono>
#include <time.h>

namespace Starfish {

static uint64_t monotonicMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100
#define STARFISH_MSE_SUBMIT_BYTES_RATE 0.3
#define STARFISH_MSE_MIN_MARGIN_IN_MS 3000
// Post-seek, how close (ms) the nearest buffered packet must be to the seek
// target to count as "landed". Wide enough to absorb sparse-keyframe / segment
// alignment, narrow enough to reject stale post-seek data still buffered far
// ahead after a backward seek into an evicted range.
#define SEEK_LAND_TOLERANCE_MS 5000

// How far ahead of the playback clock the decode pipeline may run, in ms.
// Each queued frame is a full-resolution RGBA buffer (e.g. 1080x1920 ~= 8MB),
// so a wide window keeps that many decoded frames resident -> memory pressure
// and GC churn (the main cause of bursty mid-playback stalls at high
// resolutions). Keep the window at 400ms to bound queued frame storage.
#define STARFISH_DECODE_LOOKAHEAD_MS_DEFAULT 400

static uint64_t decodeLookaheadMs()
{
    return STARFISH_DECODE_LOOKAHEAD_MS_DEFAULT;
}

#define RETURN_WHEN_PLAYER_ERROR(...) \
    if (ret != PLAYER_ERROR_NONE) {   \
        PLAYER_LOGE(__VA_ARGS__);     \
        printNativePlayerError(ret);  \
        handlePlayerError();          \
        return;                       \
    }

void MediaPlayerFFmpeg::printNativePlayerError(int errorCode)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::printNativePlayerError\n");
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
    MediaPlayerFFmpeg* player = (MediaPlayerFFmpeg*)data;
    player->handleEnded();
}

static void preparedCallback(void* data)
{
    PLAYER_LOGI("preparedCallback\n");
    MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
    self->handlePrepared();
}

static void seekedCallback(void* data)
{
    PLAYER_LOGI("player_set_play_position_cb");
    MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
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
    // recycles a slot freed by a prior MediaPlayerFFmpeg destroy/dispose
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
    std::function<void(FFmpegMediaPacket* buffer, void* data)>
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
        if (m_fmtCtx == nullptr ||
            (m_videoStreamIndex < 0 && m_audioStreamIndex < 0)) {
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
    // This object is allocated with PointerFreeGC, so the destructor never
    // runs; the decoding thread must be joined here or it keeps calling the
    // frame callback on the already-disposed MediaPlayerFFmpeg.
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_state = State::STOPPED;
        m_stopRequested = true;
    }
    m_statecv.notify_all();
    if (m_decodingThread.joinable()) {
        m_decodingThread.join();
    }

    if (m_codecCtx != nullptr) {
        avcodec_free_context(&m_codecCtx);
    }
    if (m_audioCodecCtx) {
        avcodec_free_context(&m_audioCodecCtx);
    }
    if (m_fmtCtx != nullptr) {
        avformat_close_input(&m_fmtCtx);
    }
}

bool FfmpegWrapperPlayer::prepare(
    const std::function<void(void* data)>& preparedCallback, void* data)
{
    std::unique_lock<std::mutex> lock(m_stateMutex);

    if (m_state != State::STOPPED) {
        PLAYER_LOGI(
            "[FfmpegWrapperPlayer] Error: Player is not in stopped state\n");
        return false;
    }

    if (m_url) {
        PLAYER_LOGI("[FfmpegWrapperPlayer] Opening url : %s\n",
                    m_url->urlString()->toUTF8NonGCString().c_str());

        // Let destroy() abort a network read that would otherwise block the
        // decoding thread, and so the join, until data arrives.
        m_fmtCtx = avformat_alloc_context();
        if (m_fmtCtx == nullptr) {
            return false;
        }
        m_fmtCtx->interrupt_callback.callback = [](void* data) -> int {
            return ((FfmpegWrapperPlayer*)data)->m_stopRequested.load();
        };
        m_fmtCtx->interrupt_callback.opaque = this;

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

        auto openStream = [this](AVMediaType type, int& index,
                                 AVCodecContext*& context) {
            index = av_find_best_stream(m_fmtCtx, type, -1, -1, nullptr, 0);
            if (index < 0) {
                return true;
            }
            AVCodecParameters* parameters = m_fmtCtx->streams[index]->codecpar;
            const AVCodec* codec = avcodec_find_decoder(parameters->codec_id);
            if (!codec) {
                return false;
            }
            context = avcodec_alloc_context3(codec);
            return context &&
                   avcodec_parameters_to_context(context, parameters) >= 0 &&
                   avcodec_open2(context, codec, nullptr) >= 0;
        };
        if (!openStream(AVMEDIA_TYPE_VIDEO, m_videoStreamIndex, m_codecCtx) ||
            !openStream(AVMEDIA_TYPE_AUDIO, m_audioStreamIndex,
                        m_audioCodecCtx) ||
            (m_videoStreamIndex < 0 && m_audioStreamIndex < 0)) {
            return false;
        }
    }

    m_state = State::PREPARED;
    lock.unlock();
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

    std::lock_guard<std::mutex> lock(m_stateMutex);

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
            static_cast<int>(m_state.load()));
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
    switch (m_state.load()) {
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
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    SwsContext* scale = nullptr;
    if (!packet || !frame) {
        av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    auto epoch = std::chrono::steady_clock::now();
    int64_t offsetMs = m_currentPositionMs.load();
    bool resetClock = true;
    bool eof = false;
    int64_t seekFloorMs = 0;
    auto deliver = [&](AVCodecContext* codec, int index) {
        while (avcodec_receive_frame(codec, frame) >= 0) {
            int64_t timestamp = frame->best_effort_timestamp;
            if (timestamp == AV_NOPTS_VALUE) {
                timestamp = frame->pts;
            }
            int64_t position =
                timestamp == AV_NOPTS_VALUE
                    ? m_currentPositionMs.load()
                    : av_rescale_q(timestamp,
                                   m_fmtCtx->streams[index]->time_base,
                                   AVRational{ 1, 1000 });
            position = std::max<int64_t>(0, position);
            if (position < seekFloorMs) {
                av_frame_unref(frame);
                continue;
            }
            {
                std::unique_lock<std::mutex> lock(m_stateMutex);
                if (resetClock) {
                    epoch = std::chrono::steady_clock::now();
                    offsetMs = m_currentPositionMs.load();
                    resetClock = false;
                }
                // Pace both audio-only and video streams by their timestamps.
                // Control changes interrupt the wait, including paused seeks.
                m_statecv.wait_until(
                    lock,
                    epoch + std::chrono::milliseconds(position - offsetMs),
                    [this]() {
                        return m_stopRequested || m_seekRequested ||
                               m_state != State::PLAYING;
                    });
                if (m_stopRequested || m_seekRequested ||
                    m_state != State::PLAYING) {
                    av_frame_unref(frame);
                    resetClock = true;
                    return;
                }
            }
            m_currentPositionMs.store(position);
            if (index == m_audioStreamIndex) {
                // The common PCM publisher uses milliseconds for MSE frames.
                frame->best_effort_timestamp = position;
                if (m_audioDecodedCallback) {
                    m_audioDecodedCallback(frame);
                }
            } else {
                int width = frame->width;
                int height = frame->height;
                if (width <= 0 || height <= 0 ||
                    width > STARFISH_VIDEO_MAX_WIDTH ||
                    height > STARFISH_VIDEO_MAX_HEIGHT) {
                    av_frame_unref(frame);
                    continue;
                }
                scale = sws_getCachedContext(
                    scale, width, height,
                    static_cast<AVPixelFormat>(frame->format), width, height,
                    AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
                uint8_t* buffer =
                    static_cast<uint8_t*>(malloc(width * height * 4));
                if (scale && buffer) {
                    uint8_t* planes[] = { buffer, nullptr, nullptr, nullptr };
                    int strides[] = { width * 4, 0, 0, 0 };
                    sws_scale(scale, frame->data, frame->linesize, 0, height,
                              planes, strides);
                    FFmpegMediaPacket* decoded =
                        new FFmpegMediaPacket(width, height, width * 4);
                    decoded->setBuffer(buffer);
                    if (m_framedecodedCallback) {
                        m_framedecodedCallback(decoded,
                                               m_framedecodedCallbackData);
                    }
                } else {
                    free(buffer);
                }
            }
            av_frame_unref(frame);
        }
    };
    while (!m_stopRequested) {
        {
            std::unique_lock<std::mutex> lock(m_stateMutex);
            m_statecv.wait(lock, [this]() {
                return m_stopRequested || m_seekRequested ||
                       m_state == State::PLAYING;
            });
            if (m_stopRequested) {
                break;
            }
        }
        if (m_seekRequested.exchange(false)) {
            int64_t target = m_seekTargetMs.load();
            int result = av_seek_frame(m_fmtCtx, -1, target * 1000,
                                       AVSEEK_FLAG_BACKWARD);
            if (result >= 0) {
                if (m_codecCtx) {
                    avcodec_flush_buffers(m_codecCtx);
                }
                if (m_audioCodecCtx) {
                    avcodec_flush_buffers(m_audioCodecCtx);
                }
                if (m_audioFlushCallback) {
                    m_audioFlushCallback();
                }
                m_currentPositionMs.store(target);
                seekFloorMs = target;
                eof = false;
                resetClock = true;
            }
            std::function<void(void*)> callback;
            void* data;
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                callback = m_seekCompleteCallback;
                data = m_seekCompleteData;
            }
            if (callback) {
                callback(data);
            }
            continue;
        }
        if (eof) {
            break;
        }
        int result = av_read_frame(m_fmtCtx, packet);
        if (result < 0) {
            if (result == AVERROR_EOF) {
                if (m_codecCtx) {
                    avcodec_send_packet(m_codecCtx, nullptr);
                    deliver(m_codecCtx, m_videoStreamIndex);
                }
                if (m_audioCodecCtx) {
                    avcodec_send_packet(m_audioCodecCtx, nullptr);
                    deliver(m_audioCodecCtx, m_audioStreamIndex);
                }
                // Keep the thread available for a replay seek.
                eof = true;
                {
                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    m_state = State::PAUSED;
                }
                if (!m_stopRequested && m_completeCallback) {
                    m_completeCallback(m_completeCallbackData);
                }
                continue;
            }
            if (!m_stopRequested && m_errorCallback) {
                m_errorCallback(result, m_errorCallbackData);
            }
            break;
        }
        int index = packet->stream_index;
        AVCodecContext* codec = index == m_videoStreamIndex   ? m_codecCtx
                                : index == m_audioStreamIndex ? m_audioCodecCtx
                                                              : nullptr;
        if (codec) {
            result = avcodec_send_packet(codec, packet);
            if (result == AVERROR(EAGAIN)) {
                deliver(codec, index);
                result = avcodec_send_packet(codec, packet);
            }
            if (result >= 0) {
                deliver(codec, index);
            }
        }
        av_packet_unref(packet);
    }
    sws_freeContext(scale);
    av_frame_free(&frame);
    av_packet_free(&packet);
}

class MediaPlayerFFmpegMediaSourceClient : public MediaSourceClient {
public:
    MediaPlayerFFmpegMediaSourceClient(MediaPlayerFFmpeg* player)
        : MediaSourceClient()
        , m_player(player)
    {
#ifndef NDEBUG
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                PLAYER_LOGI(
                    "[TRACE_MSE_GC] "
                    "MediaPlayerFFmpegMediaSourceClient::~"
                    "MediaPlayerFFmpegMediaSourceClient (%p)",
                    obj);
            },
            NULL, NULL, NULL);
#endif
    }

    virtual void activeSourceComputed()
    {
        PLAYER_LOGI(
            "MediaPlayerFFmpegMediaSourceClient::activeSourceComputed\n");
        if (m_player != nullptr && m_player->alive() == true) {
            m_player->prepareMediaSource();
        }
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
        PLAYER_LOGI(
            "MediaPlayerFFmpegMediaSourceClient::"
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
            "MediaPlayerFFmpegMediaSourceClient::"
            "activeAudioSourceBufferUpdated\n");
        if (m_player != nullptr && m_player->alive() == true &&
            m_player->activeSourceBuffer(StreamTypeAudio) == s) {
            m_player->fillBufferIfNeeded(StreamTypeAudio);
        }
    }

    MediaPlayerFFmpeg* m_player;
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

MediaPlayerFFmpeg::MediaPlayerFFmpeg(HTMLMediaElement* element)
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
    PLAYER_LOGI("MediaPlayerFFmpeg::MediaPlayerFFmpeg\n");
    STARFISH_ASSERT(element != nullptr);

    m_nativePlayer = new (PointerFreeGC) FfmpegWrapperPlayer();
#if defined(STARFISH_ENABLE_WEBAUDIO)
    if (element->audioSourceNode()) {
        element->audioSourceNode()->setPlaybackState(audioPlaybackState());
    }
#endif
}

#if defined(STARFISH_ENABLE_WEBAUDIO)
MediaAudioPlaybackState* MediaPlayerFFmpeg::audioPlaybackState()
{
    std::lock_guard<std::mutex> lock(m_audioWriteMutex);
    if (!m_audioPlaybackState) {
        m_audioPlaybackState = new MediaAudioPlaybackState;
        m_audioPlaybackState->setStreamingPosition(
            currentTime(), playbackState() == PLAYBACK_STATE_PLAYING);
        m_audioPlaybackState->setVolume(m_container->volume());
        m_audioPlaybackState->setMuted(m_container->muted());
        // MSE bytes are supplied by script. Progressive network media must
        // remain opaque until its final CORS response label is available.
        // https://webaudio.github.io/web-audio-api/#MediaElementAudioSourceNode-security
        m_audioPlaybackState->setOriginClean(isMSE());
        // Only subsequent decoded frames can enter the graph: frames already
        // sent to the old sink are unavailable. Late attachment may therefore
        // be silent for the remaining decode-lookahead interval.
        for (auto& item : m_audioWriteQueue) {
            av_free(item.first);
        }
        m_audioWriteQueue.clear();
        m_audioWriteQueueBytes = 0;
        ++m_audioGeneration;
        if (m_audioOutput && m_audioOutputOpened) {
            m_audioFlushPending = true;
            m_audioOutput->interrupt();
        }
        m_audioWriteCv.notify_all();
    }
    return m_audioPlaybackState.value();
}

void MediaPlayerFFmpeg::syncAudioPlaybackState()
{
    std::lock_guard<std::mutex> lock(m_audioWriteMutex);
    if (m_audioPlaybackState) {
        m_audioPlaybackState->setStreamingPosition(
            currentTime(), playbackState() == PLAYBACK_STATE_PLAYING);
        m_audioPlaybackState->setVolume(m_container->volume());
        m_audioPlaybackState->setMuted(m_container->muted());
        m_audioPlaybackState->setOriginClean(isMSE());
    }
}
#endif

void MediaPlayerFFmpeg::handlePlayerError()
{
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerFFmpeg* player = (MediaPlayerFFmpeg*)data;
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

void MediaPlayerFFmpeg::fillBufferIfNeeded(StreamType type)
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

void MediaPlayerFFmpeg::seek(double time)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::seek\n");
    if (m_inPrepare == true) {
        PLAYER_LOGI("MediaPlayerFFmpeg::seek -> seeking failed saving time %lf",
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
        PLAYER_LOGI("MediaPlayerFFmpeg::seek() reaches EOS");
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
            MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
            PLAYER_LOGI("MediaPlayerFFmpeg::seek() : timeout");
            self->handleSeekTimeout();
        },
        MAX_WAITING_SECONDS_FOR_SEEK_OPERATION, this);

    seekOperation((int)(time * 1000.0));
}

void MediaPlayerFFmpeg::seekOperation(int timeInMS)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::seekOperation() (time: %d)", timeInMS);
    if (isMSE()) {
        // MSE has no native player seek (FFmpeg av_seek only drives the
        // file/URL path). Do the seek entirely in software: repoint the
        // demuxer feed and the soft playback clock at the target, drop the
        // pre-seek decoded video frames and pending audio, flush the
        // decoders, wake the feed, and complete the seek. Without this the
        // element stayed SEEKING until the 30s timeout, so the page (e.g. the
        // YouTube player) never received a `seeked` event: playback froze at
        // the old position and stayed paused. Mirrors the capi backend's feed
        // reset in MediaPlayerTizen::seekOperation.
        {
            Locker<Mutex> locker(*m_fillBufferMutex);
            if (m_audioStream != nullptr) {
                m_audioStream->setLastSubmittedDTS(timeInMS);
                m_audioStream->setWaitingDemuxer(true);
                m_audioStream->setSeekHoldTargetMs(timeInMS);
                if (m_audioStream->codecContext() != nullptr) {
                    avcodec_flush_buffers(m_audioStream->codecContext());
                }
            }
            if (m_videoStream != nullptr) {
                m_videoStream->setLastSubmittedDTS(timeInMS);
                m_videoStream->setWaitingDemuxer(true);
                m_videoStream->setSeekHoldTargetMs(timeInMS);
                if (m_videoStream->codecContext() != nullptr) {
                    avcodec_flush_buffers(m_videoStream->codecContext());
                }
            }
            if (activeSourceBuffer(StreamTypeAudio) != nullptr) {
                activeSourceBuffer(StreamTypeAudio)->clearPacketAccessCache();
            }
            if (activeSourceBuffer(StreamTypeVideo) != nullptr) {
                activeSourceBuffer(StreamTypeVideo)->clearPacketAccessCache();
            }
        }
        // Drop decoded video frames queued for the old position.
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
        }
        // Drop pending audio so no stale pre-seek samples play out.
        {
            std::lock_guard<std::mutex> lk(m_audioWriteMutex);
#if defined(STARFISH_ENABLE_WEBAUDIO)
            if (m_audioPlaybackState) {
                m_audioPlaybackState->clearStreamingPCM();
            }
#endif
            for (auto& item : m_audioWriteQueue) {
                if (item.first != nullptr) {
                    av_free(item.first);
                }
            }
            m_audioWriteQueue.clear();
            m_audioWriteQueueBytes = 0;
        }
        flushAudioSink();
        // Repoint the soft clock so currentTime() reports the target at once
        // (video promotion and the decode-lookahead gate both key off it).
        m_clockOffsetSec = timeInMS / 1000.0;
        if (playbackState() == PLAYBACK_STATE_PLAYING) {
            m_clockStartMs = monotonicMilliseconds();
        } else {
            m_clockStartMs = 0;
        }
#if defined(STARFISH_ENABLE_WEBAUDIO)
        syncAudioPlaybackState();
#endif
        fillBufferIfNeeded(StreamTypeAudio);
        fillBufferIfNeeded(StreamTypeVideo);
        // Complete the seek off the current operation-queue call stack so the
        // `seeked` event / any chained seek do not run reentrantly here. We
        // are on the main thread, so use the main-thread idler (the
        // *InOtherThread variant asserts !isMainThread).
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdler(
            m_container->window(),
            [](size_t, void* d) { ((MediaPlayerFFmpeg*)d)->handleSeeked(); },
            this);
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
            MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
            MessageLoop* msgLoop = self->container()->webView()->messageLoop();
            msgLoop->addIdlerWithNoGCRootingInOtherThread(
                self->container()->window(),
                [](size_t, void* d) {
                    ((MediaPlayerFFmpeg*)d)->handleSeeked();
                },
                self);
        },
        this);
}

void MediaPlayerFFmpeg::handleSeeked()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::handleSeeked\n");
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

void MediaPlayerFFmpeg::handleSeekTimeout()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::handleSeekTimeout\n");
    // Treat a timed-out seek as completed so the element does not stay stuck in
    // the seeking state.
    handleSeeked();
}

void MediaPlayerFFmpeg::handleEnded()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::handleEnded\n");
    if (!isMainThread()) {
        m_container->webView()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                m_container->window(),
                [](size_t, void* data) {
                    auto* player = static_cast<MediaPlayerFFmpeg*>(data);
                    if (player->alive()) {
                        player->handleEnded();
                    }
                },
                this);
        return;
    }
    if (playbackState() == PLAYBACK_STATE_END) {
        return;
    }
    pause();
    setPlaybackState(PLAYBACK_STATE_END);
    if (loop()) {
        m_container->mediaPlayerRequestRestartItsContainer();
    } else {
        m_container->mediaPlayerNotifyEndedItsContainer();
    }
}

double MediaPlayerFFmpeg::currentTime()
{
    if (isMSE()) {
        if (playbackState() != PLAYBACK_STATE_PLAYING || m_clockStartMs == 0) {
            return m_clockOffsetSec;
        }
        uint64_t nowMs = monotonicMilliseconds();
        double elapsed = (nowMs - m_clockStartMs) / 1000.0;
        return m_clockOffsetSec + elapsed;
    }
    if (m_nativePlayer) {
        return m_nativePlayer->getPlayPosition() / 1000.0;
    }
    return 0;
}

void MediaPlayerFFmpeg::destroy()
{
    STARFISH_ASSERT(isMainThread());
    if (m_alive == false) {
        return;
    }
    PLAYER_LOGI("MediaPlayerFFmpeg::destroy()");
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

double MediaPlayerFFmpeg::duration()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::duration\n");
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
    MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
    if (self->seeking() == true || self->alive() == false) {
        // Do not update time while seeking
        return;
    }
    double position = self->currentTime();
#if defined(STARFISH_ENABLE_WEBAUDIO)
    self->syncAudioPlaybackState();
#endif
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

void MediaPlayerFFmpeg::play()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::play\n");
    STARFISH_RELEASE_ASSERT(isMainThread());
    if (playbackState() == PLAYBACK_STATE_PLAYING) {
        return;
    }

    player_state_e state = m_nativePlayer->getState();
    PLAYER_LOGI("MediaPlayerFFmpeg::play() state : %d state2: %d ms: %p",
                (int)state, (int)playbackState(), m_activeMediaSource);
    if (state < PLAYER_STATE_READY) {
        m_pendingPlay = true;
        return;
    }
    m_pendingPlay = false;
    {
        std::lock_guard<std::mutex> lock(m_audioWriteMutex);
        m_audioPaused = false;
    }
    setPlaybackState(PLAYBACK_STATE_PLAYING);
    if (isMSE()) {
        m_clockStartMs = monotonicMilliseconds();
    }
    m_nativePlayer->play();
#if defined(STARFISH_ENABLE_WEBAUDIO)
    syncAudioPlaybackState();
#endif
    m_container->executionContext()->addPointerInRootSet(this);
    m_currentTimeUpdateTimer = m_container->window()->setInterval(
        updateTimeCallback, isMSE() ? 16 : 250, this);
}

void MediaPlayerFFmpeg::pause()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::pause\n");
    if (playbackState() != PLAYBACK_STATE_PLAYING) {
        return;
    }
    if (isMSE() && m_clockStartMs != 0) {
        uint64_t nowMs = monotonicMilliseconds();
        m_clockOffsetSec += (nowMs - m_clockStartMs) / 1000.0;
        m_clockStartMs = 0;
    }
    PLAYER_LOGI("pause()");
    setPlaybackState(PLAYBACK_STATE_PAUSED);
#if defined(STARFISH_ENABLE_WEBAUDIO)
    syncAudioPlaybackState();
#endif
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
    {
        std::lock_guard<std::mutex> lock(m_audioWriteMutex);
        m_audioPaused = true;
    }
    flushAudioSink();
    m_currentTimeUpdateTimer = TimerInvalidID;
}

void MediaPlayerFFmpeg::setNativePlayerDefaultOptions(ResourceURL* url)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::setNativePlayerDefaultOptions\n");
}

void MediaPlayerFFmpeg::setNativePlayerDisplayModeWithGL()
{
    m_nativePlayer->setVideoFrameDecodedCB(
        [](FFmpegMediaPacket* packet, void* data) {
            MediaPlayerFFmpeg* player = (MediaPlayerFFmpeg*)data;
            {
                Locker<Mutex> l(*player->m_decodedVideoFrameMutex);
                FFmpegMediaPacket* oldPacket = player->m_lastDecodedVideoPacket;
                player->m_lastDecodedVideoPacket = packet;
                if (oldPacket != nullptr) {
                    player->freeFramePacketLocked(oldPacket);
                }
            }
            player->window()
                ->webView()
                ->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    player->window(),
                    [](size_t, void* data) {
                        MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
                        if (self->alive() && self->container() != nullptr &&
                            self->container()->frame() != nullptr) {
                            self->container()->setNeedsComposite();
                        }
                    },
                    player);
        },
        this);
}

void MediaPlayerFFmpeg::openPreparingMode()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::openPreparingMode\n");
    STARFISH_ASSERT(!m_inPrepare);
    m_inPrepare = true;
    m_container->executionContext()->addPointerInRootSet(this);
}

void MediaPlayerFFmpeg::closePreparingMode()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::closePreparingMode\n");
    if (m_inPrepare == true) {
        m_container->executionContext()->removePointerFromRootSet(this);
        m_inPrepare = false;
    }
}

void MediaPlayerFFmpeg::prepare(ResourceURL* url)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::prepare\n");
    m_currentURL = url;
    m_nativePlayer->m_audioDecodedCallback = [this](AVFrame* frame) {
        publishDecodedAudioFrame(frame,
                                 m_nativePlayer->getPlayPosition() / 1000.0);
    };
    m_nativePlayer->m_audioFlushCallback = [this]() { flushAudioSink(); };
    m_nativePlayer->setVolume(m_container->volume());
    m_nativePlayer->mute(m_container->muted());
    m_nativePlayer->setLooping(m_isLooping);
    m_nativePlayer->setErrorCB(
        [](int errorCode, void* data) {
            PLAYER_LOGI("MediaPlayerFFmpeg::player_error_cb");
            MediaPlayerFFmpeg* player = (MediaPlayerFFmpeg*)data;
            player->printNativePlayerError(errorCode);
            player->handlePlayerError();
        },
        this);
    m_nativePlayer->setcompleteCB(completeCallback, this);
    m_nativePlayer->setBufferingCB(
        [](int percent, void* data) {
            PLAYER_LOGI("MediaPlayerFFmpeg -> buffering state... %d", percent);
        },
        this);

    setNativePlayerDefaultOptions(url);
    if (url->isBlobURL() == true) {
        BlobURLStore store;
        if (WebBase::stringToBlobURLString(url->urlString(), store) == false) {
            PLAYER_LOGE(
                "MediaPlayerFFmpeg::prepare, seturl, FAIL - INVALID BLOB URL");
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
            m_mseClient = new MediaPlayerFFmpegMediaSourceClient(this);
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
        PLAYER_LOGE("MediaPlayerFFmpeg::player_prepare_async return error !!!");
        m_foundError = true;
        handlePrepared();
        return;
    }
    setNativePlayerDisplayModeWithGL();
}

void MediaPlayerFFmpeg::handlePrepared()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::handlePrepared\n");
    if (isMainThread() == false) {
        PLAYER_LOGI("MediaPlayerFFmpeg::handlePrepared in non-MainThread");
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* user_data) {
                MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)user_data;
                self->handlePrepared();
            },
            this);
        return;
    }
    PLAYER_LOGI("MediaPlayerFFmpeg::handlePrepared in MainThread");
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

    PLAYER_LOGI("MediaPlayerFFmpeg::prepare ok %s %s %d %d", videoCodec,
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
                "MediaPlayerFFmpeg::handlePrepared failed to set default start "
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
    // HTML's paused state changes on play(), not on metadata becoming ready.
    // https://html.spec.whatwg.org/multipage/media.html#dom-media-play
    if (m_pendingPlay) {
        m_pendingPlay = false;
        play();
    }
}

void MediaPlayerFFmpeg::dispose()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::dispose");

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
#if defined(STARFISH_ENABLE_WEBAUDIO)
    if (m_audioPlaybackState) {
        m_audioPlaybackState->release();
        m_audioPlaybackState = NullOption;
    }
#endif
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

void MediaPlayerFFmpeg::setVolume(double volume)
{
#if defined(STARFISH_ENABLE_WEBAUDIO)
    syncAudioPlaybackState();
#endif
    PLAYER_LOGI("MediaPlayerFFmpeg::setVolume(%f)", volume);
    if (m_nativePlayer == nullptr) {
        return;
    }
    if (!m_nativePlayer->setVolume(volume)) {
        PLAYER_LOGE("**ERROR: player_set_volume");
    }
}

void MediaPlayerFFmpeg::setMuted(bool muted)
{
#if defined(STARFISH_ENABLE_WEBAUDIO)
    syncAudioPlaybackState();
#endif
    PLAYER_LOGI("MediaPlayerFFmpeg::setMuted(%s)", muted ? "true" : "false");
    if (m_nativePlayer == nullptr) {
        return;
    }

    if (!m_nativePlayer->mute(muted)) {
        PLAYER_LOGE("**ERROR: player_set_mute");
    }
}

void MediaPlayerFFmpeg::willDrawVideo(Compositor* canvas,
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
                MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
                if (self->alive() && self->container() != nullptr &&
                    self->container()->frame() != nullptr) {
                    self->container()->setNeedsComposite();
                }
            },
            delay, this);
    }
}

void MediaPlayerFFmpeg::didDrawVideo(Compositor* canvas,
                                     const LayoutRect& videoRect,
                                     const LayoutRect& absVideoRect)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::didDrawVideo\n");
}

#ifdef STARFISH_RUN_MSE_THREAD
static void* threadFillingBuffer(void* data)
{
    MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
    PLAYER_LOGI("FFmpeg MSE feed started");
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

void MediaPlayerFFmpeg::prepareMediaSource()
{
#if defined(STARFISH_ENABLE_WEBAUDIO)
    syncAudioPlaybackState();
#endif
    PLAYER_LOGI("MediaPlayerFFmpeg::prepareMediaSource\n");
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
        m_mseThread = new Thread(NullOption, "MediaPlayerFFmpeg thread");
        m_mseThread->run(m_container->webView()->messageLoop(),
                         threadFillingBuffer, this);
#endif
    }
}

void MediaPlayerFFmpeg::fillBuffer(MediaPlayerSourceStream* stream)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::fillBuffer\n");
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

void MediaPlayerFFmpeg::enterUnderrunState()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::enterUnderrunState\n");
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerFFmpeg* player = (MediaPlayerFFmpeg*)data;
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
        PLAYER_LOGI("MediaPlayerFFmpeg::enterUnderrunState");
        m_underrunMode = true;
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_CURRENT_DATA);
    }
}

void MediaPlayerFFmpeg::exitUnderrunState()
{
    PLAYER_LOGI("MediaPlayerFFmpeg::exitUnderrunState\n");
    if (isMainThread() == false) {
        MessageLoop* msgLoop = m_container->webView()->messageLoop();
        msgLoop->addIdlerWithNoGCRootingInOtherThread(
            m_container->window(),
            [](size_t, void* data) {
                MediaPlayerFFmpeg* player = (MediaPlayerFFmpeg*)data;
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
        PLAYER_LOGI("MediaPlayerFFmpeg::exitUnderrunState");
        m_underrunMode = false;
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_ENOUGH_DATA);
    }
}

void MediaPlayerFFmpeg::handlePlayerBuffer(StreamType type,
                                           uint64_t currentBytes)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::handlePlayerBuffer\n");
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

void MediaPlayerFFmpeg::fillBufferWithoutGuard(MediaPlayerSourceStream* stream)
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
    // Reused across iterations. copyProperMediaPacket snapshots the packet's
    // encoded bytes here under the source-buffer lock, so decoding is done
    // from a caller-owned copy that a concurrent remove()/eviction (e.g. the
    // buffer clear a page issues on seek) cannot free out from under us.
    std::vector<uint8_t> packetData;
    while (submitBytes < sizeUpTo) {
        if (lastDTS > currentMs + lookaheadMs) {
            break;
        }
        SourceBuffer::MediaPacketView packet =
            sb->copyProperMediaPacket(streamIdx, lastDTS, packetData);
        if (!packet.m_found) {
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
        if (packet.m_dts < lastDTS) {
            // Already-consumed packet; defer to next demuxer event.
            sb->clearPacketAccessCache();
            stream->setWaitingDemuxer(true);
            break;
        }
        int64_t seekHold = stream->seekHoldTargetMs();
        if (seekHold >= 0) {
            // A seek repointed lastDTS at the target. If the buffered data at
            // (or just after) the target is present, clear the hold and feed
            // normally. If instead the nearest packet is far ahead of the
            // target, the target region is not appended yet (typical for a
            // backward seek into an evicted range): wait for the demuxer to
            // deliver it rather than gap-skipping forward onto the stale
            // post-seek data still buffered ahead, which would drag the clock
            // back to the old position and undo the seek.
            if (packet.m_dts <= (uint64_t)seekHold + SEEK_LAND_TOLERANCE_MS) {
                stream->setSeekHoldTargetMs(-1);
            } else {
                sb->clearPacketAccessCache();
                stream->setWaitingDemuxer(true);
                break;
            }
        }
        if (packet.m_dts - lastDTS > 500) {
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
            if (stream->isVideo() && isMSE() && packet.m_dts > currentMs) {
                uint64_t nowMonoMs = monotonicMilliseconds();
                m_clockOffsetSec = packet.m_dts / 1000.0;
                m_clockStartMs = nowMonoMs;
                currentMs = packet.m_dts;
            }
            lastDTS = packet.m_dts;
        }
        // FFmpeg bitstream readers require zeroed padding after encoded data.
        packetData.resize(packet.m_dataSize + AV_INPUT_BUFFER_PADDING_SIZE, 0);
        MediaPacket tmp;
        tmp.m_data = packetData.data();
        tmp.m_dataSize = packet.m_dataSize;
        tmp.m_pts = packet.m_pts;
        tmp.m_dts = packet.m_dts;
        tmp.m_duration = packet.m_duration;
        tmp.m_hasIdr = packet.m_hasIdr;
        decodeAndDeliverPacket(stream, &tmp);
        // Data is flowing again; clear the wait latch so it reflects reality.
        stream->setWaitingDemuxer(false);
        submitBytes += packet.m_dataSize;
        lastDTS = packet.m_dts + packet.m_duration;
    }
    stream->setLastSubmittedDTS(lastDTS);
    if (stream->bufferState() != MediaPlayerSourceStream::BUFFERSTATE_EOS) {
        stream->setBufferState(
            MediaPlayerSourceStream::BUFFERSTATE_NEED_PACKET);
    }
}

void MediaPlayerFFmpeg::setLoop(bool loop)
{
    PLAYER_LOGI("MediaPlayerFFmpeg::setLoop\n");
    m_isLooping = loop;
    if (m_nativePlayer != nullptr) {
        m_nativePlayer->setLooping(loop);
    }
}

bool MediaPlayerFFmpeg::isMSEBufferEOS()
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

bool MediaPlayerFFmpeg::createDecoderForStream(MediaPlayerSourceStream* stream,
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
            "MediaPlayerFFmpeg::createDecoderForStream: no AVCodecID "
            "mapping for codec '%s'",
            info->codecString());
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(codecId);
    if (codec == nullptr) {
        STARFISH_LOG_INFO(
            "MediaPlayerFFmpeg::createDecoderForStream: libavcodec lacks a "
            "decoder for '%s' (AVCodecID=%d) — build without that decoder?",
            info->codecString(), (int)codecId);
        return false;
    }

    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (ctx == nullptr) {
        STARFISH_LOG_INFO(
            "MediaPlayerFFmpeg::createDecoderForStream: "
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

    PLAYER_LOGI("FFmpeg decoder codec=%s native=%s video=%dx%d audio=%d/%d",
                info->codecString(), codec->name, ctx->width, ctx->height,
                ctx->sample_rate, ctx->ch_layout.nb_channels);
    int openResult = avcodec_open2(ctx, codec, nullptr);
    if (openResult < 0) {
        STARFISH_LOG_ERROR("FFmpeg decoder open failed codec=%s error=%d",
                           codec->name, openResult);
        avcodec_free_context(&ctx);
        return false;
    }

    destroyDecoderForStream(stream);
    stream->setCodecContext(ctx);
    return true;
}

void MediaPlayerFFmpeg::destroyDecoderForStream(MediaPlayerSourceStream* stream)
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

FFmpegMediaPacket* MediaPlayerFFmpeg::takePooledFramePacketLocked(int width,
                                                                  int height)
{
    if (width == m_framePoolWidth && height == m_framePoolHeight &&
        !m_framePool.empty()) {
        FFmpegMediaPacket* p = m_framePool.back();
        m_framePool.pop_back();
        return p;
    }
    return nullptr;
}

void MediaPlayerFFmpeg::freeFramePacketLocked(FFmpegMediaPacket* packet)
{
    free(packet->buffer());
    delete packet;
}

void MediaPlayerFFmpeg::releaseFramePacketLocked(FFmpegMediaPacket* packet)
{
    if (packet->width() == m_framePoolWidth &&
        packet->height() == m_framePoolHeight &&
        m_framePool.size() < kMaxPooledFramePackets) {
        m_framePool.push_back(packet);
    } else {
        freeFramePacketLocked(packet);
    }
}

void MediaPlayerFFmpeg::flushFramePoolLocked(int newWidth, int newHeight)
{
    for (size_t i = 0; i < m_framePool.size(); i++) {
        freeFramePacketLocked(m_framePool[i]);
    }
    m_framePool.clear();
    m_framePoolWidth = newWidth;
    m_framePoolHeight = newHeight;
}

void MediaPlayerFFmpeg::publishDecodedFrame(AVFrame* frame)
{
    if (frame == nullptr || frame->width <= 0 || frame->height <= 0) {
        return;
    }
    int width = frame->width;
    int height = frame->height;
    int stride = width * 4;
    if (m_swsCtx == nullptr) {
        PLAYER_LOGI("FFmpeg decoded video=%dx%d format=%d pts=%lld", width,
                    height, frame->format,
                    static_cast<long long>(frame->best_effort_timestamp));
    }

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
                "MediaPlayerFFmpeg::publishDecodedFrame sws_getContext failed");
            return;
        }
        {
            Locker<Mutex> l(*m_decodedVideoFrameMutex);
            flushFramePoolLocked(width, height);
        }
    }

    FFmpegMediaPacket* mediaPacket = nullptr;
    {
        Locker<Mutex> l(*m_decodedVideoFrameMutex);
        mediaPacket = takePooledFramePacketLocked(width, height);
    }
    if (mediaPacket == nullptr) {
        uint8_t* buffer = (uint8_t*)malloc((size_t)stride * height);
        if (buffer == nullptr) {
            return;
        }
        mediaPacket = new FFmpegMediaPacket(width, height, stride);
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
                        MediaPlayerFFmpeg* self = (MediaPlayerFFmpeg*)data;
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

void MediaPlayerFFmpeg::promoteVideoFrameForCurrentTime()
{
    uint64_t nowMs = (uint64_t)(currentTime() * 1000.0);

    Locker<Mutex> l(*m_decodedVideoFrameMutex);
    if (m_decodedVideoQueue.empty()) {
        return;
    }

    FFmpegMediaPacket* picked = nullptr;
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

void MediaPlayerFFmpeg::ensureAudioSink(int channels, int sampleRate)
{
    {
        std::lock_guard<std::mutex> lock(m_audioWriteMutex);
        if (m_audioOutput && m_audioSinkChannels == channels &&
            m_audioSinkRate == sampleRate) {
            return;
        }
    }
    teardownAudioSink();
    FFmpegAudioOutput* output = FFmpegAudioOutput::create();
    {
        std::lock_guard<std::mutex> lock(m_audioWriteMutex);
        m_audioOutput = output;
        m_audioSinkChannels = channels;
        m_audioSinkRate = sampleRate;
        m_audioOutputReady = false;
        m_audioOutputOpened = false;
    }
    m_audioWriterThread = new std::thread([this, output, channels,
                                           sampleRate]() {
        bool opened = output->open(channels, sampleRate);
        PLAYER_LOGI("FFmpeg audio device channels=%d rate=%d opened=%d",
                    channels, sampleRate, opened);
        {
            std::lock_guard<std::mutex> lock(m_audioWriteMutex);
            m_audioOutputOpened = opened;
            m_audioOutputReady = true;
        }
        m_audioWriteCv.notify_all();
        if (opened) {
            while (true) {
                std::pair<uint8_t*, size_t> item(nullptr, 0);
                uint64_t generation;
                {
                    std::unique_lock<std::mutex> lock(m_audioWriteMutex);
                    m_audioWriteCv.wait(lock, [this]() {
                        return m_audioWriterStop || m_audioFlushPending ||
                               !m_audioWriteQueue.empty();
                    });
                    if (m_audioWriterStop) {
                        break;
                    }
                    if (m_audioFlushPending) {
                        // Keep the lock through reset so another interrupt
                        // cannot be lost while the output clears its event.
                        output->flush();
                        m_audioFlushPending = false;
                    }
                    if (m_audioWriteQueue.empty()) {
                        continue;
                    }
                    item = m_audioWriteQueue.front();
                    m_audioWriteQueue.pop_front();
                    m_audioWriteQueueBytes -= item.second;
                    generation = m_audioGeneration;
                }
                bool written = output->write(item.first, item.second);
                av_free(item.first);
                if (!written) {
                    std::lock_guard<std::mutex> lock(m_audioWriteMutex);
                    if (!m_audioWriterStop && generation == m_audioGeneration) {
                        m_audioOutputOpened = false;
                        STARFISH_LOG_ERROR("FFmpeg audio device write failed");
                        break;
                    }
                }
            }
        } else {
            STARFISH_LOG_INFO("FFmpeg audio output unavailable");
        }
        {
            std::lock_guard<std::mutex> lock(m_audioWriteMutex);
            m_audioOutput = NullOption;
            m_audioOutputOpened = false;
        }
        delete output;
    });
    std::unique_lock<std::mutex> lock(m_audioWriteMutex);
    m_audioWriteCv.wait(lock, [this]() { return m_audioOutputReady; });
}

void MediaPlayerFFmpeg::flushAudioSink()
{
    std::lock_guard<std::mutex> lock(m_audioWriteMutex);
    for (auto& item : m_audioWriteQueue) {
        av_free(item.first);
    }
    m_audioWriteQueue.clear();
    m_audioWriteQueueBytes = 0;
    ++m_audioGeneration;
    if (m_audioOutput && m_audioOutputOpened) {
        m_audioFlushPending = true;
        m_audioOutput->interrupt();
        m_audioWriteCv.notify_all();
    }
}

void MediaPlayerFFmpeg::teardownAudioSink()
{
    if (m_audioWriterThread) {
        {
            std::lock_guard<std::mutex> lock(m_audioWriteMutex);
            m_audioWriterStop = true;
            if (m_audioOutputOpened) {
                m_audioOutput->interrupt();
            }
        }
        m_audioWriteCv.notify_all();
        m_audioWriterThread->join();
        delete m_audioWriterThread;
        m_audioWriterThread = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(m_audioWriteMutex);
        m_audioOutput = NullOption;
        for (auto& item : m_audioWriteQueue) {
            av_free(item.first);
        }
        m_audioWriteQueue.clear();
        m_audioWriteQueueBytes = 0;
        m_audioWriterStop = false;
        m_audioOutputOpened = false;
        m_audioFlushPending = false;
    }
    m_audioSinkChannels = 0;
    m_audioSinkRate = 0;
    if (m_swrCtx) {
        swr_free(&m_swrCtx);
    }
    m_swrChannels = 0;
    m_swrRate = 0;
    m_swrSrcFmt = -1;
    m_swrOutputChannels = 0;
}

void MediaPlayerFFmpeg::publishDecodedAudioFrame(AVFrame* frame,
                                                 double fallbackPosition)
{
    if (frame == nullptr || frame->nb_samples <= 0) {
        return;
    }
    int channels = frame->ch_layout.nb_channels;
    int sampleRate = frame->sample_rate;
    int srcFmt = frame->format;
    if (channels <= 0 || channels > 32 || sampleRate <= 0) {
        return;
    }

    bool routed = false;
#if defined(STARFISH_ENABLE_WEBAUDIO)
    // Hold a reference across the unlocked conversion below, independent of
    // when the player drops its own.
    struct PlaybackReference {
        ~PlaybackReference()
        {
            if (state) {
                state->release();
            }
        }
        MediaAudioPlaybackState* state{ nullptr };
    } playback;
    {
        std::lock_guard<std::mutex> lk(m_audioWriteMutex);
        if (m_audioPlaybackState) {
            playback.state = m_audioPlaybackState.value();
            playback.state->retain();
        }
        routed = !!playback.state;
    }
#endif
    int outputChannels = routed ? channels : std::min(channels, 2);
    if (!routed) {
        ensureAudioSink(outputChannels, sampleRate);
        std::lock_guard<std::mutex> lock(m_audioWriteMutex);
        if (!m_audioOutputOpened) {
            return;
        }
    }

    if (m_swrCtx == nullptr || m_swrChannels != channels ||
        m_swrRate != sampleRate || m_swrSrcFmt != srcFmt ||
        m_swrOutputChannels != outputChannels) {
        if (m_swrCtx != nullptr) {
            swr_free(&m_swrCtx);
            m_swrCtx = nullptr;
        }
        SwrContext* swr = nullptr;
        AVChannelLayout outputLayout;
        av_channel_layout_default(&outputLayout, outputChannels);
        int rc = swr_alloc_set_opts2(
            &swr, &outputLayout, AV_SAMPLE_FMT_S16, sampleRate,
            &frame->ch_layout, (AVSampleFormat)srcFmt, sampleRate, 0, nullptr);
        av_channel_layout_uninit(&outputLayout);
        if (rc < 0 || swr == nullptr) {
            STARFISH_LOG_INFO(
                "MediaPlayerFFmpeg::publishDecodedAudioFrame "
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
        m_swrOutputChannels = outputChannels;
    }

    int outSamples = frame->nb_samples;
    int bytesPerSample = 2; // S16
    int outBufSize = av_samples_get_buffer_size(
        nullptr, outputChannels, outSamples, AV_SAMPLE_FMT_S16, 1);
    if (outBufSize < 0) {
        return;
    }
    uint8_t* outBuf = (uint8_t*)av_malloc(outBufSize);
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

#if defined(STARFISH_ENABLE_WEBAUDIO)
    if (routed) {
        int64_t timestamp = frame->best_effort_timestamp;
        if (timestamp == AV_NOPTS_VALUE) {
            timestamp = frame->pts;
        }
        double position =
            timestamp == AV_NOPTS_VALUE ? fallbackPosition : timestamp / 1000.0;
        playback.state->appendStreamingPCM(
            reinterpret_cast<const int16_t*>(outBuf), converted, channels,
            sampleRate, position);
        av_free(outBuf);
        return;
    }
#endif
    size_t writeBytes = (size_t)converted * outputChannels * bytesPerSample;
    float gain = m_nativePlayer->audioGain();
    int16_t* samples = reinterpret_cast<int16_t*>(outBuf);
    if (gain != 1.0f) {
        for (size_t i = 0; i < writeBytes / sizeof(int16_t); ++i) {
            samples[i] = static_cast<int16_t>(samples[i] * gain);
        }
    }
    {
        std::lock_guard<std::mutex> lk(m_audioWriteMutex);
        // Bound pending PCM to one second, including oversized frames.
        size_t maxBytes = (size_t)sampleRate * (size_t)outputChannels * 2u;
        if (writeBytes > maxBytes || !m_audioOutputOpened || m_audioPaused) {
            av_free(outBuf);
            return;
        }
        while (m_audioWriteQueueBytes + writeBytes > maxBytes &&
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

void MediaPlayerFFmpeg::decodeAndDeliverPacket(MediaPlayerSourceStream* stream,
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

    if (ctx->frame_num == 0) {
        PLAYER_LOGI("FFmpeg packet codec=%s size=%d pts=%lld dts=%lld",
                    ctx->codec->name, avPkt->size, (long long)avPkt->pts,
                    (long long)avPkt->dts);
    }
    int ret = avcodec_send_packet(ctx, avPkt);
    av_packet_free(&avPkt);
    if (ret < 0) {
        PLAYER_LOGE("FFmpeg send failed codec=%s error=%d", ctx->codec->name,
                    ret);
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
            publishDecodedAudioFrame(frame, packet->m_pts / 1000.0);
        }
        av_frame_free(&frame);
    }
}

void MediaPlayerFFmpeg::initVideoStreamInfo(size_t initSegmentIndex)
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

void MediaPlayerFFmpeg::initAudioStreamInfo(size_t initSegmentIndex)
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

void MediaPlayerFFmpeg::updateStreamInfo(MediaPlayerSourceStream* stream,
                                         size_t pastInitIndex,
                                         size_t newInitIndex)
{
    if (stream->isVideo() == true) {
        updateVideoStreamInfo(stream, pastInitIndex, newInitIndex);
    } else {
        updateAudioStreamInfo(stream, pastInitIndex, newInitIndex);
    }
}

void MediaPlayerFFmpeg::updateVideoStreamInfo(MediaPlayerSourceStream* stream,
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
    PLAYER_LOGI("MediaPlayerFFmpeg::updateVideoStreamInfo %dx%d",
                (int)m_videoWidth, (int)m_videoHeight);
}

void MediaPlayerFFmpeg::updateAudioStreamInfo(MediaPlayerSourceStream* stream,
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
    PLAYER_LOGI("MediaPlayerFFmpeg::updateAudioStreamInfo channels=%d rate=%d",
                (int)info->audioChannels(), (int)info->audioSampleRate());
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element, ResourceURL* url)
{
    return new MediaPlayerFFmpeg(element);
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
