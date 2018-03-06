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
#include <curl/curl.h>
namespace StarFish {

const char HTTPHeaderMap::kCacheControl[] = "Cache-Control";
const char HTTPHeaderMap::kConnection[] = "Connection";
const char HTTPHeaderMap::kContentTransferEncoding[] =
    "Content-Transfer-Encoding";
const char HTTPHeaderMap::kContentLanguage[] = "Content-Language";
const char HTTPHeaderMap::kContentLength[] = "Content-Length";
const char HTTPHeaderMap::kContentType[] = "Content-type";
const char HTTPHeaderMap::kDate[] = "Date";
const char HTTPHeaderMap::kETag[] = "ETag";
const char HTTPHeaderMap::kPragma[] = "Pragma";
const char HTTPHeaderMap::kTrailer[] = "Trailer";
const char HTTPHeaderMap::kTransferEncoding[] = "Transfer-Encoding";
const char HTTPHeaderMap::kUpgrade[] = "Upgrade";
const char HTTPHeaderMap::kVia[] = "Via";
const char HTTPHeaderMap::kWarning[] = "Warning";
const char HTTPHeaderMap::kAccept[] = "Accept";
const char HTTPHeaderMap::kAcceptCharset[] = "Accept-Charset";
const char HTTPHeaderMap::kAcceptEncoding[] = "Accept-Encoding";
const char HTTPHeaderMap::kAcceptLanguage[] = "Accept-Language";
const char HTTPHeaderMap::kAuthorization[] = "Authorization";
const char HTTPHeaderMap::kAge[] = "Age";
const char HTTPHeaderMap::kCookie[] = "Cookie";
const char HTTPHeaderMap::kSetCookie[] = "Set-Cookie";
const char HTTPHeaderMap::kSetCookie2[] = "Set-Cookie2";
const char HTTPHeaderMap::kExpect[] = "Expect";
const char HTTPHeaderMap::kFrom[] = "From";
const char HTTPHeaderMap::kHost[] = "Host";
const char HTTPHeaderMap::kIfMatch[] = "If-Match";
const char HTTPHeaderMap::kIfModifiedSince[] = "If-Modified-Since";
const char HTTPHeaderMap::kIfNoneMatch[] = "If-None-Match";
const char HTTPHeaderMap::kIfRange[] = "If-Range";
const char HTTPHeaderMap::kIfUnmodifiedSince[] = "If-Unmodified-Since";
const char HTTPHeaderMap::kLastModified[] = "Last-Modified";
const char HTTPHeaderMap::kMaxForwards[] = "Max-Forwards";
const char HTTPHeaderMap::kOrigin[] = "Origin";
const char HTTPHeaderMap::kProxyAuthorization[] = "Proxy-Authorization";
const char HTTPHeaderMap::kRange[] = "Range";
const char HTTPHeaderMap::kReferer[] = "Referer";
const char HTTPHeaderMap::kTE[] = "TE";
const char HTTPHeaderMap::kUserAgent[] = "User-Agent";
const char HTTPHeaderMap::kLocation[] = "Location";
const char HTTPHeaderMap::kUpgradeInsecureRequests[] =
    "Upgrade-Insecure-Requests";

HTTPHeaderMap::HTTPHeaderMap()
{
}

HTTPHeaderMap::~HTTPHeaderMap()
{
}

HTTPHeaderMap::HTTPHeaderMap(const HTTPHeaderMap& rhs)
{
    *this = rhs;
}

HTTPHeaderMap& HTTPHeaderMap::operator=(const HTTPHeaderMap& rhs)
{
    if (this == &rhs) {
        return *this;
    }
    this->m_headerMap = rhs.m_headerMap;

    return *this;
}

unsigned long HTTPHeaderMap::length()
{
    return m_headerMap.size();
}

HeaderMap::iterator HTTPHeaderMap::findHeader(const std::string& key)
{
    HeaderMap::iterator it = m_headerMap.find(key);
    if (it == m_headerMap.end()) {
        return m_headerMap.end();
    }
    return it;
}

HeaderMap::const_iterator HTTPHeaderMap::findHeader(
    const std::string& key) const
{
    HeaderMap::const_iterator it = m_headerMap.find(key);
    if (it == m_headerMap.end()) {
        return m_headerMap.end();
    }
    return it;
}

void HTTPHeaderMap::setHeader(const std::string& key, const std::string& value)
{
    auto it = findHeader(key);
    if (it == m_headerMap.end()) {
        m_headerMap[key] = value;
    } else {
        m_headerMap[key] += ", " + value;
    }
}

void HTTPHeaderMap::removeHeader(const std::string& key)
{
    m_headerMap.erase(key);
}

void HTTPHeaderMap::clear()
{
    m_headerMap.clear();
}

struct curl_slist* HTTPHeaderMap::generateCurlList()
{
    curl_slist* list = nullptr;
    std::string header;
    for (auto it = m_headerMap.begin(); it != m_headerMap.end(); ++it) {
        header = std::string(it->first) + ": " + it->second;
        list = curl_slist_append(list, header.data());
        if (list == nullptr) {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }

    return list;
}

#ifdef STARFISH_ENABLE_TEST
void HTTPHeaderMap::dump()
{
    STARFISH_LOG_INFO("dump header map\n");
    for (auto it = m_headerMap.begin(); it != m_headerMap.end(); ++it) {
        STARFISH_LOG_INFO("key : %s\n", it->first.c_str());
        STARFISH_LOG_INFO("value : %s\n", it->second.c_str());
    }
}
#endif
}
