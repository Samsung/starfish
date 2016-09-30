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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishDemuxer__)
#define __StarFishDemuxer__

namespace StarFish {

struct MediaPacket {
    size_t m_streamIndex;
    uint8_t* m_data;
    size_t m_dataSize;
    uint64_t m_pts;
};


class DemuxerSource : public gc {
public:
    enum SeekWhence {
        SeekWhenceSet,
        SeekWhenceCurrent,
        SeekWhenceEnd,
        SeekWhenceLookSize,
    };
    virtual ~DemuxerSource() { }
    virtual int64_t onSeek(int64_t position, SeekWhence whence) = 0;
    virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead, uint8_t* buffer) = 0;
};

struct StreamInfo {
    size_t m_streamIndex;
    const char* m_codecName;
    int m_bitRate;
};

struct VideoStreamInfo : public StreamInfo {
    int m_timeBaseNum;
    int m_timeBaseDen;
    int m_width;
    int m_height;
    // TODO add pixel format
};

enum AudioSampleFormat {
    AudioSampleFormatNone = -1,
    AudioSampleFormatU8, // unsigned 8 bits
    AudioSampleFormatS16, // signed 16 bits
    AudioSampleFormatS32, // signed 32 bits
    AudioSampleFormatFLT, // float
    AudioSampleFormatDBL, // double
    AudioSampleFormatU8P, // unsigned 8 bits, planar
    AudioSampleFormatS16P, // signed 16 bits, planar
    AudioSampleFormatS32P, // signed 32 bits, planar
    AudioSampleFormatFLTP, // float, planar
    AudioSampleFormatDBLP, // double, planar
};

struct AudioStreamInfo : public StreamInfo {
    AudioSampleFormat m_sampleFormat;
    int m_channels;
    int m_sampleRate;
};

class DemuxerClient : public gc {
public:
    virtual ~DemuxerClient() { }
    virtual void onDetectVideoStream(const VideoStreamInfo& info) { }
    virtual void onDetectAudioStream(const AudioStreamInfo& info) { }
    virtual void onDetectPacket(const MediaPacket& packet) { }
};


class Demuxer : public gc {
public:
    static Demuxer* create(DemuxerSource* source, String* formatHint);
    virtual bool findStreamPacket() = 0;
    virtual bool findStreamInfo() = 0;

    void addClient(DemuxerClient* client)
    {
        m_demuxerClients.push_back(client);
    }

    void removeClient(DemuxerClient* client)
    {
        m_demuxerClients.erase(std::find(m_demuxerClients.begin(), m_demuxerClients.end(), client));
    }

protected:
    Demuxer(DemuxerSource* src)
        : m_demuxerSource(src)
    {
    }

    DemuxerSource* m_demuxerSource;
    std::vector<DemuxerClient*, gc_allocator<DemuxerClient*>> m_demuxerClients;
};

}

#endif
