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
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)
#ifndef __StarfishMediaPlayerTizen__
#define __StarfishMediaPlayerTizen__

#include "platform/multimedia/MediaPlayer.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <media/player.h>

#if !defined(STARFISH_TIZEN_USERAPP_SDK_API_ONLY)
#ifndef EFL_BETA_API_SUPPORT
#define EFL_BETA_API_SUPPORT
#endif
#endif

#if !defined(STARFISH_TIZEN_USERAPP_SDK_API_ONLY)
#include <media/player_internal.h>
#if defined(STARFISH_TIZEN_TV)
#include <media/player_product.h>
#endif
#endif

#ifndef MAX_WAITING_SECONDS_FOR_SEEK_OPERATION
#define MAX_WAITING_SECONDS_FOR_SEEK_OPERATION 30000
#endif

#define STARFISH_RUN_MSE_THREAD

namespace Starfish {

class CanvasSurface;
class MediaSource;
class MediaPlayerTizenMediaSourceClient;
class Mutex;

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
    media_format_h mediaFormat()
    {
        return m_mediaFormat;
    }
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

#if defined(STARFISH_TIZEN_TV) && !defined(STARFISH_TIZEN_USERAPP_SDK_API_ONLY)
    player_media_stream_audio_extra_info_s* audioFormatExtra()
    {
        STARFISH_ASSERT(m_type == StreamTypeAudio);
        return &(m_formatExtra.m_audioFormatExtra);
    }
    player_media_stream_video_extra_info_s* videoFormatExtra()
    {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        return &(m_formatExtra.m_videoFormatExtra);
    }
#endif

protected:
    StreamType m_type;
    volatile BufferState m_bufferState;
    Mutex* m_mediaStreamMutex;

    media_format_h m_mediaFormat;
#if defined(STARFISH_TIZEN_TV) && !defined(STARFISH_TIZEN_USERAPP_SDK_API_ONLY)
    union MediaFormatExtra {
        MediaFormatExtra()
            : m_audioFormatExtra()
        {
        }
        player_media_stream_audio_extra_info_s m_audioFormatExtra;
        player_media_stream_video_extra_info_s m_videoFormatExtra;
    } m_formatExtra;
#endif

    void initFormatExtraForAudio();
    void initFormatExtraForVideo();
    void createMediaFormatStreamType();
    void releaseMediaFormatStreamType();

    volatile uint64_t m_maxBufferSize;
    volatile uint64_t m_lastSubmittedDTS;
    volatile size_t m_initSegmentIndex;
    volatile size_t m_lastBufferBytes;
    volatile bool m_waitingDemuxer;
};

class MediaPlayerTizen : public MediaPlayer {
public:
    friend class MediaPlayerTizenMediaSourceClient;

    MediaPlayerTizen(HTMLMediaElement* element);

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

    virtual double currentTime()
    {
        int s;
        int ret = player_get_play_position(m_nativePlayer, &s);
        if (ret) {
            PLAYER_LOGI("player_get_play_position: failed");
            printNativePlayerError(ret);
            return 0;
        }
        return s / 1000.0;
    }

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
    MediaPlayerTizenMediaSourceClient* m_mseClient;
    Mutex* m_fillBufferMutex;

    Mutex* m_decodedVideoFrameMutex;
    media_packet_h m_lastDecodedVideoPacket;

    ResourceURL* m_currentURL;

    Mutex* m_setNeedsCompositeEventIdlerHandleMutex;
    volatile size_t m_setNeedsCompositeEventIdlerHandle;

    player_h m_nativePlayer;
#if defined(STARFISH_RUN_MSE_THREAD)
    Thread* m_mseThread;
#endif
    volatile bool* m_playerDeadFlag;
#if defined(STARFISH_RUN_MSE_THREAD)
    // Leaf-level wake channel for the MSE feed thread. Never hold
    // m_mseWakeMutex while taking m_fillBufferMutex or any stream mutex.
    std::mutex m_mseWakeMutex;
    std::condition_variable m_mseWakeCv;
    bool m_mseWakePending{ false };

    void wakeMseThread()
    {
        {
            std::lock_guard<std::mutex> lock(m_mseWakeMutex);
            m_mseWakePending = true;
        }
        m_mseWakeCv.notify_one();
    }
#endif
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

    static void seekedCallback(void* data)
    {
        PLAYER_LOGI("player_set_play_position_cb");
        MediaPlayerTizen* self = (MediaPlayerTizen*)data;
        self->handleSeeked();
    }

protected:
    int playerSetPlayPosition(int& timeInMS);
    void disposePlayer();
    void initCanvasSurface();
    void setNativePlayerDisplayMode();
    void setNativePlayerDisplayModeWithGL();
    void setPlayerDisplayVideoAtPausedState(int& ret);
    // HW video overlay output is controlled at runtime via the public LWE
    // Settings (Settings::SetVideoOverlayEnabled), read off the WebView.
    bool videoOverlayEnabled();
    void punchHole(Compositor* canvas, const LayoutRect& videoRect,
                   const LayoutRect& absVideoRect);
    void setMediaFormatExtraForVideo(media_format_h& mediaFormat,
                                     StreamInfo* info);
    void setMediaFormatExtraForAudio(media_format_h& mediaFormat,
                                     StreamInfo* info);
    void videoFramerateChanged(MediaPlayerSourceStream* stream, int num,
                               int den);
};
} // namespace Starfish

#endif
#endif
#endif
