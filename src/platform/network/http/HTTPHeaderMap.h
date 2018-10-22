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

#ifndef __StarFishHTTPHeaderMap__
#define __StarFishHTTPHeaderMap__

#include "HTTPUtil.h"
struct curl_slist;

namespace StarFish {

#define FOR_EACH_HTTPHEADERS(F)                                       \
    F(kCacheControl, "Cache-Control")                                 \
    F(kConnection, "Connection")                                      \
    F(kContentTransferEncoding, "Content-Transfer-Encoding")          \
    F(kContentLanguage, "Content-Language")                           \
    F(kContentLength, "Content-Length")                               \
    F(kContentType, "Content-type")                                   \
    F(kContentDispoition, "Content-Disposition")                      \
    F(kDNT, "DNT")                                                    \
    F(kDate, "Date")                                                  \
    F(kETag, "ETag")                                                  \
    F(kPragma, "Pragma")                                              \
    F(kTrailer, "Trailer")                                            \
    F(kTransferEncoding, "Transfer-Encoding")                         \
    F(kUpgrade, "Upgrade")                                            \
    F(kVia, "Via")                                                    \
    F(kWarning, "Warning")                                            \
    F(kAccept, "Accept")                                              \
    F(kAcceptCharset, "Accept-Charset")                               \
    F(kAcceptEncoding, "Accept-Encoding")                             \
    F(kAcceptLanguage, "Accept-Language")                             \
    F(kAuthorization, "Authorization")                                \
    F(kAge, "Age")                                                    \
    F(kCookie, "Cookie")                                              \
    F(kCookie2, "Cookie2")                                            \
    F(kSetCookie, "Set-Cookie")                                       \
    F(kSetCookie2, "Set-Cookie2")                                     \
    F(kExpect, "Expect")                                              \
    F(kFrom, "From")                                                  \
    F(kHost, "Host")                                                  \
    F(kIfMatch, "If-Match")                                           \
    F(kIfModifiedSince, "If-Modified-Since")                          \
    F(kIfNoneMatch, "If-None-Match")                                  \
    F(kIfRange, "If-Range")                                           \
    F(kIfUnmodifiedSince, "If-Unmodified-Since")                      \
    F(kLastModified, "Last-Modified")                                 \
    F(kMaxForwards, "Max-Forwards")                                   \
    F(kOrigin, "Origin")                                              \
    F(kProxyAuthorization, "Proxy-Authorization")                     \
    F(kRange, "Range")                                                \
    F(kReferer, "Referer")                                            \
    F(kReferrerPolicy, "Referrer-Policy")                             \
    F(kTE, "TE")                                                      \
    F(kUserAgent, "User-Agent")                                       \
    F(kLocation, "Location")                                          \
    F(kKeepAlive, "Keep-Alive")                                       \
    F(kUpgradeInsecureRequests, "Upgrade-Insecure-Requests")          \
    F(kAccessControlRequestHeaders, "Access-Control-Request-Headers") \
    F(kAccessControlRequestMethod, "Access-Control-Request-Method")

class HTTPHeaderMap {
public:
#define DECLARE_HEADERS(name, ...) static const char name[];
    FOR_EACH_HTTPHEADERS(DECLARE_HEADERS);
#undef DECLARE_HEADERS

    HTTPHeaderMap();
    ~HTTPHeaderMap();
    HTTPHeaderMap(const HTTPHeaderMap& rhs);
    HTTPHeaderMap& operator=(const HTTPHeaderMap&);

    HeaderMap& headerMap()
    {
        return m_headerMap;
    }

    // map interface
    unsigned long length();
    HeaderMap::iterator findHeader(const std::string& key);
    HeaderMap::const_iterator findHeader(const std::string& key) const;

    void setHeader(const std::string& key, const std::string& value);
    void removeHeader(const std::string& key);
    void clear();

    struct curl_slist* generateCurlList();

#ifdef STARFISH_ENABLE_TEST
    void dump();
#endif

private:
    HeaderMap m_headerMap;
};
}
#endif
