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
#if defined(STARFISH_ENABLE_HTTPCACHE)
#ifndef __StarFishHTTPCacheEntry_
#define __StarFishHTTPCacheEntry_

#include "platform/network/http/HTTPUtil.h"

namespace StarFish {
class ResourceURL;
class Mutex;

struct EntryFileInfo {
    EntryFileInfo()
        : entryFilePath()
        , lastModificationTime(-1)
        , byteLength(0)
    {
    }

    std::string entryFilePath;
    int64_t lastModificationTime;
    size_t byteLength;
};

class HTTPCacheEntry : public gc {
public:
    HTTPCacheEntry(ResourceURL* url, CacheControl& cacheControl,
                   HTTPContentInfo& cinfo, HTTPFreshnessInfo& finfo);
    HTTPCacheEntry(ResourceURL* url, CacheControl& cacheControl,
                   HTTPContentInfo& cinfo, HTTPFreshnessInfo& finfo,
                   EntryFileInfo& einfo);

    ~HTTPCacheEntry();
    HTTPCacheEntry(const HTTPCacheEntry& rhs);

    ResourceURL* url() const
    {
        return m_url;
    }

    bool shouldReValidate() const
    {
        return !isFresh() || m_httpFreshnessInfo.etag.size() ||
               m_cacheControl.mustRevalidate || m_cacheControl.noCache;
    }

    bool shouldExpire() const
    {
        return (m_usingCount == 0 &&
                ((m_cacheControl.noStore ||
                  (!isFresh() && m_httpFreshnessInfo.etag.size() == 0))));
    }

    bool canUse() const
    {
        return !m_cacheControl.noStore &&
               (m_httpFreshnessInfo.lastModified > 0 ||
                m_httpFreshnessInfo.etag.size() > 0);
    }

    void setEntryFileNameUsingCachePath(String* cachePath);
    bool writeRawDataToEntryFile(std::vector<char>& rawData);
    bool readRawDataFromEntryFile(std::vector<char>& out);
    void readEntryHeaders(HeaderMap& out);
    bool isFresh() const;

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

    EntryFileInfo entryFileInfo() const
    {
        return m_entryFileInfo;
    }
    void setEntryFileInfo(EntryFileInfo& info);

    String* toString() const;

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

    void setNeedsRawDataUpdate(bool value);
    bool needsRawDataUpdate();

    void setNeedsPropertiesUpdate(bool value);
    bool needsPropertiesUpdate();

    bool isConsistent();
    bool good();
    void setToBad();

    static const char* kSeparator;

private:
    ResourceURL* m_url;
    CacheControl m_cacheControl;
    HTTPContentInfo m_httpContentInfo;
    HTTPFreshnessInfo m_httpFreshnessInfo;
    EntryFileInfo m_entryFileInfo;

    Mutex* m_mutex;
    size_t m_usingCount;
    bool m_needsRawDataUpdate;
    bool m_needsPropertiesUpdate;
    bool m_good;
};

typedef GCUnorderedMultiMap<size_t, HTTPCacheEntry*> HTTPCacheEntryMultiMap;
}
#endif
#endif
