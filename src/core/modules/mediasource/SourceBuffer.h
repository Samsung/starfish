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

#if STARFISH_ENABLE_MULTIMEDIA
#ifndef __StarFishSourceBuffer__
#define __StarFishSourceBuffer__

#include "core/dom/EventTarget.h"
#include "platform/multimedia/Demuxer.h"
#include "binding/ArrayBufferViewOrArrayBufferUnion.h"

namespace StarFish {

class AudioTrackList;
class SourceBuffer;
class TextTrackList;
class TimeRange;
class TimeRanges;
class MediaSource;
class Mutex;
class VideoTrackList;
class DemuxerClientSourceBuffer;

struct SourceBufferData : public gc {
    SourceBuffer* m_sourceBuffer;
    Demuxer* m_currentDemuxer;
    const uint8_t* m_data;
    unsigned long m_length;
    ScriptValue m_originScript;
    SourceBufferData(SourceBuffer* buf, const uint8_t* data,
                     unsigned long length, ScriptValue origin);
    DemuxerClientSourceBuffer* currentDemuxerClient();
};

struct MediaPacketGroup {
    size_t m_streamIndex;
    size_t m_initSegmentIndex;
    size_t m_dataSize;
    StreamInfo* m_streamInfo;
    uint64_t m_maxFrameDuration;
    uint64_t m_groupTimestampStart;
    uint64_t m_groupTimestampEnd;
    std::vector<MediaPacket*> m_packets;
    MediaPacketGroup(size_t idx, size_t initSegmentIdx, StreamInfo* streamInfo,
                     uint64_t duration = 0,
                     uint64_t start = std::numeric_limits<uint64_t>::max(),
                     uint64_t end = 0)
        : m_streamIndex(idx)
        , m_initSegmentIndex(initSegmentIdx)
        , m_dataSize(0)
        , m_streamInfo(streamInfo)
        , m_maxFrameDuration(duration)
        , m_groupTimestampStart(start)
        , m_groupTimestampEnd(end)
    {
    }

    void refresh()
    {
        m_maxFrameDuration = 0;
        m_groupTimestampStart = std::numeric_limits<uint64_t>::max();
        m_groupTimestampEnd = 0;
        m_dataSize = 0;
        for (size_t i = 0; i < m_packets.size(); i++) {
            updateGroupInfo(m_packets[i]);
        }
    }

    void updateGroupInfo(MediaPacket* packet)
    {
        if (m_maxFrameDuration < packet->m_duration) {
            m_maxFrameDuration = packet->m_duration;
        }
        if (packet->m_pts < m_groupTimestampStart) {
            m_groupTimestampStart = packet->m_pts;
        }
        if (packet->m_pts + packet->m_duration > m_groupTimestampEnd) {
            m_groupTimestampEnd = packet->m_pts + packet->m_duration;
        }
        m_dataSize += packet->m_dataSize;
    }

    void pushMediaPacket(MediaPacket* packet)
    {
        m_packets.push_back(packet);
        updateGroupInfo(packet);
    }
};

typedef GCVector<uint8_t> SourceBufferDataVector;

class SourceBuffer : public EventTarget {
public:
    friend class MediaSource;
    friend class SourceBufferList;
    friend class DemuxerClientSourceBuffer;
    enum UpdateState {
        Success,
        Error,
        Abort,
    };

    enum AppendMode {
        Segments,
        Sequence,
    };

    enum AppendState {
        WaitingForSegment,
        ParsingInitSegment,
        ParsingMediaSegment
    };

    SourceBuffer(Document* document, String* type);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSourceBuffer() const override;

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(updatestart);
    DECLARE_EVENT_LISTENER(update);
    DECLARE_EVENT_LISTENER(updateend);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(abort);
#undef VIRTUAL
#undef OVERRIDE

    void appendBuffer(const uint8_t* data, unsigned long length,
                      ScriptValue origin);
    void appendBuffer(ArrayBufferViewOrArrayBuffer buffer);
    void abort();
    void abortInternal();
    void remove(double start, double end);

    MediaSource* parentMediaSource()
    {
        return m_parentMediaSource;
    }

    void setMode(AppendMode mode);

    void setMode(String* modeStr);

    String* mode() const
    {
        switch (m_mode) {
        case Segments:
            return String::createASCIIString("segments");
        case Sequence:
            return String::createASCIIString("sequence");
        }
        STARFISH_ASSERT_NOT_REACHED();
        return String::emptyString;
    }

    AppendMode modeValue() const
    {
        return m_mode;
    }

    bool updating()
    {
        return m_updating;
    }

    // these methods are thread-safe
    std::pair<MediaPacket*, size_t> findProperMediaPacket(
        size_t streamIdx, uint64_t startPositionInDTSWantToFind);
    void revertLastCacheIfPossible(size_t streamIdx);
    uint64_t lastBufferedTimestamp(size_t streamIdx);
    void clearPacketAccessCache();
    void initializePacketAccessCache(size_t streamCount);

    TimeRanges* buffered();

    double timestampOffset()
    {
        return m_timestampOffset;
    }

    void setTimestampOffset(double timeoffset);

    AudioTrackList* audioTracks()
    {
        return m_audioTracks;
    }

    VideoTrackList* videoTracks()
    {
        return m_videoTracks;
    }

    TextTrackList* textTracks()
    {
        return m_textTracks;
    }

    double appendWindowStart()
    {
        return m_appendWindowStart;
    }

    void setAppendWindowStart(double timeStamp);

    double appendWindowEnd()
    {
        return m_appendWindowEnd;
    }

    void setAppendWindowEnd(double timeStamp);

    StreamInfo* streamInfo(size_t initSegmentIndex, size_t streamIndex);

    String* type()
    {
        return m_type;
    }

    void increaseUsedBufferSize(size_t amount);
    void decreaseUsedBufferSize(size_t amount);
    void clearAll();

    Demuxer* demuxer()
    {
        return m_demuxer;
    }
    DemuxerClientSourceBuffer* demuxerClient();
    void resetParserState();
    void appendError();

protected:
    // this method needs packet group lock
    void rangeRemovalWithoutGuard(
        uint64_t start, uint64_t end,
        StreamType type = (StreamType)((int)StreamTypeVideo |
                                       (int)StreamTypeAudio |
                                       (int)StreamTypeSubtitle));
    void rangeRemoval(uint64_t start, uint64_t end,
                      StreamType type = (StreamType)((int)StreamTypeVideo |
                                                     (int)StreamTypeAudio |
                                                     (int)StreamTypeSubtitle));
    void setUpdating(bool flag, UpdateState state);

    void attachedToParent(MediaSource* ms)
    {
        STARFISH_ASSERT(m_isAttachedToParent == false);
        m_parentMediaSource = ms;
        m_isAttachedToParent = true;
    }

    void detachFromParent()
    {
        m_parentMediaSource = nullptr;
        m_isAttachedToParent = false;
    }

    void prepareAppend(size_t newDataSize);
    bool codedFrameEviction(size_t newDataSize);
    void bufferAppend(SourceBufferData* data);
    void postBufferAppend(SourceBufferData* data);
    void setBufferedRangeNeedsUpdate()
    {
        m_buffered = nullptr;
    }

    AppendMode m_mode;
    bool m_isAttachedToParent;
    bool m_updating;
    size_t m_initSegmentCount;
    StarFish* m_starFish;
    Demuxer* m_demuxer;
    TimeRanges* m_buffered;
    double m_timestampOffset;
    AudioTrackList* m_audioTracks;
    VideoTrackList* m_videoTracks;
    TextTrackList* m_textTracks;
    double m_appendWindowStart;
    double m_appendWindowEnd;
    double m_groupStartTimestamp;
    double m_groupEndTimestamp;
    String* m_type;
    MediaSource* m_parentMediaSource;
    GCVector<GCVector<StreamInfo*>> m_streamInfo;
    std::vector<MediaPacketGroup*> m_packetGroups;
    GCVector<std::pair<size_t, size_t>> m_packetAccessCachePerStream;
    Mutex* m_packetGroupsMutex;
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
