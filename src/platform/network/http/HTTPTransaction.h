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

#ifndef __StarFishHTTPTransaction__
#define __StarFishHTTPTransaction__

#include <curl/curl.h>

namespace StarFish {

class HTTPRequest;
class HTTPResponse;

typedef int (*ProgressCallBack)(void* clientp, curl_off_t dltotal,
                                curl_off_t dlnow, curl_off_t ultotal,
                                curl_off_t ulnow);
typedef size_t (*WriteHeaderCallback)(void* ptr, size_t size, size_t nmemb,
                                      void* data);
typedef size_t (*WriteCallback)(void* ptr, size_t size, size_t nmemb,
                                void* data);

class HTTPTransaction {
public:
    static std::unique_ptr<HTTPTransaction> create()
    {
        return std::unique_ptr<HTTPTransaction>(new HTTPTransaction());
    }

    ~HTTPTransaction();

    // Transaction interface
    void start();
    void abort()
    {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    void setHTTPRequest(std::unique_ptr<HTTPRequest> httpRequest)
    {
        m_httpRequest = std::move(httpRequest);
    }

    HTTPRequest& httpRequest()
    {
        return *m_httpRequest;
    }
    HTTPResponse& httpResponse()
    {
        return *m_httpResponse;
    }
    void updateTransactionStatus();
    void didReceiveHeader(const std::string& header);

    void setTimeout(const long timeout)
    {
        m_timeout = timeout;
    }

    CURLcode res()
    {
        return m_res;
    }

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
    HTTPTransaction();

    std::unique_ptr<HTTPRequest> m_httpRequest;
    std::unique_ptr<HTTPResponse> m_httpResponse;

    unsigned long m_timeout;
    CURL* m_curl;
    CURLcode m_res;

    // progress
    ProgressCallBack m_procCB;
    void* m_procData; // NetworkURLWorkerData

    // writeheader
    WriteHeaderCallback m_writeHeaderCB;
    void* m_writeHeaderData; // ResourceRequest

    // write
    WriteCallback m_writeCB;
    void* m_writeData; // NetworkURLWorkerData
};
}

#endif
