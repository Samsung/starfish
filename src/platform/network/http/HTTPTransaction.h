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
struct CurlMultiRequestData;

typedef int (*ProgressCallBack)(void* clientp, curl_off_t dltotal,
                                curl_off_t dlnow, curl_off_t ultotal,
                                curl_off_t ulnow);

typedef size_t (*Callback)(void* ptr, size_t size, size_t nmemb, void* data);

// Process-wide override of TLS certificate verification, toggled at runtime by
// CDP Security.setIgnoreCertificateErrors. When set, every new HTTPTransaction
// and every new wss: WebSocket skips verification regardless of the build/env
// defaults. Read where those connections are created (the IO thread, and
// the thread that runs the WebSocket's script), written on the main thread
// (CDP); implemented with an atomic. Free functions so callers (e.g. the CDP
// dispatcher) need not pull in the full HTTPTransaction class header chain.
void setGlobalIgnoreSSLVerify(bool b);
bool globalIgnoreSSLVerify();

// Whether a new TLS connection skips certificate verification: always in
// STARFISH_ENABLE_TEST and STARFISH_IGNORE_SSL_VERIFYPEER builds, otherwise
// when IGNORE_SSL_VERIFY is set or the CDP override above is on. Shared by
// HTTPTransaction and SocketLWS so that https: and wss: follow one rule.
bool shouldIgnoreSSLVerify();

class HTTPTransaction {
public:
    static std::unique_ptr<HTTPTransaction> create(
        CurlMultiRequestData* curlMultiRequestData)
    {
        return std::unique_ptr<HTTPTransaction>(
            new HTTPTransaction(curlMultiRequestData));
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
    // CURLINFO_REDIRECT_COUNT as of the last updateTransactionStatus(): the
    // Locations curl followed, plus its auth reloads and Expect retries.
    long redirectCount() const
    {
        return m_redirectCount;
    }
    void updateLastLocationIfNeeds(const std::string& header);
    void didReceiveHeader(const std::string& header);
    char* effectiveURL();
    // Stops sending and storing cookies for the rest of the redirect chain,
    // keeping what the hops so far stored. Call from the header callback
    // of the redirect response before curl follows it.
    void excludeCredentialsFromRemainingRedirects();
    // Leaves the conditional headers that revalidate a stored response out
    // of the requests that follow redirects. Call from the header callback
    // of the redirect response before curl follows it.
    void excludeValidatorsFromRemainingRedirects();

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

    void setUseHttp2(bool b)
    {
        m_useHttp2 = b;
    }

    bool useHttp2()
    {
        return m_useHttp2;
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
    HTTPTransaction(CurlMultiRequestData* curlMultiRequestData);
    void preprocess();
    void postprocess();
    void registerCurlHandlers();
    void startRequest();

    std::unique_ptr<HTTPRequest> m_httpRequest;
    std::unique_ptr<HTTPResponse> m_httpResponse;

    unsigned long m_timeout;
    CURL* m_curl;
    CURLcode m_res;
    long m_redirectCount;
    // The request headers of the transfer start() runs.
    struct curl_slist* m_headerList;

    CurlMultiRequestData* m_curlMultiRequestData;

    std::string m_proxyURL;

    ProgressCallBack m_procCB;
    void* m_procData;

    Callback m_writeHeaderCB;
    void* m_writeHeaderData;

    Callback m_writeCB;
    void* m_writeData;

    Callback m_uploadBufferDataCB;
    void* m_uploadData;

    // Master store snapshot seeded into a credentialed transfer's private
    // cookie engine; consumed by mergeTransferCookies.
    struct curl_slist* m_injectedCookies;
    bool m_credentialsExcluded;

    bool m_inPreflightRequest;
    bool m_isPreflightReqeustDone;
    bool m_useHttp2;

#ifdef STARFISH_ENABLE_TEST
    void printCurlRequestDump();
    bool m_enableLog;
#endif
    bool m_ignoreSSLVerify;
};
} // namespace Starfish

#endif
