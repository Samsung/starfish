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
}
