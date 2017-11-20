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

#include "platform/network/http/HTTPUtil.h"

namespace StarFish {
class ResourceURL;
class Mutex;

class HTTPCacheEntry : public gc {
public:
    HTTPCacheEntry(ResourceURL* url, HTTPFreshnessInfo& info,
                   CacheControl& cacheControl);
    HTTPCacheEntry(ResourceURL* url, HTTPFreshnessInfo& info,
                   CacheControl& cacheControl, String* entryFileName);
    ~HTTPCacheEntry();

    ResourceURL* url()
    {
        return m_url;
    }

    String* entryFileName()
    {
        return m_entryFileName;
    }

    bool shouldRevalidate()
    {
        return isFresh() || m_httpFreshnessInfo.etag.size();
    }

    void setEntryFileNameUsingCachePath(String* cachePath);
    bool writeRawDataToEntryFile(std::vector<char>& rawData);
    bool readRawDataFromEntryFile(std::vector<char>& out);
    bool isFresh();

    size_t entryKey() const;

    HTTPFreshnessInfo httpFreshnessInfo() const
    {
        return m_httpFreshnessInfo;
    }
    void setHTTPFreshnessInfo(HTTPFreshnessInfo& info);

    CacheControl cacheControl() const
    {
        return m_cacheControl;
    }
    void setCacheControl(CacheControl& info);

    String* toString();

    bool operator==(const HTTPCacheEntry& other) const
    {
        return this->entryKey() == other.entryKey() &&
               *this->m_url == *other.m_url;
    }

private:
    ResourceURL* m_url;
    HTTPFreshnessInfo m_httpFreshnessInfo;
    CacheControl m_cacheControl;
    String* m_entryFileName;
    Mutex* m_mutex;
};

typedef GCUnorderedMultiMap<size_t, HTTPCacheEntry*> HTTPCacheEntryMultiMap;
typedef std::vector<std::string> HTTPCacheLRUList;
}
#endif
#endif
