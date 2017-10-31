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

class HTTPCacheEntry : public gc {
public:
    HTTPCacheEntry(ResourceURL* url, time_t date, time_t maxAge);
    HTTPCacheEntry(ResourceURL* url, time_t date, time_t maxAge,
                   String* entryFileName);
    ~HTTPCacheEntry();

    ResourceURL* url()
    {
        return m_url;
    }
    String* entryFileName()
    {
        return m_entryFileName;
    }

    void setEntryFileNameUsingCachePath(String* cachePath);
    bool writeRawDataToEntryFile(std::vector<char>& rawData);

    size_t entryKey() const;
    time_t maxAge() const
    {
        return m_maxAge;
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
    time_t m_maxAge;
    String* m_entryFileName;
};

typedef GCUnorderedMultiMap<size_t, HTTPCacheEntry*> HTTPCacheEntryMultiMap;
}
#endif
#endif
