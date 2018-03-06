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

class HTTPHeaderMap {
public:
    static const char kCacheControl[];
    static const char kConnection[];
    static const char kContentTransferEncoding[];
    static const char kContentLanguage[];
    static const char kContentLength[];
    static const char kContentType[];
    static const char kDate[];
    static const char kETag[];
    static const char kPragma[];
    static const char kTrailer[];
    static const char kTransferEncoding[];
    static const char kUpgrade[];
    static const char kVia[];
    static const char kWarning[];
    static const char kAccept[];
    static const char kAcceptCharset[];
    static const char kAcceptEncoding[];
    static const char kAcceptLanguage[];
    static const char kAuthorization[];
    static const char kAge[];
    static const char kCookie[];
    static const char kSetCookie[];
    static const char kSetCookie2[];
    static const char kExpect[];
    static const char kFrom[];
    static const char kHost[];
    static const char kIfMatch[];
    static const char kIfModifiedSince[];
    static const char kIfNoneMatch[];
    static const char kIfRange[];
    static const char kIfUnmodifiedSince[];
    static const char kLastModified[];
    static const char kMaxForwards[];
    static const char kOrigin[];
    static const char kProxyAuthorization[];
    static const char kRange[];
    static const char kReferer[];
    static const char kTE[];
    static const char kUserAgent[];
    static const char kLocation[];
    static const char kUpgradeInsecureRequests[];

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
