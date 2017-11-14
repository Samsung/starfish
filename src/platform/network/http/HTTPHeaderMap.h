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

#ifndef __StarFishHTTPHeaderMap__
#define __StarFishHTTPHeaderMap__

#include <curl/curl.h>
#include <unordered_map>

namespace StarFish {

class HTTPHeaderMap {
public:
    typedef std::unordered_map<std::string, std::string> HeaderMap;

    static const char kCacheControl[];
    static const char kConnection[];
    static const char kContentTransferEncoding[];
    static const char kContentLanguage[];
    static const char kContentLength[];
    static const char kContentType[];
    static const char kDate[];
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
    static const char kMaxForwards[];
    static const char kOrigin[];
    static const char kProxyAuthorization[];
    static const char kRange[];
    static const char kReferer[];
    static const char kTE[];
    static const char kUserAgent[];
    static const char kLocation[];
    static const char kUpgradeInsecureRequests[];

    static std::string tryToConvertToHeaderMapString(const std::string& header);

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
