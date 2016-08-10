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
#include "extra/Blob.h"
#include "platform/window/Window.h"
#include "dom/Document.h"

namespace StarFish {

URL::URL(String* baseURL, String* url)
    : ScriptWrappable(this), m_string(url)
{
    m_urlString = URL::parseURLString(baseURL, url);
    m_username = NULL;
    m_password = NULL;
    m_port = NULL;
}

String* URL::parseURLString(String* baseURL, String* url)
{
    unsigned numLeadingSpaces = 0;
    unsigned numTrailingSpaces = 0;

    size_t urlLength = url->length();
    for (; numLeadingSpaces < urlLength; ++numLeadingSpaces) {
        if (!String::isSpaceOrNewline(url->charAt(numLeadingSpaces)))
            break;
    }
    if (numLeadingSpaces != urlLength) {
        for (; numTrailingSpaces < urlLength; ++numTrailingSpaces) {
            if (!String::isSpaceOrNewline(url->charAt(urlLength - 1 - numTrailingSpaces)))
                break;
        }
        STARFISH_ASSERT(numLeadingSpaces + numTrailingSpaces < urlLength);

        if (numLeadingSpaces || numTrailingSpaces) {
            url = url->substring(numLeadingSpaces, urlLength - (numLeadingSpaces + numTrailingSpaces));
        }
    }


    if (url->startsWith("data:")) {
        return url;
    }

    if (url->startsWith("blob:")) {
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

String* URL::createObjectURL(Blob* blob)
{
    BlobURLStore store;
    if (blob->starFish()->isValidBlobURL(blob)) {
        store = blob->starFish()->findBlobURL(blob);
    } else {
        store = blob->starFish()->addBlobInBlobURLStore(blob);
    }

    std::string url = "blob:";
    url += blob->starFish()->window()->document()->documentURI()->urlString()->utf8Data();
    url += "/";

    union {
        struct {
            uint16_t a;
            uint16_t b;
        } small;
        uint32_t big;
    } spliter;

#ifdef STARFISH_64
    union {
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t d;
        } small;
        uint64_t big;
    } spliter64;
#endif

    char buf[32];
#ifdef STARFISH_64
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter64.big = (uint64_t)blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.b);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.c);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.d);
    url += buf;
#else
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter.big = store.m_c;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;

    spliter.big = (uint32_t)blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
#endif
    return String::createASCIIString(url.data());
}

String* URL::origin()
{
    if (m_urlString) {
        // FIXME needs unicode serialization
        return m_urlString;
    } else
        return String::emptyString;
}

String* URL::getProtocol()
{
    if (m_urlString) {
        size_t pos = m_urlString->find(":");
        STARFISH_ASSERT(pos != SIZE_MAX);
        return m_urlString->substring(0, pos + 1);
    } else
        return String::emptyString;
}

String* URL::getUsername()
{
    if (m_username)
        return m_username;
    else
        return String::emptyString;
}

String* URL::getPassword()
{
    if (m_password)
        return m_password;
    else
        return String::emptyString;
}

String* URL::getHost()
{
    return m_urlString;
}

String* URL::getHostname()
{
    return m_urlString;
}

String* URL::getPort()
{
    if (m_port)
        return m_port;
    else
        return String::emptyString;
}

String* URL::getPathname()
{
    size_t pos = m_urlString->find("://");
    STARFISH_ASSERT(pos != SIZE_MAX);
    size_t pos2 = m_urlString->find("/", pos + 3);
    if (pos2 != SIZE_MAX) {
        return m_urlString->substring(pos2, m_urlString->length() - pos2);
    } else {
        return String::createASCIIString("/");
    }
}

String* URL::getSearch()
{
    size_t idx = m_urlString->find("?");
    size_t idx2 = m_urlString->find("#", idx + 1);
    if (idx != SIZE_MAX) {
        if (idx2 != SIZE_MAX) {
            return m_urlString->substring(idx, idx2 - idx);
        } else {
            return m_urlString->substring(idx, m_urlString->length() - idx);
        }
    } else {
        return String::emptyString;
    }
}

String* URL::getHash()
{
    size_t idx = m_urlString->find("#");
    if (idx != SIZE_MAX) {
        return m_urlString->substring(idx, m_urlString->length() - idx);
    } else {
        return String::emptyString;
    }

}





}
