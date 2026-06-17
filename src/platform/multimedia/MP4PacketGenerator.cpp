/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "StarfishConfig.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/DemuxerSource.h"
#include "platform/multimedia/PacketGenerator.h"

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#define MP4PACKET_GENERATOR_LOG(pg, STR, ...) \
    STARFISH_LOG_INFO(                        \
        "[PacketGenerator|%s] "               \
        "" STR,                               \
        mediaCodecToString(pg->codec()), ##__VA_ARGS__);
#else
#define MP4PACKET_GENERATOR_LOG(pg, ...)
#endif

namespace Starfish {

static const uint8_t NALUTypeUnknown = 0;
static const uint8_t NALUTypeNonIDRSlice = 1; // VCL
static const uint8_t NALUTypeSliceDataA = 2;  // VCL
static const uint8_t NALUTypeSliceDataB = 3;  // VCL
static const uint8_t NALUTypeSliceDataC = 4;  // VCL
static const uint8_t NALUTypeIDRSlice = 5;    // VCL
static const uint8_t NALUTypeSEIMessage = 6;
static const uint8_t NALUTypeSPS = 7;
static const uint8_t NALUTypePPS = 8;
static const uint8_t NALUTypeAUD = 9;
static const uint8_t NALUTypeEOSeq = 10;
static const uint8_t NALUTypeEOStream = 11;
static const uint8_t NALUTypeFiller = 12;
static const uint8_t NALUTypeSPSExt = 13;
static const uint8_t NALUType14 = 14;
static const uint8_t NALUType15 = 15;
static const uint8_t NALUType16 = 16;
static const uint8_t NALUType17 = 17;
static const uint8_t NALUType18 = 18;
static const uint8_t NALUTypeCodedSliceAux = 19;
static const uint8_t NALUTypeCodedSliceExtension = 20;

static const uint8_t AnnexBHeader[] = { 0, 0, 0, 1 };
static const int AnnexBHeaderSize = 4;

class MP4AVCParser {
public:
    struct NALUnit {
        uint8_t m_type;
        size_t m_size;
        size_t m_startPos;
    };

    // Parses one length-prefixed NALU from in-memory sample data.
    // cursor is advanced only on success. m_startPos is an offset
    // within the sample.
    static bool parseNext(const uint8_t* data, size_t dataSize, size_t& cursor,
                          unsigned char lengthSize, NALUnit& result)
    {
        if (dataSize - cursor < lengthSize) {
            return false;
        }
        size_t naluSize = 0;
        for (size_t i = 0; i < lengthSize; i++) {
            naluSize = (naluSize << 8) + data[cursor + i];
        }
        if (naluSize == 0) {
            return false;
        }
        if (naluSize > dataSize - cursor - lengthSize) {
            return false;
        }
        result.m_type = data[cursor + lengthSize] & 0x1f;
        result.m_size = naluSize + lengthSize;
        result.m_startPos = cursor;
        cursor += result.m_size;
        return true;
    }
};

MP4PacketGenerator::MP4PacketGenerator()
    : PacketGenerator()
    , m_H264NalSizeLength(2)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            MP4PacketGenerator* self = (MP4PacketGenerator*)obj;
            MP4PACKET_GENERATOR_LOG(self,
                                    "MP4PacketGenerator::~MP4PacketGenerator");
            std::vector<uint8_t>().swap(self->m_extraData);
            std::vector<uint8_t>().swap(self->m_sampleBuffer);
            decltype(self->m_naluScratch)().swap(self->m_naluScratch);
        },
        NULL, NULL, NULL);
}

bool MP4PacketGenerator::generate(DemuxerSource* from, size_t validLength,
                                  MediaPacket*& packet)
{
    packet = nullptr;
    switch (m_codec) {
    case MediaCodecVideoH264:
        return generateForAVC(from, validLength, packet);
    case MediaCodecVideoHEVC:
        return generateForHEVC(from, validLength, packet);
    case MediaCodecVideoAV1:
        return generateForAV1(from, validLength, packet);
    default:
        return generateDefault(from, validLength, packet);
    }
    return false;
}

bool MP4PacketGenerator::generateForAVC(DemuxerSource* from, size_t validLength,
                                        MediaPacket*& packet)
{
    STARFISH_ASSERT(m_codec == MediaCodecVideoH264);
    if (!(m_H264NalSizeLength == 1 || m_H264NalSizeLength == 2 ||
          m_H264NalSizeLength == 4)) {
        // Invalid lengthSize value
        MP4PACKET_GENERATOR_LOG(this, "Invalid length size");
        return false;
    }
    // Read whole sample once into reusable buffer
    int64_t start = from->onSeek(0, DemuxerSource::SeekWhenceCurrent);
    if (m_sampleBuffer.size() < validLength) {
        m_sampleBuffer.resize(validLength);
    }
    int error = 0;
    size_t readAmount = 0;
    from->onRead(validLength, readAmount, error, m_sampleBuffer.data());
    if (readAmount < validLength) {
        from->onSeek(start, DemuxerSource::SeekWhenceSet);
        MP4PACKET_GENERATOR_LOG(this, "Fail to read sample data");
        return false;
    }
    const uint8_t* sample = m_sampleBuffer.data();
    // Check AnnexB type
    bool foundSPS = false;
    bool foundPPS = false;
    size_t cursor = 0;
    size_t extraInsertPos = SIZE_MAX;
    bool hasIdr = false;

    // Scan pass: walk the sample once, recording each NALU's (startPos, size)
    // so the copy pass below does not have to re-parse the length fields.
    m_naluScratch.clear();
    while (cursor < validLength) {
        MP4AVCParser::NALUnit nalu;
        if (!MP4AVCParser::parseNext(sample, validLength, cursor,
                                     m_H264NalSizeLength, nalu)) {
            from->onSeek(start, DemuxerSource::SeekWhenceSet);
            MP4PACKET_GENERATOR_LOG(
                this, "Fail to parse to NALU by MP4AVCParser (1)");
            return false;
        }
        m_naluScratch.push_back(std::make_pair(nalu.m_startPos, nalu.m_size));
        switch (nalu.m_type) {
        case NALUTypeSPS:
            foundSPS = true;
            break;
        case NALUTypePPS:
            foundPPS = true;
            break;
        case NALUTypeIDRSlice:
            if (!foundSPS && !foundPPS) {
                extraInsertPos = nalu.m_startPos;
            }
            hasIdr = true;
            break;
        default:
            break;
        }
    }
    // TODO Validate AnnexB type (DEBUG)
    if (cursor != validLength) {
        from->onSeek(start, DemuxerSource::SeekWhenceSet);
        MP4PACKET_GENERATOR_LOG(this,
                                "Fail to parse to NALU by MP4AVCParser (2)");
        return false;
    }
    // Generate data from source
    size_t resultSize = validLength;
    if (m_H264NalSizeLength < AnnexBHeaderSize) {
        resultSize +=
            (m_naluScratch.size() * (AnnexBHeaderSize - m_H264NalSizeLength));
    }
    if (extraInsertPos != SIZE_MAX) {
        resultSize += m_extraData.size();
    }
    MediaPacket* pkt = MediaPacket::create(resultSize);
    uint8_t* result = pkt->m_data;
    size_t pos = 0;
    // Copy pass: replay the recorded NALU boundaries.
    for (size_t i = 0; i < m_naluScratch.size(); i++) {
        const size_t naluStart = m_naluScratch[i].first;
        const size_t naluSize = m_naluScratch[i].second;
        if (naluStart == extraInsertPos) {
            memcpy(&result[pos], m_extraData.data(), m_extraData.size());
            pos += m_extraData.size();
        }
        // Insert AnnexB header instead of length field
        memcpy(&result[pos], AnnexBHeader, AnnexBHeaderSize);
        pos += AnnexBHeaderSize;
        // Copy data
        size_t dataSize = naluSize - m_H264NalSizeLength;
        memcpy(&result[pos], &sample[naluStart + m_H264NalSizeLength],
               dataSize);
        pos += dataSize;
    }
    pkt->m_hasIdr = hasIdr;
    packet = pkt;
    return true;
}

bool MP4PacketGenerator::generateForHEVC(DemuxerSource* from,
                                         size_t validLength,
                                         MediaPacket*& packet)
{
    STARFISH_ASSERT(m_codec == MediaCodecVideoHEVC);
    STARFISH_UNSUPPORTED("Media: HEVC codec is not supported");
    return false;
}

bool MP4PacketGenerator::generateForAV1(DemuxerSource* from, size_t validLength,
                                        MediaPacket*& packet)
{
    STARFISH_ASSERT(m_codec == MediaCodecVideoAV1);
    // AV1 uses OBU structure, no need for special processing like H.264/HEVC
    // Just read the raw data directly
    return generateDefault(from, validLength, packet);
}

bool MP4PacketGenerator::generateDefault(DemuxerSource* from,
                                         size_t validLength,
                                         MediaPacket*& packet)
{
    packet = MediaPacket::create(validLength);
    int error = 0;
    size_t readAmount = 0;
    from->onRead(validLength, readAmount, error, packet->m_data);
    if (readAmount < validLength) {
        MP4PACKET_GENERATOR_LOG(this, "Fail to read sample data");
        MediaPacket::destroy(packet);
        packet = nullptr;
        return false;
    }
    return true;
}

// AVC(H264)
void MP4PacketGenerator::setAVCExtraData(H264SPSVector& spsVector,
                                         H264PPSVector& ppsVector)
{
    STARFISH_ASSERT(m_codec == MediaCodecVideoH264);
    std::vector<uint8_t>().swap(m_extraData);
    // Insert SPS with AnnexB header
    for (size_t i = 0; i < spsVector.size(); i++) {
        m_extraData.insert(m_extraData.end(), AnnexBHeader,
                           AnnexBHeader + AnnexBHeaderSize);
        m_extraData.insert(m_extraData.end(), spsVector[i].begin(),
                           spsVector[i].end());
    }
    // Insert PPS with AnnexB header
    for (size_t i = 0; i < ppsVector.size(); i++) {
        m_extraData.insert(m_extraData.end(), AnnexBHeader,
                           AnnexBHeader + AnnexBHeaderSize);
        m_extraData.insert(m_extraData.end(), ppsVector[i].begin(),
                           ppsVector[i].end());
    }
}

// HEVC(H265)
void MP4PacketGenerator::setHEVCExtraData(std::vector<uint8_t>& extraData)
{
    STARFISH_ASSERT(m_codec == MediaCodecVideoHEVC);
    STARFISH_UNSUPPORTED("Media: HEVC codec is not supported");
    return;
}
} // namespace Starfish
#undef MP4PACKET_GENERATOR_LOG
#endif
