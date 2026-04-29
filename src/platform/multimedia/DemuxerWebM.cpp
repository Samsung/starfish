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
#include "StarfishConfig.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/DemuxerSource.h"

#include "../third_party/webm/mkvparser.hpp"

class MkvReaderAdapter : public mkvparser::IMkvReader {
public:
    MkvReaderAdapter(Starfish::DemuxerSource* source)
        : m_source(source)
    {
    }
    virtual int Read(long long pos, long len, unsigned char* buf)
    {
        size_t sizeSuccessToRead = 0;
        size_t sizeWantToToRead = len;
        int errorCode = 0;
        if (m_source->onSeek(pos, Starfish::DemuxerSource::SeekWhenceSet) !=
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
            m_source->onSeek(0, Starfish::DemuxerSource::SeekWhenceLookSize);
        if (total) {
            *total = size;
        }
        if (available) {
            *available = size;
        }
        return 0;
    }

    Starfish::DemuxerSource* m_source;
};

// Wraps a DemuxerSource so that mkvparser sees a synthetic
// "Segment of unknown size" element prepended to the actual buffer.
// MSE delivers media segments as bare Cluster elements (not wrapped in a
// Segment), but mkvparser::Segment::CreateInstance only succeeds if the
// reader starts with a Segment element ID. By prepending the canonical
// header bytes "18 53 80 67 FF" (Segment ID + 1-byte unknown-size VINT),
// we let mkvparser walk the buffer's clusters directly.
class WrappedSegmentReader : public mkvparser::IMkvReader {
public:
    static const long kHeaderLen = 5;
    static const unsigned char kHeader[kHeaderLen];

    WrappedSegmentReader(Starfish::DemuxerSource* source)
        : m_source(source)
    {
    }

    virtual int Read(long long pos, long len, unsigned char* buf)
    {
        long fromHeader = 0;
        if (pos < (long long)kHeaderLen) {
            long avail = (long)((long long)kHeaderLen - pos);
            fromHeader = (avail < len) ? avail : len;
            memcpy(buf, kHeader + pos, fromHeader);
            buf += fromHeader;
            len -= fromHeader;
            pos += fromHeader;
        }
        if (len == 0) {
            return 0;
        }
        long long innerPos = pos - kHeaderLen;
        if (m_source->onSeek(
                innerPos, Starfish::DemuxerSource::SeekWhenceSet) != innerPos) {
            return -1;
        }
        size_t got = 0;
        int err = 0;
        m_source->onRead((size_t)len, got, err, (uint8_t*)buf);
        if (err || (long)got != len) {
            return -1;
        }
        return 0;
    }

    virtual int Length(long long* total, long long* available)
    {
        long long inner = (long long)m_source->onSeek(
            0, Starfish::DemuxerSource::SeekWhenceLookSize);
        if (total) {
            *total = inner + kHeaderLen;
        }
        if (available) {
            *available = inner + kHeaderLen;
        }
        return 0;
    }

    Starfish::DemuxerSource* m_source;
};

const unsigned char
    WrappedSegmentReader::kHeader[WrappedSegmentReader::kHeaderLen] = {
        0x18, 0x53, 0x80, 0x67, 0xFF
    };

namespace Starfish {

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
                STARFISH_LOG_INFO("DemuxerWebM::~DemuxerWebM");
                delete ((DemuxerWebM*)obj)->m_headerSegment;
            },
            NULL, NULL, NULL);
    }

    virtual bool findStreamInfo(DemuxerSource* source, String* formatHint)
    {
        // Reentry: MSE feeds buffers repeatedly; subsequent appends may not
        // contain the EBML/Segment header. Once tracks were detected, treat
        // further calls as a no-op so the SourceBuffer can move on to packet
        // extraction.
        if (m_isStreamFinded) {
            return true;
        }
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

        std::unique_ptr<mkvparser::Segment> pSegment(segment);

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

        // findStreamInfo may have left the source past the consumed init
        // segment bytes (e.g. past Info/Tracks); preserve that position when
        // we have nothing to extract this time.
        long long entryPos =
            (long long)source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
        long long bufferSize =
            (long long)source->onSeek(0, DemuxerSource::SeekWhenceLookSize);

        // MSE feeds bare Cluster elements. Pretend a Segment-of-unknown-size
        // wraps the buffer so mkvparser can stream Clusters out of it.
        WrappedSegmentReader src(source);
        mkvparser::Segment* segment = nullptr;
        long long ignoredPos = 0;
        long long ret =
            mkvparser::Segment::CreateInstance(&src, ignoredPos, segment);
        if (ret) {
            // Should not happen: synthetic header is always a valid Segment.
            source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
            return true;
        }

        // After CreateInstance, segment->m_pos sits past the Segment header
        // (i.e. WrappedSegmentReader::kHeaderLen). LoadCluster scans forward.
        // Track the highest synthetic offset we have FULLY consumed so we can
        // tell tryDemuxing how many bytes of the SourceBuffer to retire.
        // Anything past the last fully-parsed Cluster must remain in
        // m_bufferUnprocessed for the next appendBuffer.
        long long lastConsumedSyntheticEnd =
            (long long)WrappedSegmentReader::kHeaderLen;
        bool sawAnyCluster = false;

        ret = segment->LoadCluster();
        if (ret < 0) {
            // Incomplete data; wait for more in the next appendBuffer.
            delete segment;
            source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
            return true;
        }
        if (ret > 0) {
            // No Cluster element in this buffer (e.g. init segment that only
            // carries EBML/Info/Tracks). Discard it -- everything has been
            // examined and there's nothing of value to keep around.
            delete segment;
            source->onSeek(bufferSize, DemuxerSource::SeekWhenceSet);
            return true;
        }

        const mkvparser::Cluster* pCluster = segment->GetFirst();

        while ((pCluster != NULL) && !pCluster->EOS()) {
            sawAnyCluster = true;
            const mkvparser::BlockEntry* pBlockEntry;
            long status = pCluster->GetFirst(pBlockEntry);

            if (status < 0) { // error
                delete segment;
                source->onSeek(lastConsumedSyntheticEnd -
                                   (long long)WrappedSegmentReader::kHeaderLen,
                               DemuxerSource::SeekWhenceSet);
                return true;
            }

            while ((pBlockEntry != NULL) && !pBlockEntry->EOS()) {
                const mkvparser::Block* const pBlock = pBlockEntry->GetBlock();
                const long long trackNum = pBlock->GetTrackNumber();
                const size_t tn = static_cast<size_t>(trackNum);

                const int frameCount = pBlock->GetFrameCount();
                // GetTime() and GetDiscardPadding() walk into Segment::GetInfo
                // which is NULL on our per-call synthetic Segment. We only
                // need GetTimeCode() (cluster-relative) for packet PTS.

                for (int i = 0; i < frameCount; ++i) {
                    const mkvparser::Block::Frame& theFrame =
                        pBlock->GetFrame(i);
                    const long size = theFrame.len;

                    uint64_t pts = pBlock->GetTimeCode(pCluster);
                    uint8_t* dataPtr = (unsigned char*)malloc((size_t)size);
                    theFrame.Read(&src, dataPtr);

                    MediaPacket packet;
                    packet.m_data = dataPtr;
                    packet.m_dataSize = size;
                    // TimeCode units are cluster TimeCodeScale ticks; in WebM
                    // the default scale is 1ms. WebM has no DTS reordering for
                    // a single Cluster, so DTS == PTS.
                    packet.m_pts = pts;
                    packet.m_dts = pts;
                    // TODO : find duration.
                    packet.m_duration = 33; // temp soluation
                    packet.m_hasIdr = pBlock->IsKey();

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
                    delete segment;
                    source->onSeek(
                        lastConsumedSyntheticEnd -
                            (long long)WrappedSegmentReader::kHeaderLen,
                        DemuxerSource::SeekWhenceSet);
                    return true;
                }
            }

            // This cluster has been fully iterated. Mark its end as consumed.
            long long clusterEnd =
                pCluster->m_element_start + pCluster->GetElementSize();
            if (clusterEnd > lastConsumedSyntheticEnd) {
                lastConsumedSyntheticEnd = clusterEnd;
            }

            ret = segment->LoadCluster();
            if (ret < 0) {
                // Next cluster header is incomplete; stop and leave remaining
                // bytes for the next appendBuffer.
                break;
            }
            pCluster = segment->GetNext(pCluster);
        }

        delete segment;

        long long innerConsumed;
        if (ret < 0) {
            // Partial trailing cluster -- keep its bytes unprocessed.
            innerConsumed = lastConsumedSyntheticEnd -
                            (long long)WrappedSegmentReader::kHeaderLen;
        } else {
            // ret == 1 (or 0 then EOS): mkvparser walked through the rest of
            // the buffer without finding more clusters. Discard everything we
            // examined.
            innerConsumed = bufferSize;
            (void)sawAnyCluster;
        }
        if (innerConsumed < 0) {
            innerConsumed = 0;
        }
        if (innerConsumed > bufferSize) {
            innerConsumed = bufferSize;
        }
        source->onSeek(innerConsumed, DemuxerSource::SeekWhenceSet);
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
} // namespace Starfish

#endif /* STARFISH_ENABLE_MULTIMEDIA */
