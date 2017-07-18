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
    void clearAllCurlHandleDataCach();
    void prunningIfNeed();
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
    CurlHandleDataMultiMap m_CurlHandleDataCache;
    size_t m_cacheClearTimerID;
    std::string m_cookieStoreFilePath;
    Mutex* m_mutexes[MutexKindMax];
};
}

#endif
