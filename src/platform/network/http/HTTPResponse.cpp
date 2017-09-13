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
#include "HTTPHeaderMap.h"
#include "HTTPResponse.h"
#include "HTTPStatus.h"

namespace StarFish {

HTTPResponse::HTTPResponse()
    : m_responseCode(0)
    , m_reasonPhrase()
    , m_headers()
    , m_entityBody()
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

std::string HTTPResponse::responseStatusText()
{
    return httpStatusCodeToText(m_responseCode);
}
}
