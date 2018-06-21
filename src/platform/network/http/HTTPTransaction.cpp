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

#include "StarFishConfig.h"
#include "HTTPHeaderMap.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPTransaction.h"
#include "HTTPUtil.h"
#include "platform/network/NetworkSharedResourceManager.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/modules/profiling/Profiling.h"

namespace StarFish {

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

#ifdef STARFISH_IGNORE_SSL_VERIFYPEER
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
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
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);

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

    if (m_proxyURL.size()) {
        curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxyURL.data());
    }

    // Enable all encoding (zlib, gzip)
    curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");

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

    m_httpRequest->setRequestTime(timestamp() / 1000);
    m_res = curl_easy_perform(m_curl);
    m_httpResponse->setResponseTime(timestamp() / 1000);

    updateTransactionStatus();

    char* LastEffectiveURL = nullptr;
    curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &LastEffectiveURL);

#ifdef STARFISH_ENABLE_TEST
    if (m_res == CURLE_OK && enableLog) {
        STARFISH_LOG_INFO("==============Dump Request info==============\n");
        STARFISH_LOG_INFO("[Request url : %s]\n", m_httpRequest->url().data());
        double val = 0.0;
        CURLcode res;
        res = curl_easy_getinfo(m_curl, CURLINFO_SIZE_DOWNLOAD, &val);
        if ((CURLE_OK == res) && (val > 0)) {
            STARFISH_LOG_INFO("[Data downloaded: %.0fbytes]\n", val);
        }

        res = curl_easy_getinfo(m_curl, CURLINFO_TOTAL_TIME, &val);
        if ((CURLE_OK == res) && (val > 0)) {
            STARFISH_LOG_INFO("[Total download time: %.5f sec]\n", val);
        }

        res = curl_easy_getinfo(m_curl, CURLINFO_SPEED_DOWNLOAD, &val);
        if ((CURLE_OK == res) && (val > 0)) {
            STARFISH_LOG_INFO("[Average download speed: %.0f kbyte/sec]\n",
                              val / 1024);
        }

        res = curl_easy_getinfo(m_curl, CURLINFO_NAMELOOKUP_TIME, &val);
        if ((CURLE_OK == res) && (val > 0)) {
            STARFISH_LOG_INFO("[Name lookup time: %.5f sec]\n", val);
        }

        res = curl_easy_getinfo(m_curl, CURLINFO_CONNECT_TIME, &val);
        if ((CURLE_OK == res) && (val > 0)) {
            STARFISH_LOG_INFO("[Connect time: %.5f sec]\n", val);
        }
        STARFISH_LOG_INFO("=============================================\n");
    }
#endif

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

        StringUtils::trim(key);
        StringUtils::trim(value);

        std::string converted = HTTPUtil::tryToConvertToHeaderMapString(key);
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
