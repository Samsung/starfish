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

#ifndef __StarFishHTTPRequest__
#define __StarFishHTTPRequest__

namespace StarFish {

class HTTPHeaderMap;

class HTTPRequest {
public:
    static std::unique_ptr<HTTPRequest> create(const std::string& url,
                                               const std::string& method,
                                               const HTTPHeaderMap& headers,
                                               const std::string& entityBody)
    {
        return std::unique_ptr<HTTPRequest>(
            new HTTPRequest(url, method, headers, entityBody));
    }

    ~HTTPRequest();

    HTTPHeaderMap& headers()
    {
        return m_headers;
    }

    std::string method()
    {
        return m_method;
    }

    std::string url()
    {
        return m_url;
    }

    std::string entityBody()
    {
        return m_entityBody;
    }

private:
    HTTPRequest(const std::string& url, const std::string& method,
                const HTTPHeaderMap& headers, const std::string& entityBody);
    // Use std::string because it does not inherit gc
    std::string m_url;
    std::string m_method;
    HTTPHeaderMap m_headers;
    std::string m_entityBody;
};
}

#endif
