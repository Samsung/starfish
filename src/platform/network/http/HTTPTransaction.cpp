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

#include "StarfishConfig.h"
#include "HTTPHeaderMap.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPTransaction.h"
#include "HTTPUtil.h"
#include "platform/network/curl/NetworkSharedResourceManager.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/profiling/Profiling.h"

namespace Starfish {

HTTPTransaction::HTTPTransaction(CurlMultiRequestData* curlMultiRequestData)
    : m_httpRequest()
    , m_timeout(0)
    , m_curl(nullptr)
    , m_res(CURLE_OK)
    , m_curlMultiRequestData(curlMultiRequestData)
    , m_procCB(nullptr)
    , m_procData(nullptr)
    , m_writeHeaderCB(nullptr)
    , m_writeHeaderData(nullptr)
    , m_writeCB(nullptr)
    , m_writeData(nullptr)
    , m_uploadBufferDataCB(nullptr)
    , m_uploadData(nullptr)
    , m_inPreflightRequest(false)
    , m_isPreflightReqeustDone(false)
    , m_useHttp2(false)
#ifdef STARFISH_ENABLE_TEST
    , m_enableLog(false)
#endif
{
}

HTTPTransaction::~HTTPTransaction()
{
}

void HTTPTransaction::preprocess()
{
    m_httpResponse.reset(new HTTPResponse());

    CurlHandleData cd =
        NetworkSharedResourceManager::getInstance()->getCurlHandleData(
            m_httpRequest->baseURL());
    m_curl = cd.curl;

#ifdef STARFISH_ENABLE_TEST
    const char* verbose = getenv("NETWORK_LOG_VERBOSE");
    if (verbose && strlen(verbose)) {
        m_enableLog = true;
    } else {
        m_enableLog = false;
    }
    if (m_enableLog) {
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
    }
#endif

#if defined(STARFISH_IGNORE_SSL_VERIFYPEER) || defined(STARFISH_ENABLE_TEST)
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
#if defined(STARFISH_ANDROID)
    STARFISH_ASSERT(getenv("STARFISH_CURL_CA_BUNDLE"));
    curl_easy_setopt(m_curl, CURLOPT_CAINFO, getenv("STARFISH_CURL_CA_BUNDLE"));
#endif
    if (m_useHttp2) {
        curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
    } else {
        curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    }
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
    if (m_timeout) {
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, m_timeout);
    } else {
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS,
                         10 * 60 * 1000); // default timeout is 10min
    }

    curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt(m_curl, CURLOPT_BUFFERSIZE, 1024 * 64);
#ifdef CURLOPT_UPLOAD_BUFFERSIZE
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, 1024 * 64);
#endif

    if (m_proxyURL.size()) {
        curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxyURL.data());
    }

    // Enable all encoding (zlib, gzip)
    curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");

    if (m_curlMultiRequestData) {
#ifndef CURLPIPE_MULTIPLEX
/* This little trick will just make sure that we don't enable pipelining for
   libcurls old enough to not have this symbol. It is _not_ defined to zero in
   a recent libcurl header. */
#define CURLPIPE_MULTIPLEX 0
#endif
#if (CURLPIPE_MULTIPLEX > 0)
        /* wait for pipe connection to confirm */
        curl_easy_setopt(m_curl, CURLOPT_PIPEWAIT, 1L);
#endif
    }

    registerCurlHandlers();

    STARFISH_ASSERT(m_curl);
}

void HTTPTransaction::postprocess()
{
    STARFISH_ASSERT(m_curl);

#ifdef STARFISH_ENABLE_TEST
    printCurlRequestDump();
#endif

    CurlHandleData cd = { m_curl, 0 };
    NetworkSharedResourceManager::getInstance()->cachingCurlHandleData(
        m_httpRequest->baseURL(), cd);

    m_curl = nullptr;
}

void HTTPTransaction::start()
{
    bool includeCredentials = m_httpRequest->includeCredentials();

    preprocess();

#if defined(STARFISH_IGNORE_SSL_VERIFYPEER) || defined(STARFISH_ENABLE_TEST)
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
#if defined(STARFISH_ANDROID)
    STARFISH_ASSERT(getenv("STARFISH_CURL_CA_BUNDLE"));
    curl_easy_setopt(m_curl, CURLOPT_CAINFO, getenv("STARFISH_CURL_CA_BUNDLE"));
#endif
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 128);
    curl_easy_setopt(m_curl, CURLOPT_URL, m_httpRequest->url().data());
    if (includeCredentials) {
        curl_easy_setopt(
            m_curl, CURLOPT_SHARE,
            NetworkSharedResourceManager::getInstance()->curlShareHandle());
        if (NetworkSharedResourceManager::getInstance()
                ->cookieStoreFilePath()
                .compare("") != 0) {
            curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR,
                             NetworkSharedResourceManager::getInstance()
                                 ->cookieStoreFilePath()
                                 .data());
            curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE,
                             NetworkSharedResourceManager::getInstance()
                                 ->cookieStoreFilePath()
                                 .data());
        }
    } else {
        curl_easy_setopt(m_curl, CURLOPT_SHARE,
                         NetworkSharedResourceManager::getInstance()
                             ->curlNonCookieShareHandle());
    }

    struct curl_slist* list = m_httpRequest->headers().generateCurlList();
    // Disable Expect: 100-contiune
    // https://gms.tf/when-curl-sends-100-continue.html
    if (m_httpRequest->method().compare("POST") == 0 ||
        m_httpRequest->method().compare("PUT") == 0) {
        if (m_httpRequest->headers().find(HTTPHeaderMap::kExpect) ==
            m_httpRequest->headers().headerMap().end()) {
            list = curl_slist_append(list, "Expect:");
        }
    }
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, list);

    if (m_httpRequest->method().compare("POST") == 0) {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE,
                         m_httpRequest->entityBody().length());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS,
                         m_httpRequest->entityBody().data());
    } else if (m_httpRequest->method().compare("GET") == 0) {
        curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
    } else if (m_httpRequest->method().compare("HEAD") == 0) {
        curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
    } else if (m_httpRequest->method().compare("PUT") == 0) {
        curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(m_curl, CURLOPT_INFILESIZE_LARGE,
                         m_httpRequest->entityBody().length());
    } else if (m_httpRequest->method().compare("PATCH") == 0) {
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE,
                         m_httpRequest->entityBody().length());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS,
                         m_httpRequest->entityBody().data());
    } else {
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST,
                         m_httpRequest->method().c_str());
    }
    m_httpRequest->setRequestTime(timestamp() / 1000);

    startRequest();

#if defined(STARFISH_IGNORE_SSL_VERIFYPEER) || defined(STARFISH_ENABLE_TEST)
    if (m_res == CURLE_RECV_ERROR) {
        // when gives CURLOPT_SSL_VERIFYHOST to curl,
        // we got CURLE_RECV_ERROR but connection was successful
        m_res = CURLE_OK;
    }
#endif
    m_httpResponse->setResponseTime(timestamp() / 1000);

    updateTransactionStatus();
    curl_slist_free_all(list);
    postprocess();
}

void HTTPTransaction::startPreFlightRequest()
{
    preprocess();

    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 0);
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    curl_easy_setopt(m_curl, CURLOPT_URL, m_httpRequest->url().data());
    curl_easy_setopt(m_curl, CURLOPT_SHARE,
                     NetworkSharedResourceManager::getInstance()
                         ->curlNonCookieShareHandle());
    curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR, "");
    curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");

    curl_slist* list =
        m_httpRequest->headers().generateCurlListToPreflightRequest();

    std::string accessControlRequestMethod(
        HTTPHeaderMap::kAccessControlRequestMethod);
    accessControlRequestMethod.append(": ");
    accessControlRequestMethod.append(m_httpRequest->method());
    list = curl_slist_append(list, accessControlRequestMethod.data());

    auto accessControlRequestHeaders =
        m_httpRequest->headers().generateAccessControlRequestHeaders();
    list = curl_slist_append(list, accessControlRequestHeaders.data());

    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, list);

    m_inPreflightRequest = true;

    startRequest();

    m_inPreflightRequest = false;
    m_isPreflightReqeustDone = true;

    updateTransactionStatus();

    curl_slist_free_all(list);
    list = nullptr;

    postprocess();
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
        m_httpResponse->headers().append(converted, value);
    }
}

void HTTPTransaction::updateTransactionStatus()
{
    if (m_curl) {
        long responseCode;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
        m_httpResponse->setResponseCode(responseCode);
    }
}

void HTTPTransaction::updateLastLocationIfNeeds(const std::string& header)
{
    size_t pos = header.find(":");
    if (pos != std::string::npos) {
        std::string key = header.substr(0, pos);
        std::string value = header.substr(pos + 1);

        StringUtils::trim(key);
        StringUtils::trim(value);

        std::string converted = HTTPUtil::tryToConvertToHeaderMapString(key);
        if (converted == HTTPHeaderMap::kLocation) {
            m_httpResponse->setLastLocation(value);
        }
    }
}

char* HTTPTransaction::effectiveURL()
{
    char* effectiveURL = nullptr;
    if (m_curl) {
        curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &effectiveURL);
    }
    return effectiveURL;
}

void HTTPTransaction::registerCurlHandlers()
{
    if (m_proxyURL.size()) {
        curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxyURL.data());
    }

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

    if (m_uploadBufferDataCB) {
        curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, m_uploadBufferDataCB);
    }

    if (m_uploadData) {
        curl_easy_setopt(m_curl, CURLOPT_READDATA, m_uploadData);
    }
}

void HTTPTransaction::startRequest()
{
    if (m_curlMultiRequestData) {
        m_curlMultiRequestData->m_mutex->lock();

        m_curlMultiRequestData->m_curl = m_curl;

        NetworkSharedResourceManager::getInstance()->appendPendingMultiRequest(
            httpRequest().baseURL(), m_curlMultiRequestData);

        // wait until end
        m_curlMultiRequestData->m_mutex->lock();
        m_res = m_curlMultiRequestData->m_result;
        m_curlMultiRequestData->m_mutex->unlock();
    } else {
        m_res = curl_easy_perform(m_curl);
    }
}

#ifdef STARFISH_ENABLE_TEST
void HTTPTransaction::printCurlRequestDump()
{
    if (m_enableLog) {
        STARFISH_LOG_INFO("==============Dump Request info==============\n");
        STARFISH_LOG_INFO("[Request url : %s]\n", m_httpRequest->url().data());
        double val = 0.0;
        CURLcode res;
        res = curl_easy_getinfo(m_curl, CURLINFO_SIZE_DOWNLOAD, &val);
        if ((CURLE_OK == res) && (val > 0)) {
            STARFISH_LOG_INFO("[Data downloaded: %.0fbytes]\n", val);
        }

        res = curl_easy_getinfo(m_curl, CURLINFO_SIZE_UPLOAD, &val);
        if ((CURLE_OK == res) && (val > 0)) {
            STARFISH_LOG_INFO("[Data uploaded: %.0fbytes]\n", val);
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
}
#endif
} // namespace Starfish
