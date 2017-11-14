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

namespace StarFish {

const char HTTPHeaderMap::kCacheControl[] = "Cache-Control";
const char HTTPHeaderMap::kConnection[] = "Connection";
const char HTTPHeaderMap::kContentTransferEncoding[] =
    "Content-Transfer-Encoding";
const char HTTPHeaderMap::kContentLanguage[] = "Content-Language";
const char HTTPHeaderMap::kContentLength[] = "Content-Length";
const char HTTPHeaderMap::kContentType[] = "Content-type";
const char HTTPHeaderMap::kDate[] = "Date";
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

std::string HTTPHeaderMap::tryToConvertToHeaderMapString(
    const std::string& header)
{
    size_t len = header.length();
    std::string ret = header;
    std::string lower;
    lower.resize(header.length());
    std::transform(header.begin(), header.end(), lower.begin(), ::tolower);

    switch (len) {
    case 2:
        if (lower.compare("te") == 0) {
            ret = HTTPHeaderMap::kTE;
        }
        break;
    case 3:
        if (lower.compare("via") == 0) {
            ret = HTTPHeaderMap::kVia;
        } else if (lower.compare("age") == 0) {
            ret = HTTPHeaderMap::kAge;
        }
        break;
    case 4:
        if (lower.compare("date") == 0) {
            ret = HTTPHeaderMap::kDate;
        } else if (lower.compare("from") == 0) {
            ret = HTTPHeaderMap::kFrom;
        } else if (lower.compare("host") == 0) {
            ret = HTTPHeaderMap::kHost;
        }
        break;
    case 5:
        if (lower.compare("range") == 0) {
            ret = HTTPHeaderMap::kRange;
        }
        break;
    case 6:
        if (lower.compare("origin") == 0) {
            ret = HTTPHeaderMap::kOrigin;
        } else if (lower.compare("pragma") == 0) {
            ret = HTTPHeaderMap::kPragma;
        } else if (lower.compare("accept") == 0) {
            ret = HTTPHeaderMap::kAccept;
        } else if (lower.compare("cookie") == 0) {
            ret = HTTPHeaderMap::kCookie;
        } else if (lower.compare("expect") == 0) {
            ret = HTTPHeaderMap::kExpect;
        }
        break;
    case 7:
        if (lower.compare("warning") == 0) {
            ret = HTTPHeaderMap::kWarning;
        } else if (lower.compare("upgrade") == 0) {
            ret = HTTPHeaderMap::kUpgrade;
        } else if (lower.compare("trailer") == 0) {
            ret = HTTPHeaderMap::kTrailer;
        } else if (lower.compare("referer") == 0) {
            ret = HTTPHeaderMap::kReferer;
        }
        break;
    case 8:
        if (lower.compare("location") == 0) {
            ret = HTTPHeaderMap::kLocation;
        } else if (lower.compare("if-range") == 0) {
            ret = HTTPHeaderMap::kIfRange;
        } else if (lower.compare("if-match") == 0) {
            ret = HTTPHeaderMap::kIfMatch;
        }
        break;
    case 10:
        if (lower.compare("user-agent") == 0) {
            ret = HTTPHeaderMap::kUserAgent;
        } else if (lower.compare("set-cookie") == 0) {
            ret = HTTPHeaderMap::kSetCookie;
        }
        break;
    case 11:
        if (lower.compare("set-cookie2") == 0) {
            ret = HTTPHeaderMap::kSetCookie2;
        }
        break;
    case 12:
        if (lower.compare("max-forwards") == 0) {
            ret = HTTPHeaderMap::kMaxForwards;
        } else if (lower.compare("content-type") == 0) {
            ret = HTTPHeaderMap::kContentType;
        }
        break;
    case 13:
        if (lower.compare("if-none-match") == 0) {
            ret = HTTPHeaderMap::kIfNoneMatch;
        } else if (lower.compare("cache-control") == 0) {
            ret = HTTPHeaderMap::kCacheControl;
        } else if (lower.compare("authorization") == 0) {
            ret = HTTPHeaderMap::kAuthorization;
        }
        break;
    case 14:
        if (lower.compare("accept-charset") == 0) {
            ret = HTTPHeaderMap::kAcceptCharset;
        } else if (lower.compare("content-length") == 0) {
            ret = HTTPHeaderMap::kContentLength;
        }
        break;
    case 15:
        if (lower.compare("accept-encoding") == 0) {
            ret = HTTPHeaderMap::kAcceptEncoding;
        } else if (lower.compare("accept-language") == 0) {
            ret = HTTPHeaderMap::kAcceptLanguage;
        }
        break;
    case 16:
        if (lower.compare("content-language") == 0) {
            ret = HTTPHeaderMap::kContentLanguage;
        }
        break;
    case 17:
        if (lower.compare("if-modified-since") == 0) {
            ret = HTTPHeaderMap::kIfModifiedSince;
        } else if (lower.compare("transfer-encoding") == 0) {
            ret = HTTPHeaderMap::kTransferEncoding;
        }
        break;
    case 19:
        if (lower.compare("if-unmodified-since") == 0) {
            ret = HTTPHeaderMap::kIfUnmodifiedSince;
        } else if (lower.compare("proxy-authorization") == 0) {
            ret = HTTPHeaderMap::kProxyAuthorization;
        }
        break;
    case 25:
        if (lower.compare("upgrade-insecure-requests") == 0) {
            ret = HTTPHeaderMap::kUpgradeInsecureRequests;
        } else if (lower.compare("content-transfer-encoding") == 0) {
            ret = HTTPHeaderMap::kContentTransferEncoding;
        }
        break;
    default:
        break;
    }
    return ret;
}

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

HTTPHeaderMap::HeaderMap::iterator HTTPHeaderMap::findHeader(
    const std::string& key)
{
    HeaderMap::iterator it = m_headerMap.find(key);
    if (it == m_headerMap.end()) {
        return m_headerMap.end();
    }
    return it;
}

HTTPHeaderMap::HeaderMap::const_iterator HTTPHeaderMap::findHeader(
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
