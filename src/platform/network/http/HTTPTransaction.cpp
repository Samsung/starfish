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
#include "HTTPTransaction.h"
#include "HTTPRequest.h"
#include "HTTPHeaderList.h"

#ifdef STARFISH_TIZEN_WEARABLE
#include <net_connection.h>
#endif

namespace StarFish {

HTTPTransaction::HTTPTransaction(HTTPRequest* request, unsigned long timeout)
    : m_httpRequest(request)
    , m_timeout(timeout)
    , m_curl(nullptr)
    , m_res()
    , m_response_code(0)
    , m_procCB(nullptr)
    , m_procData(nullptr)
    , m_writeHeaderCB(nullptr)
    , m_writeHeaderData(nullptr)
    , m_writeCB(nullptr)
    , m_writeData(nullptr)
{
}

void HTTPTransaction::start()
{
    m_curl = curl_easy_init();
    STARFISH_ASSERT(m_curl);

    curl_easy_setopt(m_curl, CURLOPT_URL, m_httpRequest->url()->utf8Data());
    STARFISH_LOG_INFO("sending network request to %s\n",
                      m_httpRequest->url()->utf8Data());
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER,
                     m_httpRequest->headers()->unwrap());

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
        // curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, m_orgProxy);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, m_writeData);
    }

#ifdef STARFISH_ENABLE_TEST
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
    if (m_httpRequest->method()->equals("POST")) {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS,
                         m_httpRequest->body()->utf8Data());
    } else if (!m_httpRequest->method()->equals("GET")) {
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
    m_res = curl_easy_perform(m_curl);

    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_response_code);

    // TODO : reuse curl for persistant conntection
    curl_easy_cleanup(m_curl);
    m_curl = nullptr;
}

long HTTPTransaction::responseCode()
{
    if (m_curl) {
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_response_code);
    }
    return m_response_code;
}
}
