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
#ifndef __StarfishSourceBuffer__
#define __StarfishSourceBuffer__

#include "core/dom/EventTarget.h"
#include "platform/multimedia/Demuxer.h"
#include "binding/generated/ArrayBufferViewOrArrayBufferUnion.h"

namespace Starfish {

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
    size_t m_maxFrameDuration;
    uint64_t m_groupTimestampStart;
    uint64_t m_groupTimestampEnd;
    uint64_t m_groupDtsTimestampStart;
    uint64_t m_groupDtsTimestampEnd;

    std::vector<MediaPacket*> m_packets;
    MediaPacketGroup(size_t idx, size_t initSegmentIdx, StreamInfo* streamInfo,
                     size_t maxFrameDuration = 0,
                     uint64_t start = std::numeric_limits<uint64_t>::max(),
                     uint64_t end = 0)
        : m_streamIndex(idx)
        , m_initSegmentIndex(initSegmentIdx)
        , m_dataSize(0)
        , m_streamInfo(streamInfo)
        , m_maxFrameDuration(maxFrameDuration)
        , m_groupTimestampStart(start)
        , m_groupTimestampEnd(end)
        // These were left uninitialized: updateGroupInfo only min/maxes them,
        // so a garbage initial value (esp. a huge one for the end) was never
        // corrected, leaving the group's DTS range corrupt. Every DTS-keyed
        // lookup (findProperMediaPacket, the feed cursor) then mismatched,
        // manifesting as phantom "gaps" / skip-ahead blocks mid-playback.
        // Match refresh(): min-accumulator starts at max, max-accumulator at 0.
        , m_groupDtsTimestampStart(std::numeric_limits<uint64_t>::max())
        , m_groupDtsTimestampEnd(0)
    {
    }

    void refresh()
    {
        m_maxFrameDuration = 0;
        m_groupTimestampStart = std::numeric_limits<uint64_t>::max();
        m_groupTimestampEnd = 0;
        m_groupDtsTimestampStart = std::numeric_limits<uint64_t>::max();
        m_groupDtsTimestampEnd = 0;
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
        if (packet->m_dts < m_groupDtsTimestampStart) {
            m_groupDtsTimestampStart = packet->m_dts;
        }
        if (packet->m_pts + packet->m_duration > m_groupTimestampEnd) {
            m_groupTimestampEnd = packet->m_pts + packet->m_duration;
        }
        if (packet->m_dts + packet->m_duration > m_groupDtsTimestampEnd) {
            m_groupDtsTimestampEnd = packet->m_dts + packet->m_duration;
        }
        m_dataSize += packet->m_dataSize;
    }

    void pushMediaPacket(MediaPacket* packet)
    {
        m_packets.push_back(packet);
        updateGroupInfo(packet);
    }
};

// Atomic (unscanned) GC allocation: holds pure media bytes, so the
// conservative GC must not scan it. NOTE resize() does not zero new
// regions — always write (memcpy) before reading them.
typedef GCAtomicVector<uint8_t> SourceBufferDataVector;

class SourceBuffer : public EventTarget, public DocumentHoldable {
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

    virtual ExecutionContext* executionContext() const override;
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
    void changeType(String* type);
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
    // Like findProperMediaPacket, but copies the located packet's encoded
    // bytes + metadata into `outData` / the returned view under the packet
    // lock, so the result stays valid even if a concurrent remove()/eviction
    // frees the underlying MediaPacket while the caller is still decoding it.
    // (findProperMediaPacket hands back a raw MediaPacket* that becomes a
    // use-after-free the moment the lock is dropped.) Thread-safe.
    struct MediaPacketView {
        uint64_t m_dts = 0;
        uint64_t m_pts = 0;
        size_t m_duration = 0;
        size_t m_dataSize = 0;
        size_t m_initSegmentIndex = SIZE_MAX;
        bool m_hasIdr = false;
        bool m_found = false;
    };
    MediaPacketView copyProperMediaPacket(size_t streamIdx,
                                          uint64_t startPositionInDTSWantToFind,
                                          std::vector<uint8_t>& outData);
    // Metadata-only variant of copyProperMediaPacket: snapshots the view
    // under the packet lock but skips copying the encoded bytes. Use when
    // only m_dts/m_hasIdr/etc. are needed (e.g. IDR-align probing).
    // Unlike find/copy it does NOT advance the packet access cache, so the
    // peeked packet is still returned by the next find/copy for the same
    // DTS. Thread-safe.
    MediaPacketView peekProperMediaPacket(
        size_t streamIdx, uint64_t startPositionInDTSWantToFind);
    void revertLastCacheIfPossible(size_t streamIdx);
    uint64_t lastBufferedTimestamp(size_t streamIdx);
    void clearPacketAccessCache();
    // Finds the DTS of the closest keyframe (IDR) at or before `dts` on
    // the stream. Returns false when none is buffered (outDTS untouched).
    // Used to restart ES submission after a seek from a decodable
    // position; thread-safe.
    bool findNearestIdrDTSBefore(size_t streamIdx, uint64_t dts,
                                 uint64_t& outDTS);
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
    std::pair<MediaPacket*, size_t> findProperMediaPacketLocked(
        size_t streamIdx, uint64_t startPositionInDTSWantToFind);
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
        clearAll();
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
    void setMaxBufferSizeNeedsUpdate()
    {
        m_maxBufferSizeCache = 0;
    }

    AppendMode m_mode;
    bool m_isAttachedToParent;
    bool m_updating;
    size_t m_initSegmentCount;
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
    // Max m_groupTimestampEnd per stream; UINT64_MAX = invalid, recompute
    // lazily in lastBufferedTimestamp(). Guarded by m_packetGroupsMutex.
    GCVector<uint64_t> m_lastBufferedTimestampCachePerStream;
    // Memoized video-resolution-based max buffer size computed in
    // codedFrameEviction(). 0 = invalid, recompute on next eviction.
    // Main thread only: read in codedFrameEviction(), invalidated in
    // postBufferAppend() where m_streamInfo grows; both serialized by
    // m_updating.
    size_t m_maxBufferSizeCache;
    Mutex* m_packetGroupsMutex;
};
} // namespace Starfish

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
