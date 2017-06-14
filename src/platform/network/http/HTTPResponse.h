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

#ifndef __StarFishHTTPResponse__
#define __StarFishHTTPResponse__

#include <curl/curl.h>

namespace StarFish {

class HTTPHeaderMap;

class HTTPResponse {
public:
    typedef std::vector<char> EntityBody;

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

private:
    HTTPResponse();
    CURL* m_curl;
    long m_responseCode;
    std::string m_reasonPhrase;
    HTTPHeaderMap m_headers;
    EntityBody m_entityBody;
};
}

#endif
