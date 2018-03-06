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

#ifndef __StarFishHTTPRequest__
#define __StarFishHTTPRequest__

namespace StarFish {

class HTTPHeaderMap;

class HTTPRequest {
public:
    static std::unique_ptr<HTTPRequest> create(const std::string& url,
                                               const std::string& baseURL,
                                               const std::string& method,
                                               const HTTPHeaderMap& headers,
                                               const std::string& entityBody)
    {
        return std::unique_ptr<HTTPRequest>(
            new HTTPRequest(url, baseURL, method, headers, entityBody));
    }

    ~HTTPRequest();

    HTTPHeaderMap& headers()
    {
        return m_headers;
    }

    std::string method() const
    {
        return m_method;
    }

    std::string url() const
    {
        return m_url;
    }

    std::string baseURL() const
    {
        return m_baseURL;
    }

    std::string entityBody() const
    {
        return m_entityBody;
    }

    void setRequestTime(time_t requestTime)
    {
        m_requestTime = requestTime;
    }

    time_t requestTime() const
    {
        return m_requestTime;
    }

private:
    HTTPRequest(const std::string& url, const std::string& baseURL,
                const std::string& method, const HTTPHeaderMap& headers,
                const std::string& entityBody);
    // Use std::string because it does not inherit gc
    std::string m_url;
    std::string m_baseURL;
    std::string m_method;
    HTTPHeaderMap m_headers;
    std::string m_entityBody;
    time_t m_requestTime;
};
}

#endif
