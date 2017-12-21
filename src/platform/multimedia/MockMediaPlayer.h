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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#if defined(STARFISH_USE_MOCK_MEDIAPLAYER) || !defined(STARFISH_TIZEN)
#ifndef __StarFishMockMediaPlayer__
#define __StarFishMockMediaPlayer__

#include "platform/multimedia/MediaPlayer.h"

namespace StarFish {

class Canvas;
class HTMLMediaElement;
class MediaPlayerMediaSourceClient;
class URL;

class MockMediaStream : public gc {
public:
    enum BufferState {
        BUFFERSTATE_INITIAL,
        BUFFERSTATE_UNDER_RUN,   // < 1%
        BUFFERSTATE_NEED_PACKET, // < 30%
        BUFFERSTATE_NORMAL,
        BUFFERSTATE_EOS,
    };

    MockMediaStream(StreamType type)
        : m_type(type)
        , m_bufferState(BUFFERSTATE_INITIAL)
        , m_maxBufferSize(0)
        , m_lastSubmittedDTS(0)
        , m_initSegmentIndex(0)
        , m_lastBufferBytes(0)
        , m_waitingDemuxer(false)
    {
    }
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
    uint64_t maxBufferSize()
    {
        return m_maxBufferSize;
    }
    void setMaxBufferSize(uint64_t value)
    {
        m_maxBufferSize = value;
    }
    uint64_t lastSubmittedDTS()
    {
        return m_lastSubmittedDTS;
    }
    void setLastSubmittedDTS(uint64_t value)
    {
        m_lastSubmittedDTS = value;
    }
    bool needPacket()
    {
        return m_bufferState == BUFFERSTATE_UNDER_RUN ||
               m_bufferState == BUFFERSTATE_NEED_PACKET;
    }
    BufferState bufferState()
    {
        return m_bufferState;
    }
    void setBufferState(BufferState value)
    {
        m_bufferState = value;
    }
    bool isBufferState(BufferState value)
    {
        return m_bufferState == value;
    }
    bool waitingDemuxer()
    {
        return m_waitingDemuxer;
    }
    void setWaitingDemuxer(bool value)
    {
        m_waitingDemuxer = value;
    }
    size_t initSegmentIndex()
    {
        return m_initSegmentIndex;
    }
    void setInitSegmentIndex(size_t value)
    {
        m_initSegmentIndex = value;
    }
    uint64_t lastBufferBytes()
    {
        return m_lastBufferBytes;
    }
    void setLastBufferBytes(size_t value)
    {
        m_lastBufferBytes = value;
    }

protected:
    StreamType m_type;
    BufferState m_bufferState;
    uint64_t m_maxBufferSize;
    uint64_t m_lastSubmittedDTS;
    size_t m_initSegmentIndex;
    size_t m_lastBufferBytes;
    bool m_waitingDemuxer;
};

class MockMediaPlayer : public MediaPlayer {
    friend class MediaPlayer;

public:
    virtual void close();

    virtual void play();

    virtual void pause();

    virtual void seek(double time);

    virtual void prepare(ResourceURL* url);
    virtual double currentTime()
    {
        return m_currentTimestamp / 1000.0;
    }

    virtual double duration()
    {
        if (m_activeMediaSource) {
            return activeMediaSource()->duration();
        }

        // fake time to test
        return 60;
    }
    virtual void setVolume(double volume)
    {
    }
    virtual void setMuted(bool muted)
    {
    }
    virtual void prepareMediaSource();

    virtual void drawVideo(Compositor* canvas, const LayoutRect& videoRect,
                           const LayoutRect& absVideoRect);

    void handleEnded();
    void handleSeeked();
    void fillBuffer(MockMediaStream* stream);
    void fillBufferIfNeeded(StreamType type);
    MockMediaStream* currentStream(StreamType type)
    {
        return type == StreamTypeAudio ? m_audioStream : m_videoStream;
    }
    uint64_t currentTimeInMS()
    {
        return m_currentTimestamp;
    }
    void setCurrentTimeInMS(uint64_t time)
    {
        m_currentTimestamp = time;
    }
    bool seeking()
    {
        return m_seeking;
    }

protected:
    MockMediaPlayer(HTMLMediaElement* element)
        : MediaPlayer(element)
        , m_currentTimestamp(0)
        , m_audioStream(nullptr)
        , m_videoStream(nullptr)
        , m_seeking(false)
        , m_seekingTimer(SIZE_MAX)
        , m_mseClient(nullptr)
    {
    }
    uint64_t m_currentTimestamp;
    MockMediaStream* m_audioStream;
    MockMediaStream* m_videoStream;
    bool m_seeking;
    size_t m_seekingTimer;
    MediaPlayerMediaSourceClient* m_mseClient;
};
}

#endif
#endif
#endif
