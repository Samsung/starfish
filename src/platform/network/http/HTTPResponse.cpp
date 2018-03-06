/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "HTTPHeaderMap.h"
#include "HTTPResponse.h"
#include "HTTPStatus.h"

namespace StarFish {

HTTPResponse::HTTPResponse()
    : m_responseCode(0)
    , m_reasonPhrase()
    , m_headers()
    , m_entityBody()
    , m_responseTime(0)
{
}

HTTPResponse::~HTTPResponse()
{
}

bool HTTPResponse::isSuccessfulResponseStatus()
{
    if (HTTPStatusCode::HTTP_STATUS_OK <= m_responseCode &&
        m_responseCode < HTTPStatusCode::HTTP_STATUS_MULTIPLE_CHOICES) {
        return true;
    }
    return false;
}

bool HTTPResponse::isRedirectionResponseStatus()
{
    if (HTTPStatusCode::HTTP_STATUS_MULTIPLE_CHOICES <= m_responseCode &&
        m_responseCode < HTTPStatusCode::HTTP_STATUS_BAD_REQUEST) {
        return true;
    }
    return false;
}
}
