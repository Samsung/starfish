/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
    virtual ~Demuxer()
    {
    }
    static Demuxer* createDemuxer(String* mimeTypeOfContainer);
    static Demuxer* createWebMDemuxer();
    static Demuxer* createMP4Demuxer();
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
