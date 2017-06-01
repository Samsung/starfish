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

#ifndef __StarFishSecurityOriginData__
#define __StarFishSecurityOriginData__

#include "StarFishConfig.h"

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
        return *data1 == *data2;
    }
};
}

#endif
