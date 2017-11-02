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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishDemuxer__)
#define __StarFishDemuxer__

#include "platform/multimedia/StreamInfo.h"

namespace StarFish {

class DemuxerSource;
class PacketGenerator;

struct MediaPacket {
    uint8_t* m_data;
    size_t m_dataSize;
    uint64_t m_pts;    // ms
    uint64_t m_dts;    // ms
    size_t m_duration; // ms
    bool m_hasIdr : 1;
};

class DemuxerClient : public gc {
public:
    virtual ~DemuxerClient()
    {
    }
    virtual void onDetectStream(const StreamInfo& info)
    {
    }
    // return true means client consume packet data
    virtual bool onDetectPacket(size_t streamIndex, const MediaPacket& packet)
    {
        return false;
    }
};

class Demuxer : public gc {
public:
    static Demuxer* createDemuxer(String* mimeTypeOfContainer);
    static Demuxer* createWebMDemuxer();
    static Demuxer* createMP4Demuxer();
    static Demuxer* createFFmpegDemuxer();
    virtual bool findStreamPacket(DemuxerSource* source) = 0;
    virtual bool findStreamInfo(DemuxerSource* source, String* formatHint) = 0;

    void addClient(DemuxerClient* client)
    {
        m_demuxerClients.push_back(client);
    }

    void removeClient(DemuxerClient* client)
    {
        m_demuxerClients.erase(std::find(m_demuxerClients.begin(),
                                         m_demuxerClients.end(), client));
    }

    DemuxerClient* client(size_t idx)
    {
        return m_demuxerClients[idx];
    }

    size_t clientSize()
    {
        return m_demuxerClients.size();
    }

    virtual bool isFindedStreamInfo()
    {
        return false;
    }

protected:
    Demuxer()
    {
    }

    GCVector<DemuxerClient*> m_demuxerClients;
};
}

#endif
