/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishHeadersData__
#define __StarfishHeadersData__
#include "platform/network/http/HTTPHeaderMap.h"

namespace Starfish {

class Headers;

// https://fetch.spec.whatwg.org/#concept-headers-guard
enum class Guard { None, Immutable, Request, RequestNoCors, Response };

class HeadersData : public gc {
public:
    static bool isValidHTTPToken(const String* name);
    static bool isValidHTTPHeaderValue(const String* value);
    static bool isForbiddenHeaderName(const String* name);

    HeadersData();

    void append(String* name, String* value, bool* typeErrorOccurred = nullptr);
    void deleteHeader(String* name, bool* typeErrorOccurred = nullptr);
    Nullable<String*> get(String* name, bool* typeErrorOccurred = nullptr);
    Nullable<std::string> noCheckValidGet(const std::string& name);
    bool has(String* name, bool* typeErrorOccurred = nullptr);
    bool noCheckValidHas(const std::string& lowerCaseName);
    void set(String* name, String* value, bool* typeErrorOccurred = nullptr);
    void noCheckValidSet(const std::string& lowerCaseName,
                         const std::string& value);
    Guard guard();
    void setGuard(Guard guard);
    String* extractMIMEType();

    HTTPHeaderMap* httpHeaderMap()
    {
        return &m_httpHeaderMap;
    }

private:
    HTTPHeaderMap m_httpHeaderMap;
    Guard m_guard;
};
} // namespace Starfish

#endif
