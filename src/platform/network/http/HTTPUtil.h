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

#ifndef __StarFishHTTPUtil__
#define __StarFishHTTPUtil__

#include <string>
#include <unordered_map>

namespace StarFish {

class ScriptBindingInstance;

typedef std::unordered_map<std::string, std::string> HeaderMap;
typedef std::vector<char> EntityBody;

struct CacheControl {
    CacheControl()
        : noCache(false)
        , noStore(false)
        , mustRevalidate(false)
        , maxAge(0)
    {
    }

    bool noCache : 1;
    bool noStore : 1;
    bool mustRevalidate : 1;
    int64_t maxAge;
};
struct HTTPContentInfo {
    HTTPContentInfo()
        : contentLanguage()
        , contentLength(0)
        , contentType()
        , contentTransferEncoding()
    {
    }

    std::string contentLanguage;
    size_t contentLength;
    std::string contentType;
    std::string contentTransferEncoding;
};

struct HTTPFreshnessInfo {
    HTTPFreshnessInfo()
        : date(0)
        , age(0)
        , requestTime(0)
        , responseTime(0)
        , lastModified(0)
        , etag()
    {
    }

    int64_t date;
    int64_t age;
    int64_t requestTime;
    int64_t responseTime;
    int64_t lastModified;
    std::string etag;
};

class HTTPUtil {
public:
    static std::string tryToConvertToHeaderMapString(const std::string& header);
    static CacheControl parseCacheControl(std::string directives);
    static HTTPFreshnessInfo getHTTPFreshnessInfoFromHeaders(
        ScriptBindingInstance* instance, const HeaderMap& headers);
    static HTTPContentInfo getHTTPContentInfoFromHeaders(
        const HeaderMap& headers);
};
}

#endif
