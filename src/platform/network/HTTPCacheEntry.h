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
    HTTPCacheEntry(ResourceURL* url, CacheControl& cacheControl,
                   HTTPContentInfo& cinfo, HTTPFreshnessInfo& finfo,
                   int64_t lastModifyFileTime);
    HTTPCacheEntry(ResourceURL* url, CacheControl& cacheControl,
                   HTTPContentInfo& cinfo, HTTPFreshnessInfo& finfo,
                   int64_t lastModifyFileTime, String* entryFileName);
    ~HTTPCacheEntry();
    HTTPCacheEntry(const HTTPCacheEntry& rhs);

    ResourceURL* url()
    {
        return m_url;
    }

    int64_t lastModifyFileTime()
    {
        return m_lastModifyFileTime;
    }

    String* entryFileName()
    {
        return m_entryFileName;
    }

    bool shouldReValidate()
    {
        return !isFresh() || m_httpFreshnessInfo.etag.size() ||
               m_cacheControl.mustRevalidate || m_cacheControl.noCache;
    }

    void setEntryFileNameUsingCachePath(String* cachePath);
    bool writeRawDataToEntryFile(std::vector<char>& rawData);
    bool readRawDataFromEntryFile(std::vector<char>& out);
    void readEntryHeaders(HeaderMap& out);
    bool isFresh();

    size_t entryKey() const;

    CacheControl cacheControl() const
    {
        return m_cacheControl;
    }
    void setCacheControl(CacheControl& info);

    HTTPContentInfo httpContentInfo() const
    {
        return m_httpContentInfo;
    }
    void setHTTPContentInfo(HTTPContentInfo& info);

    HTTPFreshnessInfo httpFreshnessInfo() const
    {
        return m_httpFreshnessInfo;
    }
    void setHTTPFreshnessInfo(HTTPFreshnessInfo& info);

    String* toString();

    bool operator==(const HTTPCacheEntry& other) const
    {
        return this->entryKey() == other.entryKey() &&
               *this->m_url == *other.m_url;
    }

    void increaseUsingCount();
    void decreaseUsingCount();
    size_t usingCount()
    {
        return m_usingCount;
    }

    static const char* kSeparator;

private:
    ResourceURL* m_url;
    CacheControl m_cacheControl;
    HTTPContentInfo m_httpContentInfo;
    HTTPFreshnessInfo m_httpFreshnessInfo;
    int64_t m_lastModifyFileTime;
    String* m_entryFileName;

    Mutex* m_mutex;
    size_t m_usingCount;
};

typedef GCUnorderedMultiMap<size_t, HTTPCacheEntry*> HTTPCacheEntryMultiMap;
}
#endif
#endif
