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

#ifndef __StarFishHTTPCacheEntry_
#define __StarFishHTTPCacheEntry_

namespace StarFish {
class ResourceURL;

class HTTPCacheEntry : public gc {
public:
    HTTPCacheEntry();
    ~HTTPCacheEntry();

    size_t getHashKey() const;

    bool operator==(const HTTPCacheEntry& other) const
    {
        return this->m_url == other.m_url;
    }

private:
    ResourceURL* m_url;
    size_t m_hashKey;
};

struct HTTPCacheEntryHash {
    size_t operator()(const HTTPCacheEntry* data) const
    {
        return data->getHashKey();
    }
};

struct HTTPCacheEntryEqual {
    bool operator()(const HTTPCacheEntry* data1,
                    const HTTPCacheEntry* data2) const
    {
        return data1 == data2 ? true : *data1 == *data2;
    }
};

typedef GCUnorderedSet<HTTPCacheEntry*, HTTPCacheEntryHash, HTTPCacheEntryEqual>
    HTTPCacheEntrySet;
}
#endif
