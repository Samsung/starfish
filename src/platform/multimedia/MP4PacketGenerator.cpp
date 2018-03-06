/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "StarFishConfig.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/DemuxerSource.h"
#include "platform/multimedia/PacketGenerator.h"

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#define MP4PACKET_GENERATOR_LOG(pg, ...)                \
    STARFISH_LOG_INFO("[PacketGenerator|%s] ",          \
                      mediaCodecToString(pg->codec())); \
    STARFISH_LOG_INFO(__VA_ARGS__);
#else
#define MP4PACKET_GENERATOR_LOG(pg, ...)
#endif

namespace StarFish {

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

    static bool parseNext(DemuxerSource* from, unsigned char lengthSize,
                          NALUnit& result)
    {
        int error = 0;
        size_t readAmount = 0;
        uint8_t sizeBuf[4];
        size_t start = from->onSeek(0, DemuxerSource::SeekWhenceCurrent);
        // Length field
        from->onRead(lengthSize, readAmount, error, sizeBuf);
        if (readAmount < lengthSize) {
            from->onSeek(start, DemuxerSource::SeekWhenceSet);
            return false;
        }
        size_t naluSize = 0;
        for (size_t i = 0; i < lengthSize; i++) {
            naluSize = (naluSize << 8) + sizeBuf[i];
        }
        if (naluSize == 0) {
            from->onSeek(start, DemuxerSource::SeekWhenceSet);
            return false;
        }
        // Type field
        uint8_t naluType = 0;
        from->onRead(1, readAmount, error, &naluType);
        if (readAmount < 1) {
            from->onSeek(start, DemuxerSource::SeekWhenceSet);
            return false;
        }
        naluType = naluType & 0x1f;
        // Data field
        size_t expected = start + lengthSize + naluSize;
        size_t real =
            from->onSeek(naluSize - 1, DemuxerSource::SeekWhenceCurrent);
        if (real != expected) {
            from->onSeek(start, DemuxerSource::SeekWhenceSet);
            return false;
        }
        // Fill result
        result.m_type = naluType;
        result.m_size = naluSize + lengthSize;
        result.m_startPos = start;
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
            MP4PACKET_GENERATOR_LOG(
                self, "MP4PacketGenerator::~MP4PacketGenerator\n");
            std::vector<uint8_t>().swap(self->m_extraData);
        },
        NULL, NULL, NULL);
}

bool MP4PacketGenerator::generate(DemuxerSource* from, size_t validLength,
                                  MediaPacket& packet)
{
    switch (m_codec) {
    case MediaCodecVideoH264:
        return generateForAVC(from, validLength, packet);
    case MediaCodecVideoHEVC:
        return generateForHEVC(from, validLength, packet);
    default:
        return generateDefault(from, validLength, packet);
    }
    return false;
}

bool MP4PacketGenerator::generateForAVC(DemuxerSource* from, size_t validLength,
                                        MediaPacket& packet)
{
    STARFISH_ASSERT(m_codec == MediaCodecVideoH264);
    if (!(m_H264NalSizeLength == 1 || m_H264NalSizeLength == 2 ||
          m_H264NalSizeLength == 4)) {
        // Invalid lengthSize value
        MP4PACKET_GENERATOR_LOG(this, "Invalid length size\n");
        return false;
    }
    // Check AnnexB type
    bool foundSPS = false;
    bool foundPPS = false;
    size_t naluCount = 0;
    size_t readSoFar = 0;
    size_t extraInsertPos = SIZE_MAX;
    size_t start = from->onSeek(0, DemuxerSource::SeekWhenceCurrent);
    packet.m_hasIdr = false;

    while (readSoFar < validLength) {
        MP4AVCParser::NALUnit nalu;
        if (!MP4AVCParser::parseNext(from, m_H264NalSizeLength, nalu)) {
            MP4PACKET_GENERATOR_LOG(
                this, "Fail to parse to NALU by MP4AVCParser (1)\n");
            return false;
        }
        readSoFar += nalu.m_size;
        naluCount++;
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
            packet.m_hasIdr = true;
            break;
        default:
            break;
        }
    }
    // TODO Validate AnnexB type (DEBUG)
    from->onSeek(start, DemuxerSource::SeekWhenceSet);
    if (readSoFar != validLength) {
        MP4PACKET_GENERATOR_LOG(this,
                                "Fail to parse to NALU by MP4AVCParser (2)\n");
        return false;
    }
    // Generate data from source
    size_t resultSize = validLength;
    if (m_H264NalSizeLength < AnnexBHeaderSize) {
        resultSize += (naluCount * (AnnexBHeaderSize - m_H264NalSizeLength));
    }
    if (extraInsertPos != SIZE_MAX) {
        resultSize += m_extraData.size();
    }
    uint8_t* result = new uint8_t[resultSize];
    size_t pos = 0;
    for (size_t i = 0; i < naluCount; i++) {
        MP4AVCParser::NALUnit nalu;
        // TODO remove debug code
        if (!MP4AVCParser::parseNext(from, m_H264NalSizeLength, nalu)) {
            MP4PACKET_GENERATOR_LOG(
                this, "Fail to parse to NALU by MP4AVCParser (3)\n");
            delete[] result;
            return false;
        }
        if (nalu.m_startPos == extraInsertPos) {
            memcpy(&result[pos], m_extraData.data(), m_extraData.size());
            pos += m_extraData.size();
        }
        // Insert AnnexB header instead of length field
        memcpy(&result[pos], AnnexBHeader, AnnexBHeaderSize);
        pos += AnnexBHeaderSize;
        // Copy data
        int error = 0;
        size_t readAmount = 0;
        size_t dataSize = nalu.m_size - m_H264NalSizeLength;
        from->onSeek(nalu.m_startPos + m_H264NalSizeLength,
                     DemuxerSource::SeekWhenceSet);
        from->onRead(dataSize, readAmount, error, &result[pos]);
        pos += dataSize;
    }
    packet.m_data = result;
    packet.m_dataSize = resultSize;
    return true;
}

bool MP4PacketGenerator::generateForHEVC(DemuxerSource* from,
                                         size_t validLength,
                                         MediaPacket& packet)
{
    STARFISH_ASSERT(m_codec == MediaCodecVideoHEVC);
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return false;
}

bool MP4PacketGenerator::generateDefault(DemuxerSource* from,
                                         size_t validLength,
                                         MediaPacket& packet)
{
    uint8_t* data = new uint8_t[validLength];
    int error;
    size_t readAmount;
    from->onRead(validLength, readAmount, error, data);
    packet.m_data = data;
    packet.m_dataSize = validLength;
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
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return;
}
}
#undef MP4PACKET_GENERATOR_LOG
#endif
