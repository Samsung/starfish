/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLFormElement.h"

#define MAX_PORT_DIGITS 5
#define MAX_PORT_NUMBER 65535

namespace StarFish {

static bool toHexAndAppend(StringBuilder& builder, char32_t ch)
{
    unsigned char dig1 = (ch & 0xF0) >> 4;
    unsigned char dig2 = (ch & 0x0F);

    if (dig1 > 15 || dig2 > 15) {
        return false;
    }
    char ch1 = (dig1 <= 9) ? dig1 + '0' : dig1 - 10 + 'A';
    char ch2 = (dig2 <= 9) ? dig2 + '0' : dig2 - 10 + 'A';

    builder.appendChar('%');
    builder.appendChar(ch1);
    builder.appendChar(ch2);

    return true;
}

inline static bool isDecimalDigit(char32_t ch)
{
    return ('0' <= ch && ch <= '9');
}

inline static bool isHexadecimalDigit(char32_t ch)
{
    return isDecimalDigit(ch) || ('A' <= ch && ch <= 'F') ||
           ('a' <= ch && ch <= 'f');
}

static bool isFormUrlEncodingExceptional(char32_t c)
{
    return isASCIILower(c) || isASCIIUpper(c) || isASCIIDigit(c) ||
           (c == '*') || (c == '-') || (c == '.') || (c == '_');
}

static bool isURIReserved(char32_t c)
{
    // RFC 3986 section 2.2 Reserved Characters (January 2005)
    return (c == ':') || (c == '/') || (c == '?') || (c == '#') || (c == '[') ||
           (c == ']') || (c == '@') || (c == '!') || (c == '$') || (c == '&') ||
           (c == '\'') || (c == '(') || (c == ')') || (c == '*') ||
           (c == '+') || (c == ',') || (c == ';') || (c == '=');
}

static bool isURIUnreserved(char32_t c)
{
    // RFC 3986 section 2.3 Unreserved Characters (January 2005)
    return isASCIILower(c) || isASCIIUpper(c) || isASCIIDigit(c) ||
           (c == '-') || (c == '.') || (c == '_') || (c == '~');
}

String* ResourceURL::createPercentEncodingString(String* src, bool forForm)
{
    StringBufferAccessData dat = src->bufferAccessData();

    bool needsEncoding = false;
    for (size_t i = 0; i < dat.length; i++) {
        char32_t ch32 = dat.charAt(i);
        bool urlEncoded = false;
        if (forForm) {
            if (ch32 == U' ') {
                needsEncoding = true;
                break;
            } else if (isFormUrlEncodingExceptional(ch32)) {
                urlEncoded = true;
            }
        } else {
            if (isURIReserved(ch32) || isURIUnreserved(ch32) || ch32 == U'%') {
                urlEncoded = true;
            } else {
                // needs encoding
            }
        }
        if (!urlEncoded) {
            needsEncoding = true;
            break;
        }
    }

    if (!needsEncoding) {
        return src;
    }

    StringBuilder encoded;
    for (size_t i = 0; i < dat.length; i++) {
        char32_t ch32 = dat.charAt(i);
        bool urlEncoded = false;
        if (forForm) {
            if (ch32 == U' ') {
                encoded.appendChar('+');
                urlEncoded = true;
            } else if (isFormUrlEncodingExceptional(ch32)) {
                encoded.appendChar(ch32);
                urlEncoded = true;
            }
        } else {
            if (isURIReserved(ch32) || isURIUnreserved(ch32) || ch32 == U'%') {
                encoded.appendChar(ch32);
                urlEncoded = true;
            }
        }

        if (!urlEncoded) {
            // https://tools.ietf.org/html/rfc3629#section-3
            if (ch32 <= 0x007F) {
                toHexAndAppend(encoded, ch32);
            } else if (0x0080 <= ch32 && ch32 <= 0x07FF) {
                toHexAndAppend(encoded, 0x00C0 + (ch32 & 0x07C0) / 0x0040);
                toHexAndAppend(encoded, 0x0080 + (ch32 & 0x003F));
            } else if (0x0800 <= ch32 && ch32 <= 0xFFFF) {
                toHexAndAppend(encoded, 0x00E0 + (ch32 & 0xF000) / 0x1000);
                toHexAndAppend(encoded, 0x0080 + (ch32 & 0x0FC0) / 0x0040);
                toHexAndAppend(encoded, 0x0080 + (ch32 & 0x003F));
            } else if (0x10000 <= ch32 && ch32 <= 0x10FFFF) {
                toHexAndAppend(encoded, 0x00F0 + (ch32 & 0x1C0000) / 0x40000);
                toHexAndAppend(encoded, 0x0080 + (ch32 & 0x3F000) / 0x1000);
                toHexAndAppend(encoded, 0x0080 + (ch32 & 0x0FC0) / 0x0040);
                toHexAndAppend(encoded, 0x0080 + (ch32 & 0x003F));
            } else {
                STARFISH_LOG_INFO(
                    "Got invalid unicode while convert to "
                    "PercentEncoding(URI Encoding). Ignore it");
            }
        }
    }

    return encoded.finalize();
}

static bool twoCharToHex(char32_t ch1, char32_t ch2, unsigned char* res)
{
    if (!isHexadecimalDigit(ch1) || !isHexadecimalDigit(ch2))
        return false;
    *res = (((ch1 & 0x10) ? (ch1 & 0xf) : ((ch1 & 0xf) + 9)) << 4) |
           ((ch2 & 0x10) ? (ch2 & 0xf) : ((ch2 & 0xf) + 9));
    return true;
}

inline static bool codeUnitToHex(String* str, size_t start, unsigned char* res)
{
    STARFISH_ASSERT(str && str->length() > start + 2);
    if (str->charAt(start) != '%')
        return false;
    bool succeed =
        twoCharToHex(str->charAt(start + 1), str->charAt(start + 2), res);
    // The two most significant bits of res should be 10.
    return succeed && (*res & 0xC0) == 0x80;
}

String* ResourceURL::createPercentDecodingString(String* src)
{
    StringBuilder decoded;

    for (size_t i = 0; i < src->length(); i++) {
        char32_t ch32 = src->charAt(i);
        if (ch32 != '%') {
            decoded.appendChar(ch32);
        } else {
            size_t start = i;
            if (i + 2 >= src->length())
                break;
            char32_t next = src->charAt(i + 1);
            char32_t nextnext = src->charAt(i + 2);

            // char to hex
            unsigned char b = 0;
            if (!twoCharToHex(next, nextnext, &b))
                break;
            i += 2;

            // most significant bit in b is 0
            if (!(b & 0x80)) {
                // let C be the character with code unit value B.
                // if C is not in reservedSet, then let S be the String
                // containing only the character C.
                // else, C is in reservedSet, Let S be the substring of string
                // from position start to position k included.
                const char32_t c = b & 0x7f;
                decoded.appendChar(c);
            } else { // most significant bit in b is 1
                unsigned char b_tmp = b;
                int n = 1;
                while (n < 5) {
                    b_tmp <<= 1;
                    if ((b_tmp & 0x80) == 0) {
                        break;
                    }
                    n++;
                }
                if (n == 1 || n == 5 || (i + (3 * (n - 1)) >= src->length())) {
                    break;
                }
                unsigned char octets[4];
                octets[0] = b;

                int j = 1;
                while (j < n) {
                    if (!codeUnitToHex(src, ++i, &b)) // "%XY" type
                        break;
                    i += 2;
                    octets[j] = b;
                    j++;
                }
                STARFISH_ASSERT(n == 2 || n == 3 || n == 4);
                char32_t v = 0;
                if (n == 2) {
                    v = (octets[0] & 0x1F) << 6 | (octets[1] & 0x3F);
                    if ((octets[0] == 0xC0) || (octets[0] == 0xC1)) {
                        break;
                    }
                } else if (n == 3) {
                    v = (octets[0] & 0x0F) << 12 | (octets[1] & 0x3F) << 6 |
                        (octets[2] & 0x3F);
                    if ((0xD800 <= v && v <= 0xDFFF) ||
                        ((octets[0] == 0xE0) &&
                         ((octets[1] < 0xA0) || (octets[1] > 0xBF)))) {
                        break;
                    }
                } else if (n == 4) {
                    v = (octets[0] & 0x07) << 18 | (octets[1] & 0x3F) << 12 |
                        (octets[2] & 0x3F) << 6 | (octets[3] & 0x3F);
                    if ((octets[0] == 0xF0) &&
                        ((octets[1] < 0x90) || (octets[1] > 0xBF))) {
                        break;
                    }
                }

                decoded.appendChar(v);
            }
        }
    }

    return decoded.finalize();
}

ResourceURL::ResourceURL(String* url)
    : ResourceURL(url, String::emptyString)
{
}

ResourceURL::ResourceURL(String* url, String* baseURL)
    : m_baseURL(baseURL)
{
    unsigned numLeadingSpaces = 0;
    unsigned numTrailingSpaces = 0;
    size_t urlLength = url->length();
    for (; numLeadingSpaces < urlLength; ++numLeadingSpaces) {
        if (!String::isSpaceOrNewline(url->charAt(numLeadingSpaces))) {
            break;
        }
    }
    if (numLeadingSpaces != urlLength) {
        for (; numTrailingSpaces < urlLength; ++numTrailingSpaces) {
            if (!String::isSpaceOrNewline(
                    url->charAt(urlLength - 1 - numTrailingSpaces))) {
                break;
            }
        }
        STARFISH_ASSERT(numLeadingSpaces + numTrailingSpaces < urlLength);

        if (numLeadingSpaces || numTrailingSpaces) {
            url = url->substring(numLeadingSpaces,
                                 urlLength -
                                     (numLeadingSpaces + numTrailingSpaces));
        }
    }

    url = ResourceURL::createPercentEncodingString(url, false);
    m_string = url;
    m_protocolEnd = m_usernameStart = m_usernameEnd = m_passwordEnd =
        m_hostEnd = m_portEnd = m_pathEnd = m_searchEnd = m_hashEnd = 0;
    m_isValid = false;

    parseURLString(baseURL, url);
}

void* ResourceURL::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ResourceURL));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(ResourceURL)] = { 0 };
        fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ResourceURL));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void ResourceURL::fillGCDescriptor(GC_word* desc)
{
    GC_set_bit(desc, GC_WORD_OFFSET(ResourceURL, m_string));
    GC_set_bit(desc, GC_WORD_OFFSET(ResourceURL, m_urlString));
    GC_set_bit(desc, GC_WORD_OFFSET(ResourceURL, m_baseURL));
}

bool ResourceURL::isValidURL(String* url)
{
    // TODO: checking for the validity of a url requires a regexp check.
    // For the time being, we check for valid characters only.
    if (!(url->startsWith("http://") || url->startsWith("https://") ||
          url->startsWith("file://") || url->startsWith("blob://") ||
          url->startsWith("data://") || url->startsWith("about:"))) {
        return false;
    }

    for (size_t i = 0; i < url->length(); i++) {
        if ('A' <= url->charAt(i) && url->charAt(i) <= 'Z') {
            continue;
        } else if ('a' <= url->charAt(i) && url->charAt(i) <= 'z') {
            continue;
        } else if ('0' <= url->charAt(i) && url->charAt(i) <= '9') {
            continue;
        }
        switch (url->charAt(i)) {
        case '-':
        case '.':
        case '_':
        case '~':
        case ':':
        case '/':
        case '?':
        case '#':
        case '[':
        case ']':
        case '@':
        case '!':
        case '$':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case ';':
        case '=':
        case '`':
            continue;
        default:
            return false;
        }
    }
    return true;
}

static String* removingDots(String* origPath)
{
    StringBufferAccessData str = origPath->bufferAccessData();
    size_t pos = 0;
    size_t pathLen = str.length;
    bool removed = false;

    STARFISH_ASSERT(str[pos] == '/');

    pos++;

    while (pos < pathLen) {
        if (str[pos + 0] == '.' && str[pos - 1] == '/') {
            if (pos + 1 == pathLen || str[pos + 1] == '/') {
                removed = true;
                break;
            } else if (str[pos + 1] == '.' &&
                       (pos + 2 == pathLen || str[pos + 2] == '/')) {
                removed = true;
                break;
            }
        }
        pos++;
    }

    if (!removed) {
        return origPath;
    }

    {
        UTF8StringDataNonGCStd str = origPath->toUTF8NonGCString();
        UTF8StringDataNonGCStd dst = origPath->toUTF8NonGCString();
        size_t pos = 0;
        size_t dstPos = 0;
        size_t pathLen = str.length();

        STARFISH_ASSERT(str[pos] == '/');

        pos++;
        dstPos++;

        while (pos < pathLen) {
            if (str[pos + 0] == '.' && str[pos - 1] == '/') {
                if (pos + 1 == pathLen || str[pos + 1] == '/') {
                    pos += 2;
                    continue;
                } else if (str[pos + 1] == '.' &&
                           (pos + 2 == pathLen || str[pos + 2] == '/')) {
                    pos += 3;
                    if (dstPos > 1) {
                        dstPos--;
                    }
                    while (dstPos > 0 && dst[dstPos - 1] != '/') {
                        dstPos--;
                    }
                    continue;
                }
            }
            dst[dstPos] = str[pos];
            pos++;
            dstPos++;
        }

        return String::fromUTF8(dst.data(), dstPos);
    }
}

void ResourceURL::resolvePositions()
{
    size_t pos = m_urlString->find(":");
    if (pos == SIZE_MAX) {
        return;
    }

    m_protocolEnd = pos + 1;

    bool hierarchical = m_urlString->charAt(m_protocolEnd) == '/';
    bool hasSecondSlash =
        hierarchical && m_urlString->charAt(m_protocolEnd + 1) == '/';

    // username & password
    m_usernameStart = m_protocolEnd;
    if (hierarchical) {
        m_usernameStart++;
        if (hasSecondSlash) {
            m_usernameStart++;
            if (m_protocol != FILE_PROTOCOL) {
                while (m_urlString->charAt(m_usernameStart) == '/') {
                    m_usernameStart++;
                }
            }
        }
    }

    m_usernameEnd = m_passwordEnd = m_usernameStart;

    // host
    pos = m_urlString->find("/", m_usernameStart);
    if (pos != SIZE_MAX) {
        m_hostEnd = m_portEnd = pos;

        size_t pos2 = m_urlString->find("?", pos);
        if (pos2 != SIZE_MAX) {
            m_pathEnd = pos2;
            size_t pos3 = m_urlString->find("#", pos);
            if (pos3 != SIZE_MAX) {
                m_searchEnd = pos3;
                if (m_searchEnd < m_pathEnd) {
                    m_searchEnd = m_pathEnd;
                }
                m_hashEnd = m_urlString->length();
            } else {
                m_searchEnd = m_hashEnd = m_urlString->length();
            }
        } else {
            size_t pos3 = m_urlString->find("#", pos);
            if (pos3 != SIZE_MAX) {
                m_pathEnd = m_searchEnd = pos3;
                m_hashEnd = m_urlString->length();
            } else {
                m_pathEnd = m_searchEnd = m_hashEnd = m_urlString->length();
            }
        }
    } else {
        size_t pos2 = m_urlString->find("?");
        if (pos2 != SIZE_MAX) {
            m_hostEnd = m_portEnd = m_pathEnd = pos2;
            size_t pos3 = m_urlString->find("#", pos2);
            if (pos3 != SIZE_MAX) {
                m_searchEnd = pos3;
                m_hashEnd = m_urlString->length();
            } else {
                m_searchEnd = m_hashEnd = m_urlString->length();
            }
        } else {
            size_t pos3 = m_urlString->find("#");
            if (pos3 != SIZE_MAX) {
                m_hostEnd = m_portEnd = m_pathEnd = m_searchEnd = pos3;
                m_hashEnd = m_urlString->length();
            } else {
                m_hostEnd = m_portEnd = m_pathEnd = m_searchEnd = m_hashEnd =
                    m_urlString->length();
            }
        }
    }

    // username & password & port
    pos = m_urlString->find("@", m_usernameStart);
    if (pos != SIZE_MAX && pos < m_hostEnd) {
        m_usernameEnd = m_passwordEnd = pos;

        // ':' for username
        pos = m_urlString->find(":", m_usernameStart);
        if (pos != SIZE_MAX && pos < m_usernameEnd) {
            m_usernameEnd = pos;
        }
        // ':' for port
        pos = m_urlString->find(":", m_usernameEnd + 1);
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
        m_usernameEnd = m_usernameStart;
        m_passwordEnd = m_usernameEnd;
        m_hostEnd = m_passwordEnd;
        m_portEnd = m_hostEnd;
    }

    STARFISH_ASSERT(m_protocolEnd);
    STARFISH_ASSERT(m_usernameStart);
    STARFISH_ASSERT(m_usernameEnd);
    STARFISH_ASSERT(m_passwordEnd);
    STARFISH_ASSERT(m_hostEnd);
    STARFISH_ASSERT(m_portEnd);
    STARFISH_ASSERT(m_pathEnd);
    STARFISH_ASSERT(m_searchEnd);
    STARFISH_ASSERT(m_hashEnd);
}

void ResourceURL::parseURLString(String* baseURL, String* url)
{
    size_t urlLength = url->length();
    m_isValid = true;

    bool isAbsolute = false;

    if (url->startsWith("data:", false) || url->startsWith("blob:", false) ||
        url->startsWith("about:", false) ||
        url->startsWith("javascript:", false) || url->contains("://")) {
        isAbsolute = true;
    }

    if (baseURL->equals(String::emptyString)) {
        if (!isAbsolute) {
            url = String::createASCIIString("about:blank");
            urlLength = url->length();
            isAbsolute = true;
        }
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
        if (baseURL->startsWith("file://", false)) {
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

    if (!isAbsolute && !baseURL->equals("about:blank") && !url->contains(":")) {
        STARFISH_ASSERT(baseURL->contains("://"));
        bool baseEndsWithSlash = baseURL->charAt(baseURL->length() - 1) == '/';

        if (url->startsWith("./")) {
            url = url->substring(2, url->length() - 2);
        }

        if (baseEndsWithSlash) {
            url = baseURL->concat(url);
        } else if (url->startsWith("?") || url->startsWith("#") ||
                   urlLength == 0) {
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
    } else if (m_urlString->startsWith("about", false)) {
        m_protocol = ABOUT_PROTOCOL;
    } else if (m_urlString->startsWith("javascript", false)) {
        m_protocol = JAVASCRIPT_PROTOCOL;
    } else {
        m_protocol = UNKNOWN;
    }

    resolvePositions();

    if (m_urlString->charAt(m_protocolEnd) == '/') {
        String* origPath = pathname();
        String* newPath = removingDots(origPath);
        if (newPath != origPath) {
            setPathname(newPath, false);
        }
    }

    // TODO: need to check validity for other components (protocol, host,
    // etc)
    m_isValid = isValidPort();
}

bool ResourceURL::isValidPort()
{
    String* p = port();
    size_t len = p->length();

    if (len == 0) {
        return true;
    } else if (len > MAX_PORT_DIGITS) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        if (!String::isASCIIDigit(p->charAt(i))) {
            return false;
        }
    }

    if (String::parseInt(p) > MAX_PORT_NUMBER) {
        return false;
    }

    return true;
}

String* ResourceURL::mergeDocumentURIWithURIString(Document* document,
                                                   String* url)
{
    ResourceURL u(url, document->baseURL()->baseURI());
    String* ret = u.href();
    return ret;
}

String* ResourceURL::mergeDocumentURIWithURIString(String* documentURI,
                                                   String* url)
{
    ResourceURL u(url, documentURI);
    String* ret = u.href();
    return ret;
}

String* ResourceURL::baseURI() const
{
    if (m_protocol == Protocol::DATA_PROTOCOL ||
        m_protocol == Protocol::JAVASCRIPT_PROTOCOL ||
        m_protocol == Protocol::ABOUT_PROTOCOL) {
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

String* ResourceURL::urlStringWithoutSearchPart() const
{
    size_t idx = m_urlString->lastIndexOf('?');
    if (idx != SIZE_MAX) {
        return m_urlString->substring(0, idx);
    } else {
        return m_urlString;
    }
}

String* ResourceURL::getUrlPathString() const
{
    size_t search = m_urlString->indexOf('?');
    size_t hash = m_urlString->indexOf('#');
    size_t min = search > hash ? hash : search;

    if (min != SIZE_MAX) {
        return m_urlString->substring(0, min);
    } else {
        return m_urlString;
    }
}

String* ResourceURL::origin()
{
    if (m_protocol == FILE_PROTOCOL) {
        return m_urlString->substring(0, 7)->toASCIILower(); // "file://"
    }

    if (m_protocol >= HTTP_PROTOCOL && m_protocol <= HTTPS_PROTOCOL) {
        if (!hostname()->isEmpty()) {
            return protocol()->concat("//")->concat(host());
        } else {
            return String::createASCIIString("null");
        }
    } else if (m_protocol == UNKNOWN) {
        if (!protocol()->isEmpty()) {
            return protocol()->concat("//");
        } else if (m_baseURL) {
            return (new ResourceURL(m_baseURL))->origin();
        }
    }
    return String::createASCIIString("null");
}

String* ResourceURL::href()
{
    return m_urlString;
}

ResourceURL* ResourceURL::setHref(String* newHref)
{
    return new ResourceURL(newHref);
}

String* ResourceURL::protocol()
{
    return m_urlString->substring(0, m_protocolEnd)->toASCIILower();
}

ResourceURL* ResourceURL::setProtocol(String* newProtocol)
{
    STARFISH_ASSERT(newProtocol->length());
    return new ResourceURL(newProtocol->concat(m_urlString->substring(
        m_protocolEnd - 1, m_urlString->length() - m_protocolEnd + 1)));
}

String* ResourceURL::username()
{
    return m_urlString->substring(m_usernameStart,
                                  m_usernameEnd - m_usernameStart);
}

ResourceURL* ResourceURL::setUsername(String* newUsername)
{
    if (m_protocol >= HTTP_PROTOCOL && m_protocol <= HTTPS_PROTOCOL) {
        StringBuilder builder;
        builder.appendString(m_urlString->substring(0, m_usernameStart));
        builder.appendString(newUsername);
        if (m_passwordEnd == m_usernameEnd &&
            m_usernameStart == m_usernameEnd) {
            builder.appendChar('@');
        }
        builder.appendString(m_urlString->substring(
            m_usernameEnd, m_urlString->length() - m_usernameEnd));
        return new ResourceURL(builder.finalize());
    }
    return new ResourceURL(m_urlString);
}

String* ResourceURL::password()
{
    if (m_passwordEnd != m_usernameEnd) {
        return m_urlString->substring(m_usernameEnd + 1,
                                      m_passwordEnd - m_usernameEnd - 1);
    } else {
        return String::emptyString;
    }
}

ResourceURL* ResourceURL::setPassword(String* newPassword)
{
    if (m_protocol >= HTTP_PROTOCOL && m_protocol <= HTTPS_PROTOCOL) {
        StringBuilder builder;
        // password exists
        if (m_passwordEnd != m_usernameEnd) {
            builder.appendString(m_urlString->substring(0, m_usernameEnd + 1));
            builder.appendString(newPassword);
        } else {
            builder.appendString(m_urlString->substring(0, m_usernameEnd));
            builder.appendChar(':');
            builder.appendString(newPassword);
            // user not exists
            if (m_usernameStart == m_usernameEnd) {
                builder.appendChar('@');
            }
        }
        builder.appendString(m_urlString->substring(
            m_passwordEnd, m_urlString->length() - m_passwordEnd));
        return new ResourceURL(builder.finalize());
    }
    return new ResourceURL(m_urlString);
}

String* ResourceURL::host()
{
    String* port = ResourceURL::port();
    String* hostname = ResourceURL::hostname();
    if (port != String::emptyString) {
        return hostname->concat(":")->concat(port);
    } else {
        return hostname;
    }
}

ResourceURL* ResourceURL::setHost(String* newHost)
{
    if (!newHost || newHost->isEmpty()) {
        return this;
    }
    size_t start =
        (m_passwordEnd == m_usernameStart) ? m_passwordEnd : m_passwordEnd + 1;
    StringBuilder builder;
    builder.appendString(m_urlString->substring(0, start));
    builder.appendString(newHost);
    if (newHost->find(":") == SIZE_MAX) {
        builder.appendString(m_urlString->substring(
            m_portEnd, m_urlString->length() - m_portEnd));
    }
    return new ResourceURL(builder.finalize());
}

String* ResourceURL::hostname()
{
    size_t start =
        (m_passwordEnd == m_usernameStart) ? m_passwordEnd : m_passwordEnd + 1;
    return m_urlString->substring(start, m_hostEnd - start);
}

ResourceURL* ResourceURL::setHostname(String* newHostname)
{
    if (!newHostname || newHostname->isEmpty()) {
        return this;
    }

    size_t start =
        (m_passwordEnd == m_usernameStart) ? m_passwordEnd : m_passwordEnd + 1;
    StringBuilder builder;
    builder.appendString(m_urlString->substring(0, start));
    builder.appendString(newHostname);
    builder.appendString(
        m_urlString->substring(m_portEnd, m_urlString->length() - m_portEnd));
    return new ResourceURL(builder.finalize());
}

String* ResourceURL::port()
{
    if (m_hostEnd != m_portEnd) {
        return m_urlString->substring(m_hostEnd + 1, m_portEnd - m_hostEnd - 1);
    } else {
        return String::emptyString;
    }
}

ResourceURL* ResourceURL::setPort(String* newPort)
{
    if (m_protocol >= HTTP_PROTOCOL && m_protocol <= HTTPS_PROTOCOL) {
        StringBuilder builder;
        builder.appendString(m_urlString->substring(0, m_hostEnd));

        if (newPort->length()) {
            uint16_t port = String::parseInt(newPort);
            builder.appendChar(':');
            builder.appendString(String::fromInt(port));
        }

        builder.appendString(m_urlString->substring(
            m_portEnd, m_urlString->length() - m_portEnd));
        return new ResourceURL(builder.finalize());
    } else {
        return new ResourceURL(m_urlString);
    }
}

String* ResourceURL::pathname()
{
    if (m_portEnd != m_pathEnd) {
        return m_urlString->substring(m_portEnd, m_pathEnd - m_portEnd);
    } else {
        return String::createASCIIString("/");
    }
}

ResourceURL* ResourceURL::setPathname(String* newPath, bool needRemovingDots)
{
    StringBuilder builder;
    builder.appendString(m_urlString->substring(0, m_portEnd));
    if (!newPath->length() || newPath->charAt(0) != '/') {
        newPath = String::createASCIIString("/")->concat(newPath);
    }
    if (needRemovingDots) {
        newPath = removingDots(newPath);
    }
    builder.appendString(newPath);
    return new ResourceURL(builder.finalize());
}

String* ResourceURL::search()
{
    if (m_pathEnd != m_searchEnd) {
        return m_urlString->substring(m_pathEnd, m_searchEnd - m_pathEnd);
    } else {
        return String::emptyString;
    }
}

ResourceURL* ResourceURL::setSearch(String* newSearch)
{
    StringBuilder builder;
    builder.appendString(m_urlString->substring(0, m_pathEnd));
    if (newSearch->length() && newSearch->charAt(0) != '?') {
        builder.appendChar('?');
    }
    builder.appendString(newSearch);
    builder.appendString(m_urlString->substring(
        m_searchEnd, m_urlString->length() - m_searchEnd));
    return new ResourceURL(builder.finalize());
}

String* ResourceURL::hash()
{
    if (m_searchEnd != m_hashEnd) {
        return m_urlString->substring(m_searchEnd, m_hashEnd - m_searchEnd);
    } else {
        return String::emptyString;
    }
}

ResourceURL* ResourceURL::setHash(String* newHash)
{
    StringBuilder builder;
    builder.appendString(m_urlString->substring(0, m_searchEnd));
    if (newHash->length() && newHash->charAt(0) != '#') {
        builder.appendChar('#');
    }
    builder.appendString(newHash);
    return new ResourceURL(builder.finalize());
}

DocumentURL::DocumentURL(String* url)
    : DocumentURL(url, nullptr)
{
}

DocumentURL::DocumentURL(String* url, FormSubmitData* formSubmitData)
    : ResourceURL(url)
    , m_formSubmitData(formSubmitData)
{
}

DocumentURL::DocumentURL(ResourceURL* url)
    : DocumentURL(url, nullptr)
{
}

DocumentURL::DocumentURL(ResourceURL* url, FormSubmitData* formSubmitData)
    : ResourceURL(*url)
    , m_formSubmitData(formSubmitData)
{
}

void* DocumentURL::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(DocumentURL));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(DocumentURL)] = { 0 };
        ResourceURL::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(DocumentURL, m_formSubmitData));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(DocumentURL));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

ReferrerURL::ReferrerURL(String* referrer)
    : ResourceURL(referrer)
    , m_policy(NoReferrer)
{
}

ReferrerURL::ReferrerURL(String* referrer, String* policy)
    : ReferrerURL(referrer)
{
    if (url()) {
        m_policy = policyFromString(policy);
    }
}

ReferrerURL::ReferrerURL(ResourceURL* referrer)
    : ResourceURL(*referrer)
    , m_policy(NoReferrer)
{
}

ReferrerURL::ReferrerURL(ResourceURL* referrer, String* policy)
    : ReferrerURL(referrer)
{
    if (url()) {
        m_policy = policyFromString(policy);
    }
}

void* ReferrerURL::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ReferrerURL));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(ReferrerURL)] = { 0 };
        ResourceURL::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ReferrerURL));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* ReferrerURL::referrerString(ResourceURL* url)
{
    STARFISH_ASSERT(url->url());
    switch (m_policy) {
    case NoReferrer:
        return String::emptyString;
    case NoReferrerWhenDowngrade:
        if (protocolKind() == HTTPS_PROTOCOL &&
            url->protocolKind() == HTTP_PROTOCOL) {
            return String::emptyString;
        }
        return urlString();
    case Origin:
        return origin();
    case OriginWhenCrossOrigin:
        if (origin()->equalsIgnoreCase(url->origin())) {
            return urlString();
        }
        return origin();
    case SameOrigin:
        if (origin()->equalsIgnoreCase(url->origin())) {
            return urlString();
        }
        return String::emptyString;
    case StrictOrigin:
        if (protocolKind() == HTTPS_PROTOCOL) {
            if (url->protocolKind() == HTTPS_PROTOCOL) {
                return origin();
            } else {
                return String::emptyString;
            }
        }
        return origin();
    case StrictOriginWhenCrossOrigin:
        if (origin()->equalsIgnoreCase(url->origin())) {
            return urlString();
        } else if (protocolKind() == HTTPS_PROTOCOL &&
                   url->protocolKind() == HTTP_PROTOCOL) {
            return String::emptyString;
        }
        return origin();
    case UnsafeUrl:
        return urlString();
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }

    return nullptr;
}

ReferrerURL::ReferrerPolicy ReferrerURL::policy()
{
    return m_policy;
}

bool ReferrerURL::isValidPolicy(String* policy)
{
    if (policy->equalsIgnoreCase("no-referrer") ||
        policy->equalsIgnoreCase("no-referrer-when-downgrade") ||
        policy->equalsIgnoreCase("origin") ||
        policy->equalsIgnoreCase("origin-when-cross-origin") ||
        policy->equalsIgnoreCase("same-origin") ||
        policy->equalsIgnoreCase("strict-origin") ||
        policy->equalsIgnoreCase("strict-origin-when-cross-origin") ||
        policy->equalsIgnoreCase("unsafe-url")) {
        return true;
    }
    return false;
}

ReferrerURL::ReferrerPolicy ReferrerURL::policyFromString(String* policy)
{
    if (policy->equalsIgnoreCase("no-referrer")) {
        return NoReferrer;
    } else if (policy->isEmpty() ||
               policy->equalsIgnoreCase("no-referrer-when-downgrade")) {
        return NoReferrerWhenDowngrade;
    } else if (policy->equalsIgnoreCase("origin")) {
        return Origin;
    } else if (policy->equalsIgnoreCase("origin-when-cross-origin")) {
        return OriginWhenCrossOrigin;
    } else if (policy->equalsIgnoreCase("same-origin")) {
        return SameOrigin;
    } else if (policy->equalsIgnoreCase("strict-origin")) {
        return StrictOrigin;
    } else if (policy->equalsIgnoreCase("strict-origin-when-cross-origin")) {
        return StrictOriginWhenCrossOrigin;
    } else if (policy->equalsIgnoreCase("unsafe-url")) {
        return UnsafeUrl;
    }
    return NoReferrerWhenDowngrade;
}
}
