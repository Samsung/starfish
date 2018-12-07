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
    // https://fetch.spec.whatwg.org/#cors-safelisted-request-header
    auto lower = StringUtils::toLowerCase(name);

    if (lower == "accept") {
        if (isCorsUnsafeRequestHeaderValue(value)) {
            return false;
        }
    } else if (lower == "accept-languag" || lower == "content-language") {
        if (!isValidLanguageValue(value)) {
            return false;
        }
    } else if (lower == "content-type") {
        if (isCorsUnsafeRequestHeaderValue(value)) {
            return false;
        }
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
             value[i] == 0x3B || value[i] == 0x3D)) {
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
}
