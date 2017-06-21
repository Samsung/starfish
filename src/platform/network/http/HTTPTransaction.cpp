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

#include "StarFishConfig.h"
#include "HTTPHeaderMap.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPTransaction.h"
#include "platform/network/NetworkSharedResourceManager.h"

#ifdef STARFISH_TIZEN_WEARABLE
#include <net_connection.h>
#endif

namespace StarFish {

// trim from start (in place)
static inline void ltrim(std::string& s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace)))
                .base(),
            s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrimmed(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrimmed(std::string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trimmed(std::string s)
{
    trim(s);
    return s;
}

static void skipSpaces(const std::string& input, unsigned long int& startIndex)
{
    while (startIndex < input.length() && input[startIndex] == ' ') {
        ++startIndex;
    }
}

static std::vector<std::string> split(const std::string& s, char seperator)
{
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find(seperator, pos)) != std::string::npos) {
        std::string substring(s.substr(prev_pos, pos - prev_pos));
        output.push_back(substring);
        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word
    return output;
}

HTTPTransaction::HTTPTransaction()
    : m_httpRequest()
    , m_httpResponse()
    , m_timeout(0)
    , m_curl(nullptr)
    , m_res()
    , m_procCB(nullptr)
    , m_procData(nullptr)
    , m_writeHeaderCB(nullptr)
    , m_writeHeaderData(nullptr)
    , m_writeCB(nullptr)
    , m_writeData(nullptr)
{
}

HTTPTransaction::~HTTPTransaction()
{
}

void HTTPTransaction::start()
{
    CURLSH* curlsh =
        NetworkSharedResourceManager::getInstance()->curlShareHandle();
    m_curl = curl_easy_init();
    m_httpResponse = HTTPResponse::create();

    STARFISH_ASSERT(m_curl);
    STARFISH_ASSERT(curlsh);
    STARFISH_ASSERT(m_httpResponse);

    struct curl_slist* list = m_httpRequest->headers().generateCurlList();
    curl_easy_setopt(m_curl, CURLOPT_URL, m_httpRequest->url().data());
    curl_easy_setopt(m_curl, CURLOPT_SHARE, curlsh);

    if (NetworkSharedResourceManager::getInstance()->storeCookieFile()) {
        curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE,
                         NetworkSharedResourceManager::getInstance()
                             ->cookieJarFileName()
                             .data());
        curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR,
                         NetworkSharedResourceManager::getInstance()
                             ->cookieJarFileName()
                             .data());
    }

#ifdef STARFISH_ENABLE_NETWORK_TEST
    curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
#endif
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, list);

    curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 128);

    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);

    if (m_procCB) {
        curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, m_procCB);
    }

    if (m_procData) {
        curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, m_procData);
    }

    if (m_writeHeaderCB) {
        curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, m_writeHeaderCB);
    }

    if (m_writeHeaderData) {
        curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, m_writeHeaderData);
    }

    if (m_writeCB) {
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, m_writeCB);
    }

    if (m_writeData) {
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, m_writeData);
    }

#ifdef STARFISH_ENABLE_NETWORK_TEST
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
    if (m_httpRequest->method().compare("POST") == 0) {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS,
                         m_httpRequest->entityBody().data());
    } else if (!(m_httpRequest->method().compare("GET") == 0)) {
        STARFISH_ASSERT_NOT_REACHED();
    }

#ifdef STARFISH_TIZEN_WEARABLE
    connection_h connection;
    int conn_err;
    conn_err = connection_create(&connection);
    char* proxy_address = NULL;
    if (conn_err == CONNECTION_ERROR_NONE) {
        connection_get_proxy(connection, CONNECTION_ADDRESS_FAMILY_IPV4,
                             &proxy_address);
        if (proxy_address) {
            curl_easy_setopt(m_curl, CURLOPT_PROXY, proxy_address);
            free(proxy_address);
        }
        connection_destroy(connection);
    } else {
        STARFISH_LOG_INFO("got error while opening tizen network connection\n");
    }
#endif

#ifdef STARFISH_ENABLE_NETWORK_TEST
    STARFISH_LOG_INFO("sending network request to %s\n",
                      m_httpRequest->url().data());
#endif
    m_res = curl_easy_perform(m_curl);

    updateTransactionStatus();

    // TODO : reuse curl for persistant conntection
    curl_easy_cleanup(m_curl);
    curl_slist_free_all(list);
    m_curl = nullptr;
}

void HTTPTransaction::didReceiveHeader(const std::string& header)
{
    size_t pos = header.find(":");
    if (pos != std::string::npos) {
        std::string key = header.substr(0, pos);
        std::string value = header.substr(pos + 1);

        trim(key);
        trim(value);

        m_httpResponse->headers().setHeader(key, value);
    }
}

void HTTPTransaction::updateTransactionStatus()
{
    if (m_curl) {
        long responseCode;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
        m_httpResponse->setResponseCode(responseCode);
    }
    // TODO : Implement additional state management
}
}
