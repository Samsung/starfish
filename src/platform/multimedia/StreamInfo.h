/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishStreamInfo__)
#define __StarFishStreamInfo__

namespace StarFish {

class PacketGenerator;

struct Framerate {
    Framerate()
        : m_num(0)
        , m_den(0)

    {
    }
    Framerate(int num, int den)
        : m_num(num)
        , m_den(den)
    {
    }
    double toDouble() const
    {
        return m_den ? static_cast<double>(m_num) / m_den : 0;
    }
    bool isValid()
    {
        return m_num > 0 && m_den > 0;
    }
    static Framerate createFromLL(int64_t num, int64_t den);
    int m_num;
    int m_den;
};

enum StreamType {
    StreamTypeUnknown = 1,
    StreamTypeAudio = 1 << 1,
    StreamTypeVideo = 1 << 2,
    StreamTypeSubtitle = 1 << 3,
};

enum MediaCodec {
    MediaCodecUnknown,
    // Audio
    MediaCodecAudioAAC,
    MediaCodecAudioMP3,
    MediaCodecAudioVorbis,
    // Video
    MediaCodecVideoH264,
    MediaCodecVideoHEVC,
    MediaCodecVideoVP9,
};

enum AudioSampleFormat {
    AudioSampleFormatNone = -1,
    AudioSampleFormatU8,   // unsigned 8 bits
    AudioSampleFormatS16,  // signed 16 bits
    AudioSampleFormatS32,  // signed 32 bits
    AudioSampleFormatFLT,  // float
    AudioSampleFormatDBL,  // double
    AudioSampleFormatU8P,  // unsigned 8 bits, planar
    AudioSampleFormatS16P, // signed 16 bits, planar
    AudioSampleFormatS32P, // signed 32 bits, planar
    AudioSampleFormatFLTP, // float, planar
    AudioSampleFormatDBLP, // double, planar
};

MediaCodec suggestAudioCodecFromString(String* name);
MediaCodec suggestVideoCodecFromString(String* name);
const char* mediaCodecToString(MediaCodec codec);

class StreamInfo : public gc {
public:
    StreamInfo();

    StreamInfo(StreamInfo&& info)
    {
        m_extraData = std::move(info.m_extraData);
        m_streamIndex = info.m_streamIndex;
        m_timescale = info.m_timescale;
        m_rawDuration = info.m_rawDuration;
        m_codec = info.m_codec;
        m_type = info.m_type;
        m_data = info.m_data;
    }

    StreamInfo(const StreamInfo& info)
    {
        m_extraData = info.m_extraData;
        m_streamIndex = info.m_streamIndex;
        m_timescale = info.m_timescale;
        m_rawDuration = info.m_rawDuration;
        m_codec = info.m_codec;
        m_type = info.m_type;
        m_data = info.m_data;
    }

    bool isAudio()
    {
        return m_type == StreamTypeAudio;
    }
    bool isVideo()
    {
        return m_type == StreamTypeVideo;
    }
    size_t streamIndex()
    {
        return m_streamIndex;
    }
    void setStreamIndex(size_t streamIndex)
    {
        m_streamIndex = streamIndex;
    }
    size_t timescale()
    {
        return m_timescale;
    }
    void setTimescale(size_t timescale)
    {
        m_timescale = timescale;
    }
    uint64_t duration()
    {
        return codedTimeToMilliseconds(m_rawDuration);
    }
    uint64_t rawDuration()
    {
        return m_rawDuration;
    }
    void setRawDuration(uint64_t rawDuration)
    {
        m_rawDuration = rawDuration;
    }
    const char* codecString()
    {
        return mediaCodecToString(m_codec);
    }
    bool isCodec(MediaCodec codec)
    {
        return codec == m_codec;
    }
    void setCodec(MediaCodec codec)
    {
        m_codec = codec;
    }
    StreamType type()
    {
        return m_type;
    }
    void setType(StreamType type)
    {
        m_type = type;
    }
    uint32_t audioSampleRate()
    {
        STARFISH_ASSERT(m_type == StreamTypeAudio);
        return m_data.m_audioData.m_sampleRate;
    }
    void setAudioSampleRate(uint32_t sampleRate)
    {
        m_data.m_audioData.m_sampleRate = sampleRate;
    }
    uint16_t audioChannels()
    {
        STARFISH_ASSERT(m_type == StreamTypeAudio);
        return m_data.m_audioData.m_channels;
    }
    void setAudioChannels(uint16_t channels)
    {
        m_data.m_audioData.m_channels = channels;
    }
    uint32_t videoWidth()
    {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        return m_data.m_videoData.m_width;
    }
    void setVideoWidth(uint32_t width)
    {
        m_data.m_videoData.m_width = width;
    }
    uint32_t videoHeight()
    {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        return m_data.m_videoData.m_height;
    }
    void setVideoHeight(uint32_t height)
    {
        m_data.m_videoData.m_height = height;
    }
    Framerate videoFramerate()
    {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        return m_data.m_videoData.m_framerate;
    }
    void setVideoFramerate(Framerate framerate)
    {
        m_data.m_videoData.m_framerate = framerate;
    }
    bool videoHasFramerate()
    {
        STARFISH_ASSERT(m_type == StreamTypeVideo);
        return m_data.m_videoData.m_hasFramerate;
    }
    void setVideoHasFramerate(bool flag)
    {
        m_data.m_videoData.m_hasFramerate = flag;
    }
    size_t codedTimeToMilliseconds(size_t raw)
    {
        return (raw * 1000LL) / (m_timescale == 0 ? 1 : m_timescale);
    }

public:
    std::vector<uint8_t> m_extraData;

protected:
    size_t m_streamIndex;
    size_t m_timescale;
    uint64_t m_rawDuration;
    MediaCodec m_codec;
    StreamType m_type;
    union Data {
        Data()
            : m_videoData()
        {
        }
        struct {
            uint32_t m_sampleRate;
            uint16_t m_channels;
        } m_audioData;
        struct {
            uint32_t m_width;
            uint32_t m_height;
            bool m_hasFramerate;
            Framerate m_framerate;
        } m_videoData;
    } m_data;
};
}
#endif
