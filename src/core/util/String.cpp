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
    while (end != src) {
        char32_t c;
        src += utf8ToUtf32(src, end, c);
        data[i++] = c;
    }
    m_data = data;
}

// TODO use BufferAccessData for performance
size_t String::indexOf(char32_t ch) const
{
    for (size_t i = 0; i < length(); i++) {
        if (charAt(i) == ch) {
            return i;
        }
    }
    return SIZE_MAX;
}

// TODO use BufferAccessData for performance
size_t String::lastIndexOf(char32_t ch) const
{
    for (size_t i = length(); i > 0; i--) {
        if (charAt(i - 1) == ch) {
            return i - 1;
        }
    }
    return SIZE_MAX;
}

// TODO use BufferAccessData for performance
bool String::equalsWithoutCase(const char* str) const
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
    size_t srcLen = strlen(str);
    if (srcLen != length()) {
        return false;
    }
    for (size_t i = 0; i < length(); i++) {
        if (tolower(charAt(i)) != tolower((char32_t)str[i])) {
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

    if (data.hasASCIIContent) {
        for (size_t i = 0; i < data.length; i++) {
            if (data.asciiData()[i] != str[i]) {
                return false;
            }
        }
    } else {
        for (size_t i = 0; i < data.length; i++) {
            if (data.utf32Data()[i] != (char32_t)str[i]) {
                return false;
            }
        }
    }

    return true;
}

// TODO use BufferAccessData for performance
bool String::equals(const char32_t* str) const
{
    size_t srcLen = 0;
    for (; str[srcLen]; srcLen++) {
    }

    if (srcLen != length()) {
        return false;
    }
    for (size_t i = 0; i < length(); i++) {
        if (charAt(i) != str[i]) {
            return false;
        }
    }
    return true;
}

// TODO use BufferAccessData for performance
bool String::containsWhitespace(size_t start, size_t end)
{
    if (end == SIZE_MAX) {
        end = length();
    }

    for (size_t i = start; i < end; i++) {
        if (isASCIISpace(charAt(i))) {
            return true;
        }
    }
    return false;
}

// TODO use BufferAccessData for performance
bool String::containsOnlyWhitespace(size_t start, size_t end)
{
    if (end == SIZE_MAX) {
        end = length();
    }

    for (size_t i = start; i < end; i++) {
        if (!isASCIISpace(charAt(i))) {
            return false;
        }
    }
    return true;
}

bool String::containsOnlyASCIIChars() const
{
    auto data = bufferAccessData();
    if (data.hasASCIIContent)
        return true;

    for (size_t i = 0; i < data.length; i++) {
        const char32_t c = data.utf32Data()[i];
        if (c > 127) {
            return false;
        }
    }
    return true;
}

icu::UnicodeString String::toUnicodeString() const
{
    auto data = bufferAccessData();
    if (data.hasASCIIContent) {
        return icu::UnicodeString((const char*)data.buffer, data.length,
                                  US_INV);
    } else {
        return icu::UnicodeString::fromUTF32((const UChar32*)data.buffer,
                                             data.length);
    }
}

icu::UnicodeString String::toUnicodeString(size_t start, size_t end) const
{
    auto data = bufferAccessData();
    size_t len = end - start;
    if (data.hasASCIIContent) {
        return icu::UnicodeString(((const char*)data.buffer) + start, len,
                                  US_INV);
    } else {
        return icu::UnicodeString::fromUTF32(
            ((const UChar32*)data.buffer) + start, len);
    }
}

size_t String::hashValue() const
{
    auto data = bufferAccessData();
    size_t len = data.length;
    size_t hash;
    if (LIKELY(data.hasASCIIContent)) {
        auto ptr = (const char*)data.buffer;
        hash = stringHash(ptr, len);
    } else {
        auto ptr = (const char32_t*)data.buffer;
        hash = stringHash(ptr, len);
    }

    if (UNLIKELY((hash % sizeof(size_t)) == 0)) {
        hash++;
    }

    return hash;
}

const char* String::utf8Data()
{
    auto data = bufferAccessData();
    if (data.isNullTerminated) {
        if (data.hasASCIIContent) {
            return data.asciiData();
        } else {
            return utf32ToUtf8(data.utf32Data(), data.length);
        }
    } else {
        if (data.hasASCIIContent) {
            char* newBuffer =
                (char*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(data.length + 1);
            memcpy(newBuffer, data.asciiData(), data.length);
            newBuffer[data.length] = 0;
            data.buffer = newBuffer;
            return data.asciiData();
        } else {
            return utf32ToUtf8(data.utf32Data(), data.length);
        }
    }
}

const char* String::utf8DataIgnoreZeroWidthChar()
{
    auto data = bufferAccessData();
    if (data.hasASCIIContent) {
        ASCIIString newStr;
        for (size_t i = 0; i < data.length; i++) {
            if (!String::isZeroWidthChar(data.asciiData()[i])) {
                newStr += data.asciiData();
            }
        }
        return newStr.data();
    } else {
        return utf32ToUtf8IgnoreZeroWidthChar(data.utf32Data(), data.length);
    }
}

String* String::fromUTF8(const char* src, size_t len)
{
    for (unsigned i = 0; i < len; i++) {
        if (src[i] & 0x80) {
            return new StringDataUTF32(src, len);
        }
    }
    return new StringDataASCII(src, len);
}

String* String::fromUTF8(const char* str)
{
    const char* p = str;
    while (*p) {
        if (*p & 0x80) {
            return new StringDataUTF32(str, strlen(str));
        }
        p++;
    }

    return new StringDataASCII(str);
}

String* String::fromUTF16(const char16_t* src, size_t len)
{
    for (unsigned i = 0; i < len; i++) {
        if (src[i] > 127) {
            UTF32String utf32;
            for (size_t i = 0; i < len; /* U16_NEXT post-increments */) {
                char32_t c;
                U16_NEXT(src, i, len, c);
                utf32 += c;
            }
            return new StringDataUTF32(std::move(utf32));
        }
    }

    ASCIIString ascii;
    for (unsigned i = 0; i < len; i++) {
        ascii.push_back((char)src[i]);
    }

    return new StringDataASCII(std::move(ascii));
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

String* String::createASCIIStringFromUTF32SourceIfPossible(
    const UTF32String& src)
{
    for (size_t i = 0; i < src.length(); i++) {
        const char32_t c = src[i];
        if (c > 127) {
            return String::createUTF32String(src);
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
    if (data.hasASCIIContent) {
        return NullableUTF8String(data.asciiData(), data.length);
    } else {
        size_t len = 0;
        const char* ptr = utf32ToUtf8(data.utf32Data(), data.length, &len);
        return NullableUTF8String(ptr, len - 1);
    }
}

String* String::substring(size_t pos, size_t len)
{
    return new StringView(this, pos, pos + len);
}

String* String::toUpper()
{
    auto data = bufferAccessData();
    if (data.hasASCIIContent) {
        ASCIIString str(data.asciiData(), data.length);
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return new StringDataASCII(std::move(str));
    } else {
        UTF32String str(data.utf32Data(), data.length);
        // TODO use icu to transform utf-32 string
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return new StringDataUTF32(std::move(str));
    }
}

String* String::toLower()
{
    auto data = bufferAccessData();
    if (data.hasASCIIContent) {
        ASCIIString str(data.asciiData(), data.length);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return new StringDataASCII(std::move(str));
    } else {
        UTF32String str(data.utf32Data(), data.length);
        // TODO use icu to transform utf-32 string
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return new StringDataUTF32(std::move(str));
    }
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

    auto dataA = bufferAccessData();

    if (dataA.hasASCIIContent) {
        ASCIIString str;
        str.reserve(dataA.length + srcLen);
        str.append(dataA.asciiData(), dataA.length);
        str.append(src, srcLen);
        return new StringDataASCII(std::move(str));
    } else {
        UTF32String str;
        str.resize(dataA.length + srcLen);
        if (dataA.hasASCIIContent) {
            for (size_t i = 0; i < dataA.length; i++) {
                str[i] = dataA.asciiData()[i];
            }
        } else {
            for (size_t i = 0; i < dataA.length; i++) {
                str[i] = dataA.utf32Data()[i];
            }
        }

        for (size_t i = 0; i < srcLen; i++) {
            str[i + dataA.length] = src[i];
        }

        return new StringDataUTF32(std::move(str));
    }
}

String* String::concat(String* str)
{
    if (length() == 0) {
        return str;
    }
    if (str->length() == 0) {
        return this;
    }

    auto dataA = bufferAccessData();
    auto dataB = str->bufferAccessData();

    if (dataA.hasASCIIContent && dataB.hasASCIIContent) {
        ASCIIString str;
        str.reserve(dataA.length + dataB.length);
        str.append(dataA.asciiData(), dataA.length);
        str.append(dataB.asciiData(), dataB.length);
        return new StringDataASCII(std::move(str));
    } else {
        UTF32String str;
        str.resize(dataA.length + dataB.length);
        if (dataA.hasASCIIContent) {
            for (size_t i = 0; i < dataA.length; i++) {
                str[i] = dataA.asciiData()[i];
            }
        } else {
            for (size_t i = 0; i < dataA.length; i++) {
                str[i] = dataA.utf32Data()[i];
            }
        }

        if (dataB.hasASCIIContent) {
            for (size_t i = 0; i < dataB.length; i++) {
                str[i + dataA.length] = dataB.asciiData()[i];
            }
        } else {
            for (size_t i = 0; i < dataB.length; i++) {
                str[i + dataA.length] = dataB.utf32Data()[i];
            }
        }

        return new StringDataUTF32(std::move(str));
    }
}

String* String::replaceAll(String* from, String* to)
{
    auto data = bufferAccessData();

    if (data.hasASCIIContent) {
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

void String::split(char delim, GCVector<String*>& tokens)
{
    size_t prev_pos = 0, pos = 0;
    while ((pos = find(delim, pos)) != SIZE_MAX) {
        tokens.push_back(new StringView(this, prev_pos, pos));
        prev_pos = ++pos;
    }

    if (pos == SIZE_MAX)
        pos = length();

    tokens.push_back(new StringView(this, prev_pos, pos)); // Last word
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

GCVector<StringView> StringUtils::tokenize(String* src, const char* tokens,
                                           size_t tokensLength)
{
    GCVector<StringView> result;
    auto accessData = src->bufferAccessData();

    size_t start = 0;
    size_t end = 0;
    for (size_t i = 0; i < accessData.length; i++) {
        char c = accessData.charAt(i);
        bool isToken = false;
        for (size_t j = 0; j < tokensLength; j++) {
            if (c == tokens[j]) {
                isToken = true;
                break;
            }
        }

        if (isToken) {
            result.push_back(StringView(src, start, end));
            end = start = i;
        } else {
            end++;
        }
    }

    if (end - start) {
        result.push_back(StringView(src, start, end));
    }

    return result;
}

static int utf32ToUtf16(char32_t i, char16_t* u)
{
    if (i < 0xffff) {
        *u = (char16_t)(i & 0xffff);
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
    if (data.hasASCIIContent) {
        result.append(data.asciiData(), data.length);
    } else {
        result.reserve(data.length);
        for (size_t i = 0; i < data.length; i++) {
            char buffer[16];
            size_t len = utf32ToUtf8(data.utf32Data()[i], buffer);
            result.append(buffer, len);
        }
    }

    return result;
}

UTF32String String::toUTF32String()
{
    UTF32String result;

    auto data = bufferAccessData();
    if (data.hasASCIIContent) {
        result.reserve(data.length);
        for (size_t i = 0; i < data.length; i++) {
            result.push_back(data.asciiData()[i]);
        }
    } else {
        result.append(data.utf32Data(), data.length);
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
    if (data.hasASCIIContent) {
        out.assign(data.asciiData() + start, data.asciiData() + end);
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
    if (data.hasASCIIContent) {
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
    } else {
        return utf32ToUtf8(data.utf32Data(), start, end, ignoreZeroWidthChar);
    }
}

UTF8StringDataNonGCStd String::toUTF8NonGCString() const
{
    auto data = bufferAccessData();
    if (data.hasASCIIContent) {
        UTF8StringDataNonGCStd ret(data.asciiData(), data.length);
        return ret;
    } else {
        return utf32ToUtf8(data.utf32Data(), 0, data.length, false);
    }
}

bool String::equals(const String* str) const
{
    auto dataA = bufferAccessData();
    auto dataB = str->bufferAccessData();

    if (dataA.length == dataB.length) {
        bool aa = dataA.hasASCIIContent;
        bool bb = dataB.hasASCIIContent;
        if (aa && bb) {
            return stringEqual(dataA.asciiData(), dataB.asciiData(),
                               dataA.length);
        } else if (aa && !bb) {
            return stringEqual(dataB.utf32Data(), dataA.asciiData(),
                               dataA.length);
        } else if (!aa && bb) {
            return stringEqual(dataA.utf32Data(), dataB.asciiData(),
                               dataA.length);
        } else {
            return stringEqual(dataA.utf32Data(), dataB.utf32Data(),
                               dataA.length);
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

bool stringEqualWithoutCase(const char32_t* s, const char* s1,
                            const size_t& len)
{
    for (size_t i = 0; i < len; i++) {
        if (towlower(s[i]) != towlower((unsigned char)s1[i])) {
            return false;
        }
    }
    return true;
}

bool String::equalsWithoutCase(const String* str) const
{
    auto dataA = bufferAccessData();
    auto dataB = str->bufferAccessData();

    if (dataA.length == dataB.length) {
        bool aa = dataA.hasASCIIContent;
        bool bb = dataB.hasASCIIContent;
        if (aa && bb) {
            return stringEqualWithoutCase(dataA.asciiData(), dataB.asciiData(),
                                          dataA.length);
        } else if (aa && !bb) {
            return stringEqualWithoutCase(dataB.utf32Data(), dataA.asciiData(),
                                          dataA.length);
        } else if (!aa && bb) {
            return stringEqualWithoutCase(dataA.utf32Data(), dataB.asciiData(),
                                          dataA.length);
        } else {
            return stringEqualWithoutCase(dataA.utf32Data(), dataB.utf32Data(),
                                          dataA.length);
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
    return startsWith(createASCIIString(str), caseSensitive);
}

bool String::startsWith(String* str, bool caseSensitive)
{
    size_t len = length();
    size_t strLen = str->length();
    if (strLen > len) {
        return false;
    }

    if (caseSensitive) {
        for (size_t i = 0; i < strLen; i++) {
            if (str->charAt(i) != charAt(i)) {
                return false;
            }
        }
    } else {
        for (size_t i = 0; i < strLen; i++) {
            if (tolower(str->charAt(i)) != tolower(charAt(i))) {
                return false;
            }
        }
    }

    return true;
}

bool String::endsWith(const char* str, bool caseSensitive)
{
    STARFISH_ASSERT(isASCIIStringData(str));
    return endsWith(createASCIIString(str), caseSensitive);
}

bool String::endsWith(String* str, bool caseSensitive)
{
    size_t len = length();
    size_t strLen = str->length();

    if (strLen > len) {
        return false;
    }

    size_t startOffset = len - strLen;

    if (caseSensitive) {
        for (size_t i = 0; i < strLen; i++) {
            if (str->charAt(i) != charAt(i + startOffset)) {
                return false;
            }
        }
    } else {
        for (size_t i = 0; i < strLen; i++) {
            if (tolower(str->charAt(i)) != tolower(charAt(i + startOffset))) {
                return false;
            }
        }
    }

    return true;
}

size_t String::find(String* str, size_t pos)
{
    const size_t srcLen = str->length();
    const size_t dstLen = length();

    if (srcLen == 0)
        return pos <= dstLen ? pos : SIZE_MAX;

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
    return SIZE_MAX;
}

size_t String::find(const char* str, size_t pos)
{
    const size_t srcLen = strlen(str);
    const size_t dstLen = length();

    if (srcLen == 0) {
        return pos <= dstLen ? pos : SIZE_MAX;
    }

    if (srcLen <= dstLen) {
        char32_t src0 = (char32_t)str[0];
        for (; pos <= dstLen - srcLen; ++pos) {
            if (charAt(pos) == src0) {
                bool same = true;
                for (size_t k = 1; k < srcLen; k++) {
                    if (charAt(pos + k) != (char32_t)str[k]) {
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
    const size_t srcLen = 1;
    const size_t dstLen = length();

    if (srcLen <= dstLen) {
        char32_t src0 = (char32_t)ch;
        for (; pos <= dstLen - srcLen; ++pos) {
            if (charAt(pos) == src0) {
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
                              void* data)
{
    auto bufData = bufferAccessData();
    if (bufData.isNullTerminated) {
        if (bufData.hasASCIIContent) {
            return cb(bufData.asciiData(), bufData.length, data);
        } else {
            char* buf = ALLOCA((bufData.length * 6) + 1, char);
            size_t realUsage = 0;
            for (size_t i = 0; i < bufData.length; i++) {
                char32_t ch = bufData.utf32Data()[i];
                realUsage += utf32ToUtf8(ch, buf + realUsage);
            }

            buf[realUsage] = 0;
            STARFISH_ASSERT(realUsage <= (bufData.length * 6) + 1);
            return cb(buf, realUsage, data);
        }
    } else {
        if (bufData.hasASCIIContent) {
            char* newBuffer = ALLOCA(bufData.length + 1, char);
            memcpy(newBuffer, bufData.asciiData(), bufData.length);
            newBuffer[bufData.length] = 0;
            return cb(newBuffer, bufData.length, data);
        } else {
            char* buf = ALLOCA((bufData.length * 6) + 1, char);
            size_t realUsage = 0;
            for (size_t i = 0; i < bufData.length; i++) {
                char32_t ch = bufData.utf32Data()[i];
                realUsage += utf32ToUtf8(ch, buf + realUsage);
            }

            buf[realUsage] = 0;
            STARFISH_ASSERT(realUsage <= (bufData.length * 6) + 1);
            return cb(buf, realUsage, data);
        }
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

void StringBuilder::appendPiece(String* str, size_t s, size_t e)
{
    if (e - s > 0) {
        StringBuilderPiece piece;
        piece.m_string = str;
        piece.m_start = s;
        piece.m_end = e;

        auto data = str->bufferAccessData();
        if (!data.hasASCIIContent) {
            bool hasASCII = true;
            for (size_t i = s; i < e; i++) {
                if (((char32_t*)data.buffer)[i] > 127) {
                    hasASCII = false;
                    break;
                }
            }

            if (!hasASCII) {
                m_hasASCIIContent = false;
                piece.m_type = StringBuilderPiece::Type::UTF32StringStringPiece;
            } else {
                piece.m_type = StringBuilderPiece::Type::
                    UTF32StringStringPieceButASCIIContentPiece;
            }

        } else {
            piece.m_type = StringBuilderPiece::Type::ASCIIStringPiece;
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

    if (ch > 127) {
        m_hasASCIIContent = false;
    }

    m_contentLength += 1;
    if (m_piecesInlineStorageUsage < STRING_BUILDER_INLINE_STORAGE_MAX) {
        m_piecesInlineStorage[m_piecesInlineStorageUsage++] = piece;
    } else
        m_pieces.push_back(piece);
}

void StringBuilder::takeBuilder(StringBuilder& src)
{
    m_hasASCIIContent = m_hasASCIIContent | src.m_hasASCIIContent;
    m_contentLength += src.m_contentLength;

    for (size_t i = 0; i < src.m_piecesInlineStorageUsage; i++) {
        if (m_piecesInlineStorageUsage < STRING_BUILDER_INLINE_STORAGE_MAX) {
            m_piecesInlineStorage[m_piecesInlineStorageUsage++] =
                src.m_piecesInlineStorage[i];
        } else
            m_pieces.push_back(src.m_piecesInlineStorage[i]);
    }

    for (size_t i = 0; i < src.m_pieces.size(); i++) {
        if (m_piecesInlineStorageUsage < STRING_BUILDER_INLINE_STORAGE_MAX) {
            m_piecesInlineStorage[m_piecesInlineStorageUsage++] =
                src.m_pieces[i];
        } else
            m_pieces.push_back(src.m_pieces[i]);
    }
}

String* StringBuilder::finalize()
{
    if (!m_contentLength) {
        return String::emptyString;
    }

    if (m_hasASCIIContent) {
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
                if (accessData.hasASCIIContent) {
                    memcpy(&ret[currentLength], (accessData.asciiData()) + s,
                           l);
                    currentLength += l;
                } else {
                    char32_t* b = ((char32_t*)accessData.buffer);
                    for (size_t k = s; k < e; k++) {
                        ret[currentLength++] = b[k];
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
                if (accessData.hasASCIIContent) {
                    memcpy(&ret[currentLength], accessData.asciiData() + s, l);
                    currentLength += l;
                } else {
                    char32_t* b = ((char32_t*)accessData.buffer);
                    for (size_t k = s; k < e; k++) {
                        ret[currentLength++] = b[k];
                    }
                }
            }
        }

        return new StringDataASCII(std::move(ret));
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
                size_t l = e - s;
                if (data->bufferAccessData().hasASCIIContent) {
                    auto ptr = data->bufferAccessData().asciiData();
                    ptr += s;
                    for (size_t j = 0; j < l; j++) {
                        ret[currentLength++] = ptr[j];
                    }
                } else {
                    auto ptr = data->bufferAccessData().utf32Data();
                    ptr += s;
                    for (size_t j = 0; j < l; j++) {
                        ret[currentLength++] = ptr[j];
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
                for (size_t j = 0; j < l; j++) {
                    ret[currentLength++] = data[j];
                }
            } else {
                String* data = piece.m_string;
                size_t s = piece.m_start;
                size_t e = piece.m_end;
                size_t l = e - s;
                if (data->bufferAccessData().hasASCIIContent) {
                    auto ptr = data->bufferAccessData().asciiData();
                    ptr += s;
                    for (size_t j = 0; j < l; j++) {
                        ret[currentLength++] = ptr[j];
                    }
                } else {
                    auto ptr = data->bufferAccessData().utf32Data();
                    ptr += s;
                    for (size_t j = 0; j < l; j++) {
                        ret[currentLength++] = ptr[j];
                    }
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
