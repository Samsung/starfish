/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

protected:
    MediaCodec m_codec;
};

typedef std::vector<std::vector<uint8_t>> H264SPSVector;
typedef std::vector<std::vector<uint8_t>> H264PPSVector;
struct MP4SampleInfo {
    uint32_t m_duration;
    uint32_t m_size;
    uint32_t m_ctsOffset;
};

class MP4PacketGenerator : public PacketGenerator {
public:
    MP4PacketGenerator();
    bool generate(DemuxerSource* from, size_t validLength,
                  MediaPacket& packet) override;

    void setSampleInfo(std::vector<MP4SampleInfo>&& sampleInfo)
    {
        m_sampleInfo = std::move(sampleInfo);
    }
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
    std::vector<MP4SampleInfo> m_sampleInfo;
    unsigned char m_H264NalSizeLength;
};
}
#endif
