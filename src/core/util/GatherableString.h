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

#ifndef __StarFishGatherableString__
#define __StarFishGatherableString__

// for AtomicString
#include "StarFish.h"

namespace StarFish {

// Special string for parsers
template <unsigned int InlineStorageSize>
class GatherableString : public gc {
public:
    GatherableString()
    {
        m_hasASCIIContent = true;
        m_hasBMPContent = true;
        m_length = 0;
        m_externalString = nullptr;
    }

    GatherableString(const GatherableString& src)
    {
        operator=(src);
    }

    void operator=(const GatherableString& src)
    {
        m_hasASCIIContent = src.m_hasASCIIContent;
        m_hasBMPContent = src.m_hasBMPContent;
        m_length = src.m_length;
        memcpy(m_builtInBuffer, src.m_builtInBuffer,
               sizeof(char32_t) *
                   std::min((size_t)InlineStorageSize, m_length));
        if (src.m_externalString) {
            m_externalString = new UTF32String(*src.m_externalString);
        } else {
            m_externalString = nullptr;
        }
    }

    GatherableString(GatherableString&& src)
    {
        m_hasASCIIContent = src.m_hasASCIIContent;
        m_hasBMPContent = src.m_hasBMPContent;
        m_length = src.m_length;
        memcpy(m_builtInBuffer, src.m_builtInBuffer,
               sizeof(char32_t) *
                   std::min((size_t)InlineStorageSize, m_length));
        m_externalString = src.m_externalString;

        src.m_hasASCIIContent = true;
        src.m_hasBMPContent = true;
        src.m_length = 0;
        src.m_externalString = nullptr;
    }

    void clear()
    {
        m_length = 0;
        m_hasASCIIContent = true;
        m_hasBMPContent = true;
        m_externalString = nullptr;
    }

    void appendChar(char32_t ch)
    {
        if (ch > 0xffff) {
            m_hasBMPContent = false;
            m_hasASCIIContent = false;
        } else if (ch > 127) {
            m_hasASCIIContent = false;
        }
        if (m_length < InlineStorageSize) {
            m_builtInBuffer[m_length++] = ch;
        } else {
            if (!m_externalString)
                m_externalString = new UTF32String();
            m_externalString->push_back(ch);
            m_length++;
        }
    }

    size_t indexOf(const char32_t& ch)
    {
        for (size_t i = 0; i < length(); i++) {
            if (ch == charAt(i)) {
                return i;
            }
        }
        return SIZE_MAX;
    }

    bool contains(const char32_t& ch)
    {
        return indexOf(ch) != SIZE_MAX;
    }

    size_t length() const
    {
        return m_length;
    }

    size_t size() const
    {
        return m_length;
    }

    char32_t charAt(const size_t& i) const
    {
        if (i < InlineStorageSize) {
            return m_builtInBuffer[i];
        } else {
            return (*m_externalString)[i - InlineStorageSize];
        }
    }

    char32_t operator[](const size_t& i) const
    {
        return charAt(i);
    }

    bool hasASCIIContent() const
    {
        return m_hasASCIIContent;
    }

    bool hasBMPContent() const
    {
        return m_hasBMPContent;
    }

    void appendOther(const GatherableString& src)
    {
        for (size_t i = 0; i < src.length(); i++) {
            appendChar(src.charAt(i));
        }
    }

    bool equals(const char* src) const;
    bool equalsIgnoreCase(const char* src) const;
    void toLower();
    String* toString() const;
    size_t peekASCIIBuffer(size_t (*cb)(const char* buffer, size_t len,
                                        void* data),
                           void* data) const;
    size_t peekBMPBuffer(size_t (*cb)(const char16_t* buffer, size_t len,
                                      void* data),
                         void* data) const;
    size_t peekUTF32Buffer(size_t (*cb)(const char32_t* buffer, size_t len,
                                        void* data),
                           void* data) const;
    size_t peekUTF8Buffer(size_t (*cb)(const char* buffer, size_t len,
                                       void* data),
                          void* data) const;

    AtomicString toAtomicString(StarFish* sf) const;
    AtomicString toAttrAtomicString(StarFish* sf) const;

protected:
    bool m_hasASCIIContent;
    bool m_hasBMPContent;
    size_t m_length;
    char32_t m_builtInBuffer[InlineStorageSize];
    UTF32String* m_externalString;
};

template <unsigned int InlineStorageSize>
bool GatherableString<InlineStorageSize>::equals(const char* src) const
{
    size_t len = strlen(src);
    if (len != m_length) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (i < InlineStorageSize) {
            if (m_builtInBuffer[i] != (char32_t)src[i])
                return false;
        } else {
            if ((*m_externalString)[i - InlineStorageSize] !=
                (char32_t)src[i]) {
                return false;
            }
        }
    }

    return true;
}

template <unsigned int InlineStorageSize>
bool GatherableString<InlineStorageSize>::equalsIgnoreCase(
    const char* src) const
{
    size_t len = strlen(src);

#ifndef NDEBUG
    for (size_t i = 0; i < len; i++) {
        if ('A' <= src[i] && src[i] <= 'Z') {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }
#endif

    if (len != m_length) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (i < InlineStorageSize) {
            if (::tolower(m_builtInBuffer[i]) != src[i])
                return false;
        } else {
            if (::tolower((*m_externalString)[i - InlineStorageSize]) !=
                src[i]) {
                return false;
            }
        }
    }
    return true;
}

template <unsigned int InlineStorageSize>
void GatherableString<InlineStorageSize>::toLower()
{
    size_t len = length();
    for (size_t i = 0; i < len; i++) {
        if (i < InlineStorageSize) {
            m_builtInBuffer[i] = ::tolower(m_builtInBuffer[i]);
        } else {
            (*m_externalString)[i - InlineStorageSize] =
                ::tolower((*m_externalString)[i - InlineStorageSize]);
        }
    }
}

template <unsigned int InlineStorageSize>
String* GatherableString<InlineStorageSize>::toString() const
{
    if (m_hasASCIIContent) {
        ASCIIString newStringData;
        newStringData.resize(length());

        for (size_t i = 0; i < length(); i++) {
            newStringData[i] = (char)charAt(i);
        }

        return new StringDataASCII(std::move(newStringData));
    } else if (m_hasBMPContent) {
        BMPString newStringData;
        newStringData.resize(length());

        for (size_t i = 0; i < length(); i++) {
            newStringData[i] = charAt(i);
        }

        return new StringDataBMP(std::move(newStringData));
    } else {
        UTF32String newStringData;
        newStringData.resize(length());

        for (size_t i = 0; i < length(); i++) {
            newStringData[i] = charAt(i);
        }

        return new StringDataUTF32(std::move(newStringData));
    }
}

template <unsigned int InlineStorageSize>
size_t GatherableString<InlineStorageSize>::peekASCIIBuffer(
    size_t (*cb)(const char* buffer, size_t len, void* data), void* data) const
{
    STARFISH_ASSERT(hasASCIIContent());
    char* newStringData = ALLOCA(length() + 1, char);
    for (size_t i = 0; i < length(); i++) {
        newStringData[i] = (char)charAt(i);
    }
    newStringData[length()] = 0;

    return cb(newStringData, length(), data);
}

template <unsigned int InlineStorageSize>
size_t GatherableString<InlineStorageSize>::peekBMPBuffer(
    size_t (*cb)(const char16_t* buffer, size_t len, void* data),
    void* data) const
{
    STARFISH_ASSERT(hasBMPContent());
    char16_t* newStringData = ALLOCA(length() + 1, char16_t);
    for (size_t i = 0; i < length(); i++) {
        newStringData[i] = (char16_t)charAt(i);
    }
    newStringData[length()] = 0;

    return cb(newStringData, length(), data);
}

template <unsigned int InlineStorageSize>
size_t GatherableString<InlineStorageSize>::peekUTF32Buffer(
    size_t (*cb)(const char32_t* buffer, size_t len, void* data),
    void* data) const
{
    char32_t* newStringData =
        ALLOCA((length() + 1) * sizeof(char32_t), char32_t);
    for (size_t i = 0; i < length(); i++) {
        newStringData[i] = charAt(i);
    }
    newStringData[length()] = 0;

    return cb(newStringData, length(), data);
}

template <unsigned int InlineStorageSize>
size_t GatherableString<InlineStorageSize>::peekUTF8Buffer(
    size_t (*cb)(const char* buffer, size_t len, void* data), void* data) const
{
    if (hasASCIIContent()) {
        return peekASCIIBuffer(cb, data);
    } else {
        size_t utf8Len = 0;
        for (size_t i = 0; i < length(); i++) {
            char unused[8];
            utf8Len += utf32ToUtf8(charAt(i), unused);
        }

        char* utf8Buffer = ALLOCA(utf8Len + 1, char);
        size_t pos = 0;
        for (size_t i = 0; i < length(); i++) {
            pos += utf32ToUtf8(charAt(i), &utf8Buffer[pos]);
        }
        // We don't need to fill '\0' at end
        // utf32ToUtf8 function already fills to end
        return cb(utf8Buffer, utf8Len, data);
    }
}

template <unsigned int InlineStorageSize>
AtomicString GatherableString<InlineStorageSize>::toAtomicString(
    StarFish* sf) const
{
    if (hasASCIIContent()) {
        return AtomicString((String*)peekASCIIBuffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                StarFish* sf = (StarFish*)data;
                return (size_t)AtomicString::createAtomicString(sf, buf, len)
                    .string();
            },
            sf));
    } else if (hasBMPContent()) {
        return AtomicString((String*)peekBMPBuffer(
            [](const char16_t* buf, size_t len, void* data) -> size_t {
                StarFish* sf = (StarFish*)data;
                return (size_t)AtomicString::createAtomicString(sf, buf, len)
                    .string();
            },
            sf));
    } else {
        return AtomicString((String*)peekUTF32Buffer(
            [](const char32_t* buf, size_t len, void* data) -> size_t {
                StarFish* sf = (StarFish*)data;
                return (size_t)AtomicString::createAtomicString(sf, buf, len)
                    .string();
            },
            sf));
    }
}

template <unsigned int InlineStorageSize>
AtomicString GatherableString<InlineStorageSize>::toAttrAtomicString(
    StarFish* sf) const
{
    if (hasASCIIContent()) {
        return AtomicString((String*)peekASCIIBuffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                StarFish* sf = (StarFish*)data;
                return (size_t)AtomicString::createAttrAtomicString(sf, buf,
                                                                    len)
                    .string();
            },
            sf));
    } else if (hasBMPContent()) {
        return AtomicString((String*)peekBMPBuffer(
            [](const char16_t* buf, size_t len, void* data) -> size_t {
                StarFish* sf = (StarFish*)data;
                return (size_t)AtomicString::createAttrAtomicString(sf, buf,
                                                                    len)
                    .string();
            },
            sf));
    } else {
        return AtomicString((String*)peekUTF32Buffer(
            [](const char32_t* buf, size_t len, void* data) -> size_t {
                StarFish* sf = (StarFish*)data;
                return (size_t)AtomicString::createAttrAtomicString(sf, buf,
                                                                    len)
                    .string();
            },
            sf));
    }
}
}
#endif
