/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "URL.h"

namespace StarFish {

String* URL::parseURLString(String* baseURL, String* url)
{
    unsigned numLeadingSpaces = 0;
    unsigned numTrailingSpaces = 0;

    size_t urlLength = url->length();
    for (; numLeadingSpaces < urlLength; ++numLeadingSpaces) {
        if (!String::isSpaceOrNewline(url->charAt(numLeadingSpaces)))
            break;
    }
    for (; numTrailingSpaces < urlLength; ++numTrailingSpaces) {
        if (!String::isSpaceOrNewline(url->charAt(urlLength - 1 - numTrailingSpaces)))
            break;
    }
    STARFISH_ASSERT(numLeadingSpaces + numTrailingSpaces < urlLength);

    if (numLeadingSpaces || numTrailingSpaces) {
        url = url->substring(numLeadingSpaces, urlLength - (numLeadingSpaces + numTrailingSpaces));
    }


    if (url->startsWith("data:")) {
        return url;
    }

    bool isAbsolute = url->contains("://");

    if (url->startsWith("//")) {
        isAbsolute = true;
        STARFISH_ASSERT(baseURL->length());
        size_t idx = baseURL->indexOf(':');
        url = baseURL->substring(0, idx + 1)->concat(url);
    }

    if (url->startsWith("/")) {
        isAbsolute = true;
        if (baseURL->startsWith("file://")) {
            url = String::createASCIIString("file://")->concat(url);
        } else {
            size_t pos = baseURL->find("://");
            STARFISH_ASSERT(pos != SIZE_MAX);
            size_t pos2 = baseURL->find("/", pos + 3);
            if (pos2 != SIZE_MAX) {
                baseURL = baseURL->substring(0, pos2);
                url = baseURL->concat(url);
            } else {
                url = baseURL->concat(url);
            }
        }
    }

    if (baseURL->equals(String::emptyString)) {
        STARFISH_ASSERT(isAbsolute);
    }

    if (isAbsolute) {
        return url;
    } else {
        STARFISH_ASSERT(baseURL->contains("://"));
        bool baseEndsWithSlash = baseURL->charAt(baseURL->length() - 1) == '/';

        if (url->startsWith("./")) {
            url = url->substring(2, url->length() - 2);
        }

        if (baseEndsWithSlash) {
            return baseURL->concat(url);
        } else {
            size_t f = baseURL->find("://");
            STARFISH_ASSERT(f != SIZE_MAX);
            f += 3;
            size_t f2 = baseURL->find("/", f);
            if (f2 != SIZE_MAX) {
                baseURL = baseURL->substring(0, baseURL->lastIndexOf('/'));
            }
            return baseURL->concat(String::createASCIIString("/"))->concat(url);
        }
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

URL::URL(String* baseURL, String* url)
    : m_string(url)
{
    m_urlString = URL::parseURLString(baseURL, url);
}

String* URL::baseURI() const
{
    if (m_urlString->startsWith("data:")) {
        return String::emptyString;
    }

    size_t pos = m_urlString->find("://");
    STARFISH_ASSERT(pos != SIZE_MAX);
    size_t pos2 = m_urlString->find("/", pos + 3);
    if (pos2 != SIZE_MAX) {
        return m_urlString->substring(0, m_urlString->lastIndexOf('/') + 1);
    } else {
        return m_urlString;
    }
}

String* URL::urlStringWithoutSearchPart() const
{
    size_t idx = m_urlString->lastIndexOf('?');
    if (idx != SIZE_MAX) {
        return m_urlString->substring(0, idx);
    } else {
        return m_urlString;
    }
}

}
