/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "core/util/String.h"

namespace StarFish {

class HTTPHeaderList;

class HTTPRequest : public gc {
public:
    static HTTPRequest* create(String* url, String* method,
                               HTTPHeaderList* header, String* body)
    {
        return new HTTPRequest(url, method, header, body);
    }

    HTTPHeaderList* headers()
    {
        return m_headers;
    }

    String* method()
    {
        return m_method;
    }

    String* url()
    {
        return m_url;
    }

    String* body()
    {
        return m_body;
    }

private:
    HTTPRequest(String* url, String* method, HTTPHeaderList* headers,
                String* body)
        : m_url(url)
        , m_method(method)
        , m_headers(headers)
        , m_body(body)
    {
    }

    ~HTTPRequest()
    {
    }

    String* m_url;
    String* m_method;
    HTTPHeaderList* m_headers;
    String* m_body;
};
}

#endif
