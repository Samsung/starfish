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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "SourceBuffer.h"
#include "dom/Event.h"
#include "dom/DOMException.h"
#include "dom/HTMLMediaElement.h"
#include "platform/multimedia/MediaPlayer.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/threading/ThreadPool.h"
#include "MediaSource.h"
#include "extra/TimeRanges.h"

#define STARFISH_ENABLE_TIMER
#define TRACE_MSE_GC

namespace StarFish {

#ifdef TRACE_MSE_GC
static std::list<SourceBuffer*> g_sourceBufferList;
static bool g_traceMSEGCInited = false;
#endif

class DemuxerSourceForSourceBuffer : public DemuxerSource {
public:
    DemuxerSourceForSourceBuffer(SourceBufferData* inputBuffer, GCVector<uint8_t>* bufferRemain)
        : m_inputBuffer(inputBuffer)
        , m_bufferRemain(bufferRemain)
        , m_readPos(0)
    {
        // STARFISH_LOG_INFO("start demux %d %d\n", (int)m_bufferRemain->size(), (int)m_inputBuffer->m_length);
    }

    virtual int64_t onSeek(int64_t position, SeekWhence whence)
    {
        if (whence == DemuxerSource::SeekWhenceLookSize) {
            return m_bufferRemain->size() + m_inputBuffer->m_length;
        } else if (whence == DemuxerSource::SeekWhenceSet) {
            // STARFISH_LOG_INFO("onSeek DemuxerSource::SeekWhenceSet %d\n", (int)position);
            STARFISH_ASSERT(position >= 0);
            if ((int64_t)position <= (int64_t)(m_bufferRemain->size() + m_inputBuffer->m_length)) {
                m_readPos = position;
                return m_readPos;
            } else {
                m_readPos = m_bufferRemain->size() + m_inputBuffer->m_length;
                return m_readPos;
            }
        } else if (whence == DemuxerSource::SeekWhenceCurrent) {
            m_readPos = m_readPos + position;
            STARFISH_ASSERT(m_readPos >= 0);
            if (m_readPos <= (m_bufferRemain->size() + m_inputBuffer->m_length)) {
                return m_readPos;
            } else {
                m_readPos = m_bufferRemain->size() + m_inputBuffer->m_length;
                return m_readPos;
            }
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead, int& error, uint8_t* buffer)
    {
        sizeSuccessToRead = 0;
        error = 0;
        if (m_readPos < m_bufferRemain->size()) {
            size_t fillAmount;

            if (sizeWantToRead < (m_readPos - m_bufferRemain->size())) {
                fillAmount = m_readPos - m_bufferRemain->size();
            } else {
                fillAmount = sizeWantToRead;
            }

            memcpy(buffer, m_bufferRemain->data() + m_readPos, fillAmount);
            sizeSuccessToRead += fillAmount;
            m_readPos += fillAmount;
        }

        if (sizeSuccessToRead < sizeWantToRead && m_readPos >= m_bufferRemain->size()) {
            size_t diff = m_readPos - m_bufferRemain->size();
            size_t fillAmount;

            fillAmount = sizeWantToRead - sizeSuccessToRead;

            memcpy(buffer, m_inputBuffer->m_data + diff, fillAmount);
            sizeSuccessToRead += fillAmount;
            m_readPos += fillAmount;
        }
        if (sizeWantToRead != sizeSuccessToRead) {
            error = -1;
        }
        // STARFISH_LOG_INFO("onRead pos %d read %d\n", (int)(m_readPos - sizeSuccessToRead), (int)sizeSuccessToRead);
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
        : m_isAborted(false)
        , m_currentGroupLastTimestamp(-1)
        , m_timestampOffset(0)
        , m_appendWindowStart(0)
        , m_appendWindowEnd(std::numeric_limits<double>::infinity())
    {
    }

    virtual void onDetectVideoStream(const VideoStreamInfo& info)
    {
        m_detectedVideoStream.push_back(info);
    }

    virtual void onDetectAudioStream(const AudioStreamInfo& info)
    {
        m_detectedAudioStream.push_back(info);
    }

    MediaPacketGroup* findRecentPacketGroup(size_t streamIndex)
    {
        // STARFISH_LOG_INFO("[DemuxerClinetSourceBuffer::findRecentPacketGroup] idx: %d groupSize: %d\n", (int)streamIndex, (int)m_packetGroup.size());
        for (int i = m_packetGroup.size() - 1; i >= 0; i--) {
            if (m_packetGroup[i]->m_streamIndex == streamIndex) {
                return m_packetGroup[i];
            }
        }
        MediaPacketGroup* newgroup = new MediaPacketGroup(streamIndex, SIZE_MAX, nullptr);
        m_packetGroup.push_back(newgroup);
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
        if ((int)m_streamProcessInfo.size() <= streamIndex)
            m_streamProcessInfo.resize(streamIndex + 1);
        uint64_t groupTimestampEnd = 0;
        bool ret = false;

        // Step 1 in Coded Frame Processing algorithm
        // For each coded frame in the media segment run the following steps,
        while (true) {
            // 1. Loop Top:
            // Otherwise:
            // Let presentation timestamp be a double precision floating point representation
            // of the coded frame's presentation timestamp in seconds.
            // Let decode timestamp be a double precision floating point representation
            // of the coded frame's decode timestamp in seconds.
            // FIXME? Used pts instead of dts
            uint64_t presentationTimestamp = packet.m_pts;
            uint64_t decodeTimestamp = packet.m_pts;

            // 2. Let frame duration be a double precision floating point representation of the coded frame's duration in seconds.
            uint64_t frameDuration = packet.m_duration;

            // TODO 3. If mode equals "sequence" and group start timestamp is set, then run the following steps:

            // 4. If timestampOffset is not 0, then run the following steps:
            if (m_timestampOffset != 0) {
                presentationTimestamp += m_timestampOffset;
                decodeTimestamp += m_timestampOffset;
            }

            // 5. Let track buffer equal the track buffer that the coded frame will be added to.
            StreamProcessInfo& trackbufferInfo = m_streamProcessInfo[streamIndex];

            // 6. If last decode timestamp for track buffer is set
            //    and decode timestamp is less than last decode timestamp:
            //    OR
            //    If last decode timestamp for track buffer is set and the difference
            //    between decode timestamp and last decode timestamp is greater than 2 times last frame duration:
            if (trackbufferInfo.m_lastDecodeTimestamp != -1) {
                int64_t decodedDiff = decodeTimestamp - trackbufferInfo.m_lastDecodeTimestamp;
                // STARFISH_LOG_INFO("[%d] diff: %d - %d = %d duration: %d\n", streamIndex, (int)decodeTimestamp, (int)trackbufferInfo.m_lastDecodeTimestamp, (int)decodedDiff, (int)trackbufferInfo.m_lastFrameDuration);
                if (decodedDiff < 0 || decodedDiff > 2 * trackbufferInfo.m_lastFrameDuration) {
                    // If mode equals "segments": Set group end timestamp to presentation timestamp.
                    // TODO If mode equals "sequence": Set group start timestamp equal to the group end timestamp.
                    groupTimestampEnd = presentationTimestamp;

                    unsetAllStreamProcessInfo();
                    m_currentGroupLastTimestamp = -1;
                    continue;
                }
            }

            // 7. Let frame end timestamp equal the sum of presentation timestamp and frame duration.
            uint64_t frameEndTimestamp = presentationTimestamp + frameDuration;

            // 8. If presentation timestamp is less than appendWindowStart, then set the need random access point flag to true, drop the coded frame, and jump to the top of the loop to start processing the next coded frame.
            // 9. If frame end timestamp is greater than appendWindowEnd, then set the need random access point flag to true, drop the coded frame, and jump to the top of the loop to start processing the next coded frame.
            if (presentationTimestamp < m_appendWindowStart || presentationTimestamp > m_appendWindowEnd) {
                trackbufferInfo.m_needRandomAccess = true;
                return ret;
            }

            // 10. If the need random access point flag on track buffer equals true, then run the following steps:
            if (trackbufferInfo.m_needRandomAccess) {
                // TODO If the coded frame is not a random access point, then drop the coded frame
                //      and jump to the top of the loop to start processing the next coded frame.
                // Set the need random access point flag on track buffer to false.
                trackbufferInfo.m_needRandomAccess = false;
            }

            // NOTE: this packet should put to new group
            MediaPacketGroup* group = nullptr;
            if (m_currentGroupLastTimestamp == -1) {
                MediaPacketGroup* newgroup = new MediaPacketGroup(streamIndex, SIZE_MAX, nullptr, frameDuration, decodeTimestamp);
                m_packetGroup.push_back(newgroup);
                group = newgroup;
            } else {
                group = findRecentPacketGroup((size_t) streamIndex);
            }

            MediaPacket* pkt = new MediaPacket();
            pkt->m_pts = presentationTimestamp;
            pkt->m_duration = frameDuration;
            pkt->m_streamIndex = streamIndex;
            pkt->m_dataSize = packet.m_dataSize;
            pkt->m_data = packet.m_data;
            pkt->m_hasIdr = packet.m_hasIdr;
            ret = true;
            memcpy(pkt->m_data, packet.m_data, packet.m_dataSize);
            // STARFISH_LOG_INFO("[%d] pkt data pts %d len %d %d\n",streamIndex, (int)pkt->m_pts, (int)pkt->m_dataSize, (int) m_packetGroup.size());
            group->pushMediaPacket(pkt);

            m_currentGroupLastTimestamp = decodeTimestamp;

            // TODO(?) 11-16. Consider overlapped frames

            // 17-19. Update TrackBufferInfo
            trackbufferInfo.m_lastDecodeTimestamp = decodeTimestamp;
            trackbufferInfo.m_lastFrameDuration = frameDuration;
            if (trackbufferInfo.m_highestEndTimestamp < 0 || frameEndTimestamp > (uint64_t)trackbufferInfo.m_highestEndTimestamp)
                trackbufferInfo.m_highestEndTimestamp = frameEndTimestamp;

            // 20. If frame end timestamp is greater than group end timestamp,
            //     then set group end timestamp equal to frame end timestamp.
            if (frameEndTimestamp > groupTimestampEnd)
                groupTimestampEnd = frameEndTimestamp;
            group->m_groupTimestampEnd = groupTimestampEnd;
            if (presentationTimestamp < group->m_groupTimestampStart)
                group->m_groupTimestampStart = presentationTimestamp;

            // TODO 21. If generate timestamps flag equals true, then set timestampOffset equal to frame end timestamp.
            return ret;
        }
    }

    void setTimestampInfo(double timestampOffset, double appendWindowStart, double appendWindowEnd)
    {
        m_timestampOffset = timestampOffset * 1000;
        m_appendWindowStart = appendWindowStart * 1000;
        m_appendWindowEnd = appendWindowEnd * 1000;
    }

    bool m_isAborted;
    std::vector<VideoStreamInfo> m_detectedVideoStream;
    std::vector<AudioStreamInfo> m_detectedAudioStream;
    std::vector<MediaPacketGroup*> m_packetGroup;
    std::vector<StreamProcessInfo> m_streamProcessInfo;
    int64_t m_currentGroupLastTimestamp;
    double m_timestampOffset;
    double m_appendWindowStart;
    double m_appendWindowEnd;
};

SourceBuffer::SourceBuffer(StarFish* starFish, String* type)
    : EventTarget()
    , m_isAttachedToParent(false)
    , m_starFish(starFish)
    , m_demuxer(nullptr)
    , m_type(type)
    , m_parentMediaSource(nullptr)
    , m_packetGroupMutex(new Mutex())
{
    m_mode = AppendMode::Segments;
    m_updating = false;
    m_indexPerInitSegment = 0;
    m_buffered = nullptr;
    m_timestampOffset = 0;
    m_audioTracks = nullptr;
    m_videoTracks = nullptr;
    m_textTracks = nullptr;
    m_appendWindowStart = 0;
    m_appendWindowEnd = std::numeric_limits<double>::infinity();
    m_groupStartTimestamp = std::numeric_limits<double>::quiet_NaN();
    m_groupEndTimestamp = 0;

    m_demuxer = Demuxer::createDemuxer(m_type);
    m_demuxer->addClient(new DemuxerClientSourceBuffer());

    STARFISH_LOG_INFO("[TRACE_MSE_GC] SourceBuffer::SourceBuffer (%p)\n", this);
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        STARFISH_LOG_INFO("[TRACE_MSE_GC] SourceBuffer::~SourceBuffer (%p)\n", obj);
        SourceBuffer* nr = (SourceBuffer*)obj;
        nr->clearAll();
        g_sourceBufferList.remove(nr);
    }, NULL, NULL, NULL);
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
                STARFISH_LOG_INFO("[TRACE_MSE_GC] SourceBuffer %p-----------------\n", sb);
#endif
                for (size_t i = 0; i < sb->m_packetGroup.size(); i ++) {
                    std::vector<MediaPacket*>& p = sb->m_packetGroup[i]->m_packets;
                    size_t dataSize = 0;
                    for (size_t j = 0; j < p.size(); j ++) {
                        dataSize += p[j]->m_dataSize;
                    }
#ifdef TRACE_MSE_GC_DETAIL
                    STARFISH_LOG_INFO("[TRACE_MSE_GC] packetGroupInfo %p %d->%d %fMB\n",
                        sb->m_packetGroup[i], (int)sb->m_packetGroup[i]->m_groupTimestampStart, (int)sb->m_packetGroup[i]->m_groupTimestampEnd, dataSize  / 1024.f / 1024.f);
#endif
                    totalDataSize += dataSize;
                }
                iter++;
            }
            STARFISH_LOG_INFO("[TRACE_MSE_GC] totalDataSize %fMB\n", totalDataSize / 1024.f / 1024.f);
        });
    }

#endif
}

void SourceBuffer::clearAll()
{
    for (size_t i = 0; i < m_packetGroup.size(); i ++) {
        std::vector<MediaPacket*>& p = m_packetGroup[i]->m_packets;
        for (size_t j = 0; j < p.size(); j ++) {
            delete[] p[j]->m_data;
            delete p[j];
        }
        std::vector<MediaPacket*>().swap(p);
        delete m_packetGroup[i];
    }
    std::vector<MediaPacketGroup*>().swap(m_packetGroup);
}

void SourceBuffer::setUpdating(bool flag, UpdateState state)
{
    STARFISH_ASSERT(m_updating != flag);
    m_updating = flag;

    if (m_parentMediaSource) {
        String* eventName = String::emptyString;
        if (m_updating && state == SourceBuffer::Success)
            eventName = m_parentMediaSource->starFish()->staticStrings()->m_updatestart.localName();
        else if (!m_updating) {
            if (state == SourceBuffer::Success) {
                m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(this, new Event(m_parentMediaSource->starFish()->staticStrings()->m_update.localName()));
                eventName = m_parentMediaSource->starFish()->staticStrings()->m_updateend.localName();
            } else if (state == SourceBuffer::Error)
                eventName = m_parentMediaSource->starFish()->staticStrings()->m_error.localName();
            else if (state == SourceBuffer::Abort)
                eventName = m_parentMediaSource->starFish()->staticStrings()->m_abort.localName();
            else
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else
            STARFISH_RELEASE_ASSERT_NOT_REACHED();

        m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(this, new Event(eventName));

        if (flag == false) {
            // propagate update state to mediaSource now.
            m_parentMediaSource->didSourceBufferUpdated(this);

            // Set m_buffered to nullptr for re-calculation in the future
            m_buffered = nullptr;
        }
    }
}

void SourceBuffer::abort()
{
    // If this object has been removed from the sourceBuffers attribute of the parent media source then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parent MediaSource, when executing 'abort' of SourceBuffer");
    }

    // If the readyState attribute of the parent media source is not in the "open" state then throw an InvalidStateError exception and abort these steps.
    if (m_parentMediaSource->readyState() != MediaSource::Open) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "readyState of parentMediaSource is not open, when executing 'abort' of SourceBuffer");
    }

    // TODO If the range removal algorithm is running, then throw an InvalidStateError exception and abort these steps.
    // we are running removal algorithm in main thread now.

    // If the updating attribute equals true, then run the following steps:
    if (m_updating) {
        // Queue a task to fire a simple event named abort at this SourceBuffer object.
        m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(this, new Event(m_parentMediaSource->starFish()->staticStrings()->m_abort.localName()));
        // Queue a task to fire a simple event named updateend at this SourceBuffer object.
        m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(this, new Event(m_parentMediaSource->starFish()->staticStrings()->m_updateend.localName()));

        m_updating = false;
    }

    DemuxerClientSourceBuffer* cl = (DemuxerClientSourceBuffer*)m_demuxer->client(0);
    cl->m_isAborted = true;

    m_demuxer = Demuxer::createDemuxer(m_type);
    m_demuxer->addClient(new DemuxerClientSourceBuffer());

    // Run the reset parser state algorithm.
    // TODO If the append state equals PARSING_MEDIA_SEGMENT and the input buffer contains some complete coded frames, then run the coded frame processing algorithm until all of these complete coded frames have been processed.
    // TODO Unset the last decode timestamp on all track buffers.
    // TODO Unset the last frame duration on all track buffers.
    // TODO Unset the highest end timestamp on all track buffers.
    // TODO Set the need random access point flag on all track buffers to true.
    // TODO If the mode attribute equals "sequence", then set the group start timestamp to the group end timestamp
    // Remove all bytes from the input buffer.
    // Set append state to WAITING_FOR_SEGMENT.
    m_bufferUnprocessed.clear();
    m_bufferUnprocessed.shrink_to_fit();
    m_bufferHeader.clear();
    m_bufferHeader.shrink_to_fit();

    // Set appendWindowStart to the presentation start time.
    // Set appendWindowEnd to positive Infinity.
    m_appendWindowStart = 0;
    m_appendWindowEnd = std::numeric_limits<double>::infinity();

    m_indexPerInitSegment = 0;
}

void SourceBuffer::appendBuffer(const uint8_t* data, unsigned long length)
{
    // Run the prepare append algorithm.
    prepareAppend();

    // Add data to the end of the input buffer.
    auto d = new SourceBufferData(this, data, length);

    // Set the updating attribute to true.
    // Queue a task to fire a simple event named updatestart at this SourceBuffer object.
    STARFISH_ASSERT(m_updating == false);
    setUpdating(true, UpdateState::Success);

    // Asynchronously run the buffer append algorithm.
    bufferAppend(d);
}

void SourceBuffer::prepareAppend()
{
    // 3.5.4 Prepare Append Algorithm

    // If the SourceBuffer has been removed from the sourceBuffers attribute of the parent media source then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (m_updating) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is now updating");
    }

    // TODO If the HTMLMediaElement.error attribute is not null, then throw an InvalidStateError exception and abort these steps.

    // If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    if (m_parentMediaSource->readyState() == MediaSource::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent media source.
        m_parentMediaSource->setReadyState(MediaSource::Open);
    }

    // Run the coded frame eviction algorithm.
    codedFrameEviction();

    // TODO If the buffer full flag equals true, then throw a QuotaExceededError exception and abort these step.
}

void SourceBuffer::remove(double start, double end)
{
    // If this object has been removed from the sourceBuffers attribute of the parent media source then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (m_updating) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is now updating");
    }

    // If duration equals NaN, then throw a TypeError exception and abort these steps.
    double duration = m_parentMediaSource->duration();
    if (std::isnan(duration)) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "If Duration is NaN, can not execute remove method");
    }

    // If start is negative or greater than duration, then throw a TypeError exception and abort these steps.
    if (start < 0 || start > duration) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "when executing remove, start must be greater than zero and smaller than duration");
    }

    // If end is less than or equal to start or end equals NaN, then throw a TypeError exception and abort these steps.
    if (end <= start || std::isnan(end)) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "when executing remove, end must be greater than start and not NaN");
    }

    // If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    if (parentMediaSource()->readyState() == MediaSource::ReadyState::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent media source.
        parentMediaSource()->setReadyState(MediaSource::ReadyState::Open);
    }

    setUpdating(true, UpdateState::Success);
    {
        Locker<Mutex> lock(*m_packetGroupMutex);
        rangeRemoval(start * 1000, end * 1000);
    }
    setUpdating(false, UpdateState::Success);
}

void SourceBuffer::rangeRemoval(uint64_t startTimestamp, uint64_t endTimestamp, StreamInfo::Type type)
{
    // 3.5.6 Range Removal
    size_t groupIndex = 0;
    while (groupIndex < m_packetGroup.size()) {
        MediaPacketGroup* grp = m_packetGroup[groupIndex];
        // Note : Remove Packets
        //        packet.start < endTimestamp && patcket.end > startTimestamp
        if ((grp->m_streamInfo->m_type & type) && !(startTimestamp >= grp->m_groupTimestampEnd || endTimestamp <= grp->m_groupTimestampStart)) {
            if (startTimestamp <= grp->m_groupTimestampStart && grp->m_groupTimestampEnd <= endTimestamp) {
                // STARFISH_LOG_INFO("SourceBuffer::remove all %d was(%d->%d)\n", (int)groupIndex, (int)grp->m_groupTimestampStart, (int)grp->m_groupTimestampEnd);

                for (size_t i = 0; i < grp->m_packets.size(); i ++) {
                    delete [] grp->m_packets[i]->m_data;
                    delete grp->m_packets[i];
                }

                std::vector<MediaPacket*>().swap(grp->m_packets);
                delete grp;
                m_packetGroup.erase(m_packetGroup.begin() + groupIndex);
                continue;
            } else if (grp->m_groupTimestampStart < startTimestamp && endTimestamp < grp->m_groupTimestampEnd) {
                // remove center
                size_t holeStart = 0;
                size_t holeEnd = grp->m_packets.size();

                for (size_t i = 0; i < grp->m_packets.size(); i ++) {
                    MediaPacket* pkt = grp->m_packets[i];
                    if (pkt->m_pts < startTimestamp) {
                        holeStart = i;
                    } else {
                        break;
                    }
                }

                for (size_t i = grp->m_packets.size(); i > 0; i --) {
                    MediaPacket* pkt = grp->m_packets[i - 1];
                    if ((pkt->m_pts + pkt->m_duration) > endTimestamp) {
                        holeEnd = i;
                    } else {
                        break;
                    }
                }

                // STARFISH_LOG_INFO("SourceBuffer::remove hole %d %d %d\n", (int)groupIndex, (int)holeStart, (int)holeEnd);

                MediaPacketGroup* newGroup = new MediaPacketGroup(grp->m_streamIndex, grp->m_initSegmentIndex, grp->m_streamInfo);
                for (size_t i = holeEnd; i < grp->m_packets.size(); i ++) {
                    newGroup->m_packets.push_back(grp->m_packets[i]);
                }

                if (newGroup->m_packets.size()) {
                    grp->m_packets.erase(grp->m_packets.begin() + holeEnd, grp->m_packets.end());
                    newGroup->m_groupTimestampStart = (*newGroup->m_packets.begin())->m_pts;
                    newGroup->m_groupTimestampEnd = (*(newGroup->m_packets.end() - 1))->m_pts + (*(newGroup->m_packets.end() - 1))->m_duration;
                    m_packetGroup.insert(m_packetGroup.begin() + groupIndex, newGroup);
                    groupIndex++;
                } else {
                    delete newGroup;
                }
                for (size_t i = holeStart; i < holeEnd; i ++) {
                    delete [] grp->m_packets[i]->m_data;
                    delete grp->m_packets[i];
                }
                grp->m_packets.erase(grp->m_packets.begin() + holeStart, grp->m_packets.begin() + holeEnd);

                if (grp->m_packets.size()) {
                    grp->m_groupTimestampStart = (*grp->m_packets.begin())->m_pts;
                    grp->m_groupTimestampEnd = (*(grp->m_packets.end() - 1))->m_pts + (*(grp->m_packets.end() - 1))->m_duration;
                    groupIndex++;
                } else {
                    delete grp;
                    m_packetGroup.erase(std::find(m_packetGroup.begin(), m_packetGroup.end(), grp));
                }
                continue;
            } else {
                // Remove head or tail
                size_t eraseStart = 0 , eraseEnd = grp->m_packets.size();
                if (startTimestamp <= grp->m_groupTimestampStart && endTimestamp < grp->m_groupTimestampEnd) {
                    // Remove head
                    for (eraseEnd = 0; eraseEnd < grp->m_packets.size(); eraseEnd++) {
                        MediaPacket* pkt = grp->m_packets[eraseEnd];
                        // Note : "packet.start < endTimestamp" && patcket.end > startTimestamp
                        if (pkt->m_pts >= endTimestamp) {
                            break;
                        }
                    }
                } else {
                    // Remove tail
                    for (eraseStart = grp->m_packets.size(); eraseStart > 0; eraseStart --) {
                        MediaPacket* pkt = grp->m_packets[eraseStart - 1];
                        // Note : packet.start < endTimestamp && "patcket.end > startTimestamp"
                        if (pkt->m_pts + pkt->m_duration <= startTimestamp) {
                            break;
                        }
                    }
                }

                if (eraseStart < eraseEnd) {
                    // STARFISH_LOG_INFO("SourceBuffer::remove %d %d %d\n", (int)groupIndex, (int)eraseStart, (int)eraseEnd);

                    for (size_t i = eraseStart; i < eraseEnd; i ++) {
                        delete [] grp->m_packets[i]->m_data;
                        delete grp->m_packets[i];
                    }

                    grp->m_packets.erase(grp->m_packets.begin() + eraseStart, grp->m_packets.begin() + eraseEnd);
                    if (grp->m_packets.size()) {
                        grp->m_groupTimestampStart = (*grp->m_packets.begin())->m_pts;
                        grp->m_groupTimestampEnd = (*(grp->m_packets.end() - 1))->m_pts + (*(grp->m_packets.end() - 1))->m_duration;
                        groupIndex++;
                    } else {
                        delete grp;
                        m_packetGroup.erase(m_packetGroup.begin() + groupIndex);
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
}

void SourceBuffer::codedFrameEviction()
{
    // TODO 3.5.14 Coded Frame Eviction Algorithm
}


void SourceBuffer::bufferAppend(SourceBufferData* inputBuffer)
{
    STARFISH_ASSERT(inputBuffer->m_isProcessed == false);

    m_starFish->threadPool()->addWork([](void* data) -> void* {
        SourceBufferData* inputBuffer = (SourceBufferData*)data;
#ifdef STARFISH_ENABLE_TIMER
        Timer timer("[TRACE_MSE_PROFILE] SourceBuffer::bufferAppend");
#endif
        STARFISH_LOG_INFO("SourceBuffer::bufferAppend start (size %d)\n", (int)inputBuffer->m_length);
        DemuxerSourceForSourceBuffer src(inputBuffer, &inputBuffer->m_sourceBuffer->m_bufferUnprocessed);

        int64_t before = src.onSeek(0, DemuxerSource::SeekWhenceCurrent);

        {
#ifdef STARFISH_ENABLE_TIMER
            Timer timer("[TRACE_MSE_PROFILE] SourceBuffer::bufferAppend::findSteramInfo");
#endif
            if (inputBuffer->m_sourceBuffer->m_demuxer->findStreamInfo(&src, inputBuffer->m_sourceBuffer->m_type)) {
                inputBuffer->m_foundInitSegmentHere = true;
                int64_t after = src.onSeek(0, DemuxerSource::SeekWhenceCurrent);
                // copy buffer
                inputBuffer->m_headerBuffer.resize(after - before);
                src.onSeek(before, DemuxerSource::SeekWhenceSet);
                size_t s;
                int error;
                src.onRead(after - before, s, error, (uint8_t*)inputBuffer->m_headerBuffer.data());

                STARFISH_LOG_INFO("SourceBuffer %p detect initSegment(%d->%d)\n", inputBuffer->m_sourceBuffer, (int)before, (int)after);
            }
        }

        {
#ifdef STARFISH_ENABLE_TIMER
            Timer timer("[TRACE_MSE_PROFILE] SourceBuffer::bufferAppend::findStreamPacket");
#endif
            double timestampOffset = inputBuffer->m_sourceBuffer->timestampOffset();
            double appendWindowStart = inputBuffer->m_sourceBuffer->appendWindowStart();
            double appendWindowEnd = inputBuffer->m_sourceBuffer->appendWindowEnd();
            DemuxerClientSourceBuffer* cl = (DemuxerClientSourceBuffer*)inputBuffer->m_sourceBuffer->m_demuxer->client(0);
            cl->setTimestampInfo(timestampOffset, appendWindowStart, appendWindowEnd);
            inputBuffer->m_sourceBuffer->m_demuxer->findStreamPacket(&src);
        }

        inputBuffer->m_sourceBuffer->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data, void* data2) {
            SourceBufferData* inputBuffer = (SourceBufferData*)data;
            DemuxerClientSourceBuffer* cl = (DemuxerClientSourceBuffer*)inputBuffer->m_sourceBuffer->m_demuxer->client(0);
#ifdef STARFISH_ENABLE_TIMER
            Timer timer("[TRACE_MSE_PROFILE] SourceBuffer::bufferAppend::deliverResult");
#endif
            if (cl->m_isAborted) {
                for (size_t i = 0; i < cl->m_packetGroup.size(); i ++) {
                    MediaPacketGroup* grp = cl->m_packetGroup[i];
                    for (size_t j = 0; j < grp->m_packets.size(); j ++) {
                        delete[] grp->m_packets[j]->m_data;
                        delete grp->m_packets[j];
                    }
                    std::vector<MediaPacket*>().swap(grp->m_packets);
                }

                std::vector<VideoStreamInfo>().swap(cl->m_detectedVideoStream);
                std::vector<AudioStreamInfo>().swap(cl->m_detectedAudioStream);
                std::vector<MediaPacketGroup*>().swap(cl->m_packetGroup);
                std::vector<StreamProcessInfo>().swap(cl->m_streamProcessInfo);
                return;
            }
            size_t readPos = (size_t)data2;

            size_t unprocessSizeBefore = inputBuffer->m_sourceBuffer->m_bufferUnprocessed.size();
            size_t eraseEndPos = readPos > unprocessSizeBefore ? unprocessSizeBefore : readPos;
            inputBuffer->m_sourceBuffer->m_bufferUnprocessed.erase(inputBuffer->m_sourceBuffer->m_bufferUnprocessed.begin(), inputBuffer->m_sourceBuffer->m_bufferUnprocessed.begin() + eraseEndPos);

            if (readPos < (inputBuffer->m_length + unprocessSizeBefore)) {
                size_t copyStart = 0;
                size_t copyEnd = inputBuffer->m_length;
                if (readPos > unprocessSizeBefore) {
                    copyStart = readPos - unprocessSizeBefore;
                }

                inputBuffer->m_sourceBuffer->m_bufferUnprocessed.insert(inputBuffer->m_sourceBuffer->m_bufferUnprocessed.end(), inputBuffer->m_data + copyStart, inputBuffer->m_data + copyEnd);
                STARFISH_LOG_INFO("SourceBuffer::bufferAppend got unprocessed (size %d)\n", (int)(copyEnd - copyStart));
            }

            {
                Locker<Mutex> packetGroupLocker(*inputBuffer->m_sourceBuffer->m_packetGroupMutex);

                if (inputBuffer->m_foundInitSegmentHere) {
                    if (inputBuffer->m_sourceBuffer->m_indexPerInitSegment != 0) {
                        // TODO check new init segment information is same with prev init segment
                    }

                    SourceBufferDataVector data;
                    data.insert(data.end(), inputBuffer->m_headerBuffer.begin(), inputBuffer->m_headerBuffer.end());
                    inputBuffer->m_sourceBuffer->m_bufferHeader.push_back(std::move(data));
                    std::vector<uint8_t>().swap(inputBuffer->m_headerBuffer);

                    GCVector<StreamInfo*> streamInfo;
                    if (inputBuffer->m_sourceBuffer->m_indexPerInitSegment == 0) {
                        for (size_t i = 0; i < (cl->m_detectedVideoStream.size() + cl->m_detectedAudioStream.size()); i ++) {
                            inputBuffer->m_sourceBuffer->m_packetAccessCachePerStream.push_back(std::make_pair<size_t, size_t>(SIZE_MAX, SIZE_MAX));
                        }
                    }

                    for (size_t i = 0; i < cl->m_detectedVideoStream.size(); i ++) {
                        StreamInfo* info = new VideoStreamInfo(cl->m_detectedVideoStream[i]);
                        streamInfo.push_back(info);

                    }

                    for (size_t i = 0; i < cl->m_detectedAudioStream.size(); i ++) {
                        StreamInfo* info = new AudioStreamInfo(cl->m_detectedAudioStream[i]);
                        streamInfo.push_back(info);
                    }

                    inputBuffer->m_sourceBuffer->m_streamInfo.push_back(std::move(streamInfo));
                    inputBuffer->m_sourceBuffer->m_indexPerInitSegment++;
                    STARFISH_LOG_INFO("SourceBuffer got init segment : %p, findedVideoStream %d, findedAudioStream %d\n"
                        , inputBuffer->m_sourceBuffer, (int)cl->m_detectedVideoStream.size(), (int)cl->m_detectedAudioStream.size());
                }

                if (inputBuffer->m_sourceBuffer->m_indexPerInitSegment) {
                    for (size_t i = 0; i < cl->m_packetGroup.size(); i ++) {
                        cl->m_packetGroup[i]->m_initSegmentIndex = inputBuffer->m_sourceBuffer->m_indexPerInitSegment - 1;
                        cl->m_packetGroup[i]->m_streamInfo = inputBuffer->m_sourceBuffer->streamInfo(cl->m_packetGroup[i]->m_initSegmentIndex, cl->m_packetGroup[i]->m_streamIndex);

                        inputBuffer->m_sourceBuffer->rangeRemoval(cl->m_packetGroup[i]->m_groupTimestampStart, cl->m_packetGroup[i]->m_groupTimestampEnd, cl->m_packetGroup[i]->m_streamInfo->m_type);

                        STARFISH_LOG_INFO("packetGroupInfo initSegmentIndex%d streamIndex:%d, packetCount: %d(%dms->%dms)\n", (int)cl->m_packetGroup[i]->m_initSegmentIndex
                        , (int)cl->m_packetGroup[i]->m_streamIndex, (int)cl->m_packetGroup[i]->m_packets.size(), (int)cl->m_packetGroup[i]->m_groupTimestampStart, (int)cl->m_packetGroup[i]->m_groupTimestampEnd);
                    }

                    inputBuffer->m_sourceBuffer->m_packetGroup.insert(inputBuffer->m_sourceBuffer->m_packetGroup.end(),
                        cl->m_packetGroup.begin(), cl->m_packetGroup.end());
                }

                std::vector<VideoStreamInfo>().swap(cl->m_detectedVideoStream);
                std::vector<AudioStreamInfo>().swap(cl->m_detectedAudioStream);
                std::vector<MediaPacketGroup*>().swap(cl->m_packetGroup);
                std::vector<StreamProcessInfo>().swap(cl->m_streamProcessInfo);

                inputBuffer->m_isProcessed = true;
            }
            inputBuffer->m_sourceBuffer->setUpdating(false, UpdateState::Success);
        }, inputBuffer, (void*)src.m_readPos);
        return nullptr;
    }, inputBuffer);
}

void SourceBuffer::clearPacketAccessCache()
{
    Locker<Mutex> packetGroupLocker(*m_packetGroupMutex);
    auto iter = m_packetAccessCachePerStream.begin();
    while (iter != m_packetAccessCachePerStream.end()) {
        *iter = std::make_pair<size_t, size_t>(SIZE_MAX, SIZE_MAX);
        iter++;
    }
}

std::pair<MediaPacket*, size_t> SourceBuffer::findProperMediaPacket(size_t streamIdx, uint64_t startPositionInPTSWantToFind)
{
    Locker<Mutex> packetGroupLocker(*m_packetGroupMutex);
    // STARFISH_LOG_INFO("SourceBuffer::findProperMediaPacket %d %d\n", (int)streamIdx, (int)startPositionInPTSWantToFind);

    // test cache first
    {
        auto cache = m_packetAccessCachePerStream[streamIdx];
        if (cache.first < m_packetGroup.size()) {
            MediaPacketGroup* grp = m_packetGroup[cache.first];
            if (grp->m_groupTimestampStart <= startPositionInPTSWantToFind && startPositionInPTSWantToFind <= grp->m_groupTimestampEnd) {
                size_t idx = cache.second + 1;
                if (idx < grp->m_packets.size()) {
                    m_packetAccessCachePerStream[streamIdx] = std::make_pair(cache.first, idx);
                    return std::make_pair(grp->m_packets[idx], grp->m_initSegmentIndex);
                }
            }
        }
    }

    // STARFISH_LOG_INFO("SourceBuffer::findProperMediaPacket cache miss! streamIdx(%d)\n", (int)streamIdx);

    std::pair<size_t, uint64_t> nearestPacketGroupInfo = std::make_pair(SIZE_MAX, std::numeric_limits<uint64_t>::max());

    for (size_t i = 0; i < m_packetGroup.size(); i ++) {
        MediaPacketGroup* grp = m_packetGroup[i];
        if (grp->m_streamIndex == streamIdx) {
            if (grp->m_groupTimestampStart <= startPositionInPTSWantToFind && startPositionInPTSWantToFind <= grp->m_groupTimestampEnd) {
                const std::vector<MediaPacket*>& v = grp->m_packets;
                for (size_t j = 0; j < v.size(); j++) {
                    if (v[j]->m_pts >= startPositionInPTSWantToFind) {
                        m_packetAccessCachePerStream[streamIdx] = std::make_pair(i, j);
                        return std::make_pair(v[j], grp->m_initSegmentIndex);
                    }
                }
            } else if (grp->m_groupTimestampStart > startPositionInPTSWantToFind) {
                if (grp->m_groupTimestampStart < nearestPacketGroupInfo.second) {
                    nearestPacketGroupInfo = std::make_pair(i, grp->m_groupTimestampStart);
                }
            }
        }
    }

    if (nearestPacketGroupInfo.first != SIZE_MAX) {
        m_packetAccessCachePerStream[streamIdx] = std::make_pair(nearestPacketGroupInfo.first, 0);
        return std::make_pair(m_packetGroup[nearestPacketGroupInfo.first]->m_packets[0], m_packetGroup[nearestPacketGroupInfo.first]->m_initSegmentIndex);
    }

    return std::make_pair(nullptr, SIZE_MAX);
}

uint64_t SourceBuffer::lastBufferedTimestamp(size_t streamIdx)
{
    Locker<Mutex> packetGroupLocker(*m_packetGroupMutex);
    uint64_t timestamp = 0;
    for (size_t i = 0; i < m_packetGroup.size(); i ++) {
        MediaPacketGroup* grp = m_packetGroup[i];
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
    // If this object has been removed from the sourceBuffers attribute of the parent media source, then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (m_updating) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is now updating");
    }

    // TODO If generate timestamps flag equals true and new mode equals "segments", then throw a TypeError exception and abort these steps.

    // If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    if (m_parentMediaSource->readyState() == MediaSource::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent media source.
        m_parentMediaSource->setReadyState(MediaSource::Open);
    }

    // TODO If the append state equals PARSING_MEDIA_SEGMENT, then throw an InvalidStateError and abort these steps.
    // if (m_state == ParsingMediaSegment)
    //     throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is currently parsing a media segment");

    // If the new mode equals "sequence", then set the group start timestamp to the group end timestamp.
    if (mode == Sequence)
        m_groupStartTimestamp = m_groupEndTimestamp;

    // Update the attribute to new mode.
    m_mode = mode;
}

void SourceBuffer::setTimestampOffset(double timeoffset)
{
    // If this object has been removed from the sourceBuffers attribute of the parent media source, then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (m_updating) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is now updating");
    }

    // If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    if (m_parentMediaSource->readyState() == MediaSource::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent media source.
        m_parentMediaSource->setReadyState(MediaSource::Open);
    }

    // TODO If the append state equals PARSING_MEDIA_SEGMENT, then throw an InvalidStateError and abort these steps.
    // if (m_state == ParsingMediaSegment)
    //     throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is currently parsing a media segment");

    // If the mode attribute equals "sequence", then set the group start timestamp to new timestamp offset.
    if (m_mode == Sequence)
        m_groupStartTimestamp = timeoffset;

    // Update the attribute to new timestamp offset.
    m_timestampOffset = timeoffset;
}

void SourceBuffer::setAppendWindowStart(double timeStamp)
{
    // If this object has been removed from the sourceBuffers attribute of the parent media source, then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (m_updating) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is now updating");
    }

    // If the new value is less than 0 or greater than or equal to appendWindowEnd then throw a TypeError exception and abort these steps.
    if (timeStamp < 0 || timeStamp >= m_appendWindowEnd) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "appendWindowStart should be between 0 and appendWindowEnd");
    }

    // Update the attribute to the new value.
    m_appendWindowStart = timeStamp;
}

void SourceBuffer::setAppendWindowEnd(double timeStamp)
{
    // If this object has been removed from the sourceBuffers attribute of the parent media source, then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (m_updating) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is now updating");
    }

    // If the new value equals NaN, then throw a TypeError and abort these steps.
    if (std::isnan(timeStamp)) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "appendWindowEnd should not be NaN");
    }

    // If the new value is less than or equal to appendWindowStart then throw a TypeError exception and abort these steps.
    if (timeStamp <= m_appendWindowStart) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "appendWindowEnd should be greater than appendWindowStart");
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
    if (m_streamInfo.size() == 0 || m_packetGroup.size() == 0) {
        return new TimeRanges();
    }

    // https://www.w3.org/TR/media-source/#widl-SourceBuffer-buffered
    // 1. If this object has been removed from the sourceBuffers attribute of the parent media source
    //    then throw an InvalidStateError exception and abort these steps.
    // --> Binding layer would catch that
    if (!parentMediaSource()) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::Code::INVALID_STATE_ERR);
    }

    // 2. Let highest end time be the largest track buffer ranges end time across all the track buffers managed by this SourceBuffer object.
    // +  Collect tracks (Since current version of Starfish does not support videoTracks/audioTracks/textTracks)
    std::unordered_map<size_t, std::map<uint64_t, MediaPacketGroup*>, std::hash<size_t>, std::equal_to<size_t>> tracksForSort;
    uint64_t highestEndTime = 0;
    for (auto i = m_packetGroup.begin(); i != m_packetGroup.end(); i++) {
        MediaPacketGroup* packetGroup = (*i);
        auto itr = tracksForSort.find(packetGroup->m_streamIndex);
        if (itr == tracksForSort.end()) {
            tracksForSort.insert(std::make_pair(packetGroup->m_streamIndex, std::map<uint64_t, MediaPacketGroup*>()));
            itr = tracksForSort.find(packetGroup->m_streamIndex);
        }
        std::map<uint64_t, MediaPacketGroup*>& track = itr->second;
        STARFISH_ASSERT(track.find(packetGroup->m_groupTimestampStart) == track.end());
        track.insert(std::make_pair(packetGroup->m_groupTimestampStart, packetGroup));
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
        std::pair<uint64_t, uint64_t> item = std::make_pair(oldTrack.begin()->first, oldTrack.begin()->second->m_groupTimestampEnd);
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

    // 3. Let intersection ranges equal a TimeRange object containing a single range from 0 to highest end time.
    std::vector<std::pair<uint64_t, uint64_t>> intersection;
    intersection.push_back(std::make_pair(0, highestEndTime));

    // 4. For each track buffer managed by this SourceBuffer, run the following steps:
    for (unsigned i = 0; i < tracks.size(); i++) {
        // 4-1. Let track ranges equal the track buffer ranges for the current track buffer.
        std::vector<std::pair<uint64_t, uint64_t>>& track = tracks[i];
        STARFISH_ASSERT(track.size() != 0);

        // 4-2. If readyState is "ended", then set the end time on the last range in track ranges to highest end time.
        STARFISH_ASSERT(parentMediaSource());
        if (parentMediaSource()->readyState() == MediaSource::Ended) {
            (--track.end())->second = highestEndTime;
        }
        // 4-3. Let new intersection ranges equal the intersection between the intersection ranges and the track ranges.
        std::vector<std::pair<uint64_t, uint64_t>> newIntersection;
        auto t = track.begin();
        auto j = intersection.begin();
        while (t != track.end() && j != intersection.end()) {
            if (t->second < j->first) {
                t++;
            } else if (t->first > j->second) {
                j++;
            } else {
                newIntersection.push_back(std::make_pair(std::max(t->first, j->first), std::min(t->second, j->second)));
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
        // 4-4. Replace the ranges in intersection ranges with the new intersection ranges.
        intersection.clear();
        intersection = newIntersection;
    }

    // 5. If intersection ranges does not contain the exact same range information as the current value of this attribute,
    //    then update the current value of this attribute to intersection ranges.
    m_buffered = new TimeRanges();
    for (auto i = intersection.begin(); i != intersection.end(); i++) {
        m_buffered->push_back((double)(i->first) / 1000.0, (double)(i->second) / 1000.0);
    }

    return m_buffered;
}

StreamInfo* SourceBuffer::streamInfo(size_t initSegmentIndex, size_t streamIndex)
{
    const GCVector<StreamInfo*>& streamInfo = m_streamInfo[initSegmentIndex];
    for (size_t i = 0; i < streamInfo.size(); i ++) {
        if (streamInfo[i]->m_streamIndex == streamIndex) {
            return streamInfo[i];
        }
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void SourceBufferList::scheduleEvent(String* eventName)
{
    m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(this, new Event(eventName));
}

}

#endif
