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
class Mutex;
class ResourceURL;
class String;

class NetworkSharedResourceManager {
public:
    static NetworkSharedResourceManager* getInstance();
    static void close();

    CURLSH* curlShareHandle() const; // Do not free
    std::string cookieJarFileName() const;
    void setCookieJarFileName(const std::string& name);
    Mutex* resourceMutex(curl_lock_data data);

    void enableToStoreCookiesJarAsFile();
    void disableToStoreCookiesJarAsFile();

    bool storeCookieFile() const
    {
        return m_storeCookieFile;
    }
    void initCookieSession();

    // for document.cookie
    String* cookeis(ResourceURL* url);
    void setCookies(Document* document, ResourceURL* url, String* value);

private:
    NetworkSharedResourceManager();
    ~NetworkSharedResourceManager();

    CURLSH* m_curlShareHandle;
    std::string m_cookieJarFileName;
    Mutex* m_cookieMutex;
    Mutex* m_dnsMutex;
    Mutex* m_shareMutex;
    bool m_storeCookieFile;
};
}

#endif
