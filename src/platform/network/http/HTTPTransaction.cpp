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
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/modules/profiling/Profiling.h"

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
    , m_httpResponse(HTTPResponse::create())
    , m_timeout(0)
    , m_curl(nullptr)
    , m_res(CURLE_OK)
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
    CurlHandleData cd =
        NetworkSharedResourceManager::getInstance()->getCurlHandleData(
            m_httpRequest->baseURL());

    m_curl = cd.curl;

    STARFISH_ASSERT(m_curl);
    STARFISH_ASSERT(curlsh);

#ifdef STARFISH_ENABLE_TEST
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    const char* verbose = getenv("NETWORK_LOG_VERBOSE");
    bool enableLog = false;
    if (verbose && strlen(verbose)) {
        enableLog = true;
    }
    if (enableLog) {
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
    }
#endif

    struct curl_slist* list = m_httpRequest->headers().generateCurlList();
    curl_easy_setopt(m_curl, CURLOPT_URL, m_httpRequest->url().data());
    curl_easy_setopt(m_curl, CURLOPT_SHARE, curlsh);

    if (NetworkSharedResourceManager::getInstance()
            ->cookieStoreFilePath()
            .compare("") != 0) {
        curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR,
                         NetworkSharedResourceManager::getInstance()
                             ->cookieStoreFilePath()
                             .data());
    }
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, list);

    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, 1L);
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

    if (m_httpRequest->method().compare("POST") == 0) {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE,
                         m_httpRequest->entityBody().length());
        curl_easy_setopt(m_curl, CURLOPT_COPYPOSTFIELDS,
                         m_httpRequest->entityBody().data());
#ifdef STARFISH_ENABLE_TEST
        if (enableLog) {
            STARFISH_LOG_INFO("POST FIELDS\n");
            STARFISH_LOG_INFO("%s\n", m_httpRequest->entityBody().data());
        }
#endif
    } else if (!(m_httpRequest->method().compare("GET") == 0)) {
        // Do not need to set bodyentity for get
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

    m_httpRequest->setRequestTime(timestamp() / 1000);
    m_res = curl_easy_perform(m_curl);
    m_httpResponse->setResponseTime(timestamp() / 1000);

    updateTransactionStatus();

    char* LastEffectiveURL = nullptr;
    curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &LastEffectiveURL);
    if (LastEffectiveURL) {
        m_httpResponse->setLastEffectiveURL(LastEffectiveURL);
        // Do not free LastEffectiveURL
    }

    m_curl = nullptr;

    NetworkSharedResourceManager::getInstance()->cachingCurlHandleData(
        m_httpRequest->baseURL(), cd);
    curl_slist_free_all(list);
}

void HTTPTransaction::didReceiveHeader(const std::string& header)
{
    size_t pos = header.find(":");
    if (pos != std::string::npos) {
        std::string key = header.substr(0, pos);
        std::string value = header.substr(pos + 1);

        trim(key);
        trim(value);

        std::string converted =
            HTTPHeaderMap::tryToConvertToHeaderMapString(key);
        m_httpResponse->headers().setHeader(converted, value);
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
