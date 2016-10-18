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
#include "MediaSource.h"

namespace StarFish {

class DemuxerSourceForSourceBuffer : public DemuxerSource {
public:
    DemuxerSourceForSourceBuffer(SourceBufferData* inputBuffer, std::vector<uint8_t, gc_allocator<uint8_t>>* bufferRemain)
        : m_inputBuffer(inputBuffer)
        , m_bufferRemain(bufferRemain)
        , m_readPos(0)
    {

    }

    virtual int64_t onSeek(int64_t position, SeekWhence whence)
    {
        if (whence == DemuxerSource::SeekWhenceLookSize) {
            return m_bufferRemain->size() + m_inputBuffer->m_length;
        } else if (whence == DemuxerSource::SeekWhenceSet) {
            // STARFISH_LOG_INFO("onSeek DemuxerSource::SeekWhenceSet %d\n", (int)position);
            STARFISH_ASSERT(position >= 0);
            STARFISH_ASSERT((int64_t)position <= (int64_t)(m_bufferRemain->size() + m_inputBuffer->m_length));
            m_readPos = position;
            return m_readPos;
        } else if (whence == DemuxerSource::SeekWhenceCurrent) {
            m_readPos = m_readPos + position;
            STARFISH_ASSERT(m_readPos >= 0);
            STARFISH_ASSERT(m_readPos <= (m_bufferRemain->size() + m_inputBuffer->m_length));
            return m_readPos;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead, int& error, uint8_t* buffer)
    {
        sizeSuccessToRead = 0;
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
        // STARFISH_LOG_INFO("onRead pos %d readed %d\n", (int)(m_readPos - sizeSuccessToRead), (int)sizeSuccessToRead);
    }
    SourceBufferData* m_inputBuffer;
    std::vector<uint8_t, gc_allocator<uint8_t>>* m_bufferRemain;
    size_t m_readPos;
};

class DemuxerClientSourceBuffer : public DemuxerClient {
public:
    DemuxerClientSourceBuffer()
    {

    }

    virtual void onDetectVideoStream(const VideoStreamInfo& info)
    {
        m_detectedVideoSteam.push_back(info);
    }

    virtual void onDetectAudioStream(const AudioStreamInfo& info)
    {
        m_detectedAudioSteam.push_back(info);
    }

    virtual void onDetectPacket(const MediaPacket& packet)
    {
        MediaPacketGroup* group = nullptr;
        for (size_t i = 0; i < m_packetGroup.size(); i ++) {
            if (m_packetGroup[i]->m_streamIndex == packet.m_streamIndex) {
                group = m_packetGroup[i];
                break;
            }
        }

        if (group == nullptr) {
            group = new MediaPacketGroup();
            group->m_groupTimestampStart = std::numeric_limits<uint64_t>::max();
            group->m_groupTimestampEnd = 0;
            group->m_streamIndex = packet.m_streamIndex;
            m_packetGroup.push_back(group);
        }

        MediaPacket* pkt = new MediaPacket();
        pkt->m_pts = packet.m_pts;
        pkt->m_streamIndex = packet.m_streamIndex;
        pkt->m_dataSize = packet.m_dataSize;
        pkt->m_data = new uint8_t[packet.m_dataSize];
        memcpy(pkt->m_data, packet.m_data, packet.m_dataSize);
        group->m_packets.push_back(pkt);

        if (pkt->m_pts < group->m_groupTimestampStart) {
            group->m_groupTimestampStart = pkt->m_pts;
        }
        if (pkt->m_pts > group->m_groupTimestampEnd) {
            group->m_groupTimestampEnd = pkt->m_pts;
        }
    }

    std::vector<VideoStreamInfo> m_detectedVideoSteam;
    std::vector<AudioStreamInfo> m_detectedAudioSteam;
    std::vector<MediaPacketGroup*> m_packetGroup;
};

SourceBuffer::SourceBuffer(StarFish* starFish, String* type)
    : EventTarget()
    , m_mode(AppendMode::Segments)
    , m_state(AppendState::ParsingInitSegment)
    , m_isAttachedToParent(false)
    , m_updating(false)
    , m_starFish(starFish)
    , m_demuxer(Demuxer::createDemuxer(type))
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
    , m_sourceBufferUpdateThread(nullptr)
{
    m_demuxer->addClient(new DemuxerClientSourceBuffer());

    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        STARFISH_LOG_INFO("SourceBuffer::~SourceBuffer %p\n", obj);
        SourceBuffer* nr = (SourceBuffer*)obj;
        for (size_t i = 0; i < nr->m_packetGroup.size(); i ++) {
            std::vector<MediaPacket*>& p = nr->m_packetGroup[i]->m_packets;
            for (size_t j = 0; j < p.size(); j ++) {
                delete[] p[j]->m_data;
                delete p[j];
            }
            std::vector<MediaPacket*>().swap(p);
        }
        std::vector<MediaPacketGroup*>().swap(nr->m_packetGroup);

    }, NULL, NULL, NULL);
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
                m_parentMediaSource->starFish()->messageLoop()->addIdler([](size_t, void* data, void* data2) {
                    ((MediaSource*)data)->dispatchEvent((Event*)data2);
                }, this, new Event(m_parentMediaSource->starFish()->staticStrings()->m_update.localName()));
                eventName = m_parentMediaSource->starFish()->staticStrings()->m_updateend.localName();
            } else if (state == SourceBuffer::Error)
                eventName = m_parentMediaSource->starFish()->staticStrings()->m_error.localName();
            else if (state == SourceBuffer::Abort)
                eventName = m_parentMediaSource->starFish()->staticStrings()->m_abort.localName();
            else
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else
            STARFISH_RELEASE_ASSERT_NOT_REACHED();

        m_parentMediaSource->starFish()->messageLoop()->addIdler([](size_t, void* data, void* data2) {
            ((MediaSource*)data)->dispatchEvent((Event*)data2);
        }, this, new Event(eventName));

        if (flag == false) {
            // propagate update state to mediaSource now.
            m_parentMediaSource->didSourceBufferUpdated(this);
        }
    }
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

void SourceBuffer::codedFrameEviction()
{
    // TODO 3.5.14 Coded Frame Eviction Algorithm
}


void SourceBuffer::bufferAppend(SourceBufferData* inputBuffer)
{
    STARFISH_ASSERT(inputBuffer->m_isProcessed == false);
    STARFISH_ASSERT(m_sourceBufferUpdateThread == nullptr);

    m_sourceBufferUpdateThread = new Thread();
    m_sourceBufferUpdateThread->run(m_starFish->messageLoop(), [](void* data) -> void* {
        SourceBufferData* inputBuffer = (SourceBufferData*)data;

        DemuxerSourceForSourceBuffer src(inputBuffer, &inputBuffer->m_sourceBuffer->m_bufferUnprocessed);
        if (inputBuffer->m_sourceBuffer->m_state == AppendState::ParsingInitSegment) {
            int64_t before = src.onSeek(0, DemuxerSource::SeekWhenceCurrent);
            if (inputBuffer->m_sourceBuffer->m_demuxer->findStreamInfo(&src, inputBuffer->m_sourceBuffer->m_type)) {
                int64_t after = src.onSeek(0, DemuxerSource::SeekWhenceCurrent);

                // copy buffer
                inputBuffer->m_headerBuffer.resize(after - before);
                src.onSeek(before, DemuxerSource::SeekWhenceSet);
                size_t s;
                int error;
                src.onRead(after - before, s, error, (uint8_t*)inputBuffer->m_headerBuffer.data());

                inputBuffer->m_sourceBuffer->m_state = AppendState::ParsingMediaSegment;
                STARFISH_LOG_INFO("SourceBuffer %p detect mediaSegment\n", inputBuffer->m_sourceBuffer);
            } else {
                src.onSeek(before, DemuxerSource::SeekWhenceSet);
                STARFISH_LOG_INFO("SourceBuffer %p failed detect mediaSegment\n", inputBuffer->m_sourceBuffer);
            }
        }

        if (inputBuffer->m_sourceBuffer->m_state == AppendState::ParsingMediaSegment) {
            inputBuffer->m_sourceBuffer->m_demuxer->findStreamPacket(&src);
        }

        inputBuffer->m_sourceBuffer->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data, void* data2) {
            SourceBufferData* inputBuffer = (SourceBufferData*)data;
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
            }

            if (inputBuffer->m_headerBuffer.size()) {
                inputBuffer->m_sourceBuffer->m_bufferHeader.insert(inputBuffer->m_sourceBuffer->m_bufferHeader.end(), inputBuffer->m_headerBuffer.begin(), inputBuffer->m_headerBuffer.end());
                std::vector<uint8_t>().swap(inputBuffer->m_headerBuffer);
            }


            DemuxerClientSourceBuffer* cl = (DemuxerClientSourceBuffer*)inputBuffer->m_sourceBuffer->m_demuxer->client(0);

            for (size_t i = 0; i < cl->m_detectedVideoSteam.size(); i ++) {
                StreamInfo* info = new VideoStreamInfo(cl->m_detectedVideoSteam[i]);
                inputBuffer->m_sourceBuffer->m_streamInfo.push_back(info);
            }

            for (size_t i = 0; i < cl->m_detectedAudioSteam.size(); i ++) {
                StreamInfo* info = new AudioStreamInfo(cl->m_detectedAudioSteam[i]);
                inputBuffer->m_sourceBuffer->m_streamInfo.push_back(info);
            }

            inputBuffer->m_sourceBuffer->m_packetGroup.insert(inputBuffer->m_sourceBuffer->m_packetGroup.end(),
                cl->m_packetGroup.begin(), cl->m_packetGroup.end());

            STARFISH_LOG_INFO("SourceBuffer update end : %p, findedVideoStream %d, findedAudioStream %d\n"
                , inputBuffer->m_sourceBuffer, (int)cl->m_detectedVideoSteam.size(), (int)cl->m_detectedAudioSteam.size());

            STARFISH_LOG_INFO("SourceBuffer update end packetGroupInfo\n");
            for (size_t i = 0; i < cl->m_packetGroup.size(); i ++) {
                STARFISH_LOG_INFO("packetGroupInfo streamIndex:%d, packetCount: %d(%dms->%dms)\n"
                , (int)cl->m_packetGroup[i]->m_streamIndex, (int)cl->m_packetGroup[i]->m_packets.size(), (int)cl->m_packetGroup[i]->m_groupTimestampStart, (int)cl->m_packetGroup[i]->m_groupTimestampEnd);
            }

            std::vector<VideoStreamInfo>().swap(cl->m_detectedVideoSteam);
            std::vector<AudioStreamInfo>().swap(cl->m_detectedAudioSteam);
            std::vector<MediaPacketGroup*>().swap(cl->m_packetGroup);

            inputBuffer->m_sourceBuffer->m_sourceBufferUpdateThread = nullptr;
            inputBuffer->m_isProcessed = true;
            inputBuffer->m_sourceBuffer->setUpdating(false, UpdateState::Success);
        }, inputBuffer, (void*)src.m_readPos);
        return nullptr;
    }, inputBuffer);
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

    // If the append state equals PARSING_MEDIA_SEGMENT, then throw an InvalidStateError and abort these steps.
    if (m_state == ParsingMediaSegment)
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is currently parsing a media segment");

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

    // If the append state equals PARSING_MEDIA_SEGMENT, then throw an InvalidStateError and abort these steps.
    if (m_state == ParsingMediaSegment)
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is currently parsing a media segment");

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

void SourceBufferList::scheduleEvent(String* eventName)
{
    m_starFish->messageLoop()->addIdler([](size_t, void* data, void* data2) {
        ((SourceBufferList*)data)->dispatchEvent((Event*)data2);
    }, this, new Event(eventName));
}

}

#endif
