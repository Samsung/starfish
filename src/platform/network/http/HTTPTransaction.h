/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTTPTransaction__
#define __StarFishHTTPTransaction__

#include <curl/curl.h>
#include "core/util/String.h"

namespace StarFish {

class HTTPRequest;
typedef int (*ProgressCallBack)(void* clientp, curl_off_t dltotal,
                                curl_off_t dlnow, curl_off_t ultotal,
                                curl_off_t ulnow);
typedef size_t (*WriteHeaderCallback)(void* ptr, size_t size, size_t nmemb,
                                      void* data);
typedef size_t (*WriteCallback)(void* ptr, size_t size, size_t nmemb,
                                void* data);

class HTTPTransaction : public gc {
public:
    static HTTPTransaction* create(HTTPRequest* request,
                                   unsigned long timeout = 0)
    {
        return new HTTPTransaction(request, timeout);
    }

    void start();

    void abort()
    {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    CURLcode res()
    {
        return m_res;
    }

    long responseCode();

    void setProgressCallbackAndData(ProgressCallBack cb, void* data = nullptr)
    {
        m_procCB = cb;
        m_procData = data;
    }

    void setWriteHeaderCallbackAndData(WriteHeaderCallback cb,
                                       void* data = nullptr)
    {
        m_writeHeaderCB = cb;
        m_writeHeaderData = data;
    }

    void setWriteCallbackAndData(WriteCallback cb, void* data = nullptr)
    {
        m_writeCB = cb;
        m_writeData = data;
    }

private:
    HTTPTransaction(HTTPRequest* request, unsigned long timeout);
    ~HTTPTransaction();

    HTTPRequest* m_httpRequest;
    unsigned long m_timeout;
    CURL* m_curl;
    CURLcode m_res;
    long m_response_code;

    // progress
    ProgressCallBack m_procCB;
    void* m_procData;

    // writeheader
    WriteHeaderCallback m_writeHeaderCB;
    void* m_writeHeaderData;

    // write
    WriteCallback m_writeCB;
    void* m_writeData;
};
}

#endif
