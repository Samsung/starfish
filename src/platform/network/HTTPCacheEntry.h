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
#if defined(STARFISH_ENABLE_HTTPCACHE)
#ifndef __StarFishHTTPCacheEntry_
#define __StarFishHTTPCacheEntry_

namespace StarFish {
class ResourceURL;
struct CacheControl {
    CacheControl()
        : noCache(false)
        , noStore(false)
        , mustRevalidate(false)
        , maxAge(0)
    {
    }

    bool noCache : 1;
    bool noStore : 1;
    bool mustRevalidate : 1;
    time_t maxAge;
};
class HTTPCacheEntry : public gc {
public:
    HTTPCacheEntry(ResourceURL* url, time_t date, CacheControl& cacheControl);
    HTTPCacheEntry(ResourceURL* url, time_t date, CacheControl& cacheControl,
                   String* entryFileName);
    ~HTTPCacheEntry();

    ResourceURL* url()
    {
        return m_url;
    }

    time_t date()
    {
        return m_date;
    }

    String* entryFileName()
    {
        return m_entryFileName;
    }

    void setEntryFileNameUsingCachePath(String* cachePath);
    bool writeRawDataToEntryFile(std::vector<char>& rawData);

    size_t entryKey() const;
    CacheControl cacheControl() const
    {
        return m_cacheControl;
    }

    String* toString();

    bool operator==(const HTTPCacheEntry& other) const
    {
        return this->entryKey() == other.entryKey() &&
               *this->m_url == *other.m_url;
    }

private:
    ResourceURL* m_url;
    time_t m_date;
    CacheControl m_cacheControl;
    String* m_entryFileName;
};

typedef GCUnorderedMultiMap<size_t, HTTPCacheEntry*> HTTPCacheEntryMultiMap;
}
#endif
#endif
