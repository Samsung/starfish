/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishSourceBuffer__)
#define __StarFishSourceBuffer__

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/threading/Thread.h"
#include "platform/threading/Mutex.h"
#include "platform/threading/Locker.h"

namespace StarFish {

class SourceBuffer;
class AudioTrackList;
class VideoTrackList;
class TextTrackList;
class TimeRange;
class TimeRanges;
class MediaSource;
class Demuxer;

struct SourceBufferData : public gc {
    SourceBuffer* m_sourceBuffer;
    bool m_isProcessed;
    bool m_foundInitSegmentHere;
    const uint8_t* m_data;
    unsigned long m_length;
    std::vector<uint8_t> m_headerBuffer;
    SourceBufferData(SourceBuffer* buf, const uint8_t* data, unsigned long length)
        : m_sourceBuffer(buf)
        , m_isProcessed(false)
        , m_foundInitSegmentHere(false)
        , m_data(data)
        , m_length(length)
    {
    }
};

struct MediaPacketGroup {
    size_t m_streamIndex;
    size_t m_initSegmentIndex;
    StreamInfo* m_streamInfo;
    uint64_t m_maxFrameDuration;
    uint64_t m_groupTimestampStart;
    uint64_t m_groupTimestampEnd;
    std::vector<MediaPacket*> m_packets;
    MediaPacketGroup(size_t idx, size_t initSegmentIdx, StreamInfo* streamInfo, uint64_t duration = 0, uint64_t start = std::numeric_limits<uint64_t>::max(), uint64_t end = 0)
        : m_streamIndex(idx)
        , m_initSegmentIndex(initSegmentIdx)
        , m_streamInfo(streamInfo)
        , m_maxFrameDuration(duration)
        , m_groupTimestampStart(start)
        , m_groupTimestampEnd(end)
    {
    }

    void updateMaxDurationIfNeeded(uint64_t newDuration)
    {
        if (m_maxFrameDuration < newDuration)
            m_maxFrameDuration = newDuration;
    }

    void pushMediaPacket(MediaPacket* packet)
    {
        m_packets.push_back(packet);
        updateMaxDurationIfNeeded(packet->m_duration);
    }
};

typedef std::vector<uint8_t, gc_allocator<uint8_t>> SourceBufferDataVector;
typedef std::vector<StreamInfo*, gc_allocator<StreamInfo*>> StreamInfoVector;

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

    SourceBuffer(StarFish* starFish, String* type);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::SourceBufferObject;
    }

    void appendBuffer(const uint8_t* data, unsigned long length);
    void abort();
    void remove(double start, double end);

    MediaSource* parentMediaSource() { return m_parentMediaSource; }

    void setMode(AppendMode mode);

    AppendMode mode()
    {
        return m_mode;
    }

    bool updating()
    {
        return m_updating;
    }

    const SourceBufferDataVector& bufferHeader(size_t initSegmentIndex)
    {
        return m_bufferHeader[initSegmentIndex];
    }

    // these methods are thread-safe
    std::pair<MediaPacket*, size_t> findProperMediaPacket(size_t streamIdx, uint64_t startPositionInPTSWantToFind);
    uint64_t lastBufferedTimestamp(size_t streamIdx);
    void clearPacketAccessCache();

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

protected:
    // this method needs packet group lock
    void rangeRemoval(uint64_t start, uint64_t end, StreamInfo::Type type = (StreamInfo::Type)((int)StreamInfo::Type::Video | (int)StreamInfo::Type::Audio | (int)StreamInfo::Type::Subtitle));
    void setUpdating(bool flag, UpdateState state);

    void attachedToParent(MediaSource* ms)
    {
        STARFISH_ASSERT(m_isAttachedToParent == false);
        m_parentMediaSource = ms;
        m_isAttachedToParent = true;
    }

    void detachFromParent()
    {
        clearAll();
        m_parentMediaSource = nullptr;
        m_isAttachedToParent = false;
    }

    void clearAll();
    void prepareAppend();
    void codedFrameEviction();
    void bufferAppend(SourceBufferData* data);

    AppendMode m_mode;
    bool m_isAttachedToParent;
    bool m_updating;
    size_t m_indexPerInitSegment;
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
    Thread* m_sourceBufferUpdateThread;
    SourceBufferDataVector m_bufferUnprocessed;
    std::vector<SourceBufferDataVector, gc_allocator<SourceBufferDataVector>> m_bufferHeader;
    std::vector<StreamInfoVector, gc_allocator<StreamInfoVector>> m_streamInfo;
    std::vector<MediaPacketGroup*> m_packetGroup;
    std::vector<std::pair<size_t, size_t>, gc_allocator<std::pair<size_t, size_t>>> m_packetAccessCachePerStream;
    Mutex* m_packetGroupMutex;
};

class SourceBufferList : public EventTarget {
public:
    SourceBufferList(StarFish* starFish, MediaSource* sb)
        : EventTarget()
        , m_starFish(starFish)
        , m_parentMediaSource(sb)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::SourceBufferListObject;
    }

    unsigned long length() const
    {
        return m_list.size();
    }

    void addWithoutEvent(SourceBuffer* buffer)
    {
        m_list.push_back(buffer);
    }

    void add(SourceBuffer* buffer, MediaSource* ms)
    {
        m_list.push_back(buffer);
        buffer->attachedToParent(ms);
        scheduleEvent(m_starFish->staticStrings()->m_addsourcebuffer.localName());
    }

    void remove(unsigned long index)
    {
        SourceBuffer* buf = m_list[index];
        m_list.erase(m_list.begin() + index);
        buf->detachFromParent();
        scheduleEvent(m_starFish->staticStrings()->m_removesourcebuffer.localName());
    }

    void remove(SourceBuffer* buffer)
    {
        unsigned long size = m_list.size();
        unsigned long targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            if (m_list[targetIdx] == buffer)
                break;
        }
        if (targetIdx < size) {
            remove(targetIdx);
        }
    }

    void clear()
    {
        m_list.clear();
        m_list.shrink_to_fit();
        scheduleEvent(m_starFish->staticStrings()->m_removesourcebuffer.localName());
    }

    void detachFromParent()
    {
        m_parentMediaSource = nullptr;
    }

    SourceBuffer* at(unsigned long index)
    {
        if (index >= m_list.size())
            return nullptr;
        return m_list[index];
    }

    void scheduleEvent(String* eventName);

protected:
    std::vector<SourceBuffer*, gc_allocator<SourceBuffer*>> m_list;
    StarFish* m_starFish;
    MediaSource* m_parentMediaSource;
};

}

#endif
