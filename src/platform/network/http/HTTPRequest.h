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

#ifndef __StarfishHTTPRequest__
#define __StarfishHTTPRequest__

namespace Starfish {

class HTTPHeaderMap;

class HTTPRequest {
public:
    static std::unique_ptr<HTTPRequest> create(const std::string& url,
                                               const std::string& baseURL,
                                               const std::string& method,
                                               const HTTPHeaderMap& headers,
                                               const std::string& entityBody,
                                               bool includeCredentials)
    {
        return std::unique_ptr<HTTPRequest>(new HTTPRequest(
            url, baseURL, method, headers, entityBody, includeCredentials));
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

    const std::string& entityBody() const
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

    bool includeCredentials() const
    {
        return m_includeCredentials;
    }

    void setUnsafeRequestHeaderNames(const std::vector<std::string>& headers)
    {
        m_unsafeRequestHeaderNames = headers;
    }

    std::vector<std::string> unsafeRequestHeaderNames() const
    {
        return m_unsafeRequestHeaderNames;
    }

    bool hasUnsafeRequestHeaderNames() const
    {
        return m_unsafeRequestHeaderNames.size() != 0;
    }

private:
    HTTPRequest(const std::string& url, const std::string& baseURL,
                const std::string& method, const HTTPHeaderMap& headers,
                const std::string& entityBody, bool includeCredentials);
    // Use std::string because it does not inherit gc
    std::string m_url;
    std::string m_baseURL;
    std::string m_method;
    HTTPHeaderMap m_headers;
    std::vector<std::string> m_unsafeRequestHeaderNames;
    std::string m_entityBody;
    bool m_includeCredentials;

    time_t m_requestTime;
};
} // namespace Starfish

#endif
