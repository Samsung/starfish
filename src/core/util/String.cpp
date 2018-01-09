/*
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
    Copyright (c) 2015-present Samsung Electronics Co., Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StarFishConfig.h"
#include "String.h"
#include <sstream>

namespace StarFish {

String* const String::emptyString = String::createASCIIStringWithNoGC("");
String* const String::spaceString = String::createASCIIStringWithNoGC(" ");
String* const String::inheritString =
    String::createASCIIStringWithNoGC("inherit");
String* const String::initialString =
    String::createASCIIStringWithNoGC("initial");

static size_t utf8ContentLength(const char* UTF8, size_t len)
{
    size_t contentLength = 0;
    const char* bufferEnd = &UTF8[len];
    for (size_t i = 0; i < len;) {
        // ASCII byte
        if (0 == (UTF8[i] & 0x80)) {
            i += 1;
            contentLength++;
        } else {
            // Start byte for 2byte
            if (0xC0 == (UTF8[i] & 0xE0) && &UTF8[i + 1] < bufferEnd &&
                0x80 == (UTF8[i + 1] & 0xC0)) {
                i += 2;
                contentLength++;
            } else // Start byte for 3byte
                if (0xE0 == (UTF8[i] & 0xF0) && &UTF8[i + 2] < bufferEnd &&
                    0x80 == (UTF8[i + 1] & 0xC0) &&
                    0x80 == (UTF8[i + 2] & 0xC0)) {
                i += 3;
                contentLength++;
            } else // Start byte for 4byte
                if (0xF0 == (UTF8[i] & 0xF8) && &UTF8[i + 3] < bufferEnd &&
                    0x80 == (UTF8[i + 1] & 0xC0) &&
                    0x80 == (UTF8[i + 2] & 0xC0) &&
                    0x80 == (UTF8[i + 3] & 0xC0)) {
                i += 4;
                contentLength++;
            } else // Start byte for 5byte
                if (0xF8 == (UTF8[i] & 0xFC) && &UTF8[i + 4] < bufferEnd &&
                    0x80 == (UTF8[i + 1] & 0xC0) &&
                    0x80 == (UTF8[i + 2] & 0xC0) &&
                    0x80 == (UTF8[i + 3] & 0xC0) &&
                    0x80 == (UTF8[i + 4] & 0xC0)) {
                i += 5;
                contentLength++;
            } else // Start byte for 6byte
                if (0xFC == (UTF8[i] & 0xFE) && &UTF8[i + 5] < bufferEnd &&
                    0x80 == (UTF8[i + 1] & 0xC0) &&
                    0x80 == (UTF8[i + 2] & 0xC0) &&
                    0x80 == (UTF8[i + 3] & 0xC0) &&
                    0x80 == (UTF8[i + 4] & 0xC0) &&
                    0x80 == (UTF8[i + 5] & 0xC0)) {
                i += 6;
                contentLength++;
            } else {
                i += 1;
                contentLength++;
            }
        }
    }

    return contentLength;
}

size_t utf8ToUtf32(const char* UTF8, const char* bufferEnd, char32_t& uc)
{
    size_t tRequiredSize = 0;

    uc = 0x00000000;

    // ASCII byte
    if (0 == (UTF8[0] & 0x80)) {
        uc = (char32_t)UTF8[0];
        tRequiredSize = 1;
    } else // Start byte for 2byte
        if (0xC0 == (UTF8[0] & 0xE0) && &UTF8[1] < bufferEnd &&
            0x80 == (UTF8[1] & 0xC0)) {
        uc += (UTF8[0] & 0x1F) << 6;
        uc += (UTF8[1] & 0x3F) << 0;
        tRequiredSize = 2;
    } else // Start byte for 3byte
        if (0xE0 == (UTF8[0] & 0xF0) && &UTF8[2] < bufferEnd &&
            0x80 == (UTF8[1] & 0xC0) && 0x80 == (UTF8[2] & 0xC0)) {
        uc += (UTF8[0] & 0x0F) << 12;
        uc += (UTF8[1] & 0x3F) << 6;
        uc += (UTF8[2] & 0x3F) << 0;
        tRequiredSize = 3;
    } else // Start byte for 4byte
        if (0xF0 == (UTF8[0] & 0xF8) && &UTF8[3] < bufferEnd &&
            0x80 == (UTF8[1] & 0xC0) && 0x80 == (UTF8[2] & 0xC0) &&
            0x80 == (UTF8[3] & 0xC0)) {
        uc += (UTF8[0] & 0x07) << 18;
        uc += (UTF8[1] & 0x3F) << 12;
        uc += (UTF8[2] & 0x3F) << 6;
        uc += (UTF8[3] & 0x3F) << 0;
        tRequiredSize = 4;
    } else // Start byte for 5byte
        if (0xF8 == (UTF8[0] & 0xFC) && &UTF8[4] < bufferEnd &&
            0x80 == (UTF8[1] & 0xC0) && 0x80 == (UTF8[2] & 0xC0) &&
            0x80 == (UTF8[3] & 0xC0) && 0x80 == (UTF8[4] & 0xC0)) {
        uc += (UTF8[0] & 0x03) << 24;
        uc += (UTF8[1] & 0x3F) << 18;
        uc += (UTF8[2] & 0x3F) << 12;
        uc += (UTF8[3] & 0x3F) << 6;
        uc += (UTF8[4] & 0x3F) << 0;
        tRequiredSize = 5;
    } else // Start byte for 6byte
        if (0xFC == (UTF8[0] & 0xFE) && &UTF8[5] < bufferEnd &&
            0x80 == (UTF8[1] & 0xC0) && 0x80 == (UTF8[2] & 0xC0) &&
            0x80 == (UTF8[3] & 0xC0) && 0x80 == (UTF8[4] & 0xC0) &&
            0x80 == (UTF8[5] & 0xC0)) {
        uc += (UTF8[0] & 0x01) << 30;
        uc += (UTF8[1] & 0x3F) << 24;
        uc += (UTF8[2] & 0x3F) << 18;
        uc += (UTF8[3] & 0x3F) << 12;
        uc += (UTF8[4] & 0x3F) << 6;
        uc += (UTF8[5] & 0x3F) << 0;
        tRequiredSize = 6;
    } else {
        tRequiredSize = 1;
        uc = 0xFFFD;
    }

    return tRequiredSize;
}

size_t utf32ToUtf8(char32_t uc, char* UTF8)
{
    size_t tRequiredSize = 0;

    if (uc <= 0x7f) {
        if (NULL != UTF8) {
            UTF8[0] = (char)uc;
            UTF8[1] = (char)'\0';
        }
        tRequiredSize = 1;
    } else if (uc <= 0x7ff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xc0 + uc / (0x01 << 6));
            UTF8[1] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[2] = (char)'\0';
        }
        tRequiredSize = 2;
    } else if (uc <= 0xffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xe0 + uc / (0x01 << 12));
            UTF8[1] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[2] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[3] = (char)'\0';
        }
        tRequiredSize = 3;
    } else if (uc <= 0x1fffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xf0 + uc / (0x01 << 18));
            UTF8[1] = (char)(0x80 + uc / (0x01 << 12) % (0x01 << 12));
            UTF8[2] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[3] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[4] = (char)'\0';
        }
        tRequiredSize = 4;
    } else if (uc <= 0x3ffffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xf8 + uc / (0x01 << 24));
            UTF8[1] = (char)(0x80 + uc / (0x01 << 18) % (0x01 << 18));
            UTF8[2] = (char)(0x80 + uc / (0x01 << 12) % (0x01 << 12));
            UTF8[3] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[4] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[5] = (char)'\0';
        }
        tRequiredSize = 5;
    } else if (uc <= 0x7fffffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xfc + uc / (0x01 << 30));
            UTF8[1] = (char)(0x80 + uc / (0x01 << 24) % (0x01 << 24));
            UTF8[2] = (char)(0x80 + uc / (0x01 << 18) % (0x01 << 18));
            UTF8[3] = (char)(0x80 + uc / (0x01 << 12) % (0x01 << 12));
            UTF8[4] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[5] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[6] = (char)'\0';
        }
        tRequiredSize = 6;
    } else {
        return utf32ToUtf8(0xFFFD, UTF8);
    }

    return tRequiredSize;
}

const char* utf32ToUtf8(const char32_t* t, const size_t& len,
                        size_t* bufferSize = NULL)
{
    unsigned strLength = 0;
    char buffer[8];
    for (size_t i = 0; i < len; i++) {
        int length = utf32ToUtf8(t[i], buffer);
        strLength += length;
    }

    char* result = (char*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(strLength + 1);
    if (bufferSize)
        *bufferSize = strLength + 1;
    unsigned currentPosition = 0;

    for (size_t i = 0; i < len; i++) {
        int length = utf32ToUtf8(t[i], buffer);
        memcpy(&result[currentPosition], buffer, length);
        currentPosition += length;
    }
    result[strLength] = 0;

    return result;
}

const char* utf16ToUtf8(const char16_t* t, const size_t& len,
                        size_t* bufferSize = NULL)
{
    unsigned strLength = 0;
    char buffer[8];
    for (size_t i = 0; i < len;) {
        char32_t c;
        U16_NEXT(t, i, len, c);
        int length = utf32ToUtf8(c, buffer);
        strLength += length;
    }

    char* result = (char*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(strLength + 1);
    if (bufferSize)
        *bufferSize = strLength + 1;
    unsigned currentPosition = 0;

    for (size_t i = 0; i < len; i++) {
        char32_t c;
        U16_NEXT(t, i, len, c);
        int length = utf32ToUtf8(c, buffer);
        memcpy(&result[currentPosition], buffer, length);
        currentPosition += length;
    }
    result[strLength] = 0;

    return result;
}

UTF8StringDataNonGCStd utf32ToUtf8(const char32_t* str, size_t start,
                                   size_t end, bool ignoreZeroWidthChar)
{
    UTF8StringDataNonGCStd ret;
    ret.reserve((end - start) * 2);
    char buffer[8];
    for (size_t i = start; i < end; i++) {
        if (ignoreZeroWidthChar && String::isZeroWidthChar(str[i])) {
            continue;
        }
        size_t length = utf32ToUtf8(str[i], buffer);
        if (length == 1) {
            ret.push_back(buffer[0]);
        } else if (length == 2) {
            ret.push_back(buffer[0]);
            ret.push_back(buffer[1]);
        } else if (length == 3) {
            ret.push_back(buffer[0]);
            ret.push_back(buffer[1]);
            ret.push_back(buffer[2]);
        } else if (length == 4) {
            ret.push_back(buffer[0]);
            ret.push_back(buffer[1]);
            ret.push_back(buffer[2]);
            ret.push_back(buffer[3]);
        } else if (length == 5) {
            ret.push_back(buffer[0]);
            ret.push_back(buffer[1]);
            ret.push_back(buffer[2]);
            ret.push_back(buffer[3]);
            ret.push_back(buffer[4]);
        } else if (length == 6) {
            ret.push_back(buffer[0]);
            ret.push_back(buffer[1]);
            ret.push_back(buffer[2]);
            ret.push_back(buffer[3]);
            ret.push_back(buffer[4]);
            ret.push_back(buffer[5]);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return ret;
}

const char* utf32ToUtf8IgnoreZeroWidthChar(const char32_t* t, const size_t& len,
                                           size_t* bufferSize = NULL)
{
    unsigned strLength = 0;
    char buffer[8];
    for (size_t i = 0; i < len; i++) {
        if (String::isZeroWidthChar(t[i])) {
            continue;
        }
        int length = utf32ToUtf8(t[i], buffer);
        strLength += length;
    }

    char* result = (char*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(strLength + 1);
    if (bufferSize) {
        *bufferSize = strLength + 1;
    }
    unsigned currentPosition = 0;

    for (size_t i = 0; i < len; i++) {
        if (String::isZeroWidthChar(t[i])) {
            continue;
        }
        int length = utf32ToUtf8(t[i], buffer);
        memcpy(&result[currentPosition], buffer, length);
        currentPosition += length;
    }
    result[strLength] = 0;

    return result;
}

StringDataUTF32::StringDataUTF32(const char* src, size_t len)
    : String()
{
    size_t utf32Length = utf8ContentLength(src, len);
    UTF32String data;
    data.resize(utf32Length);
    const char* end = src + len;
    size_t i = 0;
    char32_t c;
    while (end != src) {
        if (LIKELY(0 == (src[0] & 0x80))) {
            data[i++] = src[0];
            src++;
        } else {
            src += utf8ToUtf32(src, end, c);
            data[i++] = c;
        }
    }
    m_data = SimpleStringBufferHolder<char32_t>(
        data.takeBuffer(), utf32Length,
        SimpleStringBufferHolder<char32_t>::TakeBufferValue);
}

StringDataBMP::StringDataBMP(const char* src, size_t len)
    : String()
{
    size_t utf32Length = utf8ContentLength(src, len);
    BMPString data;
    data.resize(utf32Length);
    const char* end = src + len;
    size_t i = 0;
    char32_t c;
    while (end != src) {
        if (LIKELY(0 == (src[0] & 0x80))) {
            data[i++] = src[0];
            src++;
        } else {
            src += utf8ToUtf32(src, end, c);
            data[i++] = c;
        }
    }
    m_data = SimpleStringBufferHolder<char16_t>(
        data.takeBuffer(), utf32Length,
        SimpleStringBufferHolder<char16_t>::TakeBufferValue);
}

size_t String::indexOf(char32_t ch) const
{
    auto data = bufferAccessData();
    for (size_t i = 0; i < data.length; i++) {
        if (data.charAt(i) == ch) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t String::lastIndexOf(char32_t ch) const
{
    auto data = bufferAccessData();
    for (size_t i = data.length; i > 0; i--) {
        if (data.charAt(i - 1) == ch) {
            return i - 1;
        }
    }
    return SIZE_MAX;
}

bool String::equalsIgnoreCase(const char* str) const
{
#ifndef NDEBUG
    {
        const char* c = str;
        while (*c) {
            STARFISH_ASSERT(!(*c & 0x80));
            c++;
        }
    }
#endif
    auto data = bufferAccessData();
    size_t srcLen = strlen(str);

    if (srcLen != data.length) {
        return false;
    }
    for (size_t i = 0; i < data.length; i++) {
        if (tolower(data.charAt(i)) != tolower((char32_t)str[i])) {
            return false;
        }
    }
    return true;
}

bool String::equals(const char* str) const
{
    size_t srcLen = 0;
    for (; str[srcLen]; srcLen++) {
    }

    auto data = bufferAccessData();

    if (srcLen != data.length) {
        return false;
    }

    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        return memcmp(data.asciiData(), str, data.length) == 0;
    } else {
        for (size_t i = 0; i < data.length; i++) {
            if (data.charAt(i) != (char32_t)str[i]) {
                return false;
            }
        }
        return true;
    }
}

bool String::equals(const char32_t* str) const
{
    auto data = bufferAccessData();
    size_t srcLen = 0;
    for (; str[srcLen]; srcLen++) {
    }

    if (srcLen != data.length) {
        return false;
    }
    for (size_t i = 0; i < data.length; i++) {
        if (data.charAt(i) != str[i]) {
            return false;
        }
    }
    return true;
}

bool String::containsWhitespace(size_t start, size_t end)
{
    auto data = bufferAccessData();
    if (end == SIZE_MAX) {
        end = data.length;
    }

    for (size_t i = start; i < end; i++) {
        if (isASCIISpace(data.charAt(i))) {
            return true;
        }
    }
    return false;
}

bool String::containsOnlyWhitespace(size_t start, size_t end)
{
    auto data = bufferAccessData();
    if (end == SIZE_MAX) {
        end = data.length;
    }

    for (size_t i = start; i < end; i++) {
        if (!isASCIISpace(data.charAt(i))) {
            return false;
        }
    }
    return true;
}

bool String::containsOnlyASCIIChars() const
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData)
        return true;

    for (size_t i = 0; i < data.length; i++) {
        const char32_t c = data.charAt(i);
        if (c > 127) {
            return false;
        }
    }
    return true;
}

bool String::containsOnlyDigits() const
{
    auto data = bufferAccessData();
    for (size_t i = 0; i < data.length; i++) {
        const char32_t c = data.charAt(i);
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

String* String::stripAndCollapseASCIIwhitespace()
{
    StringBuilder sb;

    size_t len = length();
    size_t pt = 0;

    for (; pt < len; pt++) {
        if (!isASCIISpace(pt)) {
            break;
        }
    }

    bool inSpaceMode = false;
    for (; pt < (len - 1); pt++) {
        char32_t ch = charAt(pt);
        if (inSpaceMode) {
            if (isASCIISpace(ch)) {
            } else {
                sb.appendChar(ch);
                inSpaceMode = false;
            }
        } else {
            if (isASCIISpace(ch)) {
                inSpaceMode = true;
                sb.appendChar(' ');
            } else {
                sb.appendChar(ch);
            }
        }
    }

    char32_t ch = charAt(pt);
    if (!isASCIISpace(ch)) {
        sb.appendChar(ch);
    }

    if (len == sb.contentLength()) {
        return this;
    }
    return sb.finalize();
}

icu::UnicodeString String::toUnicodeString() const
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        return icu::UnicodeString((const char*)data.buffer, data.length,
                                  US_INV);
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        return icu::UnicodeString((const UChar*)data.buffer, data.length);
    } else {
        return icu::UnicodeString::fromUTF32((const UChar32*)data.buffer,
                                             data.length);
    }
}

icu::UnicodeString String::toUnicodeString(size_t start, size_t end) const
{
    auto data = bufferAccessData();
    size_t len = end - start;
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        return icu::UnicodeString((const char*)data.buffer + start, len,
                                  US_INV);
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        return icu::UnicodeString((const UChar*)data.buffer + start, len);
    } else {
        return icu::UnicodeString::fromUTF32(
            (const UChar32*)data.buffer + start, len);
    }
}

size_t String::hashValueSlowCase() const
{
    auto data = bufferAccessData();
    size_t len = data.length;
    size_t hash;
    if (LIKELY(data.bufferDataKind == StringBufferAccessData::ASCIIData)) {
        auto ptr = (const char*)data.buffer;
        hash = stringHash(ptr, len);
    } else if (LIKELY(data.bufferDataKind == StringBufferAccessData::BMPData)) {
        auto ptr = (const char16_t*)data.buffer;
        hash = stringHash(ptr, len);
    } else {
        auto ptr = (const char32_t*)data.buffer;
        hash = stringHash(ptr, len);
    }

    if (UNLIKELY((hash % sizeof(size_t)) == 0)) {
        hash++;
    }

    m_hashValue = hash;
    return hash;
}

bool isBMP(char32_t ch)
{
    return U_IS_BMP(ch);
}

String* String::fromUTF8(const char* src, size_t len)
{
    bool isAllBMP = true;
    bool isAllASCII = true;
    for (unsigned i = 0; i < len;) {
        if (src[i] & 0x80) {
            isAllASCII = false;
            char32_t uc;
            i += utf8ToUtf32(src + i, src + len, uc);
            if (!isBMP(uc)) {
                isAllBMP = false;
                break;
            }
        } else {
            i++;
        }
    }

    if (isAllASCII) {
        return new StringDataASCII(src, len);
    } else if (isAllBMP) {
        return new StringDataBMP(src, len);
    } else {
        return new StringDataUTF32(src, len);
    }
}

String* String::fromUTF8(const char* str)
{
    const char* p = str;
    size_t len = strlen(str);
    return fromUTF8(str, len);
}

String* String::fromUTF16(const char16_t* src, size_t len)
{
    for (unsigned i = 0; i < len; i++) {
        if (src[i] > 127) {
            bool isAllBMP = true;
            for (size_t i = 0; i < len; i++) {
                if (U16_IS_LEAD(src[i])) {
                    isAllBMP = false;
                    break;
                }
            }
            if (isAllBMP) {
                BMPString utf32;
                for (size_t i = 0; i < len; i++) {
                    utf32 += src[i];
                }
                return new StringDataBMP(std::move(utf32));
            } else {
                UTF32String utf32;
                for (size_t i = 0; i < len; /* U16_NEXT post-increments */) {
                    char32_t c;
                    U16_NEXT(src, i, len, c);
                    utf32 += c;
                }
                return new StringDataUTF32(std::move(utf32));
            }
        }
    }

    ASCIIString ascii;
    for (unsigned i = 0; i < len; i++) {
        ascii.push_back((char)src[i]);
    }

    return new StringDataASCII(std::move(ascii));
}

String* String::createASCIIString(const char c)
{
    char s[2] = { c, '\0' };
    return new StringDataASCII(s);
}

String* String::createASCIIString(const char* str)
{
    return new StringDataASCII(str);
}

String* String::createASCIIStringWithNoGC(const char* str)
{
    return new StringDataNonGCASCII(str);
}

String* String::createUTF32String(const UTF32String& src)
{
    return new StringDataUTF32(src);
}

String* String::createUTF32String(char32_t c)
{
    if (c < 128) {
        char s[2] = { (char)c, '\0' };
        return new StringDataASCII(s);
    } else if (isBMP(c)) {
        char16_t s[2] = { (char16_t)c, '\0' };
        return new StringDataBMP(s);
    }
    char32_t s[2] = { c, '\0' };
    return new StringDataUTF32(s);
}

String* String::createASCIIStringFromUTF32Source(const UTF32String& src)
{
#ifndef NDEBUG
    for (size_t i = 0; i < src.length(); i++) {
        const char32_t c = src[i];
        STARFISH_ASSERT(c < 128);
    }
#endif
    ASCIIString ascii;
    for (size_t i = 0; i < src.length(); i++) {
        ascii.push_back(src[i]);
    }
    return new StringDataASCII(std::move(ascii));
}

String* String::createBMPStringFromUTF32Source(const UTF32String& src)
{
#ifndef NDEBUG
    for (size_t i = 0; i < src.length(); i++) {
        const char32_t c = src[i];
        STARFISH_ASSERT(c < 0xffff);
    }
#endif
    BMPString ret;
    for (size_t i = 0; i < src.length(); i++) {
        ret.push_back(src[i]);
    }
    return new StringDataBMP(std::move(ret));
}

String* String::createASCIIStringFromUTF32SourceIfPossible(
    const UTF32String& src)
{
    for (size_t i = 0; i < src.length(); i++) {
        const char32_t c = src[i];
        if (c > 127) {
            bool isAllBMP = true;
            for (; i < src.length(); i++) {
                if (!isBMP(src[i])) {
                    isAllBMP = false;
                    break;
                }
            }
            if (isAllBMP) {
                BMPString utf32;
                for (size_t i = 0; i < src.length(); i++) {
                    utf32 += src[i];
                }
                return new StringDataBMP(std::move(utf32));
            } else {
                return new StringDataUTF32(src);
            }
        }
    }
    ASCIIString ascii;
    for (size_t i = 0; i < src.length(); i++) {
        ascii.push_back(src[i]);
    }
    return new StringDataASCII(std::move(ascii));
}

NullableUTF8String String::toNullableUTF8String()
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        return NullableUTF8String(data.asciiData(), data.length);
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        size_t len = 0;
        const char* ptr = utf16ToUtf8(data.utf16Data(), data.length, &len);
        return NullableUTF8String(ptr, len - 1);
    } else {
        size_t len = 0;
        const char* ptr = utf32ToUtf8(data.utf32Data(), data.length, &len);
        return NullableUTF8String(ptr, len - 1);
    }
}

String* String::substring(size_t pos, size_t len)
{
    if (!len) {
        return String::emptyString;
    }
    return new StringView(this, pos, pos + len);
}

String* String::remove(size_t pos, size_t len)
{
    StringBuilder sb;
    sb.appendSubString(this, 0, pos);
    sb.appendSubString(this, pos + len, length());
    return sb.finalize();
}

String* String::insert(String* str, size_t pos)
{
    StringBuilder sb;
    sb.appendSubString(this, 0, pos);
    sb.appendString(str);
    sb.appendSubString(this, pos, length());
    return sb.finalize();
}

String* String::toUpper()
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        for (size_t i = 0; i < data.length; i++) {
            if (::islower(data.asciiData()[i])) {
                ASCIIString str(data.asciiData(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::toupper);
                return new StringDataASCII(std::move(str));
            }
        }
        return this;
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        for (size_t i = 0; i < data.length; i++) {
            if (u_islower(data.utf16Data()[i])) {
                BMPString str(data.utf16Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), u_toupper);
                return new StringDataBMP(std::move(str));
            }
        }
        return this;
    } else {
        for (size_t i = 0; i < data.length; i++) {
            if (u_islower(data.utf32Data()[i])) {
                UTF32String str(data.utf32Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), u_toupper);
                return new StringDataUTF32(std::move(str));
            }
        }
        return this;
    }
}

String* String::toASCIIUpper()
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        for (size_t i = 0; i < data.length; i++) {
            if (::islower(data.asciiData()[i])) {
                ASCIIString str(data.asciiData(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::toupper);
                return new StringDataASCII(std::move(str));
            }
        }
        return this;
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        for (size_t i = 0; i < data.length; i++) {
            if (::islower(data.utf16Data()[i])) {
                BMPString str(data.utf16Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::toupper);
                return new StringDataBMP(std::move(str));
            }
        }
        return this;
    } else {
        for (size_t i = 0; i < data.length; i++) {
            if (::islower(data.utf32Data()[i])) {
                UTF32String str(data.utf32Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::toupper);
                return new StringDataUTF32(std::move(str));
            }
        }
        return this;
    }
}

String* String::toLower()
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        for (size_t i = 0; i < data.length; i++) {
            if (::isupper(data.asciiData()[i])) {
                ASCIIString str(data.asciiData(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                return new StringDataASCII(std::move(str));
            }
        }
        return this;
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        for (size_t i = 0; i < data.length; i++) {
            if (u_isupper(data.utf16Data()[i])) {
                BMPString str(data.utf16Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), u_tolower);
                return new StringDataBMP(std::move(str));
            }
        }
        return this;
    } else {
        for (size_t i = 0; i < data.length; i++) {
            if (u_isupper(data.utf32Data()[i])) {
                UTF32String str(data.utf32Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), u_tolower);
                return new StringDataUTF32(std::move(str));
            }
        }
        return this;
    }
}

String* String::toASCIILower()
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        for (size_t i = 0; i < data.length; i++) {
            if (::isupper(data.asciiData()[i])) {
                ASCIIString str(data.asciiData(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                return new StringDataASCII(std::move(str));
            }
        }
        return this;
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        for (size_t i = 0; i < data.length; i++) {
            if (::isupper(data.utf16Data()[i])) {
                BMPString str(data.utf16Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                return new StringDataBMP(std::move(str));
            }
        }
        return this;
    } else {
        for (size_t i = 0; i < data.length; i++) {
            if (::isupper(data.utf32Data()[i])) {
                UTF32String str(data.utf32Data(), data.length);
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                return new StringDataUTF32(std::move(str));
            }
        }
        return this;
    }
}

String* String::concat(const char c)
{
    if (length() == 0) {
        return String::createASCIIString(c);
    }

    StringBuilder builder;
    builder.appendString(this);
    builder.appendChar(c);
    return builder.finalize();
}

String* String::concat(const char32_t c)
{
    if (length() == 0) {
        return String::createUTF32String(c);
    }
    StringBuilder builder;
    builder.appendString(this);
    builder.appendChar(c);
    return builder.finalize();
}

String* String::concat(const char* src)
{
    if (length() == 0) {
        return String::createASCIIString(src);
    }
    size_t srcLen = strlen(src);
    if (srcLen == 0) {
        return this;
    }

    StringBuilder builder;
    builder.appendString(this);
    builder.appendString(src);
    return builder.finalize();
}

String* String::concat(String* str)
{
    if (length() == 0) {
        return str;
    }
    if (str->length() == 0) {
        return this;
    }

    StringBuilder builder;
    builder.appendString(this);
    builder.appendString(str);
    return builder.finalize();
}

String* String::replaceAll(String* from, String* to)
{
    auto data = bufferAccessData();

    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        std::string str = std::string(toUTF8NonGCString());
        std::string from_str = std::string(from->toUTF8NonGCString());
        std::string to_str = std::string(to->toUTF8NonGCString());

        size_t start_pos = 0;
        while ((start_pos = str.find(from_str, start_pos)) !=
               std::string::npos) {
            str.replace(start_pos, from_str.length(), to_str);
            start_pos += to_str.length(); // Handles case where 'to' is a
                                          // substring of 'from'
        }
        return fromUTF8(str.c_str(), str.length());
    } else {
        UTF32String str(toUTF32String().data());
        UTF32String from_str(from->toUTF32String().data());
        UTF32String to_str(to->toUTF32String().data());

        size_t start_pos = 0;
        while ((start_pos = str.find(from_str, start_pos)) !=
               std::string::npos) {
            str.replace(start_pos, from_str.length(), to_str);
            start_pos += to_str.length(); // Handles case where 'to' is a
                                          // substring of 'from'
        }
        return createUTF32String(str);
    }
}

String* String::fromFloat(float f)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%g", f);
    return String::fromUTF8(buf);
}

String* String::fromInt(int i)
{
    return String::fromUTF8(std::to_string(i).c_str());
}

String* String::fromInt64(int64_t i)
{
    return String::fromUTF8(std::to_string(i).c_str());
}

String* String::trim()
{
    size_t first = 0;
    size_t last = 0;
    if (length()) {
        last = length() - 1;

        for (size_t i = 0; i < length(); i++) {
            if (!String::isSpaceOrNewline(charAt(i))) {
                first = i;
                break;
            }
        }

        do {
            if (!String::isSpaceOrNewline(charAt(last))) {
                break;
            }
        } while (last--);
    } else {
        return this;
    }

    if (first == 0 && ((last + 1) == length())) {
        return this;
    }

    return substring(first, (last - first + 1));
}

void StringUtils::tokenize(String* src, const char* tokens, size_t tokensLength,
                           GCVector<StringView>& result)
{
    auto accessData = src->bufferAccessData();

    size_t start = 0;
    size_t end = 0;
    bool isToken = false;
    for (size_t i = 0; i < accessData.length; i++) {
        char32_t c = accessData.charAt(i);
        isToken = false;
        for (size_t j = 0; j < tokensLength; j++) {
            if (c == (char32_t)tokens[j]) {
                isToken = true;
                break;
            }
        }

        if (isToken) {
            result.emplace_back(src, start, end);
            end = start = i + 1;
        } else {
            end++;
        }
    }

    if (end - start) {
        result.emplace_back(src, start, end);
    }

    if (isToken) {
        result.emplace_back(src, end, end);
    }
}

bool StringUtils::equalsIgnoreCase(const std::string& a, const std::string& b)
{
    unsigned int sz = a.size();
    if (b.size() != sz) {
        return false;
    }
    for (unsigned int i = 0; i < sz; ++i) {
        if (tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }
    return true;
}

// trim from start (in place)
void StringUtils::ltrim(std::string& s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
void StringUtils::rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace)))
                .base(),
            s.end());
}

// trim from both ends (in place)
void StringUtils::trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
std::string StringUtils::ltrimmed(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
std::string StringUtils::rtrimmed(std::string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
std::string StringUtils::trimmed(std::string s)
{
    trim(s);
    return s;
}

void StringUtils::skipSpaces(const std::string& input,
                             unsigned long int& startIndex)
{
    while (startIndex < input.length() && input[startIndex] == ' ') {
        ++startIndex;
    }
}

std::vector<std::string> StringUtils::split(const std::string& s,
                                            char seperator)
{
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find(seperator, pos)) != std::string::npos) {
        std::string substring(s.substr(prev_pos, pos - prev_pos));
        output.push_back(substring);
        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word
    return output;
}

int utf32ToUtf16(char32_t i, char16_t* u)
{
    if (i <= 0xffff) {
        if (i >= 0xd800 && i <= 0xdfff) {
            // illegal conversion
            *u = 0xFFFD;
        } else {
            // normal case
            *u = (char16_t)(i & 0xffff);
        }
        return 1;
    } else if (i < 0x10ffff) {
        i -= 0x10000;
        *u++ = 0xd800 | (i >> 10);
        *u = 0xdc00 | (i & 0x3ff);
        return 2;
    } else {
        // produce error char
        // U+FFFD
        *u = 0xFFFD;
        return 1;
    }
}

UTF8String String::toUTF8String()
{
    UTF8String result;

    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        result.append(data.asciiData(), data.length);
    } else {
        result.reserve(data.length);
        for (size_t i = 0; i < data.length; i++) {
            char buffer[16];
            size_t len = utf32ToUtf8(data.charAt(i), buffer);
            result.append(buffer, len);
        }
    }

    return result;
}

UTF32String String::toUTF32String()
{
    UTF32String result;

    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        result.reserve(data.length);
        for (size_t i = 0; i < data.length; i++) {
            result.push_back(data.asciiData()[i]);
        }
    } else {
        result.reserve(data.length);
        for (size_t i = 0; i < data.length; i++) {
            result.push_back(data.charAt(i));
        }
    }

    return result;
}

UTF16String String::toUTF16String() const
{
    UTF16String out;
    size_t len = length();
    out.reserve(len);
    for (size_t i = 0; i < len; i++) {
        char32_t src = charAt(i);
        char16_t dst[2];
        int ret = utf32ToUtf16(src, dst);

        if (LIKELY(ret == 1)) {
            out.push_back(src);
        } else if (ret == 2) {
            out.push_back(dst[0]);
            out.push_back(dst[1]);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return out;
}

UTF16StringDataNonGCStd String::toUTF16NonGCString(size_t start,
                                                   size_t end) const
{
    UTF16StringDataNonGCStd out;

    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        out.assign(data.asciiData() + start, data.asciiData() + end);
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        out.reserve(end - start);
        for (size_t i = start; i < end; i++) {
            out.push_back(data.utf16Data()[i]);
        }
    } else {
        out.reserve(end - start);
        for (size_t i = start; i < end; i++) {
            char32_t src = charAt(i);
            char16_t dst[2];
            int ret = utf32ToUtf16(src, dst);

            if (LIKELY(ret == 1)) {
                out.push_back(src);
            } else if (ret == 2) {
                out.push_back(dst[0]);
                out.push_back(dst[1]);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }
    return out;
}

UTF16StringDataNonGCStd String::toUTF16NonGCString() const
{
    return toUTF16NonGCString(0, length());
}

UTF8StringDataNonGCStd String::toUTF8NonGCString(size_t start, size_t end,
                                                 bool ignoreZeroWidthChar) const
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        UTF8StringDataNonGCStd ret;
        ret.reserve(end - start);
        for (size_t i = start; i < end; i++) {
            char ch = data.asciiData()[i];
            if (ignoreZeroWidthChar && String::isZeroWidthChar(ch)) {
                continue;
            }
            ret.push_back(ch);
        }
        return ret;
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        UTF8StringDataNonGCStd result;
        result.reserve(end - start);
        for (size_t i = start; i < end; i++) {
            char buffer[16];
            if (ignoreZeroWidthChar && isZeroWidthChar(data.utf16Data()[i])) {
                continue;
            }
            size_t len = utf32ToUtf8(data.utf16Data()[i], buffer);
            result.append(buffer, len);
        }
        return result;
    } else {
        return utf32ToUtf8(data.utf32Data(), start, end, ignoreZeroWidthChar);
    }
}

UTF8StringDataNonGCStd String::toUTF8NonGCString() const
{
    auto data = bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        UTF8StringDataNonGCStd ret(data.asciiData(), data.length);
        return ret;
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        UTF8StringDataNonGCStd result;
        result.reserve(data.length);
        for (size_t i = 0; i < data.length; i++) {
            char buffer[16];
            size_t len = utf32ToUtf8(data.utf16Data()[i], buffer);
            result.append(buffer, len);
        }
        return result;
    } else {
        return utf32ToUtf8(data.utf32Data(), 0, data.length, false);
    }
}

static bool bufferEqual(StringBufferAccessData& s, StringBufferAccessData& s1,
                        const size_t& len)
{
    for (size_t i = 0; i < len; i++) {
        if (s.charAt(i) != s1.charAt(i)) {
            return false;
        }
    }
    return true;
}

bool String::equals(const String* str) const
{
    auto dataA = bufferAccessData();
    auto dataB = str->bufferAccessData();

    if (dataA.length == dataB.length) {
        bool aa = dataA.bufferDataKind == StringBufferAccessData::ASCIIData;
        bool bb = dataB.bufferDataKind == StringBufferAccessData::ASCIIData;
        if (aa && bb) {
            return stringEqual(dataA.asciiData(), dataB.asciiData(),
                               dataA.length);
        } else {
            return bufferEqual(dataA, dataB, dataA.length);
        }
    }
    return false;
}

template <typename T>
bool stringEqualWithoutCase(const T* s, const T* s1, const size_t& len)
{
    for (size_t i = 0; i < len; i++) {
        if (tolower(s[i]) != tolower(s1[i])) {
            return false;
        }
    }
    return true;
}

static bool bufferEqualWithoutCase(StringBufferAccessData& s,
                                   StringBufferAccessData& s1,
                                   const size_t& len)
{
    for (size_t i = 0; i < len; i++) {
        if (towlower(s.charAt(i)) != towlower(s1.charAt(i))) {
            return false;
        }
    }
    return true;
}

bool String::equalsIgnoreCase(const String* str) const
{
    auto dataA = bufferAccessData();
    auto dataB = str->bufferAccessData();

    if (dataA.length == dataB.length) {
        bool aa = dataA.bufferDataKind == StringBufferAccessData::ASCIIData;
        bool bb = dataB.bufferDataKind == StringBufferAccessData::ASCIIData;
        if (aa && bb) {
            return stringEqualWithoutCase(dataA.asciiData(), dataB.asciiData(),
                                          dataA.length);
        } else {
            return bufferEqualWithoutCase(dataA, dataB, dataA.length);
        }
    }
    return false;
}

bool String::isASCIIStringData(const char* str)
{
    const char* p = str;
    while (*p) {
        if (!isASCII(*p)) {
            return false;
        }
        p++;
    }

    return true;
}

bool String::startsWith(const char* str, bool caseSensitive)
{
    STARFISH_ASSERT(isASCIIStringData(str));
    StringDataOnStackASCII tmpStr(str, strlen(str));
    bool ret = startsWith(&tmpStr, caseSensitive);
    return ret;
}

bool String::startsWith(String* str, bool caseSensitive)
{
    auto dataA = bufferAccessData();
    auto dataB = str->bufferAccessData();

    size_t len = dataA.length;
    size_t strLen = dataB.length;

    if (strLen > len) {
        return false;
    }

    if (caseSensitive) {
        for (size_t i = 0; i < strLen; i++) {
            if (dataA.charAt(i) != dataB.charAt(i)) {
                return false;
            }
        }
    } else {
        for (size_t i = 0; i < strLen; i++) {
            if (tolower(dataA.charAt(i)) != tolower(dataB.charAt(i))) {
                return false;
            }
        }
    }

    return true;
}

bool String::endsWith(const char* str, bool caseSensitive)
{
    STARFISH_ASSERT(isASCIIStringData(str));
    StringDataOnStackASCII tmpStr(str, strlen(str));
    bool ret = endsWith(&tmpStr, caseSensitive);
    return ret;
}

bool String::endsWith(String* str, bool caseSensitive)
{
    auto dataA = bufferAccessData();
    auto dataB = str->bufferAccessData();

    size_t len = dataA.length;
    size_t strLen = dataB.length;

    if (strLen > len) {
        return false;
    }

    size_t startOffset = len - strLen;

    if (caseSensitive) {
        for (size_t i = 0; i < strLen; i++) {
            if (dataB.charAt(i) != dataA.charAt(i + startOffset)) {
                return false;
            }
        }
    } else {
        for (size_t i = 0; i < strLen; i++) {
            if (tolower(dataB.charAt(i)) !=
                tolower(dataA.charAt(i + startOffset))) {
                return false;
            }
        }
    }

    return true;
}

size_t String::find(String* str, size_t pos)
{
    auto srcData = str->bufferAccessData();
    auto dstData = bufferAccessData();

    const size_t srcLen = srcData.length;
    const size_t dstLen = dstData.length;

    if (srcLen == 0)
        return pos <= dstLen ? pos : SIZE_MAX;

    if (srcLen <= dstLen) {
        char32_t src0 = srcData.charAt(0);
        for (; pos <= dstLen - srcLen; ++pos) {
            if (dstData.charAt(pos) == src0) {
                bool same = true;
                for (size_t k = 1; k < srcLen; k++) {
                    if (dstData.charAt(pos + k) != srcData.charAt(k)) {
                        same = false;
                        break;
                    }
                }
                if (same) {
                    return pos;
                }
            }
        }
    }
    return SIZE_MAX;
}

size_t String::find(const char* str, size_t pos)
{
    auto dstData = bufferAccessData();

    const size_t srcLen = strlen(str);
    const size_t dstLen = dstData.length;

    if (srcLen == 0) {
        return pos <= dstLen ? pos : SIZE_MAX;
    }

    if (srcLen <= dstLen) {
        char32_t src0 = (char32_t)str[0];
        for (; pos <= dstLen - srcLen; ++pos) {
            if (dstData.charAt(pos) == src0) {
                bool same = true;
                for (size_t k = 1; k < srcLen; k++) {
                    if (dstData.charAt(pos + k) != (char32_t)str[k]) {
                        same = false;
                        break;
                    }
                }
                if (same) {
                    return pos;
                }
            }
        }
    }
    return SIZE_MAX;
}

size_t String::find(const char ch, size_t pos)
{
    auto dstData = bufferAccessData();

    const size_t srcLen = 1;
    const size_t dstLen = dstData.length;

    if (srcLen <= dstLen) {
        char32_t src0 = (char32_t)ch;
        for (; pos <= dstLen - srcLen; ++pos) {
            if (dstData.charAt(pos) == src0) {
                return pos;
            }
        }
    }
    return SIZE_MAX;
}

size_t String::find(String* str, size_t pos, bool caseSensitive)
{
    const size_t srcLen = str->length();
    const size_t dstLen = length();

    if (srcLen == 0) {
        return pos <= dstLen ? pos : SIZE_MAX;
    }

    if (caseSensitive) {
        if (srcLen <= dstLen) {
            char32_t src0 = str->charAt(0);
            for (; pos <= dstLen - srcLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < srcLen; k++) {
                        if (charAt(pos + k) != str->charAt(k)) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        return pos;
                    }
                }
            }
        }
    } else {
        if (srcLen <= dstLen) {
            char32_t src0 = str->charAt(0);
            for (; pos <= dstLen - srcLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < srcLen; k++) {
                        if (tolower(charAt(pos + k)) !=
                            tolower(str->charAt(k))) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        return pos;
                    }
                }
            }
        }
    }
    return SIZE_MAX;
}

bool String::contains(const char* str, bool caseSensitive)
{
    size_t len = length();
    size_t strLen = strlen(str);

    if (strLen == 0) {
        return true;
    }

    if (caseSensitive) {
        if (strLen <= len) {
            char32_t src0 = (char32_t)str[0];
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (charAt(pos + k) != (char32_t)str[k]) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        return true;
                    }
                }
            }
        }
    } else {
        if (strLen <= len) {
            char32_t src0 = (char32_t)str[0];
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (tolower(charAt(pos + k)) != tolower(str[k])) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool String::contains(String* str, bool caseSensitive)
{
    size_t len = length();
    size_t strLen = str->length();

    if (strLen == 0) {
        return true;
    }

    if (caseSensitive) {
        if (strLen <= len) {
            char32_t src0 = str->charAt(0);
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (charAt(pos + k) != str->charAt(k)) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        return true;
                    }
                }
            }
        }
    } else {
        if (strLen <= len) {
            char32_t src0 = str->charAt(0);
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (tolower(charAt(pos + k)) !=
                            tolower(str->charAt(k))) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

size_t String::peekUTF8Buffer(size_t (*cb)(const char* buffer, size_t len,
                                           void* data),
                              void* data) const
{
    auto bufData = bufferAccessData();
    if (bufData.isNullTerminated) {
        if (bufData.bufferDataKind == StringBufferAccessData::ASCIIData) {
            return cb(bufData.asciiData(), bufData.length, data);
        } else {
            char* buf = ALLOCA((bufData.length * 6) + 1, char);
            size_t realUsage = 0;
            for (size_t i = 0; i < bufData.length; i++) {
                char32_t ch = bufData.charAt(i);
                realUsage += utf32ToUtf8(ch, buf + realUsage);
            }

            buf[realUsage] = 0;
            STARFISH_ASSERT(realUsage <= (bufData.length * 6) + 1);
            return cb(buf, realUsage, data);
        }
    } else {
        if (bufData.bufferDataKind == StringBufferAccessData::ASCIIData) {
            char* newBuffer = ALLOCA(bufData.length + 1, char);
            memcpy(newBuffer, bufData.asciiData(), bufData.length);
            newBuffer[bufData.length] = 0;
            return cb(newBuffer, bufData.length, data);
        } else {
            char* buf = ALLOCA((bufData.length * 6) + 1, char);
            size_t realUsage = 0;
            for (size_t i = 0; i < bufData.length; i++) {
                char32_t ch = bufData.charAt(i);
                realUsage += utf32ToUtf8(ch, buf + realUsage);
            }

            buf[realUsage] = 0;
            STARFISH_ASSERT(realUsage <= (bufData.length * 6) + 1);
            return cb(buf, realUsage, data);
        }
    }
}

// this is fastest version of view utf16 data of string
// const char16_t* buffer ends with '\0'
size_t String::peekUTF16Buffer(size_t (*cb)(const char16_t* buffer, size_t len,
                                            void* data),
                               void* data) const
{
    auto bufData = bufferAccessData();
    if (bufData.bufferDataKind == StringBufferAccessData::ASCIIData) {
        char16_t* buf =
            ALLOCA((bufData.length + 1) * sizeof(char16_t), char16_t);
        for (size_t i = 0; i < bufData.length; i++) {
            buf[i] = bufData.asciiData()[i];
        }
        buf[bufData.length] = 0;
        return cb(buf, bufData.length, data);
    } else {
        char16_t* buf =
            ALLOCA((bufData.length + 1) * 2 * sizeof(char16_t), char16_t);
        size_t realUsage = 0;
        for (size_t i = 0; i < bufData.length; i++) {
            char32_t ch = bufData.charAt(i);
            realUsage += utf32ToUtf16(ch, buf + realUsage);
        }
        buf[realUsage] = 0;
        return cb(buf, realUsage, data);
    }
}

int String::parseInt(String* s)
{
    int ret;
    s->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            *((int*)data) = atoi(buf);
            return 0;
        },
        &ret);
    return ret;
}

int64_t String::parseInt64(String* s)
{
    int64_t ret;
    s->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            *((int64_t*)data) = atoll(buf);
            return 0;
        },
        &ret);
    return ret;
}

float String::parseFloat(String* s)
{
    float ret;
    s->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            *((float*)data) = atof(buf);
            return 0;
        },
        &ret);
    return ret;
}

double String::parseDouble(String* s)
{
    double ret;
    s->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            *((double*)data) = atof(buf);
            return 0;
        },
        &ret);
    return ret;
}

void* StringDataASCII::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(StringDataASCII)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StringDataASCII, m_data));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StringDataASCII));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* StringDataUTF32::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(StringDataUTF32)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StringDataUTF32, m_data));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StringDataUTF32));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* StringDataBMP::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(StringDataBMP)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StringDataBMP, m_data));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StringDataBMP));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* StringView::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(StringView)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StringView, m_string));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StringView));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void StringBuilder::appendPiece(String* str, size_t s, size_t e)
{
    if (e - s > 0) {
        StringBuilderPiece piece;
        piece.m_string = str;
        piece.m_start = s;
        piece.m_end = e;
        piece.m_type = StringBuilderPiece::Type::StringPiece;

        auto data = str->bufferAccessData();

        StringBufferAccessData::BufferDataKind needsKind =
            StringBufferAccessData::BufferDataKind::ASCIIData;
        for (size_t i = s; i < e; i++) {
            char32_t ch = data.charAt(i);
            if (ch < 127) {
            } else if (isBMP(ch)) {
                needsKind = StringBufferAccessData::BufferDataKind::BMPData;
            } else {
                needsKind = StringBufferAccessData::BufferDataKind::UTF32Data;
                break;
            }
        }

        if (m_resultBufferKind < needsKind) {
            m_resultBufferKind = needsKind;
        }

        m_contentLength += e - s;
        if (m_piecesInlineStorageUsage < STRING_BUILDER_INLINE_STORAGE_MAX) {
            m_piecesInlineStorage[m_piecesInlineStorageUsage++] = piece;
        } else
            m_pieces.push_back(piece);
    }
}

void StringBuilder::appendPiece(const char* str)
{
    StringBuilderPiece piece;
    piece.m_start = 0;
    piece.m_end = strlen(str);
    piece.m_raw = str;
    piece.m_type = StringBuilderPiece::Type::ConstChar;
    if (piece.m_end) {
        m_contentLength += piece.m_end;
        if (m_piecesInlineStorageUsage < STRING_BUILDER_INLINE_STORAGE_MAX) {
            m_piecesInlineStorage[m_piecesInlineStorageUsage++] = piece;
        } else
            m_pieces.push_back(piece);
    }
}

void StringBuilder::appendPiece(char32_t ch)
{
    StringBuilderPiece piece;
    piece.m_start = 0;
    piece.m_end = 1;
    piece.m_ch = ch;
    piece.m_type = StringBuilderPiece::Type::Char;

    if (isBMP(ch)) {
        if (ch > 127) {
            if (m_resultBufferKind == StringBufferAccessData::ASCIIData) {
                m_resultBufferKind = StringBufferAccessData::BMPData;
            }
        }
    } else {
        m_resultBufferKind = StringBufferAccessData::UTF32Data;
    }

    m_contentLength += 1;
    if (m_piecesInlineStorageUsage < STRING_BUILDER_INLINE_STORAGE_MAX) {
        m_piecesInlineStorage[m_piecesInlineStorageUsage++] = piece;
    } else
        m_pieces.push_back(piece);
}

StringView StringBuilder::finalizeToStringView()
{
    if (m_piecesInlineStorageUsage == 1 &&
        m_piecesInlineStorage[0].m_type == StringBuilderPiece::StringPiece) {
        return StringView(m_piecesInlineStorage[0].m_string,
                          m_piecesInlineStorage[0].m_start,
                          m_piecesInlineStorage[0].m_end);
    }

    String* str = finalize();
    return StringView(str, 0, str->length());
}

String* StringBuilder::finalize()
{
    if (!m_contentLength) {
        return String::emptyString;
    }

    if (m_resultBufferKind == StringBufferAccessData::ASCIIData) {
        ASCIIString ret;
        ret.resize(m_contentLength);

        size_t currentLength = 0;
        for (size_t i = 0; i < m_piecesInlineStorageUsage; i++) {
            const StringBuilderPiece& piece = m_piecesInlineStorage[i];
            if (piece.m_type == StringBuilderPiece::Char) {
                ret[currentLength++] = piece.m_ch;
            } else if (piece.m_type == StringBuilderPiece::ConstChar) {
                const char* data = piece.m_raw;
                size_t l = piece.m_end;
                memcpy(&ret[currentLength], data, l);
                currentLength += l;
            } else {
                String* data = piece.m_string;
                size_t s = piece.m_start;
                size_t e = piece.m_end;
                size_t l = e - s;
                auto accessData = data->bufferAccessData();
                if (accessData.bufferDataKind ==
                    StringBufferAccessData::ASCIIData) {
                    memcpy(&ret[currentLength], (accessData.asciiData()) + s,
                           l);
                    currentLength += l;
                } else {
                    for (size_t k = s; k < e; k++) {
                        ret[currentLength++] = accessData.charAt(k);
                    }
                }
            }
        }

        for (size_t i = 0; i < m_pieces.size(); i++) {
            const StringBuilderPiece& piece = m_pieces[i];
            if (piece.m_type == StringBuilderPiece::Char) {
                ret[currentLength++] = piece.m_ch;
            } else if (piece.m_type == StringBuilderPiece::ConstChar) {
                const char* data = piece.m_raw;
                size_t l = piece.m_end;
                memcpy(&ret[currentLength], data, l);
                currentLength += l;
            } else {
                String* data = piece.m_string;
                size_t s = piece.m_start;
                size_t e = piece.m_end;
                size_t l = e - s;
                auto accessData = data->bufferAccessData();
                if (accessData.bufferDataKind ==
                    StringBufferAccessData::ASCIIData) {
                    memcpy(&ret[currentLength], accessData.asciiData() + s, l);
                    currentLength += l;
                } else {
                    for (size_t k = s; k < e; k++) {
                        ret[currentLength++] = accessData.charAt(k);
                    }
                }
            }
        }

        return new StringDataASCII(std::move(ret));
    } else if (m_resultBufferKind == StringBufferAccessData::BMPData) {
        BMPString ret;
        ret.resize(m_contentLength);

        size_t currentLength = 0;
        for (size_t i = 0; i < m_piecesInlineStorageUsage; i++) {
            const StringBuilderPiece& piece = m_piecesInlineStorage[i];
            if (piece.m_type == StringBuilderPiece::Char) {
                ret[currentLength++] = piece.m_ch;
            } else if (piece.m_type == StringBuilderPiece::ConstChar) {
                const char* data = piece.m_raw;
                size_t l = piece.m_end;
                for (size_t j = 0; j < l; j++) {
                    ret[currentLength++] = data[j];
                }
            } else {
                String* data = piece.m_string;
                size_t s = piece.m_start;
                size_t e = piece.m_end;
                auto accessData = data->bufferAccessData();
                for (size_t k = s; k < e; k++) {
                    ret[currentLength++] = accessData.charAt(k);
                }
            }
        }

        for (size_t i = 0; i < m_pieces.size(); i++) {
            const StringBuilderPiece& piece = m_pieces[i];
            if (piece.m_type == StringBuilderPiece::Char) {
                ret[currentLength++] = piece.m_ch;
            } else if (piece.m_type == StringBuilderPiece::ConstChar) {
                const char* data = piece.m_raw;
                size_t l = piece.m_end;
                for (size_t j = 0; j < l; j++) {
                    ret[currentLength++] = data[j];
                }
            } else {
                String* data = piece.m_string;
                size_t s = piece.m_start;
                size_t e = piece.m_end;
                auto accessData = data->bufferAccessData();
                for (size_t k = s; k < e; k++) {
                    ret[currentLength++] = accessData.charAt(k);
                }
            }
        }

        return new StringDataBMP(std::move(ret));
    } else {
        UTF32String ret;
        ret.resize(m_contentLength);

        size_t currentLength = 0;
        for (size_t i = 0; i < m_piecesInlineStorageUsage; i++) {
            const StringBuilderPiece& piece = m_piecesInlineStorage[i];
            if (piece.m_type == StringBuilderPiece::Char) {
                ret[currentLength++] = piece.m_ch;
            } else if (piece.m_type == StringBuilderPiece::ConstChar) {
                const char* data = piece.m_raw;
                size_t l = piece.m_end;
                for (size_t j = 0; j < l; j++) {
                    ret[currentLength++] = data[j];
                }
            } else {
                String* data = piece.m_string;
                size_t s = piece.m_start;
                size_t e = piece.m_end;
                auto accessData = data->bufferAccessData();
                for (size_t k = s; k < e; k++) {
                    ret[currentLength++] = accessData.charAt(k);
                }
            }
        }

        for (size_t i = 0; i < m_pieces.size(); i++) {
            const StringBuilderPiece& piece = m_pieces[i];
            if (piece.m_type == StringBuilderPiece::Char) {
                ret[currentLength++] = piece.m_ch;
            } else if (piece.m_type == StringBuilderPiece::ConstChar) {
                const char* data = piece.m_raw;
                size_t l = piece.m_end;
                for (size_t j = 0; j < l; j++) {
                    ret[currentLength++] = data[j];
                }
            } else {
                String* data = piece.m_string;
                size_t s = piece.m_start;
                size_t e = piece.m_end;
                auto accessData = data->bufferAccessData();
                for (size_t k = s; k < e; k++) {
                    ret[currentLength++] = accessData.charAt(k);
                }
            }
        }
        return new StringDataUTF32(std::move(ret));
    }
}

unsigned SegmentedString::length() const
{
    unsigned length = m_currentString.m_length;
    if (m_pushedChar1) {
        ++length;
        if (m_pushedChar2) {
            ++length;
        }
    }
    if (isComposite()) {
        auto it = m_substrings.cbegin();
        auto e = m_substrings.cend();
        for (; it != e; ++it) {
            length += it->m_length;
        }
    }
    return length;
}

void SegmentedString::setExcludeLineNumbers()
{
    m_currentString.setExcludeLineNumbers();
    if (isComposite()) {
        auto it = m_substrings.begin();
        auto e = m_substrings.end();
        for (; it != e; ++it) {
            it->setExcludeLineNumbers();
        }
    }
}

void SegmentedString::clear()
{
    m_pushedChar1 = 0;
    m_pushedChar2 = 0;
    m_currentChar = 0;
    m_currentString.clear();
    m_numberOfCharactersConsumedPriorToCurrentString = 0;
    m_numberOfCharactersConsumedPriorToCurrentLine = 0;
    m_currentLine = 0;
    m_substrings.clear();
    m_closed = false;
    m_empty = true;
    m_fastPathFlags = NoFastPath;
    m_advanceFunc = &SegmentedString::advanceEmpty;
    m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
}

void SegmentedString::append(const SegmentedSubstring& s)
{
    STARFISH_ASSERT(!m_closed);
    if (!s.m_length) {
        return;
    }

    if (!m_currentString.m_length) {
        m_numberOfCharactersConsumedPriorToCurrentString +=
            m_currentString.numberOfCharactersConsumed();
        m_currentString = s;
        updateAdvanceFunctionPointers();
    } else {
        m_substrings.push_back(s);
    }
    m_empty = false;
}

void SegmentedString::prepend(const SegmentedSubstring& s)
{
    STARFISH_ASSERT(!escaped());
    STARFISH_ASSERT(!s.numberOfCharactersConsumed());
    if (!s.m_length) {
        return;
    }

    // FIXME: We're assuming that the prepend were originally consumed by
    //        this SegmentedString. We're also ASSERTing that s is a fresh
    //        SegmentedSubstring. These assumptions are sufficient for our
    //        current use, but we might need to handle the more elaborate
    //        cases in the future.
    m_numberOfCharactersConsumedPriorToCurrentString +=
        m_currentString.numberOfCharactersConsumed();
    m_numberOfCharactersConsumedPriorToCurrentString -= s.m_length;
    if (!m_currentString.m_length) {
        m_currentString = s;
        updateAdvanceFunctionPointers();
    } else {
        // Shift our m_currentString into our list.
        m_substrings.insert(m_substrings.begin(), m_currentString);
        m_currentString = s;
        updateAdvanceFunctionPointers();
    }
    m_empty = false;
}

void SegmentedString::close()
{
    // Closing a stream twice is likely a coding mistake.
    STARFISH_ASSERT(!m_closed);
    m_closed = true;
}

void SegmentedString::append(const SegmentedString& s)
{
    STARFISH_ASSERT(!m_closed);
    STARFISH_ASSERT(!s.escaped());
    append(s.m_currentString);
    if (s.isComposite()) {
        auto it = s.m_substrings.cbegin();
        auto e = s.m_substrings.cend();
        for (; it != e; ++it) {
            append(*it);
        }
    }
    m_currentChar =
        m_pushedChar1
            ? m_pushedChar1
            : (m_currentString.m_length ? m_currentString.getCurrentChar() : 0);
}

void SegmentedString::prepend(const SegmentedString& s)
{
    STARFISH_ASSERT(!escaped());
    STARFISH_ASSERT(!s.escaped());
    if (s.isComposite()) {
        auto it = s.m_substrings.rbegin();
        auto e = s.m_substrings.rend();
        for (; it != e; ++it) {
            prepend(*it);
        }
    }
    prepend(s.m_currentString);
    m_currentChar =
        m_pushedChar1
            ? m_pushedChar1
            : (m_currentString.m_length ? m_currentString.getCurrentChar() : 0);
}

void SegmentedString::advanceSubstring()
{
    if (isComposite()) {
        m_numberOfCharactersConsumedPriorToCurrentString +=
            m_currentString.numberOfCharactersConsumed();
        m_currentString = m_substrings.front();
        m_substrings.pop_front();
        // If we've previously consumed some characters of the non-current
        // string, we now account for those characters as part of the current
        // string, not as part of "prior to current string."
        m_numberOfCharactersConsumedPriorToCurrentString -=
            m_currentString.numberOfCharactersConsumed();
        updateAdvanceFunctionPointers();
    } else {
        m_currentString.clear();
        m_empty = true;
        m_fastPathFlags = NoFastPath;
        m_advanceFunc = &SegmentedString::advanceEmpty;
        m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
    }
}

String* SegmentedString::toString() const
{
    UTF32String result;
    if (m_pushedChar1) {
        result.push_back(m_pushedChar1);
        if (m_pushedChar2) {
            result.push_back(m_pushedChar2);
        }
    }
    m_currentString.appendTo(result);
    if (isComposite()) {
        auto it = m_substrings.cbegin();
        auto e = m_substrings.cend();
        for (; it != e; ++it) {
            it->appendTo(result);
        }
    }
    return new StringDataUTF32(std::move(result));
}

void SegmentedString::advance(unsigned count, char32_t* consumedCharacters)
{
    STARFISH_ASSERT(count <= length());
    for (unsigned i = 0; i < count; ++i) {
        consumedCharacters[i] = currentChar();
        advance();
    }
}

void SegmentedString::advance8()
{
    STARFISH_ASSERT(!m_pushedChar1);
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar8();
}

void SegmentedString::advance16()
{
    STARFISH_ASSERT(!m_pushedChar1);
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar32();
}

void SegmentedString::advanceAndUpdateLineNumber8()
{
    STARFISH_ASSERT(!m_pushedChar1);
    STARFISH_ASSERT(m_currentString.getCurrentChar() == m_currentChar);
    if (m_currentChar == '\n') {
        ++m_currentLine;
        m_numberOfCharactersConsumedPriorToCurrentLine =
            numberOfCharactersConsumed() + 1;
    }
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar8();
}

void SegmentedString::advanceAndUpdateLineNumber16()
{
    STARFISH_ASSERT(!m_pushedChar1);
    STARFISH_ASSERT(m_currentString.getCurrentChar() == m_currentChar);
    if (m_currentChar == '\n') {
        ++m_currentLine;
        m_numberOfCharactersConsumedPriorToCurrentLine =
            numberOfCharactersConsumed() + 1;
    }
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar32();
}

void SegmentedString::advanceSlowCase()
{
    if (m_pushedChar1) {
        m_pushedChar1 = m_pushedChar2;
        m_pushedChar2 = 0;

        if (m_pushedChar1) {
            m_currentChar = m_pushedChar1;
            return;
        }

        updateAdvanceFunctionPointers();
    } else if (m_currentString.m_length) {
        if (!--m_currentString.m_length) {
            advanceSubstring();
        }
    } else if (!isComposite()) {
        m_currentString.clear();
        m_empty = true;
        m_fastPathFlags = NoFastPath;
        m_advanceFunc = &SegmentedString::advanceEmpty;
        m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
    }
    m_currentChar =
        m_currentString.m_length ? m_currentString.getCurrentChar() : 0;
}

void SegmentedString::advanceAndUpdateLineNumberSlowCase()
{
    if (m_pushedChar1) {
        m_pushedChar1 = m_pushedChar2;
        m_pushedChar2 = 0;

        if (m_pushedChar1) {
            m_currentChar = m_pushedChar1;
            return;
        }

        updateAdvanceFunctionPointers();
    } else if (m_currentString.m_length) {
        if (m_currentString.getCurrentChar() == '\n' &&
            m_currentString.doNotExcludeLineNumbers()) {
            ++m_currentLine;
            // Plus 1 because numberOfCharactersConsumed value hasn't
            // incremented yet; it does with m_length decrement below.
            m_numberOfCharactersConsumedPriorToCurrentLine =
                numberOfCharactersConsumed() + 1;
        }
        if (!--m_currentString.m_length) {
            advanceSubstring();
        } else {
            // Only need the ++
            m_currentString.incrementAndGetCurrentChar();
        }
    } else if (!isComposite()) {
        m_currentString.clear();
        m_empty = true;
        m_fastPathFlags = NoFastPath;
        m_advanceFunc = &SegmentedString::advanceEmpty;
        m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
    }

    m_currentChar =
        m_currentString.m_length ? m_currentString.getCurrentChar() : 0;
}

void SegmentedString::advanceEmpty()
{
    STARFISH_ASSERT(!m_currentString.m_length && !isComposite());
    m_currentChar = 0;
}

void SegmentedString::updateSlowCaseFunctionPointers()
{
    m_fastPathFlags = NoFastPath;
    m_advanceFunc = &SegmentedString::advanceSlowCase;
    m_advanceAndUpdateLineNumberFunc =
        &SegmentedString::advanceAndUpdateLineNumberSlowCase;
}

OrdinalNumber SegmentedString::currentLine() const
{
    return OrdinalNumber::fromZeroBasedInt(m_currentLine);
}

OrdinalNumber SegmentedString::currentColumn() const
{
    int zeroBasedColumn = numberOfCharactersConsumed() -
                          m_numberOfCharactersConsumedPriorToCurrentLine;
    return OrdinalNumber::fromZeroBasedInt(zeroBasedColumn);
}

void SegmentedString::setCurrentPosition(OrdinalNumber line,
                                         OrdinalNumber columnAftreProlog,
                                         int prologLength)
{
    m_currentLine = line.zeroBasedInt();
    m_numberOfCharactersConsumedPriorToCurrentLine =
        numberOfCharactersConsumed() + prologLength -
        columnAftreProlog.zeroBasedInt();
}
}
