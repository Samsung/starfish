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

#ifndef __StarFishSecurityOriginData__
#define __StarFishSecurityOriginData__

namespace StarFish {

class SecurityOriginData : public gc {
public:
    SecurityOriginData(String* protocol, String* host, unsigned port)
        : m_protocol(protocol)
        , m_host(host)
        , m_port(port)
    {
    }

    String* protocol() const
    {
        return m_protocol;
    }

    String* host() const
    {
        return m_host;
    }

    unsigned port() const
    {
        return m_port;
    }

    bool operator==(const SecurityOriginData& other) const
    {
        return protocol()->equals(other.protocol()) &&
               host()->equals(other.host()) && port() == other.port();
    }

private:
    String* m_protocol;
    String* m_host;
    unsigned m_port;
};

struct SecurityOriginDataHash {
    size_t operator()(const SecurityOriginData* data) const
    {
        size_t seed = 0;
        seed = seed ^ std::hash<String*>{}(data->protocol());
        seed = seed ^ (std::hash<String*>{}(data->host()) << 1);
        seed = seed ^ (std::hash<unsigned>{}(data->port()) << 2);
        return seed;
    }
};

struct SecurityOriginDataEqual {
    bool operator()(const SecurityOriginData* data1,
                    const SecurityOriginData* data2) const
    {
        return data1 == data2 ? true : *data1 == *data2;
    }
};
}

#endif
