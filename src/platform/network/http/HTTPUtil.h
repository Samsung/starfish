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
