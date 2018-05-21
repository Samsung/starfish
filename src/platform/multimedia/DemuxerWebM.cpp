/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
// Copyright (c) 2010 The WebM project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
//
// This sample application demonstrates how to use the Matroska parser
// library, which allows clients to handle a Matroska format file.

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "StarFishConfig.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/DemuxerSource.h"

#include "../third_party/webm/mkvparser.hpp"

class MkvReaderAdapter : public mkvparser::IMkvReader {
public:
    MkvReaderAdapter(StarFish::DemuxerSource* source)
        : m_source(source)
    {
    }
    virtual int Read(long long pos, long len, unsigned char* buf)
    {
        size_t sizeSuccessToRead = 0;
        size_t sizeWantToToRead = len;
        int errorCode = 0;
        if (m_source->onSeek(pos, StarFish::DemuxerSource::SeekWhenceSet) !=
            pos) {
            return -1;
        }
        m_source->onRead(sizeWantToToRead, sizeSuccessToRead, errorCode,
                         (uint8_t*)buf);
        if (errorCode || (sizeWantToToRead != sizeSuccessToRead)) {
            return -1;
        }
        return 0;
    }

    virtual int Length(long long* total, long long* available)
    {
        size_t size =
            m_source->onSeek(0, StarFish::DemuxerSource::SeekWhenceLookSize);
        if (total) {
            *total = size;
        }
        if (available) {
            *available = size;
        }
        return 0;
    }

    StarFish::DemuxerSource* m_source;
};

namespace StarFish {

class DemuxerWebM : public Demuxer {
public:
    DemuxerWebM()
        : Demuxer()
    {
        m_isStreamFinded = false;
        m_headerSegment = nullptr;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO("DemuxerWebM::~DemuxerWebM\n");
                delete ((DemuxerWebM*)obj)->m_headerSegment;
            },
            NULL, NULL, NULL);
    }

    virtual bool findStreamInfo(DemuxerSource* source, String* formatHint)
    {
        STARFISH_ASSERT(!m_isStreamFinded);
        mkvparser::EBMLHeader ebmlHeader;
        long long pos = 0;
        MkvReaderAdapter src(source);
        long long ret = ebmlHeader.Parse(&src, pos);
        if (ret < 0) {
            return false;
        }

        mkvparser::Segment* segment;
        ret = mkvparser::Segment::CreateInstance(&src, pos, segment);
        if (ret) {
            // STARFISH_LOG_INFO("\n Segment::CreateInstance() failed.");
            return false;
        }

        std::auto_ptr<mkvparser::Segment> pSegment(segment);

        ret = pSegment->ParseHeaders();
        if (ret < 0) {
            // STARFISH_LOG_INFO("\n Segment::Load() failed.");
            return false;
        }

        const mkvparser::SegmentInfo* const pSegmentInfo = pSegment->GetInfo();
        if (pSegmentInfo == NULL) {
            // STARFISH_LOG_INFO("\n Segment::GetInfo() failed.");
            return false;
        }

        // const long long timeCodeScale = pSegmentInfo->GetTimeCodeScale();
        // const long long duration_ns = pSegmentInfo->GetDuration();
        // const double duration_sec = double(duration_ns) / 1000000000;

        const mkvparser::Tracks* pTracks = pSegment->GetTracks();

        unsigned long trackNum = 0;
        const unsigned long numTracks = pTracks->GetTracksCount();

        while (trackNum != numTracks) {
            const mkvparser::Track* const pTrack =
                pTracks->GetTrackByIndex(trackNum++);

            const long trackType = pTrack->GetType();
            const long trackNumber = pTrack->GetNumber();

            if (trackType == mkvparser::Track::kVideo) {
                const mkvparser::VideoTrack* const pVideoTrack =
                    static_cast<const mkvparser::VideoTrack*>(pTrack);

                const long long width = pVideoTrack->GetWidth();
                const long long height = pVideoTrack->GetHeight();
                const double rate = pVideoTrack->GetFrameRate();

                StreamInfo info;
                info.setType(StreamTypeVideo);
                info.setStreamIndex(trackNum - 1);
                // TODO read codec
                info.setCodec(MediaCodecVideoVP9);
                info.setVideoWidth(width);
                info.setVideoHeight(height);
                for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                    m_demuxerClients[j]->onDetectStream(info);
                }
            } else if (trackType == mkvparser::Track::kAudio) {
                const mkvparser::AudioTrack* const pAudioTrack =
                    static_cast<const mkvparser::AudioTrack*>(pTrack);

                const long long channels = pAudioTrack->GetChannels();
                const long long bitDepth = pAudioTrack->GetBitDepth();
                const double sampleRate = pAudioTrack->GetSamplingRate();
                const long long codecDelay = pAudioTrack->GetCodecDelay();
                const long long seekPreRoll = pAudioTrack->GetSeekPreRoll();
                StreamInfo info;
                info.setType(StreamTypeAudio);
                info.setStreamIndex(trackNum - 1);
                // TODO read codec
                info.setCodec(MediaCodecAudioVorbis);
                for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                    m_demuxerClients[j]->onDetectStream(info);
                }
            }

            if (pTrack == NULL) {
                continue;
            }
        }

        m_isStreamFinded = true;
        m_headerSegment = pSegment.release();

        if (source->onSeek(pSegmentInfo->m_start + pSegmentInfo->m_size,
                           DemuxerSource::SeekWhence::SeekWhenceSet) != pos) {
            // return false;
        }

        return true;
    }

    virtual bool findStreamPacket(DemuxerSource* source)
    {
        STARFISH_ASSERT(m_isStreamFinded);
        MkvReaderAdapter src(source);
        mkvparser::Segment* segment = nullptr;
        long long pos = source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
        long long ret = mkvparser::Segment::CreateInstance(&src, pos, segment);
        if (ret) {
            return false;
        }

        ret = segment->LoadCluster();
        if (ret < 0) {
            // STARFISH_LOG_INFO("\n Segment::LoadCluster() failed.");
            return false;
        }

        const unsigned long clusterCount = segment->GetCount();
        const mkvparser::Cluster* pCluster = segment->GetFirst();

        while ((pCluster != NULL) && !pCluster->EOS()) {
            const long long timeCode = pCluster->GetTimeCode();
            // STARFISH_LOG_INFO("\t\tCluster Time Code\t: %lld\n", timeCode);

            const long long time_ns = pCluster->GetTime();
            // STARFISH_LOG_INFO("\t\tCluster Time (ns)\t: %lld\n", time_ns);

            const mkvparser::BlockEntry* pBlockEntry;

            long status = pCluster->GetFirst(pBlockEntry);

            if (status < 0) { // error
                // STARFISH_LOG_INFO("\t\tError parsing first block of
                // cluster\n");
                return false;
            }

            while ((pBlockEntry != NULL) && !pBlockEntry->EOS()) {
                const mkvparser::Block* const pBlock = pBlockEntry->GetBlock();
                const long long trackNum = pBlock->GetTrackNumber();
                const size_t tn = static_cast<size_t>(trackNum);

                const int frameCount = pBlock->GetFrameCount();
                const long long time_ns = pBlock->GetTime(pCluster);
                const long long discard_padding = pBlock->GetDiscardPadding();

                for (int i = 0; i < frameCount; ++i) {
                    const mkvparser::Block::Frame& theFrame =
                        pBlock->GetFrame(i);
                    const long size = theFrame.len;
                    const long long offset = theFrame.pos;
                    // STARFISH_LOG_INFO("\t\t\t %15ld,%15llx\n", size, offset);

                    uint64_t pts = pBlock->GetTimeCode(pCluster);
                    uint8_t* dataPtr = (unsigned char*)malloc((size_t)size);
                    theFrame.Read(&src, dataPtr);

                    MediaPacket packet;
                    packet.m_data = dataPtr;
                    packet.m_dataSize = size;
                    packet.m_pts = pts;
                    // TODO : find duration.
                    packet.m_duration = 33; // temp soluation

                    for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                        if (m_demuxerClients[j]->onDetectPacket(tn - 1,
                                                                packet)) {
                            dataPtr = nullptr;
                            break;
                        }
                    }
                    free(dataPtr);
                }

                status = pCluster->GetNext(pBlockEntry, pBlockEntry);

                if (status < 0) {
                    // STARFISH_LOG_INFO("\t\t\tError parsing next block of
                    // cluster\n");
                    // fflush(stdout);
                    return false;
                }
            }
            ret = segment->LoadCluster();
            // if (ret < 0) {
            //     return false;
            // }
            pCluster = segment->GetNext(pCluster);
        }

        return true;
    }

    virtual bool isFindedStreamInfo()
    {
        return m_isStreamFinded;
    }

    mkvparser::Segment* m_headerSegment;
    bool m_isStreamFinded;
};

Demuxer* Demuxer::createWebMDemuxer()
{
    return new DemuxerWebM();
}
}

#endif /* STARFISH_ENABLE_MULTIMEDIA */
