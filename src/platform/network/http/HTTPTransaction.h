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

    void setProxyURL(const std::string& url)
    {
        m_proxyURL = url;
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

    // proxy
    std::string m_proxyURL;

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
