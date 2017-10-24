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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Event.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/mediasource/SourceBufferList.h"
#include "core/extra/TimeRanges.h"
#include "platform/multimedia/DemuxerSource.h"
#include "platform/multimedia/MediaPlayer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/page/Window.h"

#define TRACE_MSE_GC
#define SOURCEBUFFER_DEBUG
#define STARFISH_FRAME_EVICTION_BACKWARD_DUR 1

#ifdef SOURCEBUFFER_DEBUG
#define SOURCEBUFFER_LOG(sb, ...)                                            \
    STARFISH_LOG_INFO("[SourceBuffer|%p|%s] ", sb,                           \
                      sb->type()->charAt(0) == 'v'                           \
                          ? "video"                                          \
                          : sb->type()->charAt(0) == 'a' ? "audio" : "etc"); \
    STARFISH_LOG_INFO(__VA_ARGS__);
#else
#define SOURCEBUFFER_LOG(sourcebuffer, ...)
#endif

namespace StarFish {

#ifdef TRACE_MSE_GC
static std::list<SourceBuffer*> g_sourceBufferList;
static bool g_traceMSEGCInited = false;
#endif

class DemuxerSourceForSourceBuffer : public DemuxerSource {
public:
    DemuxerSourceForSourceBuffer(SourceBufferData* inputBuffer,
                                 GCVector<uint8_t>* bufferRemain)
        : m_inputBuffer(inputBuffer)
        , m_bufferRemain(bufferRemain)
        , m_readPos(0)
    {
        // STARFISH_LOG_INFO("start demux %d %d\n", (int)m_bufferRemain->size(),
        // (int)m_inputBuffer->m_length);
    }

    virtual int64_t onSeek(int64_t position, SeekWhence whence)
    {
        if (whence == DemuxerSource::SeekWhenceLookSize) {
            return m_bufferRemain->size() + m_inputBuffer->m_length;
        } else if (whence == DemuxerSource::SeekWhenceSet) {
            STARFISH_ASSERT(position >= 0);
            if ((int64_t)position <=
                (int64_t)(m_bufferRemain->size() + m_inputBuffer->m_length)) {
                m_readPos = position;
                return m_readPos;
            } else {
                m_readPos = m_bufferRemain->size() + m_inputBuffer->m_length;
                return m_readPos;
            }
        } else if (whence == DemuxerSource::SeekWhenceCurrent) {
            if (position < 0 && m_readPos < (size_t)std::llabs(position)) {
                m_readPos = 0;
            } else if (m_readPos + position >
                       m_bufferRemain->size() + m_inputBuffer->m_length) {
                m_readPos = m_bufferRemain->size() + m_inputBuffer->m_length;
            } else {
                m_readPos = m_readPos + position;
            }
            return m_readPos;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead,
                        int& error, uint8_t* buffer)
    {
        sizeSuccessToRead = 0;
        error = 0;
        if (m_readPos < m_bufferRemain->size()) {
            size_t fillAmount;

            if (sizeWantToRead > m_bufferRemain->size() - m_readPos) {
                fillAmount = m_bufferRemain->size() - m_readPos;
            } else {
                fillAmount = sizeWantToRead;
            }

            memcpy(buffer, m_bufferRemain->data() + m_readPos, fillAmount);
            sizeSuccessToRead += fillAmount;
            m_readPos += fillAmount;
        }

        if (sizeSuccessToRead < sizeWantToRead) {
            size_t diff = m_readPos - m_bufferRemain->size();
            size_t fillAmount;
            if (sizeWantToRead - sizeSuccessToRead >
                m_inputBuffer->m_length - diff) {
                fillAmount = m_inputBuffer->m_length - diff;
            } else {
                fillAmount = sizeWantToRead - sizeSuccessToRead;
            }

            memcpy(buffer + sizeSuccessToRead, m_inputBuffer->m_data + diff,
                   fillAmount);
            sizeSuccessToRead += fillAmount;
            m_readPos += fillAmount;
        }
        if (sizeWantToRead != sizeSuccessToRead) {
            error = -1;
        }
    }
    SourceBufferData* m_inputBuffer;
    GCVector<uint8_t>* m_bufferRemain;
    size_t m_readPos;
};

struct StreamProcessInfo {
    int64_t m_lastFrameDuration = -1;
    int64_t m_lastDecodeTimestamp = -1;
    int64_t m_highestEndTimestamp = -1;
    bool m_needRandomAccess = false;
};

class DemuxerClientSourceBuffer : public DemuxerClient {
public:
    DemuxerClientSourceBuffer()
        : m_foundError(false)
        , m_isAborted(false)
        , m_currentGroupLastTimestamp(-1)
        , m_timestampOffset(0)
        , m_appendWindowStart(0)
        , m_appendWindowEnd(std::numeric_limits<double>::infinity())
    {
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           STARFISH_LOG_INFO(
                                               "[TRACE_MSE_GC] "
                                               "DemuxerClientSourceBuffer::~"
                                               "DemuxerClientSourceBuffer "
                                               "(%p)\n",
                                               obj);
                                           DemuxerClientSourceBuffer* self =
                                               (DemuxerClientSourceBuffer*)obj;
                                           self->clearAll();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void onDetectStream(const StreamInfo& info)
    {
        m_detectedStream.push_back(info);
    }

    MediaPacketGroup* findRecentPacketGroup(size_t streamIndex)
    {
        for (int i = m_packetGroups.size() - 1; i >= 0; i--) {
            if (m_packetGroups[i]->m_streamIndex == streamIndex) {
                return m_packetGroups[i];
            }
        }
        MediaPacketGroup* newgroup =
            new MediaPacketGroup(streamIndex, SIZE_MAX, nullptr);
        m_packetGroups.push_back(newgroup);
        return newgroup;
    }

    void unsetAllStreamProcessInfo()
    {
        for (size_t i = 0; i < m_streamProcessInfo.size(); i++) {
            m_streamProcessInfo[i].m_lastFrameDuration = -1;
            m_streamProcessInfo[i].m_lastDecodeTimestamp = -1;
            m_streamProcessInfo[i].m_highestEndTimestamp = -1;
            m_streamProcessInfo[i].m_needRandomAccess = true;
        }
    }

    virtual bool onDetectPacket(const MediaPacket& packet)
    {
        int streamIndex = packet.m_streamIndex;
        if ((int)m_streamProcessInfo.size() <= streamIndex) {
            m_streamProcessInfo.resize(streamIndex + 1);
        }
        uint64_t groupTimestampEnd = 0;
        bool ret = false;

        // Step 1 in Coded Frame Processing algorithm
        // For each coded frame in the media segment run the following steps,
        while (true) {
            // 1. Loop Top:
            // Otherwise:
            // Let presentation timestamp be a double precision floating point
            // representation
            // of the coded frame's presentation timestamp in seconds.
            // Let decode timestamp be a double precision floating point
            // representation
            // of the coded frame's decode timestamp in seconds.
            uint64_t presentationTimestamp = packet.m_pts;
            uint64_t decodeTimestamp = packet.m_dts;

            // 2. Let frame duration be a double precision floating point
            // representation of the coded frame's duration in seconds.
            uint64_t frameDuration = packet.m_duration;

            // TODO 3. If mode equals "sequence" and group start timestamp is
            // set, then run the following steps:

            // 4. If timestampOffset is not 0, then run the following steps:
            if (m_timestampOffset != 0) {
                presentationTimestamp += m_timestampOffset;
                decodeTimestamp += m_timestampOffset;
            }

            // 5. Let track buffer equal the track buffer that the coded frame
            // will be added to.
            StreamProcessInfo& trackbufferInfo =
                m_streamProcessInfo[streamIndex];

            // 6. If last decode timestamp for track buffer is set
            //    and decode timestamp is less than last decode timestamp:
            //    OR
            //    If last decode timestamp for track buffer is set and the
            //    difference
            //    between decode timestamp and last decode timestamp is greater
            //    than 2 times last frame duration:
            if (trackbufferInfo.m_lastDecodeTimestamp != -1) {
                int64_t decodedDiff =
                    decodeTimestamp - trackbufferInfo.m_lastDecodeTimestamp;
                // STARFISH_LOG_INFO("[%d] diff: %d - %d = %d duration: %d\n",
                // streamIndex, (int)decodeTimestamp,
                // (int)trackbufferInfo.m_lastDecodeTimestamp, (int)decodedDiff,
                // (int)trackbufferInfo.m_lastFrameDuration);
                if (decodedDiff < 0 ||
                    decodedDiff > 2 * trackbufferInfo.m_lastFrameDuration) {
                    // If mode equals "segments": Set group end timestamp to
                    // presentation timestamp.
                    // TODO If mode equals "sequence": Set group start timestamp
                    // equal to the group end timestamp.
                    groupTimestampEnd = presentationTimestamp;

                    unsetAllStreamProcessInfo();
                    m_currentGroupLastTimestamp = -1;
                    continue;
                }
            }

            // 7. Let frame end timestamp equal the sum of presentation
            // timestamp and frame duration.
            uint64_t frameEndTimestamp = presentationTimestamp + frameDuration;

            // 8. If presentation timestamp is less than appendWindowStart, then
            // set the need random access point flag to true, drop the coded
            // frame, and jump to the top of the loop to start processing the
            // next coded frame.
            // 9. If frame end timestamp is greater than appendWindowEnd, then
            // set the need random access point flag to true, drop the coded
            // frame, and jump to the top of the loop to start processing the
            // next coded frame.
            if (presentationTimestamp < m_appendWindowStart ||
                presentationTimestamp > m_appendWindowEnd) {
                trackbufferInfo.m_needRandomAccess = true;
                return ret;
            }

            // 10. If the need random access point flag on track buffer equals
            // true, then run the following steps:
            if (trackbufferInfo.m_needRandomAccess) {
                // TODO If the coded frame is not a random access point, then
                // drop the coded frame
                //      and jump to the top of the loop to start processing the
                //      next coded frame.
                // Set the need random access point flag on track buffer to
                // false.
                trackbufferInfo.m_needRandomAccess = false;
            }

            // NOTE: this packet should put to new group
            MediaPacketGroup* group = nullptr;
            if (m_currentGroupLastTimestamp == -1) {
                MediaPacketGroup* newgroup =
                    new MediaPacketGroup(streamIndex, SIZE_MAX, nullptr,
                                         frameDuration, decodeTimestamp);
                m_packetGroups.push_back(newgroup);
                group = newgroup;
            } else {
                group = findRecentPacketGroup((size_t)streamIndex);
            }

            MediaPacket* pkt = new MediaPacket();
            pkt->m_dts = decodeTimestamp;
            pkt->m_pts = presentationTimestamp;
            pkt->m_duration = frameDuration;
            pkt->m_streamIndex = streamIndex;
            pkt->m_dataSize = packet.m_dataSize;
            pkt->m_data = packet.m_data;
            pkt->m_hasIdr = packet.m_hasIdr;
            ret = true;

            group->pushMediaPacket(pkt);

            m_currentGroupLastTimestamp = decodeTimestamp;

            // TODO(?) 11-16. Consider overlapped frames

            // 17-19. Update TrackBufferInfo
            trackbufferInfo.m_lastDecodeTimestamp = decodeTimestamp;
            trackbufferInfo.m_lastFrameDuration = frameDuration;
            if (trackbufferInfo.m_highestEndTimestamp < 0 ||
                frameEndTimestamp >
                    (uint64_t)trackbufferInfo.m_highestEndTimestamp) {
                trackbufferInfo.m_highestEndTimestamp = frameEndTimestamp;
            }

            // 20. If frame end timestamp is greater than group end timestamp,
            //     then set group end timestamp equal to frame end timestamp.
            if (frameEndTimestamp > groupTimestampEnd) {
                groupTimestampEnd = frameEndTimestamp;
            }

            // TODO 21. If generate timestamps flag equals true, then set
            // timestampOffset equal to frame end timestamp.
            return ret;
        }
    }

    void setTimestampInfo(double timestampOffset, double appendWindowStart,
                          double appendWindowEnd)
    {
        m_timestampOffset = timestampOffset * 1000;
        m_appendWindowStart = appendWindowStart * 1000;
        m_appendWindowEnd = appendWindowEnd * 1000;
    }

    void setTimestampInfo(SourceBuffer* sourceBuffer)
    {
        m_timestampOffset = sourceBuffer->timestampOffset() * 1000;
        m_appendWindowStart = sourceBuffer->appendWindowStart() * 1000;
        m_appendWindowEnd = sourceBuffer->appendWindowEnd() * 1000;
    }

    void clearAll()
    {
        for (size_t i = 0; i < m_packetGroups.size(); i++) {
            MediaPacketGroup* grp = m_packetGroups[i];
            for (size_t j = 0; j < grp->m_packets.size(); j++) {
                delete[] grp->m_packets[j]->m_data;
                delete grp->m_packets[j];
            }
            std::vector<MediaPacket*>().swap(grp->m_packets);
        }
        std::vector<StreamInfo>().swap(m_detectedStream);
        std::vector<MediaPacketGroup*>().swap(m_packetGroups);
        std::vector<StreamProcessInfo>().swap(m_streamProcessInfo);
        m_bufferUnprocessed.clear();
    }

    bool m_foundError;
    bool m_isAborted;
    std::vector<StreamInfo> m_detectedStream;
    std::vector<MediaPacketGroup*> m_packetGroups;
    std::vector<StreamProcessInfo> m_streamProcessInfo;
    int64_t m_currentGroupLastTimestamp;
    double m_timestampOffset;
    double m_appendWindowStart;
    double m_appendWindowEnd;
    SourceBufferDataVector m_bufferUnprocessed;
};

SourceBufferData::SourceBufferData(SourceBuffer* buf, const uint8_t* data,
                                   unsigned long length, ScriptValue origin)
    : m_sourceBuffer(buf)
    , m_currentDemuxer(buf->demuxer())
    , m_data(data)
    , m_length(length)
    , m_originScript(origin)
{
}

DemuxerClientSourceBuffer* SourceBufferData::currentDemuxerClient()
{
    return (DemuxerClientSourceBuffer*)m_currentDemuxer->client(0);
}

SourceBuffer::SourceBuffer(Document* document, String* type)
    : EventTarget(document)
    , m_mode(AppendMode::Segments)
    , m_isAttachedToParent(false)
    , m_updating(false)
    , m_initSegmentCount(0)
    , m_starFish(document->starFish())
    , m_demuxer(nullptr)
    , m_buffered(nullptr)
    , m_timestampOffset(0)
    , m_audioTracks(nullptr)
    , m_videoTracks(nullptr)
    , m_textTracks(nullptr)
    , m_appendWindowStart(0)
    , m_appendWindowEnd(std::numeric_limits<double>::infinity())
    , m_groupStartTimestamp(std::numeric_limits<double>::quiet_NaN())
    , m_groupEndTimestamp(0)
    , m_type(type)
    , m_parentMediaSource(nullptr)
    , m_packetGroupsMutex(new Mutex())
{
    m_demuxer = Demuxer::createDemuxer(m_type);
    m_demuxer->addClient(new DemuxerClientSourceBuffer());

    STARFISH_LOG_INFO("[TRACE_MSE_GC] SourceBuffer::SourceBuffer (%p)\n", this);
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            STARFISH_LOG_INFO(
                "[TRACE_MSE_GC] SourceBuffer::~SourceBuffer (%p)\n", obj);
            SourceBuffer* nr = (SourceBuffer*)obj;
            nr->clearAll();
#ifdef TRACE_MSE_GC
            g_sourceBufferList.remove(nr);
#endif
        },
        NULL, NULL, NULL);
#ifdef TRACE_MSE_GC

    // #define TRACE_MSE_GC_DETAIL
    g_sourceBufferList.push_back(this);
    if (!g_traceMSEGCInited) {
        g_traceMSEGCInited = true;
        addGCCollectionListener([](GC_EventType e) {
            if (GC_EVENT_PRE_START_WORLD != e) {
                return;
            }
            size_t totalDataSize = 0;

            auto iter = g_sourceBufferList.begin();

            while (iter != g_sourceBufferList.end()) {
                SourceBuffer* sb = *iter;
#ifdef TRACE_MSE_GC_DETAIL
                STARFISH_LOG_INFO(
                    "[TRACE_MSE_GC] SourceBuffer %p-----------------\n", sb);
#endif
                for (size_t i = 0; i < sb->m_packetGroups.size(); i++) {
                    std::vector<MediaPacket*>& p =
                        sb->m_packetGroups[i]->m_packets;
                    size_t dataSize = 0;
                    for (size_t j = 0; j < p.size(); j++) {
                        dataSize += p[j]->m_dataSize;
                    }
#ifdef TRACE_MSE_GC_DETAIL
                    STARFISH_LOG_INFO(
                        "[TRACE_MSE_GC] packetGroupInfo %p %d->%d %fMB\n",
                        sb->m_packetGroups[i],
                        (int)sb->m_packetGroups[i]->m_groupTimestampStart,
                        (int)sb->m_packetGroups[i]->m_groupTimestampEnd,
                        dataSize / 1024.f / 1024.f);
#endif
                    totalDataSize += dataSize;
                }
                iter++;
            }
            STARFISH_LOG_INFO("[TRACE_MSE_GC] totalDataSize %fMB\n",
                              totalDataSize / 1024.f / 1024.f);
        });
    }

#endif
}

void SourceBuffer::clearAll()
{
    {
        Locker<Mutex> locker(*m_packetGroupsMutex);
        size_t removedSize = 0;
        for (size_t i = 0; i < m_packetGroups.size(); i++) {
            removedSize += m_packetGroups[i]->m_dataSize;
            std::vector<MediaPacket*>& p = m_packetGroups[i]->m_packets;
            for (size_t j = 0; j < p.size(); j++) {
                delete[] p[j]->m_data;
                delete p[j];
            }
            std::vector<MediaPacket*>().swap(p);
            delete m_packetGroups[i];
        }
        std::vector<MediaPacketGroup*>().swap(m_packetGroups);
        decreaseUsedBufferSize(removedSize);
        setBufferedRangeNeedsUpdate();
    }
    clearPacketAccessCache();
}

void SourceBuffer::setUpdating(bool flag, UpdateState state)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(m_updating != flag);
    m_updating = flag;

    if (!m_parentMediaSource) {
        return;
    }
    StaticStrings* ss = m_parentMediaSource->starFish()->staticStrings();
    // NOTE Use static strings for event name
    std::vector<String*> events;
    if (m_updating && state == SourceBuffer::Success) {
        events.push_back(ss->m_updatestart.localName());
    } else if (!m_updating) {
        if (state == SourceBuffer::Success) {
            events.push_back(ss->m_update.localName());
        } else if (state == SourceBuffer::Error) {
            events.push_back(ss->m_error.localName());
        } else if (state == SourceBuffer::Abort) {
            events.push_back(ss->m_abort.localName());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        events.push_back(ss->m_updateend.localName());
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    for (size_t i = 0; i < events.size(); i++) {
        m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(
            this, new Event(document(), events[i]));
    }
    if (flag == false && state == SourceBuffer::Success) {
        // propagate update state to mediaSource now.
        m_parentMediaSource->didSourceBufferUpdated(this);

        // Set m_buffered to nullptr for re-calculation in the future
        setBufferedRangeNeedsUpdate();
    }
}

void SourceBuffer::abort()
{
    // If this object has been removed from the sourceBuffers attribute of the
    // parent media source then throw an InvalidStateError exception and abort
    // these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "SourceBuffer has been removed from from parent "
                               "MediaSource, when executing 'abort' of "
                               "SourceBuffer");
    }

    // If the readyState attribute of the parent media source is not in the
    // "open" state then throw an InvalidStateError exception and abort these
    // steps.
    if (m_parentMediaSource->readyStateValue() != MediaSource::Open) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "readyState of parentMediaSource is not open, "
                               "when executing 'abort' of SourceBuffer");
    }

    // TODO If the range removal algorithm is running, then throw an
    // InvalidStateError exception and abort these steps.
    // we are running removal algorithm in main thread now.

    // If the updating attribute equals true, then run the following steps:
    if (m_updating) {
        // Queue a task to fire a simple event named abort at this SourceBuffer
        // object.
        // Queue a task to fire a simple event named updateend at this
        // SourceBuffer object.
        setUpdating(false, SourceBuffer::Abort);
        auto currentClient = demuxerClient();
        STARFISH_ASSERT(currentClient);
        currentClient->m_isAborted = true;
    }
    // Run the reset parser state algorithm.
    resetParserState();

    // Set appendWindowStart to the presentation start time.
    // Set appendWindowEnd to positive Infinity.
    m_appendWindowStart = 0;
    m_appendWindowEnd = std::numeric_limits<double>::infinity();

    m_initSegmentCount = 0;
}

DEFINE_EVENT_LISTENER(SourceBuffer, updatestart);
DEFINE_EVENT_LISTENER(SourceBuffer, update);
DEFINE_EVENT_LISTENER(SourceBuffer, updateend);
DEFINE_EVENT_LISTENER(SourceBuffer, error);
DEFINE_EVENT_LISTENER(SourceBuffer, abort);

void SourceBuffer::appendBuffer(const uint8_t* data, unsigned long length,
                                ScriptValue origin)
{
    // Run the prepare append algorithm.
    prepareAppend(length);

    // Add data to the end of the input buffer.
    auto d = new (NoGC) SourceBufferData(this, data, length, origin);

    // Set the updating attribute to true.
    // Queue a task to fire a simple event named updatestart at this
    // SourceBuffer object.
    STARFISH_ASSERT(m_updating == false);
    setUpdating(true, UpdateState::Success);

    // Asynchronously run the buffer append algorithm.
    bufferAppend(d);
}

void SourceBuffer::appendBuffer(ArrayBufferViewOrArrayBuffer buffer)
{
    if (buffer.isArrayBufferViewValue()) {
        ScriptArrayBufferView unwrap = buffer.getArrayBufferViewValue();
        appendBuffer(arrayBufferViewRawData(unwrap),
                     arrayBufferViewSize(unwrap), createScriptValue(unwrap));
    } else {
        STARFISH_ASSERT(buffer.isArrayBufferValue());
        ScriptArrayBuffer unwrap = buffer.getArrayBufferValue();
        appendBuffer(arrayBufferRawData(unwrap), arrayBufferSize(unwrap),
                     createScriptValue(unwrap));
    }
}

void SourceBuffer::prepareAppend(size_t newDataSize)
{
    // 3.5.4 Prepare Append Algorithm

    // If the SourceBuffer has been removed from the sourceBuffers attribute of
    // the parent media source then throw an InvalidStateError exception and
    // abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(
            document(), DOMException::INVALID_STATE_ERR,
            "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError
    // exception and abort these steps.
    if (m_updating) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "SourceBuffer is now updating");
    }

    // TODO If the HTMLMediaElement.error attribute is not null, then throw an
    // InvalidStateError exception and abort these steps.

    // If the readyState attribute of the parent media source is in the "ended"
    // state then run the following steps:
    if (m_parentMediaSource->readyStateValue() == MediaSource::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent
        // media source.
        m_parentMediaSource->setReadyState(MediaSource::Open);
    }

    // Run the coded frame eviction algorithm.
    if (!codedFrameEviction(newDataSize)) {
        SOURCEBUFFER_LOG(
            this, "Failed to make buffer space: throw QUOTA_EXCEEDED_ERR\n");
        throw new DOMException(document(), DOMException::QUOTA_EXCEEDED_ERR,
                               "SourceBuffer is full");
    }
}

void SourceBuffer::remove(double start, double end)
{
    // If this object has been removed from the sourceBuffers attribute of the
    // parent media source then throw an InvalidStateError exception and abort
    // these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(
            document(), DOMException::INVALID_STATE_ERR,
            "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError
    // exception and abort these steps.
    if (m_updating) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "SourceBuffer is now updating");
    }

    // If duration equals NaN, then throw a TypeError exception and abort these
    // steps.
    double duration = m_parentMediaSource->duration();
    if (std::isnan(duration)) {
        throw new DOMException(
            document(), DOMException::TYPE_ERR,
            "If Duration is NaN, can not execute remove method");
    }

    // If start is negative or greater than duration, then throw a TypeError
    // exception and abort these steps.
    if (start < 0 || start > duration) {
        throw new DOMException(document(), DOMException::TYPE_ERR,
                               "when executing remove, start must be greater "
                               "than zero and smaller than duration");
    }

    // If end is less than or equal to start or end equals NaN, then throw a
    // TypeError exception and abort these steps.
    if (end <= start || std::isnan(end)) {
        throw new DOMException(document(), DOMException::TYPE_ERR,
                               "when executing remove, end must be greater "
                               "than start and not NaN");
    }

    // If the readyState attribute of the parent media source is in the "ended"
    // state then run the following steps:
    if (parentMediaSource()->readyStateValue() ==
        MediaSource::ReadyState::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent
        // media source.
        parentMediaSource()->setReadyState(MediaSource::ReadyState::Open);
    }

    SOURCEBUFFER_LOG(this, "Remove range (%dms->%dms)\n", (int)(start * 1000),
                     (int)(end * 1000));
    setUpdating(true, UpdateState::Success);
    rangeRemoval(start * 1000, end * 1000);
    setUpdating(false, UpdateState::Success);
}

void SourceBuffer::rangeRemoval(uint64_t startTimestamp, uint64_t endTimestamp,
                                StreamType type)
{
    Locker<Mutex> lock(*m_packetGroupsMutex);
    rangeRemovalWithoutGuard(startTimestamp, endTimestamp, type);
}

void SourceBuffer::rangeRemovalWithoutGuard(uint64_t startTimestamp,
                                            uint64_t endTimestamp,
                                            StreamType type)
{
    // 3.5.6 Range Removal
    size_t groupIndex = 0;
    size_t removedSize = 0;
    while (groupIndex < m_packetGroups.size()) {
        MediaPacketGroup* grp = m_packetGroups[groupIndex];
        // Note : Remove Packets
        //        packet.start < endTimestamp && patcket.end > startTimestamp
        if ((grp->m_streamInfo->type() & type) &&
            !(startTimestamp >= grp->m_groupTimestampEnd ||
              endTimestamp <= grp->m_groupTimestampStart)) {
            if (startTimestamp <= grp->m_groupTimestampStart &&
                grp->m_groupTimestampEnd <= endTimestamp) {
                // STARFISH_LOG_INFO("SourceBuffer::remove all %d
                // was(%d->%d)\n", (int)groupIndex,
                // (int)grp->m_groupTimestampStart,
                // (int)grp->m_groupTimestampEnd);

                for (size_t i = 0; i < grp->m_packets.size(); i++) {
                    removedSize += grp->m_packets[i]->m_dataSize;
                    delete[] grp->m_packets[i]->m_data;
                    delete grp->m_packets[i];
                }

                std::vector<MediaPacket*>().swap(grp->m_packets);
                delete grp;
                m_packetGroups.erase(m_packetGroups.begin() + groupIndex);
                continue;
            } else if (grp->m_groupTimestampStart < startTimestamp &&
                       endTimestamp < grp->m_groupTimestampEnd) {
                // remove center
                size_t holeStart = 0;
                size_t holeEnd = grp->m_packets.size();

                for (size_t i = 0; i < grp->m_packets.size(); i++) {
                    MediaPacket* pkt = grp->m_packets[i];
                    // NOTE pts packets in group are not sorted
                    //      Instead of pts, use dts here
                    if (pkt->m_dts < startTimestamp) {
                        holeStart = i;
                    } else {
                        break;
                    }
                }

                for (size_t i = grp->m_packets.size(); i > 0; i--) {
                    MediaPacket* pkt = grp->m_packets[i - 1];
                    // NOTE pts packets in group are not sorted
                    //      Instead of pts, use dts here
                    if ((pkt->m_dts + pkt->m_duration) > endTimestamp) {
                        holeEnd = i;
                    } else {
                        break;
                    }
                }

                // STARFISH_LOG_INFO("SourceBuffer::remove hole %d %d %d\n",
                // (int)groupIndex, (int)holeStart, (int)holeEnd);

                MediaPacketGroup* newGroup = new MediaPacketGroup(
                    grp->m_streamIndex, grp->m_initSegmentIndex,
                    grp->m_streamInfo);
                for (size_t i = holeEnd; i < grp->m_packets.size(); i++) {
                    newGroup->pushMediaPacket(grp->m_packets[i]);
                }

                if (newGroup->m_packets.size()) {
                    grp->m_packets.erase(grp->m_packets.begin() + holeEnd,
                                         grp->m_packets.end());
                    m_packetGroups.insert(m_packetGroups.begin() + groupIndex,
                                          newGroup);
                    groupIndex++;
                } else {
                    delete newGroup;
                }
                for (size_t i = holeStart; i < holeEnd; i++) {
                    removedSize += grp->m_packets[i]->m_dataSize;
                    delete[] grp->m_packets[i]->m_data;
                    delete grp->m_packets[i];
                }
                grp->m_packets.erase(grp->m_packets.begin() + holeStart,
                                     grp->m_packets.begin() + holeEnd);

                if (grp->m_packets.size()) {
                    grp->refresh();
                    groupIndex++;
                } else {
                    delete grp;
                    m_packetGroups.erase(std::find(m_packetGroups.begin(),
                                                   m_packetGroups.end(), grp));
                }
                continue;
            } else {
                // Remove head or tail
                size_t eraseStart = 0, eraseEnd = grp->m_packets.size();
                if (startTimestamp <= grp->m_groupTimestampStart &&
                    endTimestamp < grp->m_groupTimestampEnd) {
                    // Remove head
                    for (eraseEnd = 0; eraseEnd < grp->m_packets.size();
                         eraseEnd++) {
                        MediaPacket* pkt = grp->m_packets[eraseEnd];
                        // NOTE pts packets in group are not sorted
                        //      Instead of pts, use dts here
                        if (pkt->m_dts >= endTimestamp) {
                            break;
                        }
                    }
                } else {
                    // Remove tail
                    for (eraseStart = grp->m_packets.size(); eraseStart > 0;
                         eraseStart--) {
                        MediaPacket* pkt = grp->m_packets[eraseStart - 1];
                        // NOTE pts packets in group are not sorted
                        //      Instead of pts, use dts here
                        if (pkt->m_dts + pkt->m_duration <= startTimestamp) {
                            break;
                        }
                    }
                }

                if (eraseStart < eraseEnd) {
                    // STARFISH_LOG_INFO("SourceBuffer::remove %d %d %d\n",
                    // (int)groupIndex, (int)eraseStart, (int)eraseEnd);
                    for (size_t i = eraseStart; i < eraseEnd; i++) {
                        removedSize += grp->m_packets[i]->m_dataSize;
                        delete[] grp->m_packets[i]->m_data;
                        delete grp->m_packets[i];
                    }
                    grp->m_packets.erase(grp->m_packets.begin() + eraseStart,
                                         grp->m_packets.begin() + eraseEnd);
                    if (grp->m_packets.size()) {
                        grp->refresh();
                        groupIndex++;
                    } else {
                        delete grp;
                        m_packetGroups.erase(m_packetGroups.begin() +
                                             groupIndex);
                    }
                    continue;
                }
            }
        }
        groupIndex++;
    }

    auto iter2 = m_packetAccessCachePerStream.begin();
    while (iter2 != m_packetAccessCachePerStream.end()) {
        *iter2 = std::make_pair<size_t, size_t>(SIZE_MAX, SIZE_MAX);
        iter2++;
    }
    decreaseUsedBufferSize(removedSize);
    setBufferedRangeNeedsUpdate();
}

bool SourceBuffer::codedFrameEviction(size_t newDataSize)
{
    // 3.5.14 Coded Frame Eviction Algorithm
    STARFISH_ASSERT(m_isAttachedToParent && m_parentMediaSource);
    // NOTE
    // Data size can be increased by adding extra data
    size_t maxAssume =
        (newDataSize + demuxerClient()->m_bufferUnprocessed.size()) * 1.1;
    SOURCEBUFFER_LOG(
        this, "Run Code Frame Eviction algorithm (new: %d, available: %d)\n",
        (int)maxAssume, (int)m_parentMediaSource->availableBufferSize());

    // 1. Let new data equal the data that is about to be appended to this
    // SourceBuffer.
    // 2. If the buffer full flag equals false, then abort these steps.
    // NOTE Ignore step2 to try eviction when `bufferFull` caused by assumtion
    // failure of data size

    if (maxAssume >= STARFISH_MAX_MEDIASOURCE_BUFFERSPACE) {
        return false;
    }
    if (maxAssume >= m_parentMediaSource->availableBufferSize()) {
        HTMLMediaElement* element = m_parentMediaSource->attachedMediaElement();
        double playbackPos = element ? element->currentTime() : 0;
        // Try to remove backward packets
        double backwardPos = playbackPos - STARFISH_FRAME_EVICTION_BACKWARD_DUR;
        if (backwardPos > 0) {
            m_parentMediaSource->evict(0, backwardPos * 1000);
        }
        SOURCEBUFFER_LOG(this, "Remove backward data (available: %d)\n",
                         (int)m_parentMediaSource->availableBufferSize());
        // TODO Try to remove fragmented forward packets
        if (maxAssume >= m_parentMediaSource->availableBufferSize()) {
            return false;
        }
    }
    return true;
}

DemuxerClientSourceBuffer* SourceBuffer::demuxerClient()
{
    if (m_demuxer && m_demuxer->clientSize() > 0) {
        return (DemuxerClientSourceBuffer*)m_demuxer->client(0);
    }
    return nullptr;
}

// https://www.w3.org/TR/media-source/#sourcebuffer-reset-parser-state
void SourceBuffer::resetParserState()
{
    STARFISH_ASSERT(isMainThread());
    m_demuxer = Demuxer::createDemuxer(m_type);
    m_demuxer->addClient(new DemuxerClientSourceBuffer());
}

// https://www.w3.org/TR/media-source/#sourcebuffer-append-error
void SourceBuffer::appendError()
{
    STARFISH_ASSERT(isMainThread());
    // Run the reset parser state algorithm.
    resetParserState();
    // Set the updating attribute to false.
    // Queue a task to fire a simple event named error at this SourceBuffer
    // object.
    // Queue a task to fire a simple event named updateend at this SourceBuffer
    // object.
    setUpdating(false, UpdateState::Error);
    // Run the end of stream algorithm with the error parameter set to "decode".
    if (m_parentMediaSource) {
        m_parentMediaSource->endOfStreamInternal(MediaSource::Decode);
    }
}

// Return processed size
static size_t tryDemuxing(SourceBufferData* inputBuffer)
{
    // TODO Need to fix (m_demuxer can be replaced by abort())
    STARFISH_ASSERT(!isMainThread());
    // NOTE Do not get demuxer and demuxerClient from sourceBuffer.
    //      They may have been replaced to new one by 'abort()'.
    auto demuxer = inputBuffer->m_currentDemuxer;
    auto client = inputBuffer->currentDemuxerClient();
    DemuxerSourceForSourceBuffer src(inputBuffer, &client->m_bufferUnprocessed);

    SOURCEBUFFER_LOG(inputBuffer->m_sourceBuffer, "findStreamInfo\n");
    STARFISH_ASSERT(client->m_detectedStream.size() == 0);
    size_t maxPos = client->m_bufferUnprocessed.size() + inputBuffer->m_length;
    if (!demuxer->findStreamInfo(&src, inputBuffer->m_sourceBuffer->type())) {
        SOURCEBUFFER_LOG(inputBuffer->m_sourceBuffer,
                         "Found error while demuxing\n");
        client->m_foundError = true;
        return maxPos;
    }
    client->setTimestampInfo(inputBuffer->m_sourceBuffer);
    SOURCEBUFFER_LOG(inputBuffer->m_sourceBuffer, "findStreamPacket\n");
    if (!demuxer->findStreamPacket(&src)) {
        SOURCEBUFFER_LOG(inputBuffer->m_sourceBuffer,
                         "Found error while demuxing\n");
        client->m_foundError = true;
        return maxPos;
    }
    return src.m_readPos;
}

static void updateBufferUnprocessed(SourceBufferData* inputBuffer,
                                    size_t processedSize)
{
    STARFISH_ASSERT(isMainThread());
    // NOTE Do not get demuxer and demuxerClient from sourceBuffer.
    //      They may have been replaced to new one by 'abort()'.
    auto client = inputBuffer->currentDemuxerClient();
    SourceBufferDataVector& unp = client->m_bufferUnprocessed;
    size_t lastSize = unp.size();
    size_t amount = processedSize > lastSize ? lastSize : processedSize;
    unp.erase(unp.begin(), unp.begin() + amount);
    if (processedSize == inputBuffer->m_length + lastSize) {
        return;
    }
    size_t copyStart = 0;
    size_t copyEnd = inputBuffer->m_length;
    if (processedSize > lastSize) {
        copyStart = processedSize - lastSize;
    }
    unp.insert(unp.end(), inputBuffer->m_data + copyStart,
               inputBuffer->m_data + copyEnd);
    SOURCEBUFFER_LOG(inputBuffer->m_sourceBuffer, "Got unprocessed (size %d)\n",
                     (int)(copyEnd - copyStart));
}

void SourceBuffer::postBufferAppend(SourceBufferData* inputBuffer)
{
    STARFISH_ASSERT(isMainThread());
    // NOTE Do not get demuxer and demuxerClient from sourceBuffer.
    //      They may have been replaced to new one by 'abort()'.
    auto client = inputBuffer->currentDemuxerClient();
    // Check abort flag
    if (client->m_isAborted) {
        STARFISH_ASSERT(inputBuffer->m_sourceBuffer->demuxerClient() != client);
        SOURCEBUFFER_LOG(inputBuffer->m_sourceBuffer,
                         "Aborted. clear demuxed data.\n");
        // NOTE Delete data before GC does
        client->clearAll();
        return;
    }
    // Handle error
    if (client->m_foundError) {
        inputBuffer->m_sourceBuffer->appendError();
        return;
    }
    // Copy demuxing result to SourceBuffer
    {
        Locker<Mutex> packetGroupLocker(*m_packetGroupsMutex);
        // Move detected StreamInfo
        if (client->m_detectedStream.size() > 0) {
            GCVector<StreamInfo*> streamInfo;
            if (m_initSegmentCount == 0) {
                initializePacketAccessCache(client->m_detectedStream.size());
            }
            for (size_t i = 0; i < client->m_detectedStream.size(); i++) {
                StreamInfo* info =
                    new StreamInfo(std::move(client->m_detectedStream[i]));
                streamInfo.push_back(info);
            }

            m_streamInfo.push_back(std::move(streamInfo));
            m_initSegmentCount++;
            SOURCEBUFFER_LOG(this, "Got init segments: %d\n",
                             (int)client->m_detectedStream.size());
        }
        if (!m_initSegmentCount) {
            inputBuffer->m_sourceBuffer->appendError();
            return;
        }
        size_t addedSize = 0;
        // Validate and fill information for detected packetGroup
        for (size_t i = 0; i < client->m_packetGroups.size(); i++) {
            MediaPacketGroup* group = client->m_packetGroups[i];
            group->m_initSegmentIndex = m_initSegmentCount - 1;
            StreamInfo* stream =
                streamInfo(group->m_initSegmentIndex, group->m_streamIndex);
            group->m_streamInfo = stream;
            if (stream->isVideo()) {
                // Calculate average framerate for video
                if (!stream->videoHasFramerate()) {
                    int sampleCount = group->m_packets.size();
                    uint64_t totalDuration = group->m_groupTimestampEnd -
                                             group->m_groupTimestampStart;
                    Framerate framerate = Framerate::createFromLL(
                        (int64_t)sampleCount * 1000, (int64_t)totalDuration);
                    if (framerate.isValid()) {
                        stream->setVideoFramerate(framerate);
                        stream->setVideoHasFramerate(true);
                    }
                }
            }
            rangeRemovalWithoutGuard(group->m_groupTimestampStart,
                                     group->m_groupTimestampEnd,
                                     stream->type());
            SOURCEBUFFER_LOG(
                this,
                "Got packetGroup (initIdx%d, streamIdx:%d, count:%d, "
                "dur:%dms->%dms)\n",
                (int)group->m_initSegmentIndex, (int)group->m_streamIndex,
                (int)group->m_packets.size(), (int)group->m_groupTimestampStart,
                (int)group->m_groupTimestampEnd);
            addedSize += group->m_dataSize;
        }
        // Move detected packetGroups
        m_packetGroups.insert(m_packetGroups.end(),
                              client->m_packetGroups.begin(),
                              client->m_packetGroups.end());
        increaseUsedBufferSize(addedSize);
        std::vector<StreamInfo>().swap(client->m_detectedStream);
        std::vector<MediaPacketGroup*>().swap(client->m_packetGroups);
        std::vector<StreamProcessInfo>().swap(client->m_streamProcessInfo);
    }
    setUpdating(false, UpdateState::Success);
}

// https://www.w3.org/TR/media-source/#sourcebuffer-buffer-append
void SourceBuffer::bufferAppend(SourceBufferData* inputBuffer)
{
    m_starFish->threadPool()->addWork(
        document()->browsingContext(),
        [](void* data) -> void* {
            SourceBufferData* inputBuffer = (SourceBufferData*)data;
            SOURCEBUFFER_LOG(
                inputBuffer->m_sourceBuffer,
                "Run thread: bufferAppend (size %d + unprocessed %d)\n",
                (int)inputBuffer->m_length,
                (int)inputBuffer->currentDemuxerClient()
                    ->m_bufferUnprocessed.size());
            // Segment Parser Loop
            size_t processedSize = tryDemuxing(inputBuffer);
            // Add post task to main
            inputBuffer->m_sourceBuffer->m_starFish->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    inputBuffer->m_sourceBuffer->document()->browsingContext(),
                    [](size_t, void* data, void* data1) {
                        SourceBufferData* inputBuffer = (SourceBufferData*)data;
                        size_t processedSize = (size_t)data1;
                        // Save unprocessed data
                        updateBufferUnprocessed(inputBuffer, processedSize);
                        // Move results to sourceBuffer from demuxerClient
                        inputBuffer->m_sourceBuffer->postBufferAppend(
                            inputBuffer);
                        // Delete SourceBufferData manually (NOGC)
                        delete inputBuffer;
                    },
                    inputBuffer, (void*)processedSize);
            return nullptr;
        },
        inputBuffer);
}

void SourceBuffer::clearPacketAccessCache()
{
    Locker<Mutex> packetGroupLocker(*m_packetGroupsMutex);
    auto iter = m_packetAccessCachePerStream.begin();
    while (iter != m_packetAccessCachePerStream.end()) {
        *iter = std::make_pair<size_t, size_t>(SIZE_MAX, SIZE_MAX);
        iter++;
    }
}

void SourceBuffer::initializePacketAccessCache(size_t streamCount)
{
    for (size_t i = 0; i < streamCount; i++) {
        m_packetAccessCachePerStream.push_back(
            std::make_pair<size_t, size_t>(SIZE_MAX, SIZE_MAX));
    }
}

std::pair<MediaPacket*, size_t> SourceBuffer::findProperMediaPacket(
    size_t streamIdx, uint64_t startPositionInDTSWantToFind)
{
    Locker<Mutex> packetGroupLocker(*m_packetGroupsMutex);
    // test cache first
    {
        auto cache = m_packetAccessCachePerStream[streamIdx];
        if (cache.first < m_packetGroups.size()) {
            MediaPacketGroup* grp = m_packetGroups[cache.first];
            if (grp->m_groupTimestampStart <= startPositionInDTSWantToFind &&
                startPositionInDTSWantToFind <= grp->m_groupTimestampEnd) {
                size_t idx = cache.second + 1;
                if (idx < grp->m_packets.size()) {
                    m_packetAccessCachePerStream[streamIdx] =
                        std::make_pair(cache.first, idx);
                    return std::make_pair(grp->m_packets[idx],
                                          grp->m_initSegmentIndex);
                }
            }
        }
    }

    // STARFISH_LOG_INFO("SourceBuffer::findProperMediaPacket cache miss!
    // streamIdx(%d)\n", (int)streamIdx);

    std::pair<size_t, uint64_t> nearestPacketGroupInfo =
        std::make_pair(SIZE_MAX, std::numeric_limits<uint64_t>::max());

    for (size_t i = 0; i < m_packetGroups.size(); i++) {
        MediaPacketGroup* grp = m_packetGroups[i];
        if (grp->m_streamIndex == streamIdx) {
            if (grp->m_groupTimestampStart <= startPositionInDTSWantToFind &&
                startPositionInDTSWantToFind <= grp->m_groupTimestampEnd) {
                const std::vector<MediaPacket*>& v = grp->m_packets;
                for (size_t j = 0; j < v.size(); j++) {
                    if (v[j]->m_dts >= startPositionInDTSWantToFind) {
                        m_packetAccessCachePerStream[streamIdx] =
                            std::make_pair(i, j);
                        return std::make_pair(v[j], grp->m_initSegmentIndex);
                    }
                }
            } else if (grp->m_groupTimestampStart >
                       startPositionInDTSWantToFind) {
                if (grp->m_groupTimestampStart <
                    nearestPacketGroupInfo.second) {
                    nearestPacketGroupInfo =
                        std::make_pair(i, grp->m_groupTimestampStart);
                }
            }
        }
    }

    if (nearestPacketGroupInfo.first != SIZE_MAX) {
        m_packetAccessCachePerStream[streamIdx] =
            std::make_pair(nearestPacketGroupInfo.first, 0);
        return std::make_pair(
            m_packetGroups[nearestPacketGroupInfo.first]->m_packets[0],
            m_packetGroups[nearestPacketGroupInfo.first]->m_initSegmentIndex);
    }

    return std::make_pair(nullptr, SIZE_MAX);
}

void SourceBuffer::revertLastCacheIfPossible(size_t streamIdx)
{
    auto cache = m_packetAccessCachePerStream[streamIdx];
    if (cache.second > 0) {
        m_packetAccessCachePerStream[streamIdx] =
            std::make_pair(cache.first, cache.second - 1);
    } else {
        m_packetAccessCachePerStream[streamIdx] =
            std::make_pair<size_t, size_t>(SIZE_MAX, SIZE_MAX);
    }
    return;
}

uint64_t SourceBuffer::lastBufferedTimestamp(size_t streamIdx)
{
    Locker<Mutex> packetGroupLocker(*m_packetGroupsMutex);
    uint64_t timestamp = 0;
    for (size_t i = 0; i < m_packetGroups.size(); i++) {
        MediaPacketGroup* grp = m_packetGroups[i];
        if (grp->m_streamIndex == streamIdx) {
            if (timestamp < grp->m_groupTimestampEnd) {
                timestamp = grp->m_groupTimestampEnd;
            }
        }
    }
    return timestamp;
}

void SourceBuffer::setMode(AppendMode mode)
{
    // If this object has been removed from the sourceBuffers attribute of the
    // parent media source, then throw an InvalidStateError exception and abort
    // these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(
            document(), DOMException::INVALID_STATE_ERR,
            "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError
    // exception and abort these steps.
    if (m_updating) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "SourceBuffer is now updating");
    }

    // TODO If generate timestamps flag equals true and new mode equals
    // "segments", then throw a TypeError exception and abort these steps.

    // If the readyState attribute of the parent media source is in the "ended"
    // state then run the following steps:
    if (m_parentMediaSource->readyStateValue() == MediaSource::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent
        // media source.
        m_parentMediaSource->setReadyState(MediaSource::Open);
    }

    // TODO If the append state equals PARSING_MEDIA_SEGMENT, then throw an
    // InvalidStateError and abort these steps.
    // if (m_state == ParsingMediaSegment)
    //     throw new DOMException(m_starFish->window()->scriptBindingInstance(),
    //     DOMException::INVALID_STATE_ERR, "SourceBuffer is currently parsing a
    //     media segment");

    // If the new mode equals "sequence", then set the group start timestamp to
    // the group end timestamp.
    if (mode == Sequence) {
        m_groupStartTimestamp = m_groupEndTimestamp;
    }

    // Update the attribute to new mode.
    m_mode = mode;
}

void SourceBuffer::setMode(String* modeStr)
{
    if (modeStr->equals("segments")) {
        setMode(Segments);
    } else if (modeStr->equals("sequence")) {
        setMode(Sequence);
    } else {
        // TODO: Should generate warning messages, says the input string should
        // be segments or sequence.
        setMode(Segments);
    }
}

void SourceBuffer::setTimestampOffset(double timeoffset)
{
    // If this object has been removed from the sourceBuffers attribute of the
    // parent media source, then throw an InvalidStateError exception and abort
    // these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(
            document(), DOMException::INVALID_STATE_ERR,
            "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError
    // exception and abort these steps.
    if (m_updating) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "SourceBuffer is now updating");
    }

    // If the readyState attribute of the parent media source is in the "ended"
    // state then run the following steps:
    if (m_parentMediaSource->readyStateValue() == MediaSource::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent
        // media source.
        m_parentMediaSource->setReadyState(MediaSource::Open);
    }

    // TODO If the append state equals PARSING_MEDIA_SEGMENT, then throw an
    // InvalidStateError and abort these steps.
    // if (m_state == ParsingMediaSegment)
    //     throw new DOMException(m_starFish->window()->scriptBindingInstance(),
    //     DOMException::INVALID_STATE_ERR, "SourceBuffer is currently parsing a
    //     media segment");

    // If the mode attribute equals "sequence", then set the group start
    // timestamp to new timestamp offset.
    if (m_mode == Sequence) {
        m_groupStartTimestamp = timeoffset;
    }

    // Update the attribute to new timestamp offset.
    m_timestampOffset = timeoffset;
}

void SourceBuffer::setAppendWindowStart(double timeStamp)
{
    // If this object has been removed from the sourceBuffers attribute of the
    // parent media source, then throw an InvalidStateError exception and abort
    // these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(
            document(), DOMException::INVALID_STATE_ERR,
            "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError
    // exception and abort these steps.
    if (m_updating) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "SourceBuffer is now updating");
    }

    // If the new value is less than 0 or greater than or equal to
    // appendWindowEnd then throw a TypeError exception and abort these steps.
    if (timeStamp < 0 || timeStamp >= m_appendWindowEnd) {
        throw new DOMException(
            document(), DOMException::TYPE_ERR,
            "appendWindowStart should be between 0 and appendWindowEnd");
    }

    // Update the attribute to the new value.
    m_appendWindowStart = timeStamp;
}

void SourceBuffer::setAppendWindowEnd(double timeStamp)
{
    // If this object has been removed from the sourceBuffers attribute of the
    // parent media source, then throw an InvalidStateError exception and abort
    // these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(
            document(), DOMException::INVALID_STATE_ERR,
            "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError
    // exception and abort these steps.
    if (m_updating) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "SourceBuffer is now updating");
    }

    // If the new value equals NaN, then throw a TypeError and abort these
    // steps.
    if (std::isnan(timeStamp)) {
        throw new DOMException(document(), DOMException::TYPE_ERR,
                               "appendWindowEnd should not be NaN");
    }

    // If the new value is less than or equal to appendWindowStart then throw a
    // TypeError exception and abort these steps.
    if (timeStamp <= m_appendWindowStart) {
        throw new DOMException(
            document(), DOMException::TYPE_ERR,
            "appendWindowEnd should be greater than appendWindowStart");
    }

    // Update the attribute to the new value.
    m_appendWindowEnd = timeStamp;
}

TimeRanges* SourceBuffer::buffered()
{
    // If there was no sourceBuffer update, reuse previous m_buffered
    if (m_buffered) {
        return m_buffered;
    }

    // If sourceBuffer does not have packetGroups yet, return empty timeRanges
    if (m_streamInfo.size() == 0 || m_packetGroups.size() == 0) {
        return new TimeRanges(document());
    }

    // https://www.w3.org/TR/media-source/#widl-SourceBuffer-buffered
    // 1. If this object has been removed from the sourceBuffers attribute of
    // the parent media source
    //    then throw an InvalidStateError exception and abort these steps.
    // --> Binding layer would catch that
    if (!parentMediaSource()) {
        throw new DOMException(document(),
                               DOMException::Code::INVALID_STATE_ERR);
    }

    // 2. Let highest end time be the largest track buffer ranges end time
    // across all the track buffers managed by this SourceBuffer object.
    // +  Collect tracks (Since current version of Starfish does not support
    // videoTracks/audioTracks/textTracks)
    std::unordered_map<size_t, std::map<uint64_t, MediaPacketGroup*>,
                       std::hash<size_t>, std::equal_to<size_t>>
        tracksForSort;
    uint64_t highestEndTime = 0;
    for (auto i = m_packetGroups.begin(); i != m_packetGroups.end(); i++) {
        MediaPacketGroup* packetGroup = (*i);
        auto itr = tracksForSort.find(packetGroup->m_streamIndex);
        if (itr == tracksForSort.end()) {
            tracksForSort.insert(
                std::make_pair(packetGroup->m_streamIndex,
                               std::map<uint64_t, MediaPacketGroup*>()));
            itr = tracksForSort.find(packetGroup->m_streamIndex);
        }
        std::map<uint64_t, MediaPacketGroup*>& track = itr->second;
        STARFISH_ASSERT(track.find(packetGroup->m_groupTimestampStart) ==
                        track.end());
        track.insert(
            std::make_pair(packetGroup->m_groupTimestampStart, packetGroup));
        if (packetGroup->m_groupTimestampEnd > highestEndTime) {
            highestEndTime = packetGroup->m_groupTimestampEnd;
        }
    }

    // Merge adjacent ranges
    std::vector<std::vector<std::pair<uint64_t, uint64_t>>> tracks;
    tracks.resize(tracksForSort.size());
    unsigned j = 0;
    for (auto i = tracksForSort.begin(); i != tracksForSort.end(); i++, j++) {
        std::vector<std::pair<uint64_t, uint64_t>>& newTrack = tracks[j];
        std::map<uint64_t, MediaPacketGroup*>& oldTrack = i->second;
        STARFISH_ASSERT(oldTrack.size() != 0);

        uint64_t maxDiff = oldTrack.begin()->second->m_maxFrameDuration * 2;
        std::pair<uint64_t, uint64_t> item =
            std::make_pair(oldTrack.begin()->first,
                           oldTrack.begin()->second->m_groupTimestampEnd);
        for (auto t = ++oldTrack.begin(); t != oldTrack.end(); t++) {
            if (t->first < item.second + maxDiff) {
                item.second = t->second->m_groupTimestampEnd;
            } else {
                newTrack.push_back(item);
                item = std::make_pair(t->first, t->second->m_groupTimestampEnd);
            }
            maxDiff = t->second->m_maxFrameDuration * 2;
        }
        newTrack.push_back(item);
    }

    // 3. Let intersection ranges equal a TimeRange object containing a single
    // range from 0 to highest end time.
    std::vector<std::pair<uint64_t, uint64_t>> intersection;
    intersection.push_back(std::make_pair(0, highestEndTime));

    // 4. For each track buffer managed by this SourceBuffer, run the following
    // steps:
    for (unsigned i = 0; i < tracks.size(); i++) {
        // 4-1. Let track ranges equal the track buffer ranges for the current
        // track buffer.
        std::vector<std::pair<uint64_t, uint64_t>>& track = tracks[i];
        STARFISH_ASSERT(track.size() != 0);

        // 4-2. If readyState is "ended", then set the end time on the last
        // range in track ranges to highest end time.
        STARFISH_ASSERT(parentMediaSource());
        if (parentMediaSource()->readyStateValue() == MediaSource::Ended) {
            (--track.end())->second = highestEndTime;
        }
        // 4-3. Let new intersection ranges equal the intersection between the
        // intersection ranges and the track ranges.
        std::vector<std::pair<uint64_t, uint64_t>> newIntersection;
        auto t = track.begin();
        auto j = intersection.begin();
        while (t != track.end() && j != intersection.end()) {
            if (t->second < j->first) {
                t++;
            } else if (t->first > j->second) {
                j++;
            } else {
                newIntersection.push_back(
                    std::make_pair(std::max(t->first, j->first),
                                   std::min(t->second, j->second)));
                if (t->first >= j->first && t->second <= j->second) {
                    t++;
                } else if (t->first <= j->first && t->second >= j->second) {
                    j++;
                } else if (t->first < j->first) {
                    t++;
                } else {
                    j++;
                }
            }
        }
        // 4-4. Replace the ranges in intersection ranges with the new
        // intersection ranges.
        intersection.clear();
        intersection = newIntersection;
    }

    // 5. If intersection ranges does not contain the exact same range
    // information as the current value of this attribute,
    //    then update the current value of this attribute to intersection
    //    ranges.
    m_buffered = new TimeRanges(document());
    for (auto i = intersection.begin(); i != intersection.end(); i++) {
        m_buffered->emplace_back((double)(i->first) / 1000.0,
                                 (double)(i->second) / 1000.0);
    }

    return m_buffered;
}

StreamInfo* SourceBuffer::streamInfo(size_t initSegmentIndex,
                                     size_t streamIndex)
{
    const GCVector<StreamInfo*>& streamInfo = m_streamInfo[initSegmentIndex];
    for (size_t i = 0; i < streamInfo.size(); i++) {
        if (streamInfo[i]->streamIndex() == streamIndex) {
            return streamInfo[i];
        }
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void SourceBuffer::increaseUsedBufferSize(size_t amount)
{
    if (m_parentMediaSource) {
        SOURCEBUFFER_LOG(this, "Increased packet data (%d)\n", (int)amount);
        m_parentMediaSource->m_usedBufferSize += amount;
    }
}

void SourceBuffer::decreaseUsedBufferSize(size_t amount)
{
    if (m_parentMediaSource) {
        STARFISH_ASSERT(m_parentMediaSource->m_usedBufferSize >= amount);
        SOURCEBUFFER_LOG(this, "Removed packet data (%d)\n", (int)amount);
        m_parentMediaSource->m_usedBufferSize -= amount;
    }
}
}

#undef STARFISH_FRAME_EVICTION_BACKWARD_DUR
#ifdef SOURCEBUFFER_DEBUG
#undef SOURCEBUFFER_DEBUG
#endif
#undef SOURCEBUFFER_LOG

#endif
