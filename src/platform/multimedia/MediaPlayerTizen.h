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

namespace StarFish {

class CanvasSurface;
class MediaSource;
class MediaPlayerTizenMediaSourceClient;
class Mutex;

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
    virtual void printMediaPacketError(int errorCode);
    virtual void printMediaFormatError(int errorCode);

    void handlePlayerBuffer(StreamType type, uint64_t currentBytes);
    virtual void fillBuffer(StreamType type);
    virtual void fillBufferWithGuard(StreamType type);
    virtual void mediaEndOperation()
    {
        if (m_nativePlayer) {
            player_stop(m_nativePlayer);
        }
    }
    void fillVideoBufferIfNeeded();
    void fillAudioBufferIfNeeded();
    void unprepareOperation();

    void openPreparingMode();
    void closePreparingMode();
    void handlePrepared();
    void endOfStream();

    void startPlaying();
    void stopPlaying();

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
    void updateAudioStreamInfoWithGuard(size_t pastInitIndex,
                                        size_t newInitIndex);
    void updateVideoStreamInfoWithGuard(size_t pastInitIndex,
                                        size_t newInitIndex);

    bool m_inPrepare;
    bool m_needsPlayAfterPrepare;
    size_t m_seekingTimer;
    MediaPlayerTizenMediaSourceClient* m_mseClient;
    Mutex* m_bufferMutex;
    Mutex* m_mediaFormatMutex;
    ResourceURL* m_currentURL;
    void (*m_preparedCallback)(void*);
    void (*m_completeCallback)(void*);
    CanvasSurface* m_canvasSurface;

    player_h m_nativePlayer;
    media_format_h m_audioFormat;
    media_format_h m_videoFormat;
    player_media_stream_audio_extra_info_s m_audioFormatExtra;
    player_media_stream_video_extra_info_s m_videoFormatExtra;

    volatile bool m_isAudioBufferUnderrunState;
    volatile bool m_isVideoBufferUnderrunState;
    volatile uint64_t m_audioMaxBufferSize;
    volatile uint64_t m_videoMaxBufferSize;
    volatile uint64_t m_lastAudioDTS;
    volatile uint64_t m_lastVideoDTS;
    volatile size_t m_audioInitSegmentIndex;
    volatile size_t m_videoInitSegmentIndex;
    volatile size_t m_audioLastBufferBytes;
    volatile size_t m_videoLastBufferBytes;

    // Helpers
    media_format_h streamFormat(StreamType type);
    SourceBuffer* activeSourceBuffer(StreamType type);
    uint64_t activeStreamIndex(StreamType type);
    uint64_t maxBufferSize(StreamType type);

    bool bufferUnderrunState(StreamType type);
    void updateBufferUnderrunState(StreamType type, bool value);

    uint64_t lastSubmitDTS(StreamType type);
    void updateLastSubmitDTS(StreamType type, uint64_t value);

    size_t initSegmentIndex(StreamType type);
    void updateInitSegmentIndex(StreamType type, size_t value);

    uint64_t lastBufferBytes(StreamType type);
    void updateLastBufferBytes(StreamType type, size_t value);

    void updateStreamInfoWithGuard(StreamType type, size_t pastInitIndex,
                                   size_t newInitIndex);
};
}

#endif
