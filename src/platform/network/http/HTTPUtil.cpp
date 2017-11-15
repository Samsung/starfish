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
#include "HTTPUtil.h"
#include "HTTPHeaderMap.h"
#include "binding/ScriptWrappable.h"

#include <time.h>
#include <algorithm>

#define HTTP_DATE_FORMAT "%a, %d %m %Y %H:%M:%S %Z"

namespace StarFish {

std::string HTTPUtil::tryToConvertToHeaderMapString(const std::string& header)
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
        } else if (lower.compare("last-modified") == 0) {
            ret = HTTPHeaderMap::kLastModified;
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

CacheControl HTTPUtil::parseCacheControl(std::string directives)
{
    // https://tools.ietf.org/html/rfc7234#page-21
    // See 5.2, 5.2.1, 5.2.2

    String* str = String::fromUTF8(directives.data());

    GCVector<StringView> tokens;
    StringUtils::tokenize(str, ",", 1, tokens);

    CacheControl cc;
    for (auto directiveView : tokens) {
        String* directive = directiveView.trim();

        size_t pos = directive->find("=");

        if (pos != SIZE_MAX) {
            String* key = directive->substring(0, pos)->trim();
            String* value =
                directive->substring(pos + 1, directive->length() - pos - 1)
                    ->trim();

            if (key->equals("max-age")) {
                cc.maxAge = String::parseInt64(value);
            }
        } else {
            if (directive->equals("no-cache")) {
                cc.noCache = true;
            } else if (directive->equals("no-store")) {
                cc.noStore = true;
            } else if (directive->equals("must-revalidate")) {
                cc.mustRevalidate = true;
            }
        }
    }
    return cc;
}

HTTPFreshnessInfo HTTPUtil::getHTTPFreshnessInfoFromHeaders(
    ScriptBindingInstance* instance, const HeaderMap& headers)
{
    HTTPFreshnessInfo info;
    auto it = headers.find(HTTPHeaderMap::kContentLength);
    if (it != headers.end()) {
        String* value = String::createASCIIString(it->second.data());
        info.contentLength = String::parseInt64(value);
    }

    it = headers.find(HTTPHeaderMap::kDate);
    if (it != headers.end()) {
        String* value = String::createASCIIString(it->second.data());
        double parsedDate = parseDate(instance, value);
        if (!std::isnan(parsedDate)) {
            info.date = parsedDate / 1000.0;
        }
    }

    it = headers.find(HTTPHeaderMap::kAge);
    if (it != headers.end()) {
        String* value = String::createASCIIString(it->second.data());
        info.age = String::parseInt64(value);
    }

    it = headers.find(HTTPHeaderMap::kLastModified);
    if (it != headers.end()) {
        String* value = String::createASCIIString(it->second.data());
        double parsedDate = parseDate(instance, value);
        if (!std::isnan(parsedDate)) {
            info.lastModified = parsedDate / 1000.0;
        }
        String* utc = timeToUTCString(instance, info.lastModified * 1000);
    }
    return info;
}
}
