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

#include <climits>
#include <cstring>
#include <map>
#include <set>
#include <vector>

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
    static const long kCacheSize = 16 * 1024;
    static const long kBypassLen = kCacheSize / 2;

    // source: the wrapped DemuxerSource (immutable contents for the
    //         lifetime of this reader -- one findStreamPacket call).
    // cache:  demuxer-owned grow-only scratch (DemuxerWebM::m_readCache);
    //         storage is reused across calls, window state is not.
    WrappedSegmentReader(Starfish::DemuxerSource* source,
                         std::vector<uint8_t>* cache)
        : m_source(source)
        , m_cache(cache)
        , m_innerTotal((long long)source->onSeek(
              0, Starfish::DemuxerSource::SeekWhenceLookSize))
        , m_windowOffset(0)
        , m_windowLen(0)
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

        // Large reads (video frames via Frame::Read, the laced-block span
        // bulk read) go straight to the source: caching them would cost an
        // extra memcpy and evict the header lookahead. The window stays
        // valid -- it indexes immutable data by absolute offset.
        if (len >= kBypassLen) {
            return readDirect(innerPos, len, buf);
        }

        // Window hit: serve mkvparser's tiny header/VINT reads by memcpy.
        if (innerPos >= m_windowOffset &&
            innerPos + (long long)len <=
                m_windowOffset + (long long)m_windowLen) {
            memcpy(buf, m_cache->data() + (size_t)(innerPos - m_windowOffset),
                   (size_t)len);
            return 0;
        }

        // Miss (also covers backward reads and requests spanning the
        // window end): refill at innerPos. onRead flags an error whenever
        // got != want, so request exactly what the source can provide.
        long long availAfter = m_innerTotal - innerPos;
        if (availAfter < (long long)len) {
            return -1;
        }
        long want = (availAfter < (long long)kCacheSize) ? (long)availAfter
                                                         : kCacheSize;
        if (m_cache->size() < (size_t)kCacheSize) {
            m_cache->resize((size_t)kCacheSize);
        }
        if (m_source->onSeek(
                innerPos, Starfish::DemuxerSource::SeekWhenceSet) != innerPos) {
            return -1;
        }
        size_t got = 0;
        int err = 0;
        m_source->onRead((size_t)want, got, err, m_cache->data());
        if (err || (long)got != want) {
            m_windowLen = 0;
            return -1;
        }
        m_windowOffset = innerPos;
        m_windowLen = got;
        memcpy(buf, m_cache->data(), (size_t)len);
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

private:
    int readDirect(long long innerPos, long len, unsigned char* buf)
    {
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

    std::vector<uint8_t>* m_cache;
    long long m_innerTotal;
    long long m_windowOffset;
    size_t m_windowLen;
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
                std::vector<uint8_t>().swap(((DemuxerWebM*)obj)->m_blockBuffer);
                std::vector<uint8_t>().swap(((DemuxerWebM*)obj)->m_readCache);
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

    // Scans [entryPos, entryPos+bufferSize) for the next top-level WebM
    // element start: a Cluster id (1F 43 B6 75) or an EBML id (1A 45 DF
    // A3). Returns the absolute offset of the earliest match, entryPos if
    // the buffer already starts on one, or -1 if none is present. Restores
    // the source position before returning.
    long long scanForElementStart(DemuxerSource* source, long long entryPos,
                                  long long bufferSize)
    {
        static const uint8_t kCluster[4] = { 0x1F, 0x43, 0xB6, 0x75 };
        static const uint8_t kEbml[4] = { 0x1A, 0x45, 0xDF, 0xA3 };
        const long long kChunk = 65536;
        // Overlap by 3 so a pattern straddling a chunk boundary is not
        // missed.
        std::vector<uint8_t> buf(kChunk + 3);
        long long found = -1;
        for (long long pos = entryPos; pos < entryPos + bufferSize;
             pos += kChunk) {
            long long want = std::min(kChunk + 3, entryPos + bufferSize - pos);
            source->onSeek(pos, DemuxerSource::SeekWhenceSet);
            size_t got = 0;
            int err = 0;
            source->onRead((size_t)want, got, err, buf.data());
            if (got < 4) {
                break;
            }
            for (size_t i = 0; i + 4 <= got; i++) {
                if ((buf[i] == kCluster[0] && buf[i + 1] == kCluster[1] &&
                     buf[i + 2] == kCluster[2] && buf[i + 3] == kCluster[3]) ||
                    (buf[i] == kEbml[0] && buf[i + 1] == kEbml[1] &&
                     buf[i + 2] == kEbml[2] && buf[i + 3] == kEbml[3])) {
                    found = pos + (long long)i;
                    break;
                }
            }
            if (found >= 0) {
                break;
            }
        }
        source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
        return found;
    }

    // Checks that entryPos plausibly starts a top-level WebM element: the
    // full 4-byte element id must be one of the known top-level ids (a
    // single head-byte range check passes on truncated-Cluster garbage
    // whose first byte merely happens to be 0x11-0x1F), and a Cluster with
    // a known declared size must declare something sane. A bogus
    // multi-megabyte size makes mkvparser return "incomplete" forever
    // while appends pile up (observed after a seek: LoadCluster incomplete
    // with the unprocessed buffer growing 65KB -> 2.8MB, playback frozen).
    // Restores the source position before returning. Returns true when the
    // buffer is too short to judge (wait for more data).
    bool looksLikeTopLevelElement(DemuxerSource* source, long long entryPos)
    {
        uint8_t buf[12];
        size_t got = 0;
        int err = 0;
        source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
        source->onRead(sizeof(buf), got, err, buf);
        source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
        if (got < 1) {
            return true;
        }
        if (buf[0] == 0xEC) { // Void
            return true;
        }
        if (got < 5) {
            // Not enough bytes for id + first size byte; cannot judge yet.
            return true;
        }
        uint32_t id = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
                      ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
        static const uint32_t kTopLevelIds[] = {
            0x1A45DFA3, // EBML
            0x18538067, // Segment
            0x114D9B74, // SeekHead
            0x1549A966, // Info
            0x1654AE6B, // Tracks
            0x1F43B675, // Cluster
            0x1C53BB6B, // Cues
            0x1043A770, // Chapters
            0x1254C367, // Tags
            0x1941A469, // Attachments
        };
        bool known = false;
        for (size_t i = 0; i < sizeof(kTopLevelIds) / sizeof(uint32_t); i++) {
            if (id == kTopLevelIds[i]) {
                known = true;
                break;
            }
        }
        if (!known) {
            return false;
        }
        if (id != 0x1F43B675) {
            // Size sanity only makes sense for Clusters: a Segment
            // legitimately declares the whole remaining file.
            return true;
        }
        // Parse the EBML vint size that follows the id and sanity-check it.
        uint8_t first = buf[4];
        if (first == 0) {
            return false; // invalid vint (>8 byte length marker)
        }
        int extra = 0;
        uint8_t mask = 0x80;
        while (!(first & mask)) {
            mask >>= 1;
            extra++;
        }
        if (got < (size_t)(5 + extra)) {
            return true; // size field still incomplete; wait
        }
        uint64_t value = first & (uint64_t)(mask - 1);
        bool allOnes = (first & (uint8_t)(mask - 1)) == (uint8_t)(mask - 1);
        for (int i = 0; i < extra; i++) {
            value = (value << 8) | buf[5 + i];
            allOnes = allOnes && buf[5 + i] == 0xFF;
        }
        if (allOnes) {
            return true; // unknown-size element (streaming Cluster)
        }
        // One MSE-appended media segment is a handful of seconds; even a
        // high-bitrate Cluster stays well under this.
        const uint64_t kMaxSaneElementSize = 64ull * 1024 * 1024;
        return value <= kMaxSaneElementSize;
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

        // Resync guard. A forward seek without a preceding abort() (the
        // YouTube player does this) leaves a partial Cluster from the old
        // position in m_bufferUnprocessed; the newly appended segment is
        // concatenated after it, so the buffer now starts mid-element and
        // mkvparser never finds a Cluster (LoadCluster loops on
        // no-cluster/incomplete while data piles up to megabytes). A
        // top-level WebM element always starts with a 0x1X id byte
        // (EBML 0x1A, Segment 0x18, SeekHead/Info/Tracks/Cluster 0x11-0x1F)
        // or Void 0xEC; anything else means the buffer head is stale
        // garbage. Scan forward for the next Cluster (1F 43 B6 75) or EBML
        // (1A 45 DF A3) start and drop everything before it.
        {
            uint8_t head = 0;
            size_t got = 0;
            int err = 0;
            source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
            source->onRead(1, got, err, &head);
            source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
            bool validStart =
                got == 1 && looksLikeTopLevelElement(source, entryPos);
            if (!validStart && bufferSize > 8) {
                long long resyncPos =
                    scanForElementStart(source, entryPos, bufferSize);
                if (resyncPos > entryPos) {
                    STARFISH_LOG_INFO(
                        "[WebM] resync: dropped %lld stale bytes (head was "
                        "0x%02x, bufferSize=%lld)",
                        resyncPos - entryPos, head, bufferSize);
                    source->onSeek(resyncPos, DemuxerSource::SeekWhenceSet);
                    entryPos = resyncPos;
                } else if (resyncPos < 0) {
                    // No element start anywhere in the buffer yet: it is all
                    // stale trailing bytes. Discard so it stops piling up;
                    // the next appendBuffer brings fresh data.
                    STARFISH_LOG_INFO(
                        "[WebM] resync: no element start in %lld bytes "
                        "(head 0x%02x), discarding",
                        bufferSize, head);
                    source->onSeek(bufferSize, DemuxerSource::SeekWhenceSet);
                    return true;
                }
            }
        }

        // MSE feeds bare Cluster elements. Pretend a Segment-of-unknown-size
        // wraps the buffer so mkvparser can stream Clusters out of it.
        WrappedSegmentReader src(source, &m_readCache);
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
            STARFISH_LOG_INFO(
                "[WebM] LoadCluster incomplete (ret<0) bufferSize=%lld "
                "entryPos=%lld scale=%llu",
                bufferSize, entryPos, (unsigned long long)m_timeCodeScaleNs);
            delete segment;
            // Stall safety net: many consecutive incompletes pinned at the
            // same entryPos while appends keep arriving means the head
            // element will never complete (a corrupt block/size that the
            // strict id check above could not catch). Force a resync past
            // it instead of accumulating unprocessed data forever.
            if (entryPos == m_stallEntryPos) {
                m_stallIncompleteCount++;
            } else {
                m_stallEntryPos = entryPos;
                m_stallIncompleteCount = 1;
            }
            if (m_stallIncompleteCount >= 16 || bufferSize > (4ll << 20)) {
                m_stallEntryPos = -1;
                m_stallIncompleteCount = 0;
                long long resyncPos =
                    bufferSize > 8 ? scanForElementStart(source, entryPos + 4,
                                                         bufferSize - 4)
                                   : -1;
                if (resyncPos > entryPos) {
                    STARFISH_LOG_INFO(
                        "[WebM] stall resync: dropped %lld wedged bytes "
                        "(bufferSize=%lld)",
                        resyncPos - entryPos, bufferSize);
                    source->onSeek(resyncPos, DemuxerSource::SeekWhenceSet);
                    return true;
                }
                STARFISH_LOG_INFO(
                    "[WebM] stall resync: no element start in %lld bytes, "
                    "discarding",
                    bufferSize);
                source->onSeek(bufferSize, DemuxerSource::SeekWhenceSet);
                return true;
            }
            source->onSeek(entryPos, DemuxerSource::SeekWhenceSet);
            return true;
        }
        // Parser made progress; the stall net starts over.
        m_stallEntryPos = -1;
        m_stallIncompleteCount = 0;
        if (ret > 0) {
            // No Cluster element in this buffer (e.g. init segment that only
            // carries EBML/Info/Tracks). Discard it -- everything has been
            // examined and there's nothing of value to keep around.
            STARFISH_LOG_INFO(
                "[WebM] LoadCluster no-cluster (ret>0) bufferSize=%lld "
                "entryPos=%lld",
                bufferSize, entryPos);
            delete segment;
            source->onSeek(bufferSize, DemuxerSource::SeekWhenceSet);
            return true;
        }

        const mkvparser::Cluster* pCluster = segment->GetFirst();

        // Per-block track resolution (emit flag + default duration) depends
        // only on the track number, which is constant for every frame of a
        // block. Without caching, the std::set/std::map lookups below run once
        // per block -- i.e. once per video frame. A single-entry cache keyed on
        // the last track number collapses that to one lookup per same-track run
        // (all blocks of single-track video, and the runs within interleaved
        // A/V).
        long long cachedTrackNum = -1;
        bool cachedEmit = false;
        uint64_t cachedTrackDurationMs = 0;

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

                if (trackNum != cachedTrackNum) {
                    cachedTrackNum = trackNum;
                    cachedEmit = m_emitTrackNumbers.find(trackNum) !=
                                 m_emitTrackNumbers.end();
                    cachedTrackDurationMs =
                        cachedEmit ? defaultDurationMsForTrack(trackNum) : 0;
                }

                if (!cachedEmit) {
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
                const uint64_t trackDurationMs = cachedTrackDurationMs;

                // Laced Opus/Vorbis blocks: each Frame::Read costs an
                // onSeek+onRead round trip on the DemuxerSource (M frames =
                // M reads). Block::Parse guarantees the frames of one block
                // are contiguous after the lacing headers (it patches f.pos
                // sequentially and requires the last frame to end exactly at
                // the block payload end), so one bulk read of
                // [first.pos, last.pos + last.len) covers them all. Per-frame
                // offsets are still taken from f.pos, not running sums, so
                // correctness does not depend on contiguity. frameCount == 1
                // (all video blocks, unlaced audio) keeps the direct path:
                // reading straight into the packet buffer is already a
                // single read and skips the memcpy.
                const uint8_t* spanData = nullptr;
                long long spanStart = 0;
                if (frameCount > 1) {
                    const mkvparser::Block::Frame& firstFrame =
                        pBlock->GetFrame(0);
                    const mkvparser::Block::Frame& lastFrame =
                        pBlock->GetFrame(frameCount - 1);
                    spanStart = firstFrame.pos;
                    const long long spanLen =
                        (lastFrame.pos + lastFrame.len) - spanStart;
                    if (spanLen > 0 && spanLen <= bufferSize &&
                        spanLen <= (long long)LONG_MAX) {
                        if (m_blockBuffer.size() < (size_t)spanLen) {
                            m_blockBuffer.resize((size_t)spanLen);
                        }
                        if (src.Read(spanStart, (long)spanLen,
                                     m_blockBuffer.data()) == 0) {
                            spanData = m_blockBuffer.data();
                        }
                    }
                }

                for (int i = 0; i < frameCount; ++i) {
                    const mkvparser::Block::Frame& theFrame =
                        pBlock->GetFrame(i);
                    const long size = theFrame.len;

                    // SourceBuffer releases consumed packets with
                    // MediaPacket::destroy; allocate the combined
                    // header+payload block with MediaPacket::create to
                    // match. The MP4 path uses the same allocator
                    // (MP4PacketGenerator.cpp).
                    MediaPacket* packet = MediaPacket::create((size_t)size);
                    if (spanData) {
                        memcpy(packet->m_data,
                               spanData + (theFrame.pos - spanStart),
                               (size_t)size);
                    } else {
                        theFrame.Read(&src, packet->m_data);
                    }

                    // For laced blocks, frames within the block share the
                    // block's timecode and advance by trackDurationMs each.
                    packet->m_pts = blockPtsMs + (uint64_t)i * trackDurationMs;
                    // WebM has no DTS reordering within a Cluster, so
                    // DTS == PTS.
                    packet->m_dts = packet->m_pts;
                    packet->m_duration = (size_t)trackDurationMs;
                    packet->m_hasIdr = pBlock->IsKey();

                    for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                        if (m_demuxerClients[j]->onDetectPacket(tn - 1,
                                                                packet)) {
                            // Ownership transferred to the client.
                            packet = nullptr;
                            break;
                        }
                    }
                    // Destroy only when no client took ownership.
                    if (packet) {
                        MediaPacket::destroy(packet);
                    }
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
    size_t m_peekLogCounter = 0;
    // Stall net for findStreamPacket: consecutive LoadCluster-incomplete
    // results pinned at the same entryPos (see the ret<0 branch).
    long long m_stallEntryPos = -1;
    int m_stallIncompleteCount = 0;
    std::map<long long, uint64_t> m_trackDefaultDurationsMs;
    // Track numbers that resolved to a video/audio StreamInfo during
    // findStreamInfo. Subtitle, metadata, and other auxiliary tracks
    // share Clusters with video/audio in WebM but must not be forwarded
    // to SourceBuffer as MediaPackets.
    std::set<long long> m_emitTrackNumbers;
    // Scratch buffer for bulk-reading the frame span of laced (multi-frame)
    // blocks in findStreamPacket. Grow-only, reused across calls; heap is
    // released by the GC finalizer (same pattern as
    // MP4PacketGenerator::m_sampleBuffer).
    std::vector<uint8_t> m_blockBuffer;
    // Read-through window for WrappedSegmentReader: absorbs mkvparser's
    // 1-7 byte header/VINT reads in findStreamPacket. Grow-once to 16KB,
    // reused across calls; released by the GC finalizer (m_blockBuffer
    // pattern).
    std::vector<uint8_t> m_readCache;
};

Demuxer* Demuxer::createWebMDemuxer()
{
    return new DemuxerWebM();
}
} // namespace Starfish

#endif /* STARFISH_ENABLE_MULTIMEDIA */
