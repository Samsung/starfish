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

namespace Starfish {

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100
#define STARFISH_MSE_SUBMIT_BYTES_RATE 0.3
#define STARFISH_MSE_MIN_MARGIN_IN_MS 3000

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
    : m_stopRequested(false)
    , m_state(State::STOPPED)
    , m_muted(false)
    , m_volume(1.0f)
{
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
    std::lock_guard<std::mutex> lock(m_stateMutex);
    STARFISH_UNIMPLEMENTED();
    return false;
}

bool FfmpegWrapperPlayer::unprepare()
{
    STARFISH_UNIMPLEMENTED();
    return false;
}

void FfmpegWrapperPlayer::destory()
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

    // Start decoding thread if not already running
    if (m_state == State::PREPARED && !m_decodingThread.joinable()) {
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
    STARFISH_UNIMPLEMENTED();
}

void FfmpegWrapperPlayer::setMemoryBuffer(void* buffer, int size)
{
    STARFISH_UNIMPLEMENTED();
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
    STARFISH_UNIMPLEMENTED();
    return 0;
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
    STARFISH_UNIMPLEMENTED();
    return false;
}

void MediaPlayerSourceStream::releaseMediaFormat()
{
    STARFISH_UNIMPLEMENTED();
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
#if defined(STARFISH_RUN_MSE_THREAD)
    , m_mseThread(nullptr)
#endif
    , m_playerDeadFlag(nullptr)
    , m_audioStream(nullptr)
    , m_videoStream(nullptr)
{
    PLAYER_LOGI("MediaPlayerLinux::MediaPlayerLinux\n");
    STARFISH_ASSERT(element != nullptr);

    m_nativePlayer = new FfmpegWrapperPlayer();

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            PLAYER_LOGI("MediaPlayerLinux::~MediaPlayerLinux");
            MediaPlayerLinux* player = (MediaPlayerLinux*)obj;
            player->destroy();
        },
        NULL, NULL, NULL);
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
    STARFISH_UNIMPLEMENTED();
}

void MediaPlayerLinux::handleSeeked()
{
    PLAYER_LOGI("MediaPlayerLinux::handleSeeked\n");
    STARFISH_UNIMPLEMENTED();
}

void MediaPlayerLinux::handleSeekTimeout()
{
    PLAYER_LOGI("MediaPlayerLinux::handleSeekTimeout\n");
    STARFISH_UNIMPLEMENTED();
}

void MediaPlayerLinux::handleEnded()
{
    PLAYER_LOGI("MediaPlayerLinux::handleEnded\n");
    STARFISH_UNIMPLEMENTED();
}

double MediaPlayerLinux::currentTime()
{
    if (m_nativePlayer) {
        return m_nativePlayer->getPlayPosition() / 1000.0;
    }
    return 0;
}

void MediaPlayerLinux::destroy()
{
    PLAYER_LOGI("MediaPlayerLinux::destroy\n");
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
        STARFISH_ASSERT(!m_alive);
        return;
    }

    m_alive = false;
    // Dispatch error event when found error
    if (m_foundError == true) {
        m_container->dispatchErrorEvent();
    }

    pause();

    if (m_canvasSurface != nullptr) {
        m_canvasSurface->detachNativeBuffer();
        m_canvasSurface = nullptr;
    }
}

double MediaPlayerLinux::duration()
{
    PLAYER_LOGI("MediaPlayerLinux::duration\n");
    STARFISH_UNIMPLEMENTED();
    return 0;
}

static void updateTimeCallback(void* data)
{
    PLAYER_LOGI("updateTimeCallback\n");
    MediaPlayerLinux* self = (MediaPlayerLinux*)data;
    if (self->seeking() == true || self->alive() == false) {
        // Do not update time while seeking
        return;
    }
    if (self->isMSE() == true &&
        self->currentTime() == self->container()->officialPlaybackPosition() &&
        self->isMSEBufferEOS() == true) {
        self->handleEnded();
    } else {
        self->container()->setOfficialPlaybackPosition(self->currentTime());
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
    m_nativePlayer->play();
    m_container->executionContext()->addPointerInRootSet(this);
    m_currentTimeUpdateTimer =
        m_container->window()->setInterval(updateTimeCallback, 250, this);
}

void MediaPlayerLinux::pause()
{
    PLAYER_LOGI("MediaPlayerLinux::pause\n");
    if (playbackState() != PLAYBACK_STATE_PLAYING) {
        return;
    }
    PLAYER_LOGI("pause()");
    setPlaybackState(PLAYBACK_STATE_PAUSED);
    if (m_container != nullptr) {
        m_container->executionContext()->removePointerFromRootSet(this);
        m_container->window()->clearInterval(m_currentTimeUpdateTimer);
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

    AVCodecParameters* codecpar =
        m_nativePlayer->m_fmtCtx->streams[m_nativePlayer->m_videoStreamIndex]
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
                "player_get_video_size function tell us video has 0x0 size && "
                "m_lastDecodedVideoPacket is not null");
            STARFISH_LOG_INFO(
                "assume video size from m_lastDecodedVideoPacket");
            // TODO : Need to impl.
        }
        m_videoWidth = (unsigned long)width;
        m_videoHeight = (unsigned long)height;
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
    PLAYER_LOGI("MediaPlayerLinux::dispose\n");
    if (m_nativePlayer != nullptr) {
        pause();
        m_nativePlayer->unprepare();
        m_nativePlayer->unsetcompleteCB();
        m_nativePlayer->unsetErrorCB();
        m_nativePlayer->unsetBufferingCB();
        m_nativePlayer->destory();
        m_nativePlayer = nullptr;
    }
    if (m_lastDecodedVideoPacket != nullptr) {
        // media_packet_destroy(m_lastDecodedVideoPacket);
        m_lastDecodedVideoPacket = nullptr;
    }
    if (m_activeMediaSource != nullptr) {
        m_activeMediaSource->removeClient(m_mseClient);
        m_activeMediaSource->detach();
        m_activeMediaSource = nullptr;
    }
    if (m_mseClient != nullptr) {
        m_mseClient = nullptr;
    }
    if (m_audioStream != nullptr) {
        m_audioStream->releaseMediaFormat();
        m_audioStream = nullptr;
    }
    if (m_videoStream != nullptr) {
        m_videoStream->releaseMediaFormat();
        m_videoStream = nullptr;
    }
    if (m_container != nullptr) {
        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
            HTMLMediaElement::HAVE_NOTHING);
    }
    if (m_canvasSurface != nullptr) {
        m_canvasSurface->detachNativeBuffer();
        m_canvasSurface = nullptr;
    }
    if (m_playerDeadFlag != nullptr) {
        *m_playerDeadFlag = true;
#if defined(STARFISH_RUN_MSE_THREAD)
        m_mseThread->joinIfNeeds();
        m_mseThread = nullptr;
#endif
        free((void*)m_playerDeadFlag);
        m_playerDeadFlag = nullptr;
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
    {
        Locker<Mutex> l(*m_decodedVideoFrameMutex);
        if (m_lastDecodedVideoPacket != nullptr) {
            m_canvasSurface->attachPlatformExternalBuffer(
                m_lastDecodedVideoPacket);
        }
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
            if (audioStream != nullptr &&
                audioStream->waitingDemuxer() == false &&
                audioStream->needPacket() == true) {
                // (audioStream->needPacket() || state ==
                // MediaPlayer::PLAYBACK_STATE_PLAYING)) {
                PLAYER_LOGI("[AUDIO] Player need data");
                self->fillBuffer(audioStream);
            }
            if (videoStream != nullptr &&
                videoStream->waitingDemuxer() == false &&
                videoStream->needPacket() == true) {
                // (videoStream->needPacket() || state ==
                // MediaPlayer::PLAYBACK_STATE_PLAYING)) {
                PLAYER_LOGI("[VIDEO] Player need data");
                self->fillBuffer(videoStream);
            }
        }
#ifndef STARFISH_RUN_MSE_THREAD_WAIT_TIME
#define STARFISH_RUN_MSE_THREAD_WAIT_TIME 1000 * 25 // 25ms
#endif
        usleep(STARFISH_RUN_MSE_THREAD_WAIT_TIME);
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

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);
    openPreparingMode();

    if (!m_nativePlayer->prepare(preparedCallback, this)) {
        PLAYER_LOGE("MediaPlayerLinux::player_prepare_async return error !!!");
        m_foundError = true;
        handlePrepared();
#ifdef STARFISH_RUN_MSE_THREAD
    } else {
        m_playerDeadFlag = (bool*)malloc(sizeof(bool));
        if (m_playerDeadFlag == NULL) {
            PLAYER_LOGE("ERROR: prepareMediaSource");
            handlePlayerError();
            return;
        }

        *m_playerDeadFlag = false;
        STARFISH_ASSERT(m_mseThread == nullptr);
        m_mseThread = new Thread(m_container->webView()->threadPool());
        m_mseThread->run(m_container->webView()->messageLoop(),
                         threadFillingBuffer, this);
#endif
    }

    PLAYER_LOGI("MediaPlayerLinux::prepareMediaSource end");
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
    STARFISH_UNIMPLEMENTED();
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

void MediaPlayerLinux::initVideoStreamInfo(size_t initSegmentIndex)
{
    STARFISH_UNIMPLEMENTED();
}

void MediaPlayerLinux::initAudioStreamInfo(size_t initSegmentIndex)
{
    STARFISH_UNIMPLEMENTED();
}

void MediaPlayerLinux::updateStreamInfo(MediaPlayerSourceStream* stream,
                                        size_t pastInitIndex,
                                        size_t newInitIndex)
{
    if (stream->isVideo() == true) {
        updateVideoStreamInfo(stream, pastInitIndex, newInitIndex);
    } else {
        // TODO audio
        STARFISH_UNIMPLEMENTED();
    }
}

void MediaPlayerLinux::updateVideoStreamInfo(MediaPlayerSourceStream* stream,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
    STARFISH_UNIMPLEMENTED();
}

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerLinux(element);
}
} // namespace Starfish

#endif
#endif /* STARFISH_ENABLE_MULTIMEDIA */
