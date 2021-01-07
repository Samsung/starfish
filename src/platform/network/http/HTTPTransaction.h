/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishHTTPTransaction__
#define __StarfishHTTPTransaction__

#include <curl/curl.h>

namespace Starfish {

class HTTPRequest;
class HTTPResponse;

typedef int (*ProgressCallBack)(void* clientp, curl_off_t dltotal,
                                curl_off_t dlnow, curl_off_t ultotal,
                                curl_off_t ulnow);

typedef size_t (*Callback)(void* ptr, size_t size, size_t nmemb, void* data);

class HTTPTransaction {
public:
    static std::unique_ptr<HTTPTransaction> create()
    {
        return std::unique_ptr<HTTPTransaction>(new HTTPTransaction());
    }

    ~HTTPTransaction();

    // Transaction interface
    void start();
    void startPreFlightRequest();

    void abort()
    {
        // TODO
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
    void updateLastLocationIfNeeds(const std::string& header);
    void didReceiveHeader(const std::string& header);
    char* effectiveURL();

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

    void setWriteHeaderCallbackAndData(Callback cb, void* data = nullptr)
    {
        m_writeHeaderCB = cb;
        m_writeHeaderData = data;
    }

    void setWriteCallbackAndData(Callback cb, void* data = nullptr)
    {
        m_writeCB = cb;
        m_writeData = data;
    }

    void setUploadBufferDataCallbackAndData(Callback cb, void* data = nullptr)
    {
        m_uploadBufferDataCB = cb;
        m_uploadData = data;
    }

    bool inPreflightRequest()
    {
        return m_inPreflightRequest;
    }

    bool isPreflightReqeustDone()
    {
        return m_isPreflightReqeustDone;
    }

private:
    HTTPTransaction();
    void preprocess(bool useNewHandle);
    void postprocess(bool useNewHandle);
    void registerCurlHandlers();

    std::unique_ptr<HTTPRequest> m_httpRequest;
    std::unique_ptr<HTTPResponse> m_httpResponse;

    unsigned long m_timeout;
    CURL* m_curl;
    CURLSH* m_curlsh;
    CURLcode m_res;

    std::string m_proxyURL;

    ProgressCallBack m_procCB;
    void* m_procData;

    Callback m_writeHeaderCB;
    void* m_writeHeaderData;

    Callback m_writeCB;
    void* m_writeData;

    Callback m_uploadBufferDataCB;
    void* m_uploadData;

    bool m_inPreflightRequest;
    bool m_isPreflightReqeustDone;

#ifdef STARFISH_ENABLE_TEST
    void printCurlRequestDump();
    bool m_enableLog;
#endif
};
} // namespace Starfish

#endif
