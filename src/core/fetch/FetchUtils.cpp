/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "FetchUtils.h"
#include "core/fetch/RequestData.h"
#include "core/modules/threading/Thread.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/extra/MimeType.h"

namespace Starfish {

bool FetchUtils::isForbiddenMethod(const String* method)
{
    // https://fetch.spec.whatwg.org/#methods
    if (method->equalsIgnoreCase("CONNECT") ||
        method->equalsIgnoreCase("TRACE") ||
        method->equalsIgnoreCase("TRACK")) {
        return true;
    }
    return false;
}

String* FetchUtils::normalizeMethod(String* method)
{
    STARFISH_RELEASE_ASSERT(isMainThread());
    STARFISH_RELEASE_ASSERT(method);
    if (method->equalsIgnoreCase("DELETE")) {
        return String::createASCIIString("DELETE");
    } else if (method->equalsIgnoreCase("GET")) {
        return String::createASCIIString("GET");
    } else if (method->equalsIgnoreCase("HEAD")) {
        return String::createASCIIString("HEAD");
    } else if (method->equalsIgnoreCase("OPTIONS")) {
        return String::createASCIIString("OPTIONS");
    } else if (method->equalsIgnoreCase("POST")) {
        return String::createASCIIString("POST");
    } else if (method->equalsIgnoreCase("PUT")) {
        return String::createASCIIString("PUT");
    }
    return method;
}

bool FetchUtils::isCorsSafelistedMethod(const String* method)
{
    // https://fetch.spec.whatwg.org/#cors-safelisted-method
    if (method->equalsIgnoreCase("GET") || method->equalsIgnoreCase("HEAD") ||
        method->equalsIgnoreCase("POST")) {
        return true;
    }
    return false;
}

bool FetchUtils::isCorsSafelistedRequestHeader(const String* name,
                                               const String* value)
{
    return isCorsSafelistedRequestHeader(name->toUTF8NonGCString().data(),
                                         value->toUTF8NonGCString().data());
}

bool FetchUtils::isCorsSafelistedRequestHeader(const std::string name,
                                               const std::string value)
{
    STARFISH_ASSERT(isMainThread());
    // https://fetch.spec.whatwg.org/#cors-safelisted-request-header
    auto lower = StringUtils::toLowerCase(name);

    if (lower == "accept") {
        return !isCorsUnsafeRequestHeaderValue(value);
    } else if (lower == "accept-language" || lower == "content-language") {
        return isValidLanguageValue(value);
    } else if (lower == "content-type") {
        if (isCorsUnsafeRequestHeaderValue(value)) {
            return false;
        }
        MimeType mimeType = MimeType::parseFromString(
            String::fromUTF8(value.data(), value.size()));
        if (!mimeType.isValid()) {
            return false;
        }
        String* essence = mimeType.string();
        if (!(essence->equalsIgnoreCase("application/x-www-form-urlencoded") ||
              essence->equalsIgnoreCase("multipart/form-data") ||
              essence->equalsIgnoreCase("text/plain"))) {
            return false;
        }
        return true;
    } else if (lower == "dpr" || lower == "downlink" || lower == "save-data" ||
               lower == "viewport-width" || lower == "width" ||
               lower == "device-memory") {
        // TODO : If value, once extracted, is failure, then return false.
        return true;
    }

    return false;
}

bool FetchUtils::isCorsUnsafeRequestHeaderValue(const std::string& value)
{
    for (size_t i = 0; i < value.length(); ++i) {
        if (isCorsUnsafeRequestHeaderByte(value[i])) {
            return true;
        }
    }
    return false;
}

bool FetchUtils::isCorsUnsafeRequestHeaderByte(unsigned char c)
{
    if (c != 0x09 && c < 0x20) {
        return true;
    }
    if (c == 0x22 || c == 0x28 || c == 0x29 || c == 0x3A || c == 0x3E ||
        c == 0x3F || c == 0x40 || c == 0x5B || c == 0x5C || c == 0x5D ||
        c == 0x7B || c == 0x7D || c == 0x7F) {
        return true;
    }
    return false;
}

bool FetchUtils::isValidLanguageValue(const std::string& value)
{
    for (size_t i = 0; i < value.length(); ++i) {
        if ((isASCIILower(value[i]) || isASCIIUpper(value[i]) ||
             isASCIIDigit(value[i]) || value[i] == 0x20 || value[i] == 0x2A ||
             value[i] == 0x2C || value[i] == 0x2D || value[i] == 0x2E ||
             value[i] == 0x3B || value[i] == 0x3D || value[i] == 0x5F)) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

std::vector<std::string> FetchUtils::corsUnsafeRequestHeaderNames(
    HTTPHeaderMap& httpHeaderMap)
{
    // https://fetch.spec.whatwg.org/#cors-unsafe-request-header-names
    std::vector<std::string> unsafeNames;
    std::vector<std::string> potentiallyUnsafeNames;
    size_t safelistValueSize = 0;

    auto& headerMap = httpHeaderMap.headerMap();

    for (const auto& header : headerMap) {
        if (!FetchUtils::isCorsSafelistedRequestHeader(header.first,
                                                       header.second)) {
            unsafeNames.emplace_back(StringUtils::toLowerCase(header.first));
        } else {
            potentiallyUnsafeNames.emplace_back(
                StringUtils::toLowerCase(header.first));
            safelistValueSize++;
        }
    }
    if (safelistValueSize > 1024) {
        for (const auto& name : potentiallyUnsafeNames) {
            unsafeNames.emplace_back(name);
        }
    }
    std::sort(unsafeNames.begin(), unsafeNames.end());
    return unsafeNames;
}

bool FetchUtils::isCORSsafelistedResponseHeaderName(
    const std::string& name, const GCVector<String*>* exposedNames)
{
    // https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name
    if (StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kCacheControl) ||
        StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kContentLanguage) ||
        StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kContentLength) ||
        StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kContentType) ||
        StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kExpires) ||
        StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kLastModified) ||
        StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kPragma)) {
        return true;
    } else if (isForbiddenResponseHeaderName(name)) {
        return false;
    } else {
        if (exposedNames) {
            for (auto const& exposedName : *exposedNames) {
                if (exposedName->equalsIgnoreCase(name.data(), name.size())) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool FetchUtils::isForbiddenResponseHeaderName(const std::string& name)
{
    if (StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kSetCookie) ||
        StringUtils::equalsIgnoreCase(name, HTTPHeaderMap::kSetCookie2)) {
        return true;
    }
    return false;
}

static bool equalsIgnoreCaseWrapper(const String* name, const char* str)
{
    STARFISH_ASSERT(name != nullptr);
    STARFISH_ASSERT(str != nullptr);
    return name->equalsIgnoreCase(str, strlen(str));
}

// https://fetch.spec.whatwg.org/#forbidden-header-name
bool FetchUtils::isForbiddenHeaderName(String* name)
{
    auto lower = name->toLower();
    if (lower->startsWith("proxy-") || lower->startsWith("sec-") ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kAcceptCharset) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kAcceptEncoding) ||
        equalsIgnoreCaseWrapper(lower,
                                HTTPHeaderMap::kAccessControlRequestHeaders) ||
        equalsIgnoreCaseWrapper(lower,
                                HTTPHeaderMap::kAccessControlRequestMethod) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kConnection) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kContentLength) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kCookie) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kCookie2) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kDate) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kDNT) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kExpect) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kHost) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kKeepAlive) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kOrigin) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kReferer) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kTE) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kTrailer) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kTransferEncoding) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kUpgrade) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kUserAgent) ||
        equalsIgnoreCaseWrapper(lower, HTTPHeaderMap::kVia)) {
        return true;
    }
    return false;
}
}
