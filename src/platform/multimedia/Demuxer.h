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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarfishDemuxer__)
#define __StarfishDemuxer__

#include "platform/multimedia/StreamInfo.h"

#include <new>

namespace Starfish {

class DemuxerSource;
class PacketGenerator;

struct MediaPacket {
    uint8_t* m_data;
    size_t m_dataSize;
    uint64_t m_pts;    // ms
    uint64_t m_dts;    // ms
    size_t m_duration; // ms
    bool m_hasIdr : 1;

    MediaPacket() = default;

    // One block: [MediaPacket header][payload]. operator new[] returns
    // storage aligned for max_align_t >= alignof(MediaPacket).
    static MediaPacket* create(size_t payloadSize)
    {
        uint8_t* block = new uint8_t[sizeof(MediaPacket) + payloadSize];
        MediaPacket* pkt = new (block) MediaPacket();
        pkt->m_data = block + sizeof(MediaPacket);
        pkt->m_dataSize = payloadSize;
        return pkt;
    }
    static void destroy(MediaPacket* pkt)
    {
        // trivially destructible; block was allocated as uint8_t[]
        delete[] reinterpret_cast<uint8_t*>(pkt);
    }
};

class DemuxerClient : public gc {
public:
    virtual ~DemuxerClient()
    {
    }
    virtual void onDetectStream(const StreamInfo& info)
    {
    }
    // Returns true when the client takes ownership of the packet
    // (the client must release it with MediaPacket::destroy).
    // Returns false when the packet is not consumed; the caller
    // remains the owner and destroys it.
    virtual bool onDetectPacket(size_t streamIndex, MediaPacket* packet)
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
} // namespace Starfish

#endif
