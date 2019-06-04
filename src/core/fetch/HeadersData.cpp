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
#include "core/fetch/HeadersData.h"
#include "core/extra/MimeType.h"
#include "core/modules/threading/Thread.h"

#define SET_TYPE_ERROR()               \
    do {                               \
        if (typeErrorOccurred) {       \
            *typeErrorOccurred = true; \
        }                              \
    } while (0)

namespace Starfish {

// https://tools.ietf.org/html/rfc2616#section-2.2
bool HeadersData::isValidHTTPToken(const String* name)
{
    if (name->isEmpty()) {
        return false;
    }

    for (size_t i = 0; i < name->length(); i++) {
        auto c = name->charAt(i);
        if (c <= 0x20 || c >= 0x7F || c == '(' || c == ')' || c == '<' ||
            c == '>' || c == '@' || c == ',' || c == ';' || c == ':' ||
            c == '\\' || c == '"' || c == '/' || c == '[' || c == ']' ||
            c == '?' || c == '=' || c == '{' || c == '}') {
            return false;
        }
    }
    return true;
}

bool HeadersData::isValidHTTPHeaderValue(const String* value)
{
    for (size_t i = 0; i < value->length(); i++) {
        auto c = value->charAt(i);
        if (c > 0xFF) {
            return false;
        }
    }
    return true;
}

static bool equalsIgnoreCaseWrapper(const String* name, const char* str)
{
    STARFISH_ASSERT(name != nullptr);
    STARFISH_ASSERT(str != nullptr);
    return name->equalsIgnoreCase(str, strlen(str));
}

bool HeadersData::isForbiddenHeaderName(const String* name)
{
    // https://fetch.spec.whatwg.org/#forbidden-header-name
    if (equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kAcceptCharset) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kAcceptEncoding) ||
        equalsIgnoreCaseWrapper(name,
                                HTTPHeaderMap::kAccessControlRequestHeaders) ||
        equalsIgnoreCaseWrapper(name,
                                HTTPHeaderMap::kAccessControlRequestMethod) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kConnection) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kContentLanguage) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kCookie) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kCookie2) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kDate) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kDNT) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kExpect) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kHost) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kKeepAlive) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kOrigin) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kReferer) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kTE) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kTrailer) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kTransferEncoding) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kUpgrade) ||
        equalsIgnoreCaseWrapper(name, HTTPHeaderMap::kVia)) {
        return true;
    }
    return false;
}

HeadersData::HeadersData()
    : m_guard(Guard::None)
{
    STARFISH_RELEASE_ASSERT(isMainThread());
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            HeadersData* headersData = (HeadersData*)obj;
            headersData->m_httpHeaderMap.~HTTPHeaderMap();
        },
        NULL, NULL, NULL);
}

void HeadersData::append(String* name, String* value, bool* typeErrorOccurred)
{
    // https://fetch.spec.whatwg.org/#concept-headers-append

    // TODO : Nomalize vlaue
    if (!isValidHTTPToken(name) || !isValidHTTPHeaderValue(value)) {
        SET_TYPE_ERROR();
        return;
    }

    if (m_guard == Guard::Immutable) {
        SET_TYPE_ERROR();
        return;
    }

    if (m_guard == Guard::Request && isForbiddenHeaderName(name)) {
        return;
    }
    // TODDO : Guard::RequestNoCors, Guard::Response

    m_httpHeaderMap.append(name->toLower()->toUTF8NonGCString(),
                           value->trim()->toUTF8NonGCString());
    return;
}

void HeadersData::deleteHeader(String* name, bool* typeErrorOccurred)
{
    // https://fetch.spec.whatwg.org/#concept-headers-append

    if (!isValidHTTPToken(name)) {
        SET_TYPE_ERROR();
        return;
    }
    if (m_guard == Guard::Immutable) {
        SET_TYPE_ERROR();
        return;
    }

    if (m_guard == Guard::Request && isForbiddenHeaderName(name)) {
        return;
    }
    // TODDO : Guard::RequestNoCors, Guard::Response

    m_httpHeaderMap.remove(name->toLower()->toUTF8NonGCString());
    return;
}

Nullable<String*> HeadersData::get(String* name, bool* typeErrorOccurred)
{
    if (!isValidHTTPToken(name)) {
        SET_TYPE_ERROR();
        return nullptr;
    }

    Nullable<std::string> value =
        noCheckValidGet(name->toLower()->toUTF8NonGCString());
    if (value.hasValue() == true) {
        return String::fromUTF8(value.getValue().data(),
                                value.getValue().size());
    }
    return nullptr;
}

Nullable<std::string> HeadersData::noCheckValidGet(const std::string& name)
{
    // TODO : https://fetch.spec.whatwg.org/#concept-header-list-get
    auto it = m_httpHeaderMap.find(name);
    if (it == m_httpHeaderMap.headerMap().end()) {
        return nullptr;
    }
    return it->second;
}

bool HeadersData::has(String* name, bool* typeErrorOccurred)
{
    // https://fetch.spec.whatwg.org/#dom-headers-has
    if (!isValidHTTPToken(name)) {
        SET_TYPE_ERROR();
        return false;
    }
    return noCheckValidHas(name->toLower()->toUTF8NonGCString().data());
}

bool HeadersData::noCheckValidHas(const std::string& lowerCaseName)
{
    auto it = m_httpHeaderMap.find(lowerCaseName);
    return !(it == m_httpHeaderMap.headerMap().end());
}

void HeadersData::set(String* name, String* value, bool* typeErrorOccurred)
{
    // TODO : Nomalize vlaue
    if (!isValidHTTPToken(name) || !isValidHTTPHeaderValue(value)) {
        SET_TYPE_ERROR();
        return;
    }

    if (m_guard == Guard::Immutable) {
        SET_TYPE_ERROR();
        return;
    }

    if (m_guard == Guard::Request && isForbiddenHeaderName(name)) {
        return;
    }
    // TODDO : Guard::RequestNoCors, Guard::Response
    noCheckValidSet(name->toLower()->toUTF8NonGCString().data(),
                    value->trim()->toUTF8NonGCString().data());
}

void HeadersData::noCheckValidSet(const std::string& lowerCaseName,
                                  const std::string& value)
{
    m_httpHeaderMap.headerMap()[lowerCaseName] = value;
}

Guard HeadersData::guard()
{
    return m_guard;
}

void HeadersData::setGuard(Guard guard)
{
    m_guard = guard;
}

String* HeadersData::extractMIMEType()
{
    auto mimeType = noCheckValidGet("content-type");
    if (mimeType.hasValue() == false) {
        return String::emptyString;
    }
    return String::fromUTF8(mimeType.getValue().data(),
                            mimeType.getValue().size())
        ->toLower();
}
}
