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

#ifndef __StarFishHTTPResponse__
#define __StarFishHTTPResponse__

#include <curl/curl.h>
#include "HTTPUtil.h"
namespace StarFish {

class HTTPHeaderMap;

class HTTPResponse {
public:
    static std::unique_ptr<HTTPResponse> create()
    {
        return std::unique_ptr<HTTPResponse>(new HTTPResponse());
    }

    ~HTTPResponse();
    void setResponseCode(long responseCode)
    {
        m_responseCode = responseCode;
    }

    long responseCode()
    {
        return m_responseCode;
    }

    bool isSuccessfulResponseStatus();
    bool isRedirectionResponseStatus();

    void setReasonPhrase(const std::string& reasonPhrase)
    {
        m_reasonPhrase = reasonPhrase;
    }

    HTTPHeaderMap& headers()
    {
        return m_headers;
    }

    EntityBody& entityBody()
    {
        return m_entityBody;
    }
    void setResponseTime(int64_t responseTime)
    {
        m_responseTime = responseTime;
    }
    int64_t responseTime() const
    {
        return m_responseTime;
    }

    std::string lastEffectiveURL()
    {
        return m_lastEffectiveURL;
    }

    void setLastEffectiveURL(const std::string& url)
    {
        m_lastEffectiveURL = url;
    }

private:
    HTTPResponse();
    long m_responseCode;
    std::string m_reasonPhrase;
    std::string m_lastEffectiveURL;
    HTTPHeaderMap m_headers;
    EntityBody m_entityBody;
    int64_t m_responseTime;
};
}

#endif
