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

#include <cstring>
#include <map>
#include <set>

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
        m_timeCodeScaleNs = 1000000; // WebM default: 1ms

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO("DemuxerWebM::~DemuxerWebM");
                delete ((DemuxerWebM*)obj)->m_headerSegment;
            },
            NULL, NULL, NULL);
    }

    uint64_t timeCodeToMs(long long tc) const
    {
        // Default WebM scale is 1ms — short-circuit to avoid 64x64 multiply.
        if (m_timeCodeScaleNs == 1000000) {
            return tc < 0 ? 0 : (uint64_t)tc;
        }
        if (tc < 0) {
            return 0;
        }
        return ((uint64_t)tc * (uint64_t)m_timeCodeScaleNs) / 1000000ULL;
    }

    uint64_t defaultDurationMsForTrack(long long trackNum) const
    {
        auto it = m_trackDefaultDurationsMs.find(trackNum);
        if (it != m_trackDefaultDurationsMs.end() && it->second > 0) {
            return it->second;
        }
        // No DefaultDuration on the track; pick a sane fallback. 33ms ≈ 30fps
        // for video and is a reasonable upper bound for audio frame durations.
        return 33;
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

        const long long timeCodeScale = pSegmentInfo->GetTimeCodeScale();
        if (timeCodeScale > 0) {
            m_timeCodeScaleNs = (uint64_t)timeCodeScale;
        }

        const mkvparser::Tracks* pTracks = pSegment->GetTracks();

        unsigned long trackNum = 0;
        const unsigned long numTracks = pTracks->GetTracksCount();

        while (trackNum != numTracks) {
            const mkvparser::Track* const pTrack =
                pTracks->GetTrackByIndex(trackNum++);

            const long trackType = pTrack->GetType();
            const long trackNumber = pTrack->GetNumber();

            const char* codecId = pTrack->GetCodecId();
            size_t codecPrivateSize = 0;
            const unsigned char* codecPrivate =
                pTrack->GetCodecPrivate(codecPrivateSize);

            const unsigned long long defaultDurationNs =
                pTrack->GetDefaultDuration();
            const uint64_t trackDefaultDurationMs =
                defaultDurationNs > 0
                    ? (uint64_t)(defaultDurationNs / 1000000ULL)
                    : 0;

            if (trackType == mkvparser::Track::kVideo) {
                const mkvparser::VideoTrack* const pVideoTrack =
                    static_cast<const mkvparser::VideoTrack*>(pTrack);

                const long long width = pVideoTrack->GetWidth();
                const long long height = pVideoTrack->GetHeight();
                const double rate = pVideoTrack->GetFrameRate();

                MediaCodec codec = MediaCodecUnknown;
                if (codecId) {
                    if (!strcmp(codecId, "V_VP9")) {
                        codec = MediaCodecVideoVP9;
                    } else if (!strcmp(codecId, "V_AV1")) {
                        codec = MediaCodecVideoAV1;
                    }
                }
                if (codec == MediaCodecUnknown) {
                    // Default to VP9 — prior behavior, and the most common WebM
                    // video codec served by MSE producers like YouTube. Log
                    // so that an unrecognized codecId surfaces in failure
                    // triage instead of dead-routing to the wrong decoder.
                    STARFISH_LOG_INFO(
                        "DemuxerWebM: unknown video codecId '%s' on track "
                        "%ld, defaulting to VP9",
                        codecId ? codecId : "(null)", trackNumber);
                    codec = MediaCodecVideoVP9;
                }

                // Cache duration: prefer DefaultDuration, else derive from
                // FrameRate, else 33ms (≈30fps) — see
                // defaultDurationMsForTrack.
                uint64_t durationMs = trackDefaultDurationMs;
                if (durationMs == 0) {
                    if (rate > 0) {
                        durationMs = (uint64_t)(1000.0 / rate + 0.5);
                    }
                    if (durationMs == 0) {
                        durationMs = 33;
                    }
                }
                m_trackDefaultDurationsMs[trackNumber] = durationMs;

                StreamInfo info;
                info.setType(StreamTypeVideo);
                info.setStreamIndex(trackNum - 1);
                info.setCodec(codec);
                info.setVideoWidth(width);
                info.setVideoHeight(height);
                if (rate > 0) {
                    info.setVideoFramerate(
                        Framerate::createFromLL((int64_t)(rate * 1000), 1000));
                    info.setVideoHasFramerate(true);
                }
                if (codecPrivate && codecPrivateSize > 0) {
                    info.m_extraData.assign(codecPrivate,
                                            codecPrivate + codecPrivateSize);
                }
                for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                    m_demuxerClients[j]->onDetectStream(info);
                }
                m_emitTrackNumbers.insert((long long)trackNumber);
            } else if (trackType == mkvparser::Track::kAudio) {
                const mkvparser::AudioTrack* const pAudioTrack =
                    static_cast<const mkvparser::AudioTrack*>(pTrack);

                const long long channels = pAudioTrack->GetChannels();
                const double sampleRate = pAudioTrack->GetSamplingRate();

                MediaCodec codec = MediaCodecUnknown;
                if (codecId) {
                    if (!strcmp(codecId, "A_OPUS")) {
                        codec = MediaCodecAudioOpus;
                    } else if (!strcmp(codecId, "A_VORBIS")) {
                        codec = MediaCodecAudioVorbis;
                    } else if (!strcmp(codecId, "A_AAC")) {
                        codec = MediaCodecAudioAAC;
                    } else if (!strcmp(codecId, "A_MPEG/L3")) {
                        codec = MediaCodecAudioMP3;
                    }
                }
                if (codec == MediaCodecUnknown) {
                    // Fall back to Vorbis to preserve historical behavior.
                    // Log so that an unrecognized codecId surfaces in failure
                    // triage instead of silently routing to the wrong decoder.
                    STARFISH_LOG_INFO(
                        "DemuxerWebM: unknown audio codecId '%s' on track "
                        "%ld, defaulting to Vorbis",
                        codecId ? codecId : "(null)", trackNumber);
                    codec = MediaCodecAudioVorbis;
                }

                // Cache duration: prefer DefaultDuration, else codec-aware
                // estimate. Opus uses fixed 20ms frames in YouTube/WebRTC
                // encodings; AAC and MP3 derive from sample-count / rate.
                // Vorbis frame size is variable so we use a 21ms ballpark
                // close to AAC. See defaultDurationMsForTrack.
                uint64_t durationMs = trackDefaultDurationMs;
                if (durationMs == 0) {
                    if (codec == MediaCodecAudioOpus) {
                        durationMs = 20;
                    } else if (codec == MediaCodecAudioAAC && sampleRate > 0) {
                        durationMs =
                            (uint64_t)(1024.0 * 1000.0 / sampleRate + 0.5);
                    } else if (codec == MediaCodecAudioMP3 && sampleRate > 0) {
                        durationMs =
                            (uint64_t)(1152.0 * 1000.0 / sampleRate + 0.5);
                    } else {
                        durationMs = 21;
                    }
                }
                m_trackDefaultDurationsMs[trackNumber] = durationMs;

                StreamInfo info;
                info.setType(StreamTypeAudio);
                info.setStreamIndex(trackNum - 1);
                info.setCodec(codec);
                info.setAudioChannels((uint16_t)channels);
                info.setAudioSampleRate((uint32_t)sampleRate);
                if (codecPrivate && codecPrivateSize > 0) {
                    info.m_extraData.assign(codecPrivate,
                                            codecPrivate + codecPrivateSize);
                }
                for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                    m_demuxerClients[j]->onDetectStream(info);
                }
                m_emitTrackNumbers.insert((long long)trackNumber);
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

                if (m_emitTrackNumbers.find(trackNum) ==
                    m_emitTrackNumbers.end()) {
                    // Subtitle / metadata / other auxiliary track — share
                    // the Cluster with video/audio but must not be
                    // forwarded as MediaPackets to SourceBuffer.
                    long status2 = pCluster->GetNext(pBlockEntry, pBlockEntry);
                    if (status2 < 0) {
                        delete segment;
                        source->onSeek(
                            lastConsumedSyntheticEnd -
                                (long long)WrappedSegmentReader::kHeaderLen,
                            DemuxerSource::SeekWhenceSet);
                        return true;
                    }
                    continue;
                }

                const int frameCount = pBlock->GetFrameCount();
                // GetTime() walks Segment::GetInfo which is NULL on our
                // per-call synthetic Segment, so we use GetTimeCode() and
                // apply the TimeCodeScale captured during findStreamInfo.
                const long long blockTc = pBlock->GetTimeCode(pCluster);
                const uint64_t blockPtsMs = timeCodeToMs(blockTc);
                const uint64_t trackDurationMs =
                    defaultDurationMsForTrack(trackNum);

                for (int i = 0; i < frameCount; ++i) {
                    const mkvparser::Block::Frame& theFrame =
                        pBlock->GetFrame(i);
                    const long size = theFrame.len;

                    // SourceBuffer::clearAll frees packet bodies with
                    // delete[] (see SourceBuffer.cpp around the
                    // m_packets[j]->m_data delete[] line); allocate with
                    // new uint8_t[] to match — malloc()/delete[] would
                    // be undefined behavior. The MP4 path already uses
                    // new uint8_t[] (MP4PacketGenerator.cpp).
                    uint8_t* dataPtr = new uint8_t[(size_t)size];
                    theFrame.Read(&src, dataPtr);

                    MediaPacket packet;
                    packet.m_data = dataPtr;
                    packet.m_dataSize = size;
                    // For laced blocks, frames within the block share the
                    // block's timecode and advance by trackDurationMs each.
                    packet.m_pts = blockPtsMs + (uint64_t)i * trackDurationMs;
                    // WebM has no DTS reordering within a Cluster, so
                    // DTS == PTS.
                    packet.m_dts = packet.m_pts;
                    packet.m_duration = (size_t)trackDurationMs;
                    packet.m_hasIdr = pBlock->IsKey();

                    for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                        if (m_demuxerClients[j]->onDetectPacket(tn - 1,
                                                                packet)) {
                            dataPtr = nullptr;
                            break;
                        }
                    }
                    // Match new uint8_t[] above; previous free() was paired
                    // with the prior malloc(). delete[] on nullptr is a
                    // no-op when ownership transferred to a client.
                    delete[] dataPtr;
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
    uint64_t m_timeCodeScaleNs;
    std::map<long long, uint64_t> m_trackDefaultDurationsMs;
    // Track numbers that resolved to a video/audio StreamInfo during
    // findStreamInfo. Subtitle, metadata, and other auxiliary tracks
    // share Clusters with video/audio in WebM but must not be forwarded
    // to SourceBuffer as MediaPackets.
    std::set<long long> m_emitTrackNumbers;
};

Demuxer* Demuxer::createWebMDemuxer()
{
    return new DemuxerWebM();
}
} // namespace Starfish

#endif /* STARFISH_ENABLE_MULTIMEDIA */
