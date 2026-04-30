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
#ifndef __StarfishMediaPlayerLinux__
#define __StarfishMediaPlayerLinux__

#ifndef MAX_WAITING_SECONDS_FOR_SEEK_OPERATION
#define MAX_WAITING_SECONDS_FOR_SEEK_OPERATION 30000
#endif

#define STARFISH_RUN_MSE_THREAD

#include "platform/multimedia/MediaPlayer.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace Starfish {

class CanvasSurface;
class MediaSource;
class MediaPlayerLinuxMediaSourceClient;
class Mutex;
struct MediaPacket;

typedef enum {
    PLAYER_STATE_NONE,    /**< Player is not created */
    PLAYER_STATE_IDLE,    /**< Player is created, but not prepared */
    PLAYER_STATE_READY,   /**< Player is ready to play media */
    PLAYER_STATE_PLAYING, /**< Player is playing media */
    PLAYER_STATE_PAUSED,  /**< Player is paused while playing media */
} player_state_e;

class LinuxMediaPacket {
public:
    LinuxMediaPacket(int width, int height, int stride)
    {
        m_width = width;
        m_height = height;
        m_stride = stride;
    }

    void setBuffer(uint8_t* buffer)
    {
        m_buffer = buffer;
    }

    int width()
    {
        return m_width;
    }

    int height()
    {
        return m_height;
    }

    int stride()
    {
        return m_stride;
    }

    uint8_t* buffer()
    {
        return m_buffer;
    }

private:
    uint8_t* m_buffer;
    int m_width;
    int m_height;
    int m_stride;
};

class FfmpegWrapperPlayer : public gc {
public:
    FfmpegWrapperPlayer();
    ~FfmpegWrapperPlayer();

    // API functions
    bool prepare();
    bool prepare(const std::function<void(void* data)>& preparedCallback,
                 void* data);
    bool unprepare();
    void destroy();

    bool setPlayPosition(
        int milliseconds, bool accurate,
        const std::function<void(void* data)>& seekCompleteCallback,
        void* user_data);

    bool setVideoFrameDecodedCB(
        std::function<void(LinuxMediaPacket* buffer, void* data)>
            framedecodedCallback,
        void* data);
    bool setcompleteCB(std::function<void(void* data)> completeCallback,
                       void* data);
    bool setErrorCB(
        std::function<void(int errorCode, void* data)> errorCallback,
        void* data);
    bool setBufferingCB(
        std::function<void(int percent, void* data)> bufferingCallback,
        void* data);

    bool unsetVideoFrameDecodedCB();
    bool unsetcompleteCB();
    bool unsetErrorCB();
    bool unsetBufferingCB();

    bool play();
    bool stop();
    bool pause();
    bool mute(bool mute);
    bool setVolume(float volume);
    void setUrl(ResourceURL* url);
    void setLooping(bool looping);
    void setMemoryBuffer(void* buffer, int size);
    player_state_e getState();
    int getPlayPosition();

    ResourceURL* m_url;
    AVFormatContext* m_fmtCtx;
    AVCodecContext* m_codecCtx;
    int m_videoStreamIndex;

private:
    // Internal methods
    void decodingThread();

    // State management
    enum class State { STOPPED, PREPARED, PLAYING, PAUSED };

    std::function<void(LinuxMediaPacket* buffer, void* data)>
        m_framedecodedCallback;
    std::function<void(void* data)> m_completeCallback;
    std::function<void(int errorCode, void* data)> m_errorCallback;
    std::function<void(int percent, void* data)> m_bufferingCallback;

    void* m_framedecodedCallbackData;
    void* m_completeCallbackData;
    void* m_errorCallbackData;
    void* m_bufferingCallbackData;

    // Threading
    std::thread m_decodingThread;
    std::mutex m_stateMutex;
    std::condition_variable m_statecv;
    std::atomic<bool> m_stopRequested;

    // State
    State m_state;

    // Audio controls
    std::atomic<bool> m_muted;
    std::atomic<float> m_volume;
};

class MediaPlayerSourceStream : public gc {
public:
    enum BufferState {
        BUFFERSTATE_INITIAL,
        BUFFERSTATE_UNDER_RUN,   // < 1%
        BUFFERSTATE_NEED_PACKET, // < 30%
        BUFFERSTATE_NORMAL,
        BUFFERSTATE_EOS,
    };

    MediaPlayerSourceStream(StreamType type);
    StreamType type()
    {
        return m_type;
    }
    bool isAudio()
    {
        return m_type == StreamTypeAudio;
    }
    bool isVideo()
    {
        return m_type == StreamTypeVideo;
    }
    // media_format_h mediaFormat()
    // {
    //     return m_mediaFormat;
    // }
    bool createMediaFormat();
    void releaseMediaFormat();
    uint64_t maxBufferSize();
    void setMaxBufferSize(uint64_t value);
    uint64_t lastSubmittedDTS()
    {
        return m_lastSubmittedDTS;
    }
    void setLastSubmittedDTS(uint64_t value)
    {
        m_lastSubmittedDTS = value;
    }
    bool needPacket();
    BufferState bufferState();
    void setBufferState(BufferState value);
    bool isBufferState(BufferState value);
    bool waitingDemuxer();
    void setWaitingDemuxer(bool value);
    size_t initSegmentIndex()
    {
        return m_initSegmentIndex;
    }
    void setInitSegmentIndex(size_t value)
    {
        m_initSegmentIndex = value;
    }
    uint64_t lastBufferBytes();
    void setLastBufferBytes(size_t value);

    AVCodecContext* codecContext()
    {
        return m_codecCtx;
    }
    void setCodecContext(AVCodecContext* ctx)
    {
        m_codecCtx = ctx;
    }
    bool isAnnexB()
    {
        return m_isAnnexB;
    }
    void setIsAnnexB(bool flag)
    {
        m_isAnnexB = flag;
    }

protected:
    StreamType m_type;
    volatile BufferState m_bufferState;
    Mutex* m_mediaStreamMutex;
    // media_format_h m_mediaFormat;

    void initFormatExtraForAudio();
    void initFormatExtraForVideo();
    void createMediaFormatStreamType();
    void releaseMediaFormatStreamType();

    volatile uint64_t m_maxBufferSize;
    volatile uint64_t m_lastSubmittedDTS;
    volatile size_t m_initSegmentIndex;
    volatile size_t m_lastBufferBytes;
    volatile bool m_waitingDemuxer;
    AVCodecContext* m_codecCtx;
    bool m_isAnnexB;
};

class MediaPlayerLinux : public MediaPlayer {
public:
    friend class MediaPlayerLinuxMediaSourceClient;

    MediaPlayerLinux(HTMLMediaElement* element);

    virtual void destroy();
    virtual void play();
    virtual void pause();

    void setVolume(double volume);
    void setMuted(bool muted);
    void setLoop(bool loop);

    virtual void prepare(ResourceURL* url);
    virtual void setNativePlayerDefaultOptions(ResourceURL* url);
    virtual void printNativePlayerError(int errorCode);

    void handlePlayerBuffer(StreamType type, uint64_t currentBytes);
    void fillBufferWithoutGuard(MediaPlayerSourceStream* stream);
    void fillBuffer(MediaPlayerSourceStream* stream);
    void fillBufferIfNeeded(StreamType type);
    void dispose();

    void openPreparingMode();
    void closePreparingMode();
    void handlePrepared();

    void startPlaying();

    void handleEnded();
    void handlePlayerError();

    void seek(double time);
    virtual void seekOperation(int timeInMS);
    virtual void handleSeekTimeout();
    virtual void handleSeeked();

    virtual double currentTime();
    virtual double duration();
    virtual void didDrawVideo(Compositor* canvas, const LayoutRect& videoRect,
                              const LayoutRect& absVideoRect);
    virtual void willDrawVideo(Compositor* canvas, const LayoutRect& videoRect);
    virtual void prepareMediaSource();

    void updateStreamInfo(MediaPlayerSourceStream* stream, size_t pastInitIndex,
                          size_t newInitIndex);
    void updateAudioStreamInfo(MediaPlayerSourceStream* audio,
                               size_t pastInitIndex, size_t newInitIndex);
    void updateVideoStreamInfo(MediaPlayerSourceStream* video,
                               size_t pastInitIndex, size_t newInitIndex);
    void enterUnderrunState();
    void exitUnderrunState();

    bool m_inPrepare : 1;
    bool m_pendingPlay : 1;
    bool m_underrunMode : 1;
    size_t m_seekingTimer;
    LayoutRect m_lastAbsoluteROIArea;
    MediaPlayerLinuxMediaSourceClient* m_mseClient;
    Mutex* m_fillBufferMutex;

    Mutex* m_decodedVideoFrameMutex;
    LinuxMediaPacket* m_lastDecodedVideoPacket;

    ResourceURL* m_currentURL;

    Mutex* m_setNeedsCompositeEventIdlerHandleMutex;
    volatile size_t m_setNeedsCompositeEventIdlerHandle;

    FfmpegWrapperPlayer* m_nativePlayer;
#if defined(STARFISH_RUN_MSE_THREAD)
    Thread* m_mseThread;
#endif
    volatile bool* m_playerDeadFlag;
    MediaPlayerSourceStream* m_audioStream;
    MediaPlayerSourceStream* m_videoStream;

    // Helpers
    MediaPlayerSourceStream* currentStream(StreamType type)
    {
        return type == StreamTypeAudio ? m_audioStream : m_videoStream;
    }
    bool isMSEBufferEOS();

    void initVideoStreamInfo(size_t initSegmentIndex = 0);
    void initAudioStreamInfo(size_t initSegmentIndex = 0);

    bool createDecoderForStream(MediaPlayerSourceStream* stream,
                                StreamInfo* info);
    void destroyDecoderForStream(MediaPlayerSourceStream* stream);
    void decodeAndDeliverPacket(MediaPlayerSourceStream* stream,
                                MediaPacket* packet);
    void publishDecodedFrame(AVFrame* frame);
    void publishDecodedAudioFrame(AVFrame* frame);
    void promoteVideoFrameForCurrentTime();
    void ensureAudioSink(int channels, int sampleRate);
    void teardownAudioSink();

    struct DecodedVideoFrame {
        uint64_t ptsMs;
        LinuxMediaPacket* packet;
    };
    std::deque<DecodedVideoFrame> m_decodedVideoQueue;

    SwsContext* m_swsCtx;
    int m_swsCtxWidth;
    int m_swsCtxHeight;

    // Wall-clock based playhead tracking for the MSE path. The FFmpeg
    // packet-feed loop has no native player to query for position, so we
    // approximate currentTime() with monotonic clock + offset.
    uint64_t m_clockStartMs;
    double m_clockOffsetSec;

    void* m_audioSinkLib;
    void* m_audioSinkHandle;
    int m_audioSinkChannels;
    int m_audioSinkRate;
    SwrContext* m_swrCtx;
    int m_swrChannels;
    int m_swrRate;
    int m_swrSrcFmt;

    // Decouple PulseAudio writes from the MSE driver thread: pa_simple_write
    // is blocking, and calling it from the decode loop also stalls video
    // decoding. The writer thread drains the queue and absorbs that block.
    std::deque<std::pair<uint8_t*, size_t>> m_audioWriteQueue;
    std::mutex m_audioWriteMutex;
    std::condition_variable m_audioWriteCv;
    std::thread* m_audioWriterThread;
    volatile bool m_audioWriterStop;
    size_t m_audioWriteQueueBytes;

    static void seekedCallback(void* data)
    {
        PLAYER_LOGI("player_set_play_position_cb");
        MediaPlayerLinux* self = (MediaPlayerLinux*)data;
        self->handleSeeked();
    }

protected:
    int playerSetPlayPosition(int& timeInMS);
    void disposePlayer();
    void initCanvasSurface();
    void setNativePlayerDisplayMode();
    void setNativePlayerDisplayModeWithGL();
    void setPlayerDisplayVideoAtPausedState(int& ret);
};

} // namespace Starfish

#endif // __StarfishMediaPlayerLinux__
#endif // STARFISH_USE_FFMPEG_MEDIAPLAYER
#endif // STARFISH_ENABLE_MULTIMEDIA
