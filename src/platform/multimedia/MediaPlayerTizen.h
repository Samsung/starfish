/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    !defined(__StarFishMediaPlayerTizen__)
#define __StarFishMediaPlayerTizen__

#include "platform/multimedia/MediaPlayer.h"
#include "platform/multimedia/StreamInfo.h"

#include <media/player.h>
#include <media/player_internal.h>
#include <media/player_product.h>

#ifndef MAX_WAITING_SECONDS_FOR_SEEK_OPERATION
#define MAX_WAITING_SECONDS_FOR_SEEK_OPERATION 30000
#endif

#define STARFISH_RUN_MSE_THREAD

namespace StarFish {

class CanvasSurface;
class MediaSource;
class MediaPlayerTizenMediaSourceClient;
class Mutex;

class MediaStream : public gc {
public:
    enum BufferState {
        BUFFERSTATE_INITIAL,
        BUFFERSTATE_UNDER_RUN,   // < 1%
        BUFFERSTATE_NEED_PACKET, // < 30%
        BUFFERSTATE_NORMAL,
        BUFFERSTATE_EOS,
    };

    MediaStream(StreamType type);
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

protected:
    StreamType m_type;
    volatile BufferState m_bufferState;
    Mutex* m_mediaStreamMutex;

    media_format_h m_mediaFormat;
    union MediaFormatExtra {
        MediaFormatExtra()
            : m_audioFormatExtra()
        {
        }
        player_media_stream_audio_extra_info_s m_audioFormatExtra;
        player_media_stream_video_extra_info_s m_videoFormatExtra;
    } m_formatExtra;

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

    virtual void close();
    virtual void play();
    virtual void pause();

    void setVolume(double volume);
    void setMuted(bool muted);
    void setLoop(bool loop);

    virtual void prepare(ResourceURL* url);
    virtual void initDisplay();
    virtual void setNativePlayerDefaultOptions(ResourceURL* url);
    virtual void printNativePlayerError(int errorCode);

    void handlePlayerBuffer(StreamType type, uint64_t currentBytes);
    void fillBufferWithoutGuard(MediaStream* stream);
    void fillBuffer(MediaStream* stream);
    void fillBufferIfNeeded(StreamType type);
    void unprepareOperation();

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

    virtual unsigned long videoWidth()
    {
        if (m_hasVideo) {
            return m_videoWidth;
        } else {
            return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
        }
    }

    virtual unsigned long videoHeight()
    {
        if (m_hasVideo) {
            return m_videoHeight;
        } else {
            return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
        }
    }

    virtual double currentTime()
    {
        int s;
        int ret = player_get_play_position(m_nativePlayer, &s);
        if (ret) {
            return 0;
        }
        return s / 1000.0;
    }

    virtual double duration();
    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                           const LayoutRect& absVideoRect);
    virtual void prepareMediaSource();
    void updateStreamInfo(MediaStream* stream, size_t pastInitIndex,
                          size_t newInitIndex);
    void updateAudioStreamInfo(MediaStream* audio, size_t pastInitIndex,
                               size_t newInitIndex);
    void updateVideoStreamInfo(MediaStream* video, size_t pastInitIndex,
                               size_t newInitIndex);

    void enterUnderrunState();
    void exitUnderrunState();

    bool m_inPrepare : 1;
    bool m_pendingPlay : 1;
    bool m_underrunMode : 1;
    size_t m_seekingTimer;
    MediaPlayerTizenMediaSourceClient* m_mseClient;
    Mutex* m_fillBufferMutex;
    ResourceURL* m_currentURL;
    CanvasSurface* m_canvasSurface;

    player_h m_nativePlayer;
    bool* m_playerDeadFlag;
    MediaStream* m_audioStream;
    MediaStream* m_videoStream;

    // Helpers
    MediaStream* currentStream(StreamType type)
    {
        return type == StreamTypeAudio ? m_audioStream : m_videoStream;
    }
    SourceBuffer* activeSourceBuffer(StreamType type);
    uint64_t activeStreamIndex(StreamType type);
    bool isMSE();
    bool isMSEBufferEOS();
};
}

#endif
