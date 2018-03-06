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

#ifndef __StarFishNetworkSharedResourceManager__
#define __StarFishNetworkSharedResourceManager__

#include <curl/curl.h>

namespace StarFish {

class Document;
class HTTPTransaction;
class Mutex;
class ResourceURL;
class String;

struct CurlHandleData {
    CURL* curl;
    uint64_t lastUsedTime;
};

class NetworkSharedResourceManager {
    friend class HTTPTransaction;

public:
    typedef std::multimap<std::string, CurlHandleData> CurlHandleDataMultiMap;
    enum MutexKind {
        CookieMutex = 0,
        SSLMutex,
        DNSMutex,
        ShareMutex,
        CurlCacheMutex,
        MutexKindMax
    };

    static NetworkSharedResourceManager* getInstance();
    static void close();

    CURLSH* curlShareHandle() const; // Do not free
    std::string cookieStoreFilePath() const;
    void setCookieStoreFilePath(const std::string& name);
    Mutex* resourceMutex(curl_lock_data data);

    CurlHandleData getCurlHandleData(const std::string& host);
    void cachingCurlHandleData(const std::string& host, CurlHandleData& cd);
    void clearAllCurlHandleDataCache();
    void pruningIfNeed();
    void initCookieSession();

    // for document.cookie
    String* cookeis(ResourceURL* url);
    void setCookies(Document* document, ResourceURL* url, String* value);

    size_t cacheClearTimerID()
    {
        return m_cacheClearTimerID;
    }

    void setCacheClearTimerID(size_t timerID)
    {
        m_cacheClearTimerID = timerID;
    }

private:
    NetworkSharedResourceManager();
    ~NetworkSharedResourceManager();

    void initMutexes();
    void removeMutexes();

    CURLSH* m_curlShareHandle;
    CurlHandleDataMultiMap m_curlHandleDataCache;
    uint64_t m_lastCachePruneTime;
    size_t m_cacheClearTimerID;
    std::string m_cookieStoreFilePath;
    Mutex* m_mutexes[MutexKindMax];
};
}

#endif
