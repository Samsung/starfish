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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishPacketGenerator__)
#define __StarFishPacketGenerator__

#include "platform/multimedia/StreamInfo.h"

namespace StarFish {

class MediaPacket;
class PacketGenerator : public gc {
public:
    PacketGenerator(MediaCodec codec)
        : m_codec(codec)
    {
    }
    PacketGenerator()
        : PacketGenerator(MediaCodecUnknown)
    {
    }
    virtual ~PacketGenerator()
    {
    }

    virtual bool generate(DemuxerSource* from, size_t validLength,
                          MediaPacket& packet) = 0;
    bool isCodec(MediaCodec codec)
    {
        return codec == m_codec;
    }
    void setCodec(MediaCodec codec)
    {
        m_codec = codec;
    }
    MediaCodec codec()
    {
        return m_codec;
    }

protected:
    MediaCodec m_codec;
};

typedef std::vector<std::vector<uint8_t>> H264SPSVector;
typedef std::vector<std::vector<uint8_t>> H264PPSVector;

class MP4PacketGenerator : public PacketGenerator {
public:
    MP4PacketGenerator();
    bool generate(DemuxerSource* from, size_t validLength,
                  MediaPacket& packet) override;

    // AVC(H264)
    void setAVCExtraData(H264SPSVector& spsVector, H264PPSVector& ppsVector);
    void setAVCNALSizeLength(unsigned char length)
    {
        STARFISH_ASSERT(m_codec == MediaCodecVideoH264);
        m_H264NalSizeLength = length;
    }
    // HEVC(H265)
    void setHEVCExtraData(std::vector<uint8_t>& extraData);

protected:
    bool generateForAVC(DemuxerSource* from, size_t validLength,
                        MediaPacket& packet);
    bool generateForHEVC(DemuxerSource* from, size_t validLength,
                         MediaPacket& packet);
    bool generateDefault(DemuxerSource* from, size_t validLength,
                         MediaPacket& packet);

protected:
    std::vector<uint8_t> m_extraData;
    unsigned char m_H264NalSizeLength;
};
}
#endif
