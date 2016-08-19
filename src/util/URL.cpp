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
    m_protocolEnd = m_userStart = m_userEnd = m_passwordEnd = m_hostEnd = m_portEnd = m_pathEnd = m_queryEnd = m_fragmentEnd = 0;

    parseURLString(baseURL, url);
}

String* removingDots(String* origPath)
{
    UTF16String str = origPath->toUTF16String();
    UTF16String dst = origPath->toUTF16String();
    size_t pos = 0;
    size_t dstPos = 0;
    size_t pathLen = str.length();
    bool removed = false;

    STARFISH_ASSERT(str[pos] == '/');

    pos++;
    dstPos++;

    while (pos < pathLen) {
        if (str[pos + 0] == '.' && str[pos - 1] == '/') {
            if (pos + 1 == pathLen || str[pos + 1] == '/') {
                removed = true;
                pos += 2;
                continue;
            } else if (str[pos + 1] == '.' && (pos + 2 == pathLen || str[pos + 2] == '/')) {
                removed = true;
                pos += 3;
                if (dstPos > 1)
                    dstPos--;
                while (dstPos > 0 && dst[dstPos - 1] != '/')
                    dstPos--;
                continue;
            }
        }
        dst[dstPos] = str[pos];
        pos++;
        dstPos++;
    }
    if (removed)
        return String::fromUTF16(dst.data(), dstPos);
    else
        return origPath;
}

void URL::resolvePositions()
{
    size_t pos = m_urlString->find(":");
    STARFISH_ASSERT(pos != SIZE_MAX);
    m_protocolEnd = pos + 1;

    bool hierarchical = m_urlString->charAt(m_protocolEnd) == '/';
    bool hasSecondSlash = hierarchical && m_urlString->charAt(m_protocolEnd + 1) == '/';

    // username & password

    m_userStart = m_protocolEnd;
    if (hierarchical) {
        m_userStart++;
        if (hasSecondSlash) {
            m_userStart++;
            if (m_protocol != FILE_PROTOCOL) {
                while (m_urlString->charAt(m_userStart) == '/')
                    m_userStart++;
            }
        }
    }

    m_userEnd = m_passwordEnd = m_userStart;

    // host
    pos = m_urlString->find("/", m_userStart);
    if (pos != SIZE_MAX) {
        m_hostEnd = m_portEnd = pos;

        size_t pos2 = m_urlString->find("?", pos);
        if (pos2 != SIZE_MAX) {
            m_pathEnd = pos2;
            size_t pos3 = m_urlString->find("#", pos2);
            if (pos3 != SIZE_MAX) {
                m_queryEnd = pos3;
                m_fragmentEnd = m_urlString->length();
            } else {
                m_queryEnd = m_fragmentEnd = m_urlString->length();
            }
        } else {
            size_t pos3 = m_urlString->find("#", pos);
            if (pos3 != SIZE_MAX) {
                m_pathEnd = m_queryEnd = pos3;
                m_fragmentEnd = m_urlString->length();
            } else {
                m_pathEnd = m_queryEnd = m_fragmentEnd = m_urlString->length();
            }
        }

    } else {
        size_t pos2 = m_urlString->find("?");
        if (pos2 != SIZE_MAX) {
            m_hostEnd = m_portEnd = m_pathEnd = pos2;
            size_t pos3 = m_urlString->find("#", pos2);
            if (pos3 != SIZE_MAX) {
                m_queryEnd = pos3;
                m_fragmentEnd = m_urlString->length();
            } else {
                m_queryEnd = m_fragmentEnd = m_urlString->length();
            }
        } else {
            size_t pos3 = m_urlString->find("#");
            if (pos3 != SIZE_MAX) {
                m_hostEnd = m_portEnd = m_pathEnd = m_queryEnd = pos3;
                m_fragmentEnd = m_urlString->length();
            } else {
                m_hostEnd = m_portEnd = m_pathEnd = m_queryEnd = m_fragmentEnd = m_urlString->length();
            }
        }
    }

    // username & password & port
    pos = m_urlString->find("@", m_userStart);
    if (pos != SIZE_MAX && pos < m_hostEnd) {
        m_userEnd = m_passwordEnd = pos;

        // ':' for username
        pos = m_urlString->find(":", m_userStart);
        if (pos != SIZE_MAX && pos < m_userEnd) {
            m_userEnd = pos;
        }
        // ':' for port
        pos = m_urlString->find(":", pos + 1);
        if (pos != SIZE_MAX && m_passwordEnd < pos && pos < m_hostEnd) {
            m_hostEnd = pos;
        }
    } else { // no username & password
        pos = m_urlString->find(":", m_passwordEnd);
        if (pos != SIZE_MAX && pos < m_hostEnd) {
            m_hostEnd = pos;
        }
    }

    if (m_protocol == BLOB_PROTOCOL || m_protocol == DATA_PROTOCOL) {
        m_userEnd = m_userStart;
        m_passwordEnd = m_userEnd;
        m_hostEnd = m_passwordEnd;
        m_portEnd = m_hostEnd;
    }


    STARFISH_ASSERT(m_protocolEnd);
    STARFISH_ASSERT(m_userStart);
    STARFISH_ASSERT(m_userEnd);
    STARFISH_ASSERT(m_passwordEnd);
    STARFISH_ASSERT(m_hostEnd);
    STARFISH_ASSERT(m_portEnd);
    STARFISH_ASSERT(m_pathEnd);
    STARFISH_ASSERT(m_queryEnd);
    STARFISH_ASSERT(m_fragmentEnd);

}

void URL::parseURLString(String* baseURL, String* url)
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

    bool isAbsolute = false;

    if (url->startsWith("data:") || url->startsWith("blob:") || url->contains("://")) {
        isAbsolute = true;
    }

    if (baseURL->equals(String::emptyString)) {
        STARFISH_ASSERT(isAbsolute);
    }

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

    if (!isAbsolute) {
        STARFISH_ASSERT(baseURL->contains("://"));
        bool baseEndsWithSlash = baseURL->charAt(baseURL->length() - 1) == '/';

        if (url->startsWith("./")) {
            url = url->substring(2, url->length() - 2);
        }

        if (baseEndsWithSlash) {
            url = baseURL->concat(url);
        } else if (url->startsWith("?") || url->startsWith("#") || urlLength == 0) {
            url = baseURL->concat(url);
        } else {
            size_t f = baseURL->find("://");
            STARFISH_ASSERT(f != SIZE_MAX);
            f += 3;
            size_t f2 = baseURL->find("/", f);
            if (f2 != SIZE_MAX) {
                baseURL = baseURL->substring(0, baseURL->lastIndexOf('/'));
            }
            url = baseURL->concat(String::createASCIIString("/"))->concat(url);
        }
    }

    m_urlString = url;

    // protocol
    if (m_urlString->startsWith("file", false)) {
        m_protocol = FILE_PROTOCOL;
    } else if (m_urlString->startsWith("https", false)) {
        m_protocol = HTTPS_PROTOCOL;
    } else if (m_urlString->startsWith("http", false)) {
        m_protocol = HTTP_PROTOCOL;
    } else if (m_urlString->startsWith("blob", false)) {
        m_protocol = BLOB_PROTOCOL;
    } else if (m_urlString->startsWith("data", false)) {
        m_protocol = DATA_PROTOCOL;
    } else {
        m_protocol = UNKNOWN;
    }

    resolvePositions();

    if (m_urlString->charAt(m_protocolEnd) == '/') {
        String* origPath = getPathname();
        String* newPath = removingDots(origPath);
        if (newPath != origPath) {
            setPathname(newPath, false);
        }
    }
}

String* URL::getURLString(String* baseURL, String* url)
{
    URL* u = new URL(baseURL, url);
    String* ret = u->getHref();
    return ret;
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
    if (m_protocol == FILE_PROTOCOL) {
        return m_urlString->substring(0, 7)->toLower(); // "file://"
    }
    size_t start = (m_passwordEnd == m_userStart) ? m_passwordEnd : m_passwordEnd + 1;
    return m_urlString->substring(start, m_hostEnd - start);
}

String* URL::getHref()
{
    return m_urlString;
}

String* URL::getProtocol()
{
    return m_urlString->substring(0, m_protocolEnd)->toLower();
}

String* URL::getUsername()
{
    return m_urlString->substring(m_userStart, m_userEnd - m_userStart);
}

String* URL::getPassword()
{
    if (m_passwordEnd != m_userEnd)
        return m_urlString->substring(m_userEnd + 1, m_passwordEnd - m_userEnd - 1);
    else
        return String::emptyString;
}

String* URL::getHost()
{
    size_t start = (m_passwordEnd == m_userStart) ? m_passwordEnd : m_passwordEnd + 1;
    return m_urlString->substring(start, m_hostEnd - start);
}

String* URL::getHostname()
{
    size_t start = (m_passwordEnd == m_userStart) ? m_passwordEnd : m_passwordEnd + 1;
    return m_urlString->substring(start, m_hostEnd - start);
}

String* URL::getPort()
{
    if (m_hostEnd != m_portEnd)
        return m_urlString->substring(m_hostEnd + 1, m_portEnd - m_hostEnd - 1);
    else
        return String::emptyString;
}

String* URL::getPathname()
{
    if (m_portEnd != m_pathEnd)
        return m_urlString->substring(m_portEnd, m_pathEnd - m_portEnd);
    else
        return String::createASCIIString("/");
}

void URL::setPathname(String* newPath, bool needRemovingDots)
{
    if (!newPath->length() || newPath->charAt(0) != '/') {
        newPath = String::createASCIIString("/")->concat(newPath);
    }
    if (needRemovingDots) {
        String* tmp = removingDots(newPath);
        if (tmp != newPath)
            newPath = tmp;
    }
    m_urlString = m_urlString->substring(0, m_portEnd)->concat(newPath)->concat(m_urlString->substring(m_pathEnd, m_urlString->length() - m_pathEnd));
    resolvePositions();
}

String* URL::getSearch()
{
    if (m_pathEnd != m_queryEnd)
        return m_urlString->substring(m_pathEnd, m_queryEnd - m_pathEnd);
    else
        return String::emptyString;
}

String* URL::getHash()
{
    if (m_queryEnd != m_fragmentEnd)
        return m_urlString->substring(m_queryEnd, m_fragmentEnd - m_queryEnd);
    else
        return String::emptyString;
}


}
