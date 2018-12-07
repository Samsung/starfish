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

#ifndef __StarfishFetchUtils__
#define __StarfishFetchUtils__

namespace Starfish {

class HTTPHeaderMap;

class FetchUtils {
public:
    static bool isForbiddenMethod(const String* method);
    static String* normalizeMethod(String* method);
    static bool isCorsSafelistedMethod(const String* method);
    static bool isCorsSafelistedRequestHeader(const String* name,
                                              const String* value);
    static bool isCorsSafelistedRequestHeader(const std::string name,
                                              const std::string value);
    static bool isCorsUnsafeRequestHeaderValue(const std::string& value);
    static bool isCorsUnsafeRequestHeaderByte(unsigned char c);
    static bool isValidLanguageValue(const std::string& value);
    static std::vector<std::string> corsUnsafeRequestHeaderNames(
        HTTPHeaderMap& httpHeaderMap);
};
}

#endif
