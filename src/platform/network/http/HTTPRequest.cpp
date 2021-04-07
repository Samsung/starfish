/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "HTTPHeaderMap.h"
#include "HTTPRequest.h"

namespace Starfish {

HTTPRequest::HTTPRequest(const std::string& url, const std::string& baseURL,
                         const std::string& method,
                         const HTTPHeaderMap& headers, std::string&& entityBody,
                         bool includeCredentials)
    : m_url(url)
    , m_baseURL(baseURL)
    , m_method(method)
    , m_headers(headers)
    , m_entityBody(entityBody)
    , m_includeCredentials(includeCredentials)
    , m_requestTime(0)
{
}

HTTPRequest::~HTTPRequest()
{
    std::vector<std::string>().swap(m_unsafeRequestHeaderNames);
}
} // namespace Starfish
