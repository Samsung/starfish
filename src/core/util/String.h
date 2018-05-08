/*
    Copyright (c) 2015-present Samsung Electronics Co., Ltd
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

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

#ifndef __StarFishString__
#define __StarFishString__

#include "core/util/BasicString.h"

namespace StarFish {

typedef BasicString<char,
                    GCUtil::gc_malloc_atomic_ignore_off_page_allocator<char>>
    ASCIIString;
typedef BasicString<char,
                    GCUtil::gc_malloc_atomic_ignore_off_page_allocator<char>>
    UTF8String;
typedef BasicString<
    char16_t, GCUtil::gc_malloc_atomic_ignore_off_page_allocator<char16_t>>
    UTF16String;
typedef BasicString<
    char32_t, GCUtil::gc_malloc_atomic_ignore_off_page_allocator<char32_t>>
    UTF32String;
typedef BasicString<
    char16_t, GCUtil::gc_malloc_atomic_ignore_off_page_allocator<char16_t>>
    BMPString;

typedef std::basic_string<char, std::char_traits<char>> ASCIIStringDataNonGCStd;
typedef std::basic_string<char, std::char_traits<char>> UTF8StringDataNonGCStd;
typedef std::basic_string<char16_t, std::char_traits<char16_t>>
    UTF16StringDataNonGCStd;
typedef std::basic_string<char32_t, std::char_traits<char32_t>>
    UTF32StringDataNonGCStd;
}

namespace std {
template <>
struct hash<StarFish::ASCIIString> {
    size_t operator()(StarFish::ASCIIString const& x) const
    {
        return std::hash<StarFish::ASCIIStringDataNonGCStd>{}(
            StarFish::ASCIIStringDataNonGCStd(x.data()));
    }
};

template <>
struct equal_to<StarFish::ASCIIString> {
    bool operator()(StarFish::ASCIIString const& a,
                    StarFish::ASCIIString const& b) const
    {
        return a.compare(b) == 0;
    }
};

template <>
struct hash<StarFish::UTF16String> {
    size_t operator()(StarFish::UTF16String const& x) const
    {
        return std::hash<StarFish::UTF16StringDataNonGCStd>{}(
            StarFish::UTF16StringDataNonGCStd(x.data()));
    }
};

template <>
struct equal_to<StarFish::UTF16String> {
    bool operator()(StarFish::UTF16String const& a,
                    StarFish::UTF16String const& b) const
    {
        return a.compare(b) == 0;
    }
};

template <>
struct hash<StarFish::UTF32String> {
    size_t operator()(StarFish::UTF32String const& x) const
    {
        return std::hash<StarFish::UTF32StringDataNonGCStd>{}(
            StarFish::UTF32StringDataNonGCStd(x.data()));
    }
};

template <>
struct equal_to<StarFish::UTF32String> {
    bool operator()(StarFish::UTF32String const& a,
                    StarFish::UTF32String const& b) const
    {
        return a.compare(b) == 0;
    }
};
}

namespace StarFish {

class StringDataASCII;
class String;

template <typename CharType>
inline bool isASCII(CharType c)
{
    return !(c & ~0x7F);
}

template <typename CharType>
inline bool isASCIIUpper(CharType c)
{
    return c >= 'A' && c <= 'Z';
}

template <typename CharType>
inline bool isASCIILower(CharType c)
{
    return c >= 'a' && c <= 'z';
}

template <typename CharType>
inline bool isASCIIDigit(CharType c)
{
    return c >= '0' && c <= '9';
}

template <typename CharType>
inline CharType toASCIILower(CharType c)
{
    return c | ((c >= 'A' && c <= 'Z') << 5);
}

template <typename CharType>
inline CharType toASCIIUpper(CharType c)
{
    return static_cast<CharType>(c & ~((c >= 'a' && c <= 'z') << 5));
}

size_t utf32ToUtf8(char32_t uc, char* UTF8);
template <typename T>
size_t utf16ToUtf32(const T* UTF16, const T* bufferEnd, char32_t& uc)
{
    size_t tRequiredSize = 0;

    uc = 0x00000000;

    if (UTF16[0] >= 0xd800 && UTF16[0] <= 0xdbff) {
        if (UTF16 + 1 < bufferEnd) {
            if (UTF16[1] >= 0xdc00 && UTF16[1] <= 0xdfff) {
                uc += (UTF16[0] - 0xd800) << 10;
                uc += (UTF16[1] - 0xdc00) + 0x10000UL;
                tRequiredSize = 2;
            } else {
                uc = 0xFFFD;
                tRequiredSize = 1;
            }
        } else {
            uc = 0xFFFD;
            tRequiredSize = 1;
        }
    } else if (UTF16[0] >= 0xdc00 && UTF16[0] <= 0xdfff) {
        uc = 0xFFFD;
        tRequiredSize = 1;
    } else {
        uc = UTF16[0];
        tRequiredSize = 1;
    }

    return tRequiredSize;
}

struct NullableUTF8String : public gc {
    NullableUTF8String(const char* buffer, const size_t& bufferSize)
    {
        m_buffer = buffer;
        m_bufferSize = bufferSize;
    }
    const char* m_buffer;
    size_t m_bufferSize;
};

enum CharDirection {
    Ltr,
    Rtl,
    Mixed,
    Neutral,
};

enum CharCategory {
    NoCategory = 0,
    Other_NotAssigned = U_MASK(U_GENERAL_OTHER_TYPES),
    Letter_Uppercase = U_MASK(U_UPPERCASE_LETTER),
    Letter_Lowercase = U_MASK(U_LOWERCASE_LETTER),
    Letter_Titlecase = U_MASK(U_TITLECASE_LETTER),
    Letter_Modifier = U_MASK(U_MODIFIER_LETTER),
    Letter_Other = U_MASK(U_OTHER_LETTER),

    Mark_NonSpacing = U_MASK(U_NON_SPACING_MARK),
    Mark_Enclosing = U_MASK(U_ENCLOSING_MARK),
    Mark_SpacingCombining = U_MASK(U_COMBINING_SPACING_MARK),

    Number_DecimalDigit = U_MASK(U_DECIMAL_DIGIT_NUMBER),
    Number_Letter = U_MASK(U_LETTER_NUMBER),
    Number_Other = U_MASK(U_OTHER_NUMBER),

    Separator_Space = U_MASK(U_SPACE_SEPARATOR),
    Separator_Line = U_MASK(U_LINE_SEPARATOR),
    Separator_Paragraph = U_MASK(U_PARAGRAPH_SEPARATOR),

    Other_Control = U_MASK(U_CONTROL_CHAR),
    Other_Format = U_MASK(U_FORMAT_CHAR),
    Other_PrivateUse = U_MASK(U_PRIVATE_USE_CHAR),
    Other_Surrogate = U_MASK(U_SURROGATE),

    Punctuation_Dash = U_MASK(U_DASH_PUNCTUATION),
    Punctuation_Open = U_MASK(U_START_PUNCTUATION),
    Punctuation_Close = U_MASK(U_END_PUNCTUATION),
    Punctuation_Connector = U_MASK(U_CONNECTOR_PUNCTUATION),
    Punctuation_Other = U_MASK(U_OTHER_PUNCTUATION),

    Symbol_Math = U_MASK(U_MATH_SYMBOL),
    Symbol_Currency = U_MASK(U_CURRENCY_SYMBOL),
    Symbol_Modifier = U_MASK(U_MODIFIER_SYMBOL),
    Symbol_Other = U_MASK(U_OTHER_SYMBOL),

    Punctuation_InitialQuote = U_MASK(U_INITIAL_PUNCTUATION),
    Punctuation_FinalQuote = U_MASK(U_FINAL_PUNCTUATION)
};

class StringDataASCII;
class StringDataUTF32;

struct StringBufferAccessData {
    enum BufferDataKind {
        ASCIIData,
        BMPData,
        UTF32Data,
    };
    BufferDataKind bufferDataKind;
    bool isNullTerminated;
    size_t length;
    const void* buffer;

    bool hasASCIIData()
    {
        return bufferDataKind == ASCIIData;
    }

    char32_t charAt(size_t idx) const
    {
        if (bufferDataKind == ASCIIData) {
            return asciiData()[idx];
        } else if (bufferDataKind == BMPData) {
            return utf16Data()[idx];
        } else {
            return utf32Data()[idx];
        }
    }

    char32_t operator[](size_t idx) const
    {
        return charAt(idx);
    }

    const char* asciiData() const
    {
        STARFISH_ASSERT(bufferDataKind == ASCIIData);
        return (const char*)buffer;
    }

    // this buffer only contains Basic Multilingual Plane (BMP) codes. so always
    // (bufferLength == char number)
    const char16_t* utf16Data() const
    {
        STARFISH_ASSERT(bufferDataKind == BMPData);
        return (const char16_t*)buffer;
    }

    const char32_t* utf32Data() const
    {
        STARFISH_ASSERT(bufferDataKind == UTF32Data);
        return (const char32_t*)buffer;
    }
};

class String : public gc {
public:
    virtual ~String()
    {
    }
    static const unsigned defaultLengthLimit = 1 << 16;

    static String* const emptyString;
    static String* const spaceString;
    static String* const initialString;
    static String* const inheritString;
    static String* const unsetString;

    static String* fromUTF8(const char* src);
    static String* fromUTF8(const char* src, size_t len);
    static String* fromUTF16(const char16_t* src, size_t len);
    static String* createASCIIString(const char c);
    static String* createASCIIString(const char* src);
    static String* createASCIIStringWithNoGC(const char* src);
    static String* createUTF32String(const UTF32String& src);
    static String* createUTF32String(char32_t c);
    static String* createASCIIStringFromUTF32Source(const UTF32String& src);
    static String* createBMPStringFromUTF32Source(const UTF32String& src);
    static String* createASCIIStringFromUTF32SourceIfPossible(
        const UTF32String& src);

    static int parseInt(String* s);
    static int64_t parseInt64(String* s);
    static float parseFloat(String* s);
    static double parseDouble(String* s);
    static bool validDouble(String* s);

    virtual size_t length() const = 0;
    virtual char32_t charAt(const size_t& idx) const = 0;
    virtual StringBufferAccessData bufferAccessData() const = 0;

    virtual bool isStringView()
    {
        return false;
    }

    bool isEmpty() const
    {
        return length() == 0;
    }

    char32_t operator[](const size_t& idx) const
    {
        return charAt(idx);
    }

    size_t contentLength() const
    {
        auto data = bufferAccessData();
        if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
            return data.length;
        } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
            return data.length * 2;
        } else {
            return data.length * 4;
        }
    }

    bool equals(const String* src) const;
    bool equals(const char* src) const;

    bool equalsIgnoreCase(const String* str) const;
    bool equalsIgnoreCase(const char* str) const;
    bool equals(const char32_t* str) const;

    size_t indexOf(char32_t ch) const;
    size_t lastIndexOf(char32_t ch) const;

    UTF16String toUTF16String() const;
    UTF16StringDataNonGCStd toUTF16NonGCString() const;
    UTF16StringDataNonGCStd toUTF16NonGCString(size_t start, size_t end) const;

    UTF8StringDataNonGCStd toUTF8NonGCString(
        size_t start, size_t end, bool ignoreZeroWidthChar = false) const;

    UTF8StringDataNonGCStd toUTF8NonGCString() const;
    // 1. this method not always creates new buffer
    // 2. this method does NOT return NULL-TERMINATED char buffer!
    NullableUTF8String toNullableUTF8String();

    // this is fastest version of view utf8 data of string
    // const char* buffer ends with '\0'
    size_t peekUTF8Buffer(size_t (*)(const char* buffer, size_t len,
                                     void* data),
                          void* data) const;
    // this is fastest version of view utf16 data of string
    // const char16_t* buffer ends with '\0'
    size_t peekUTF16Buffer(size_t (*)(const char16_t* buffer, size_t len,
                                      void* data),
                           void* data) const;

    static inline bool isASCIIDigit(char32_t c)
    {
        return c >= '0' && c <= '9';
    }

    static inline bool isASCIISpace(char32_t c)
    {
        return c <= ' ' && (c == ' ' || (c <= 0xD && c >= 0x9));
    }

    static inline bool isSpaceOrNewline(char32_t c)
    {
        // Use isASCIISpace() for basic Latin-1.
        // This will include newlines, which aren't included in Unicode DirWS.
        return c <= 0x7F ? isASCIISpace(c)
                         : u_charDirection(c) == U_WHITE_SPACE_NEUTRAL;
    }

    static inline bool isNewline(char32_t c)
    {
        return isSpaceOrNewline(c) && !u_isblank(c);
    }

    static inline bool isSpace(char32_t c)
    {
        return isSpaceOrNewline(c) && u_isblank(c);
    }

    static inline bool isFixedWidthChar(char32_t c)
    {
        return (c == 0x3000 || c == 0x205F || (c >= 0x2000 && c <= 0x200A));
    }

    static inline bool isZeroWidthChar(char32_t CHAR)
    {
        if (CHAR < 32) {
            if (CHAR != 9 && CHAR != 10 && CHAR != 13) {
                return true;
            }
        }
        if (CHAR >= 0x7F && CHAR < 0xA0) {
            return true;
        }
        if (CHAR == 0xAD || CHAR == 0x200B || CHAR == 0x200E ||
            CHAR == 0x200F || CHAR == 0x202A || CHAR == 0x202B ||
            CHAR == 0x202C || CHAR == 0x202D || CHAR == 0x202E ||
            CHAR == 0xFEFF || CHAR == 0xFFFC) {
            return true;
        }
        return false;
    }

    static inline bool isNBSP(char32_t c)
    {
        return c == 0x00A0;
    }

    static inline CharCategory category(char32_t c)
    {
        return static_cast<CharCategory>(U_GET_GC_MASK(c));
    }

    static inline bool isPunctuation(char32_t c)
    {
        CharCategory charCategory = category(c);
        return charCategory == Punctuation_Open ||
               charCategory == Punctuation_Close ||
               charCategory == Punctuation_InitialQuote ||
               charCategory == Punctuation_FinalQuote ||
               charCategory == Punctuation_Other;
    }

    bool containsWhitespace(size_t start = 0, size_t end = SIZE_MAX);
    bool containsOnlyWhitespace(size_t start = 0, size_t end = SIZE_MAX);
    bool containsOnlyASCIIChars() const;
    bool containsOnlyDigits() const;
    String* stripAndCollapseASCIIwhitespace();
    template <typename T>
    static inline size_t stringHash(T* src, size_t length)
    {
        size_t hash = static_cast<size_t>(0xc70f6907UL);
        for (; length; --length)
            hash = (hash * 131) + *src++;
        return hash;
    }

    size_t hashValue() const
    {
        if (m_hashValue) {
            return m_hashValue;
        }
        return hashValueSlowCase();
    }

    static String* fromFloat(float f);
    static String* fromDouble(double d);
    static String* fromInt(int i);
    static String* fromInt64(int64_t i);

    String* substring(size_t pos, size_t len);
    String* remove(size_t pos, size_t len);
    String* insert(String* str, size_t pos);

    String* toUpper();
    String* toASCIIUpper();
    String* toLower();
    String* toASCIILower();
    String* replaceAll(String* from, String* to);

    String* concat(const char32_t c);
    String* concat(const char c);
    String* concat(const char* str);
    String* concat(String* str);
    String* trim();

    icu::UnicodeString toUnicodeString() const;
    icu::UnicodeString toUnicodeString(size_t start, size_t end) const;

    UTF32String toUTF32String();
    UTF8String toUTF8String();

    bool startsWith(const char* str, bool caseSensitive = true);
    bool startsWith(String* str, bool caseSensitive = true);

    bool endsWith(const char* str, bool caseSensitive = true);
    bool endsWith(String* str, bool caseSensitive = true);

    size_t find(const char* str, size_t pos = 0);
    size_t find(const char ch, size_t pos = 0);
    size_t find(String* str, size_t pos = 0);
    size_t find(String* str, size_t pos, bool caseSensitive);

    bool contains(const char* str, bool caseSensitive = true);
    bool contains(String* str, bool caseSensitive = true);

    static bool isASCIIPrintableKey(char c)
    {
        if (c >= 32 && c <= 126) {
            return true;
        }
        return false;
    }

protected:
    String()
    {
        m_hashValue = 0;
    }
    virtual ~String()
    {
    }

    size_t hashValueSlowCase() const;

    template <typename T>
    static bool stringEqual(const T* s, const T* s1, const size_t& len)
    {
        return memcmp(s, s1, sizeof(T) * len) == 0;
    }

    static bool stringEqual(const char32_t* s, const char* s1,
                            const size_t& len)
    {
        for (size_t i = 0; i < len; i++) {
            if (s[i] != (unsigned char)s1[i]) {
                return false;
            }
        }
        return true;
    }

    const char* utf8DataSlowCase(bool ignoreZeroWidthChar = false);

    bool isASCIIStringData(const char* str);

private:
    mutable size_t m_hashValue;
};

template <typename T>
class SimpleStringBufferHolder : public gc {
public:
    SimpleStringBufferHolder()
    {
        m_buffer = nullptr;
        m_length = 0;
    }

    SimpleStringBufferHolder(const T* buffer, size_t length)
    {
        copyFrom(buffer, length);
    }

    enum TakeBuffer { TakeBufferValue };

    // buffer length is must lager of equal than length + 1
    SimpleStringBufferHolder(T* buffer, size_t length, TakeBuffer)
    {
        if (buffer[length] == 0) {
            m_buffer = buffer;
            m_length = length;
        } else {
            copyFrom(buffer, length);
        }
    }

    const T* data() const
    {
        return m_buffer;
    }

    size_t length() const
    {
        return m_length;
    }

private:
    void copyFrom(const T* buffer, size_t length)
    {
        m_buffer =
            (T*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(sizeof(T) * (length + 1));
        memcpy(m_buffer, buffer, sizeof(T) * length);
        m_buffer[length] = 0;
        m_length = length;
    }
    T* m_buffer;
    size_t m_length;
};

class StringDataASCII : public String {
public:
    StringDataASCII(ASCIIString&& str)
        : String()
    {
        size_t length = str.length();
        m_data = SimpleStringBufferHolder<char>(
            str.takeBuffer(), length,
            SimpleStringBufferHolder<char>::TakeBufferValue);
    }

    StringDataASCII(const char* str)
        : String()
        , m_data(str, strlen(str))
    {
    }

    StringDataASCII(const char* str, size_t len)
        : String()
        , m_data(str, len)
    {
    }

    virtual size_t length() const override
    {
        return m_data.length();
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data.data()[idx];
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        StringBufferAccessData ret;
        ret.bufferDataKind = StringBufferAccessData::ASCIIData;
        ret.isNullTerminated = true;
        ret.buffer = m_data.data();
        ret.length = m_data.length();
        return ret;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    SimpleStringBufferHolder<char> m_data;
};

// WARNING: this class does not copy buffer
class StringDataOnStackASCII : public String {
public:
    StringDataOnStackASCII(const char* str, size_t length)
        : m_data(str)
        , m_length(length)
    {
    }

    inline void* operator new(size_t size) = delete;

    virtual size_t length() const override
    {
        return m_length;
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data[idx];
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        StringBufferAccessData ret;
        ret.bufferDataKind = StringBufferAccessData::ASCIIData;
        ret.isNullTerminated = true;
        ret.buffer = m_data;
        ret.length = m_length;
        return ret;
    }

protected:
    const char* m_data;
    size_t m_length;
};

class StringDataNonGCASCII : public String {
public:
    StringDataNonGCASCII(const char* str)
        : m_data(str)
    {
    }

    inline void* operator new(size_t size)
    {
        return malloc(size);
    }

    virtual size_t length() const override
    {
        return m_data.length();
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data[idx];
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        StringBufferAccessData ret;
        ret.bufferDataKind = StringBufferAccessData::ASCIIData;
        ret.isNullTerminated = true;
        ret.buffer = m_data.data();
        ret.length = m_data.length();
        return ret;
    }

protected:
    BasicString<char, std::allocator<char>> m_data;
};

class StringDataUTF32 : public String {
public:
    StringDataUTF32(const char32_t* str, size_t len)
        : String()
        , m_data(str, len)
    {
    }

    StringDataUTF32(const UTF32String& str)
        : String()
        , m_data(str.data(), str.length())
    {
    }

    StringDataUTF32(UTF32String&& str)
        : String()
    {
        size_t length = str.length();
        m_data = SimpleStringBufferHolder<char32_t>(
            str.takeBuffer(), length,
            SimpleStringBufferHolder<char32_t>::TakeBufferValue);
    }

    StringDataUTF32(const char* src, size_t len);
    StringDataUTF32(const char32_t* str)
        : String()
    {
        size_t length = 0;
        while (!str[length++]) {
        }
        m_data = SimpleStringBufferHolder<char32_t>(str, length);
    }

    virtual size_t length() const override
    {
        return m_data.length();
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data.data()[idx];
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        StringBufferAccessData ret;
        ret.bufferDataKind = StringBufferAccessData::UTF32Data;
        ret.isNullTerminated = true;
        ret.buffer = m_data.data();
        ret.length = m_data.length();
        return ret;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    SimpleStringBufferHolder<char32_t> m_data;
};

class StringDataBMP : public String {
public:
    StringDataBMP(const char16_t* str, size_t len)
        : String()
        , m_data(str, len)
    {
    }

    StringDataBMP(const char* str, size_t len);

    StringDataBMP(const BMPString& str)
        : String()
        , m_data(str.data(), str.length())
    {
    }

    StringDataBMP(BMPString&& str)
        : String()
    {
        size_t length = str.length();
        m_data = SimpleStringBufferHolder<char16_t>(
            str.takeBuffer(), length,
            SimpleStringBufferHolder<char16_t>::TakeBufferValue);
    }

    virtual size_t length() const override
    {
        return m_data.length();
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data.data()[idx];
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        StringBufferAccessData ret;
        ret.bufferDataKind = StringBufferAccessData::BMPData;
        ret.isNullTerminated = true;
        ret.buffer = m_data.data();
        ret.length = m_data.length();
        return ret;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    SimpleStringBufferHolder<char16_t> m_data;
};

// WARNING: this class does not copy buffer
class StringDataOnStackUTF32 : public String {
public:
    StringDataOnStackUTF32(const char32_t* str, size_t length)
        : m_data(str)
        , m_length(length)
    {
    }

    inline void* operator new(size_t size) = delete;

    virtual size_t length() const override
    {
        return m_length;
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data[idx];
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        StringBufferAccessData ret;
        ret.bufferDataKind = StringBufferAccessData::UTF32Data;
        ret.isNullTerminated = true;
        ret.buffer = m_data;
        ret.length = m_length;
        return ret;
    }

protected:
    const char32_t* m_data;
    size_t m_length;
};

// WARNING: this class does not copy buffer
class StringDataOnStackBMP : public String {
public:
    StringDataOnStackBMP(const char16_t* str, size_t length)
        : m_data(str)
        , m_length(length)
    {
    }

    inline void* operator new(size_t size) = delete;

    virtual size_t length() const override
    {
        return m_length;
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data[idx];
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        StringBufferAccessData ret;
        ret.bufferDataKind = StringBufferAccessData::BMPData;
        ret.isNullTerminated = true;
        ret.buffer = m_data;
        ret.length = m_length;
        return ret;
    }

protected:
    const char16_t* m_data;
    size_t m_length;
};

class StringView : public String {
public:
    StringView()
        : m_string(String::emptyString)
        , m_start(0)
        , m_end(0)
    {
    }

    StringView(String* string, size_t start, size_t end)
        : m_string(string)
        , m_start(start)
        , m_end(end)
    {
        STARFISH_ASSERT(start <= end);
        STARFISH_ASSERT(end <= m_string->length());
    }

    StringView(String* string)
        : StringView(string, 0, string->length())
    {
    }

    StringView(const StringView& src)
        : m_string(src.string())
        , m_start(src.start())
        , m_end(src.end())
    {
    }

    String* string() const
    {
        return m_string;
    }

    String* substring() const
    {
        return m_string->substring(m_start, m_end - m_start);
    }

    String* originalString() const
    {
        return m_string;
    }

    void setStart(size_t start)
    {
        m_start = start;
    }

    size_t start() const
    {
        return m_start;
    }

    void setEnd(size_t end)
    {
        m_end = end;
    }

    size_t end() const
    {
        return m_end;
    }

    virtual char32_t charAt(const size_t& idx) const
    {
        return m_string->charAt(idx + m_start);
    }

    virtual size_t length() const
    {
        return m_end - m_start;
    }

    virtual StringBufferAccessData bufferAccessData() const
    {
        auto srcData = m_string->bufferAccessData();
        StringBufferAccessData data;
        data.bufferDataKind = srcData.bufferDataKind;
        data.isNullTerminated = false;
        data.length = m_end - m_start;
        if (srcData.bufferDataKind == StringBufferAccessData::ASCIIData) {
            data.buffer = ((const char*)srcData.buffer) + m_start;
        } else if (srcData.bufferDataKind == StringBufferAccessData::BMPData) {
            data.buffer = ((char16_t*)srcData.buffer) + m_start;
        } else {
            data.buffer = ((char32_t*)srcData.buffer) + m_start;
        }
        return data;
    }

    virtual bool isStringView()
    {
        return true;
    }

    void* operator new(size_t size);
    void* operator new(size_t size, StringView* sv)
    {
        return sv;
    }

protected:
    String* m_string;
    size_t m_start, m_end;
};

class StringUtils {
public:
    // token is only 1-byte char now.
    static void tokenize(String* src, const char* tokens, size_t tokensLength,
                         GCVector<StringView>& result);
    static bool equalsIgnoreCase(const std::string& a, const std::string& b);
    static void ltrim(std::string& s);
    static void rtrim(std::string& s);
    static void trim(std::string& s);
    static std::string ltrimmed(std::string s);
    static std::string rtrimmed(std::string s);
    static std::string trimmed(std::string s);
    static void skipSpaces(const std::string& input,
                           unsigned long int& startIndex);
    static std::vector<std::string> split(const std::string& s, char seperator);
};

#ifndef STRING_BUILDER_INLINE_STORAGE_MAX
#define STRING_BUILDER_INLINE_STORAGE_MAX 64
#endif

class StringBuilder {
    struct StringBuilderPiece {
        enum Type {
            StringPiece,
            ConstChar,
            Char,
        };
        Type m_type;
        union {
            String* m_string;
            const char* m_raw;
            char32_t m_ch;
        };
        size_t m_start, m_end;
    };

    void appendPiece(char32_t ch);
    void appendPiece(const char* str);
    void appendPiece(String* str, size_t s, size_t e);

public:
    StringBuilder()
    {
        m_resultBufferKind = StringBufferAccessData::BufferDataKind::ASCIIData;
        m_contentLength = 0;
        m_piecesInlineStorageUsage = 0;
    }

    size_t contentLength()
    {
        return m_contentLength;
    }
    void appendString(const char* str)
    {
        appendPiece(str);
    }

    void appendChar(char32_t ch)
    {
        appendPiece(ch);
    }

    void appendChar(char ch)
    {
        appendPiece(ch);
    }

    void appendString(String* str)
    {
        appendPiece(str, 0, str->length());
    }

    void appendString(StringView sv)
    {
        appendPiece(sv.string(), sv.start(), sv.end());
    }

    void appendSubString(String* str, size_t s, size_t e)
    {
        appendPiece(str, s, e);
    }

    StringView finalizeToStringView();
    String* finalize();
    void clear()
    {
        m_resultBufferKind = StringBufferAccessData::BufferDataKind::ASCIIData;
        m_piecesInlineStorageUsage = 0;
        m_contentLength = 0;
        m_pieces.clear();
    }

protected:
    StringBufferAccessData::BufferDataKind m_resultBufferKind;
    size_t m_piecesInlineStorageUsage;
    size_t m_contentLength;
    StringBuilderPiece m_piecesInlineStorage[STRING_BUILDER_INLINE_STORAGE_MAX];
    GCVector<StringBuilderPiece> m_pieces;
};

class SegmentedString;
class SegmentedSubstring {
public:
    SegmentedSubstring()
        : m_length(0)
        , m_cursor(0)
        , m_accessData(String::emptyString->bufferAccessData())
        , m_doNotExcludeLineNumbers(true)
        , m_string(String::emptyString)
    {
    }

    SegmentedSubstring(String* str)
        : m_length(str->length())
        , m_cursor(0)
        , m_accessData(str->bufferAccessData())
        , m_doNotExcludeLineNumbers(true)
        , m_string(str)
    {
    }

    void clear()
    {
        m_accessData = String::emptyString->bufferAccessData();
        m_string = String::emptyString;
        m_length = 0;
        m_cursor = 0;
    }

    bool is8Bit()
    {
        return m_accessData.bufferDataKind == StringBufferAccessData::ASCIIData;
    }

    bool excludeLineNumbers() const
    {
        return !m_doNotExcludeLineNumbers;
    }
    bool doNotExcludeLineNumbers() const
    {
        return m_doNotExcludeLineNumbers;
    }

    void setExcludeLineNumbers()
    {
        m_doNotExcludeLineNumbers = false;
    }

    int numberOfCharactersConsumed() const
    {
        return m_string->length() - m_length;
    }

    void appendTo(UTF32String& builder) const
    {
        int offset = (int)m_string->length() - m_length;

        if (!offset) {
            if (m_length) {
                for (size_t i = 0; i < m_string->length(); i++) {
                    builder.push_back(m_string->charAt(i));
                }
            }
        } else {
            for (int i = offset; i < (offset + m_length); i++) {
                builder.push_back(m_string->charAt(i));
            }
        }
    }

    char32_t getCurrentChar8()
    {
        return m_accessData.asciiData()[m_cursor];
    }

    char32_t getCurrentChar32()
    {
        return m_accessData.charAt(m_cursor);
    }

    char32_t incrementAndGetCurrentChar8()
    {
        m_cursor++;
        return getCurrentChar8();
    }

    char32_t incrementAndGetCurrentChar32()
    {
        m_cursor++;
        return getCurrentChar32();
    }

    String* currentSubString(unsigned length)
    {
        int offset = m_string->length() - m_length;
        return m_string->substring(offset, length);
    }

    ALWAYS_INLINE char32_t getCurrentChar()
    {
        return getCurrentChar32();
    }

    ALWAYS_INLINE char32_t incrementAndGetCurrentChar()
    {
        return incrementAndGetCurrentChar32();
    }

public:
    int m_length;

private:
    size_t m_cursor;
    StringBufferAccessData m_accessData;
    bool m_doNotExcludeLineNumbers;
    String* m_string;
};

// An abstract number of element in a sequence. The sequence has a first
// element.
// This type should be used instead of integer because 2 contradicting
// traditions can
// call a first element '0' or '1' which makes integer type ambiguous.
class OrdinalNumber {
public:
    static OrdinalNumber fromZeroBasedInt(int zeroBasedInt)
    {
        return OrdinalNumber(zeroBasedInt);
    }
    static OrdinalNumber fromOneBasedInt(int oneBasedInt)
    {
        return OrdinalNumber(oneBasedInt - 1);
    }
    OrdinalNumber()
        : m_zeroBasedValue(0)
    {
    }

    int zeroBasedInt() const
    {
        return m_zeroBasedValue;
    }
    int oneBasedInt() const
    {
        return m_zeroBasedValue + 1;
    }

    bool operator==(OrdinalNumber other)
    {
        return m_zeroBasedValue == other.m_zeroBasedValue;
    }
    bool operator!=(OrdinalNumber other)
    {
        return !((*this) == other);
    }

    static OrdinalNumber first()
    {
        return OrdinalNumber(0);
    }
    static OrdinalNumber beforeFirst()
    {
        return OrdinalNumber(-1);
    }

private:
    OrdinalNumber(int zeroBasedInt)
        : m_zeroBasedValue(zeroBasedInt)
    {
    }
    int m_zeroBasedValue;
};

class SegmentedString {
public:
    SegmentedString()
        : m_pushedChar1(0)
        , m_pushedChar2(0)
        , m_currentChar(0)
        , m_numberOfCharactersConsumedPriorToCurrentString(0)
        , m_numberOfCharactersConsumedPriorToCurrentLine(0)
        , m_currentLine(0)
        , m_closed(false)
        , m_empty(true)
        , m_fastPathFlags(NoFastPath)
        , m_advanceFunc(&SegmentedString::advanceEmpty)
        , m_advanceAndUpdateLineNumberFunc(&SegmentedString::advanceEmpty)
    {
    }

    SegmentedString(String* str)
        : m_pushedChar1(0)
        , m_pushedChar2(0)
        , m_currentString(str)
        , m_currentChar(0)
        , m_numberOfCharactersConsumedPriorToCurrentString(0)
        , m_numberOfCharactersConsumedPriorToCurrentLine(0)
        , m_currentLine(0)
        , m_closed(false)
        , m_empty(!str->length())
        , m_fastPathFlags(NoFastPath)
    {
        if (m_currentString.m_length) {
            m_currentChar = m_currentString.getCurrentChar();
        }
        updateAdvanceFunctionPointers();
    }

    void clear();
    void close();

    void append(const SegmentedString&);
    void prepend(const SegmentedString&);

    bool excludeLineNumbers() const
    {
        return m_currentString.excludeLineNumbers();
    }
    void setExcludeLineNumbers();

    void push(char32_t c)
    {
        if (!m_pushedChar1) {
            m_pushedChar1 = c;
            m_currentChar = m_pushedChar1 ? m_pushedChar1
                                          : m_currentString.getCurrentChar();
            updateSlowCaseFunctionPointers();
        } else {
            STARFISH_ASSERT(!m_pushedChar2);
            m_pushedChar2 = c;
        }
    }

    bool isEmpty() const
    {
        return m_empty;
    }
    unsigned length() const;

    bool isClosed() const
    {
        return m_closed;
    }

    enum LookAheadResult {
        DidNotMatch,
        DidMatch,
        NotEnoughCharacters,
    };

    LookAheadResult lookAhead(String* string)
    {
        return lookAheadInline(string, true);
    }
    LookAheadResult lookAheadIgnoringCase(String* string)
    {
        return lookAheadInline(string, false);
    }

    void advance()
    {
        if (m_fastPathFlags & Use8BitAdvance) {
            STARFISH_ASSERT(!m_pushedChar1);
            bool haveOneCharacterLeft = (--m_currentString.m_length == 1);
            m_currentChar = m_currentString.incrementAndGetCurrentChar8();

            if (!haveOneCharacterLeft) {
                return;
            }

            updateSlowCaseFunctionPointers();

            return;
        }

        (this->*m_advanceFunc)();
    }

    inline void advanceAndUpdateLineNumber()
    {
        if (m_fastPathFlags & Use8BitAdvance) {
            STARFISH_ASSERT(!m_pushedChar1);

            bool haveNewLine =
                (m_currentChar == '\n') &
                !!(m_fastPathFlags & Use8BitAdvanceAndUpdateLineNumbers);
            bool haveOneCharacterLeft = (--m_currentString.m_length == 1);

            m_currentChar = m_currentString.incrementAndGetCurrentChar8();

            if (!(haveNewLine | haveOneCharacterLeft)) {
                return;
            }

            if (haveNewLine) {
                ++m_currentLine;
                m_numberOfCharactersConsumedPriorToCurrentLine =
                    m_numberOfCharactersConsumedPriorToCurrentString +
                    m_currentString.numberOfCharactersConsumed();
            }

            if (haveOneCharacterLeft) {
                updateSlowCaseFunctionPointers();
            }

            return;
        }

        (this->*m_advanceAndUpdateLineNumberFunc)();
    }

    void advanceAndASSERT(UChar expectedCharacter)
    {
        // ASSERT_UNUSED(expectedCharacter, currentChar() == expectedCharacter);
        advance();
    }

    void advanceAndASSERTIgnoringCase(UChar expectedCharacter)
    {
        // ASSERT_UNUSED(expectedCharacter,
        // WTF::Unicode::foldCase(currentChar()) ==
        // WTF::Unicode::foldCase(expectedCharacter));
        advance();
    }

    void advancePastNonNewline()
    {
        STARFISH_ASSERT(currentChar() != '\n');
        advance();
    }

    void advancePastNewlineAndUpdateLineNumber()
    {
        STARFISH_ASSERT(currentChar() == '\n');
        if (!m_pushedChar1 && m_currentString.m_length > 1) {
            int newLineFlag = m_currentString.doNotExcludeLineNumbers();
            m_currentLine += newLineFlag;
            if (newLineFlag) {
                m_numberOfCharactersConsumedPriorToCurrentLine =
                    numberOfCharactersConsumed() + 1;
            }
            decrementAndCheckLength();
            m_currentChar = m_currentString.incrementAndGetCurrentChar();
            return;
        }
        advanceAndUpdateLineNumberSlowCase();
    }

    // Writes the consumed characters into consumedCharacters, which must
    // have space for at least |count| characters.
    void advance(unsigned count, char32_t* consumedCharacters);

    bool escaped() const
    {
        return m_pushedChar1;
    }

    int numberOfCharactersConsumed() const
    {
        int numberOfPushedCharacters = 0;
        if (m_pushedChar1) {
            ++numberOfPushedCharacters;
            if (m_pushedChar2) {
                ++numberOfPushedCharacters;
            }
        }
        return m_numberOfCharactersConsumedPriorToCurrentString +
               m_currentString.numberOfCharactersConsumed() -
               numberOfPushedCharacters;
    }

    String* toString() const;

    char32_t currentChar() const
    {
        return m_currentChar;
    }

    // The method is moderately slow, comparing to currentLine method.
    OrdinalNumber currentColumn() const;
    OrdinalNumber currentLine() const;
    // Sets value of line/column variables. Column is specified indirectly by a
    // parameter columnAftreProlog
    // which is a value of column that we should get after a prolog (first
    // prologLength characters) has been consumed.
    void setCurrentPosition(OrdinalNumber line, OrdinalNumber columnAftreProlog,
                            int prologLength);

private:
    enum FastPathFlags {
        NoFastPath = 0,
        Use8BitAdvanceAndUpdateLineNumbers = 1 << 0,
        Use8BitAdvance = 1 << 1,
    };

    void append(const SegmentedSubstring&);
    void prepend(const SegmentedSubstring&);

    void advance8();
    void advance16();
    void advanceAndUpdateLineNumber8();
    void advanceAndUpdateLineNumber16();
    void advanceSlowCase();
    void advanceAndUpdateLineNumberSlowCase();
    void advanceEmpty();
    void advanceSubstring();

    void updateSlowCaseFunctionPointers();

    void decrementAndCheckLength()
    {
        STARFISH_ASSERT(m_currentString.m_length > 1);
        if (--m_currentString.m_length == 1) {
            updateSlowCaseFunctionPointers();
        }
    }

    void updateAdvanceFunctionPointers()
    {
        if ((m_currentString.m_length > 1) && !m_pushedChar1) {
            if (m_currentString.is8Bit()) {
                m_advanceFunc = &SegmentedString::advance8;
                m_fastPathFlags = Use8BitAdvance;
                if (m_currentString.doNotExcludeLineNumbers()) {
                    m_advanceAndUpdateLineNumberFunc =
                        &SegmentedString::advanceAndUpdateLineNumber8;
                    m_fastPathFlags |= Use8BitAdvanceAndUpdateLineNumbers;
                } else {
                    m_advanceAndUpdateLineNumberFunc =
                        &SegmentedString::advance8;
                }
                return;
            }

            m_advanceFunc = &SegmentedString::advance16;
            m_fastPathFlags = NoFastPath;
            if (m_currentString.doNotExcludeLineNumbers()) {
                m_advanceAndUpdateLineNumberFunc =
                    &SegmentedString::advanceAndUpdateLineNumber16;
            } else {
                m_advanceAndUpdateLineNumberFunc = &SegmentedString::advance16;
            }
            return;
        }

        if (!m_currentString.m_length && !isComposite()) {
            m_advanceFunc = &SegmentedString::advanceEmpty;
            m_fastPathFlags = NoFastPath;
            m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
        }

        updateSlowCaseFunctionPointers();
    }

    inline LookAheadResult lookAheadInline(String* string, bool caseSensitive)
    {
        if (!m_pushedChar1 &&
            string->length() <=
                static_cast<unsigned>(m_currentString.m_length)) {
            String* currentSubstring =
                m_currentString.currentSubString(string->length());
            if (currentSubstring->startsWith(string, caseSensitive)) {
                return DidMatch;
            }
            return DidNotMatch;
        }
        return lookAheadSlowCase(string, caseSensitive);
    }

    LookAheadResult lookAheadSlowCase(String* string, bool caseSensitive)
    {
        unsigned count = string->length();
        if (count > length()) {
            return NotEnoughCharacters;
        }
        UTF32String consumedCharacters;
        consumedCharacters.resize(count);
        advance(count, (char32_t*)consumedCharacters.data());
        LookAheadResult result = DidNotMatch;
        String* consumedString =
            new StringDataUTF32(std::move(consumedCharacters));
        if (consumedString->startsWith(string, caseSensitive)) {
            result = DidMatch;
        }
        prepend(SegmentedString(consumedString));
        return result;
    }

    bool isComposite() const
    {
        return !(m_substrings.size() == 0);
    }

    char32_t m_pushedChar1;
    char32_t m_pushedChar2;
    SegmentedSubstring m_currentString;
    char32_t m_currentChar;
    int m_numberOfCharactersConsumedPriorToCurrentString;
    int m_numberOfCharactersConsumedPriorToCurrentLine;
    int m_currentLine;
    GCDeque<SegmentedSubstring> m_substrings;
    bool m_closed;
    bool m_empty;
    unsigned char m_fastPathFlags;
    void (SegmentedString::*m_advanceFunc)();
    void (SegmentedString::*m_advanceAndUpdateLineNumberFunc)();
};

struct TextRun {
    StringView m_stringView;
    CharDirection m_direction;

    TextRun(String* str, size_t startPosition, size_t endPosition,
            CharDirection dir)
        : m_stringView(StringView(str, startPosition, endPosition))
    {
        m_direction = dir;
    }
#ifndef NDEBUG
    static std::string replaceAll(const std::string& str,
                                  const std::string& pattern,
                                  const std::string& replace)
    {
        std::string result = str;
        std::string::size_type pos = 0;
        std::string::size_type offset = 0;

        while ((pos = result.find(pattern, offset)) != std::string::npos) {
            result.replace(result.begin() + pos,
                           result.begin() + pos + pattern.size(), replace);
            offset = pos + replace.size();
        }

        return result;
    }

    void dump() const
    {
        UTF8StringDataNonGCStd str =
            m_stringView.substring()->toUTF8NonGCString();
        str = replaceAll(str, "\n", "\\n");
        printf("%s", m_direction == Ltr
                         ? "L"
                         : (m_direction == Rtl
                                ? "R"
                                : (m_direction == Neutral ? "N" : "M")));
        printf("[%s]", str.data());
        printf(":%zu\n", m_stringView.length());
    }
#endif
};
}

namespace std {
template <>
struct hash<StarFish::String*> {
    std::size_t operator()(const StarFish::String* s) const
    {
        return s->hashValue();
    }
};

template <>
struct equal_to<StarFish::String*> {
    bool operator()(const StarFish::String* s1,
                    const StarFish::String* s2) const
    {
        return s1->equals(s2);
    }
};
}
#endif
