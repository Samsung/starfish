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

#include "StarfishConfig.h"
#include "AtomicString.h"
#include "Starfish.h"

namespace Starfish {

AtomicString::AtomicString()
{
    m_string = String::emptyString;
}

AtomicString AtomicString::createAtomicString(Starfish* starfish, String* str)
{
    auto iter = starfish->m_atomicStringMap.find(str);
    if (starfish->m_atomicStringMap.end() == iter) {
        String* ns = str;
        if (str->isStringView()) {
            auto data = str->bufferAccessData();
            if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
                ns = new StringDataASCII((char*)data.buffer, data.length);
            } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
                ns = new StringDataBMP((char16_t*)data.buffer, data.length);
            } else {
                ns = new StringDataUTF32((char32_t*)data.buffer, data.length);
            }
        }
        starfish->m_atomicStringMap.insert(ns);
        return AtomicString(ns);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(Starfish* starfish,
                                              StringView str)
{
    auto iter = starfish->m_atomicStringMap.find(&str);
    if (starfish->m_atomicStringMap.end() == iter) {
        auto sv = new StringView(str);
        starfish->m_atomicStringMap.insert(sv);
        return AtomicString(sv);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(Starfish* starfish,
                                              const char* str)
{
    return createAtomicString(starfish, str, strlen(str));
}

AtomicString AtomicString::createAtomicString(Starfish* starfish,
                                              const char* cStr, size_t length)
{
    StringDataOnStackASCII str(cStr, length);
    auto iter = starfish->m_atomicStringMap.find(&str);
    if (starfish->m_atomicStringMap.end() == iter) {
        String* string = new StringDataASCII(cStr, length);
        starfish->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(Starfish* starfish,
                                              const char16_t* cStr,
                                              size_t length)
{
    StringDataOnStackBMP str(cStr, length);
    auto iter = starfish->m_atomicStringMap.find(&str);
    if (starfish->m_atomicStringMap.end() == iter) {
        String* string = new StringDataBMP(cStr, length);
        starfish->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(Starfish* starfish,
                                              const char32_t* cStr,
                                              size_t length)
{
    StringDataOnStackUTF32 str(cStr, length);
    auto iter = starfish->m_atomicStringMap.find(&str);
    if (starfish->m_atomicStringMap.end() == iter) {
        String* string = new StringDataUTF32(cStr, length);
        starfish->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAttrAtomicString(Starfish* starfish,
                                                  String* str)
{
    auto data = str->bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        char* buf = ALLOCA(data.length + 1, char);
        buf[data.length] = 0;
        for (size_t i = 0; i < data.length; i++) {
            buf[i] = tolower(data.asciiData()[i]);
        }
        StringDataOnStackASCII str(buf, data.length);

        auto iter = starfish->m_atomicStringMap.find(&str);
        if (starfish->m_atomicStringMap.end() == iter) {
            String* string = new StringDataASCII(buf, data.length);
            starfish->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        char16_t* buf = ALLOCA((data.length + 1) * sizeof(char16_t), char16_t);
        buf[data.length] = 0;
        for (size_t i = 0; i < data.length; i++) {
            buf[i] = tolower(data.utf16Data()[i]);
        }
        StringDataOnStackBMP str(buf, data.length);

        auto iter = starfish->m_atomicStringMap.find(&str);
        if (starfish->m_atomicStringMap.end() == iter) {
            BMPString s(buf, data.length);
            String* string = new StringDataBMP(std::move(s));
            starfish->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    } else {
        char32_t* buf = ALLOCA((data.length + 1) * sizeof(char32_t), char32_t);
        buf[data.length] = 0;
        for (size_t i = 0; i < data.length; i++) {
            buf[i] = tolower(data.utf32Data()[i]);
        }
        StringDataOnStackUTF32 str(buf, data.length);

        auto iter = starfish->m_atomicStringMap.find(&str);
        if (starfish->m_atomicStringMap.end() == iter) {
            UTF32String s(buf, data.length);
            String* string = new StringDataUTF32(std::move(s));
            starfish->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    }
}

AtomicString AtomicString::createAttrAtomicString(Starfish* starfish,
                                                  char32_t str)
{
    if (str < 128) {
        char* buf = ALLOCA(2, char);
        buf[0] = (char)str;
        buf[1] = 0;
        StringDataOnStackASCII str(buf, 1);

        auto iter = starfish->m_atomicStringMap.find(&str);
        if (starfish->m_atomicStringMap.end() == iter) {
            String* string = new StringDataASCII(buf, 1);
            starfish->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    } else {
        char32_t* buf = ALLOCA(sizeof(char32_t) * 2, char32_t);
        buf[0] = str;
        buf[1] = 0;
        StringDataOnStackUTF32 str(buf, 1);

        auto iter = starfish->m_atomicStringMap.find(&str);
        if (starfish->m_atomicStringMap.end() == iter) {
            String* string = new StringDataUTF32(buf);
            starfish->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    }
}

AtomicString AtomicString::createAttrAtomicString(Starfish* starfish,
                                                  const char* str)
{
    return AtomicString::createAttrAtomicString(starfish, str, strlen(str));
}

AtomicString AtomicString::createAttrAtomicString(Starfish* starfish,
                                                  const char* str,
                                                  size_t length)
{
    char* buf = ALLOCA(length + 1, char);
    buf[length] = 0;
    for (size_t i = 0; i < length; i++) {
        buf[i] = tolower(str[i]);
    }
    StringDataOnStackASCII newStr(buf, length);

    auto iter = starfish->m_atomicStringMap.find(&newStr);
    if (starfish->m_atomicStringMap.end() == iter) {
        String* string = new StringDataASCII(buf, length);
        starfish->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAttrAtomicString(Starfish* starfish,
                                                  const char16_t* str,
                                                  size_t length)
{
    char16_t* buf = ALLOCA((length + 1) * sizeof(char16_t), char16_t);
    buf[length] = 0;
    for (size_t i = 0; i < length; i++) {
        buf[i] = tolower(str[i]);
    }
    StringDataOnStackBMP newStr(buf, length);

    auto iter = starfish->m_atomicStringMap.find(&newStr);
    if (starfish->m_atomicStringMap.end() == iter) {
        String* string = new StringDataBMP(buf, length);
        starfish->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAttrAtomicString(Starfish* starfish,
                                                  const char32_t* str,
                                                  size_t length)
{
    char32_t* buf = ALLOCA((length + 1) * sizeof(char32_t), char32_t);
    buf[length] = 0;
    for (size_t i = 0; i < length; i++) {
        buf[i] = tolower(str[i]);
    }
    StringDataOnStackUTF32 newStr(buf, length);

    auto iter = starfish->m_atomicStringMap.find(&newStr);
    if (starfish->m_atomicStringMap.end() == iter) {
        String* string = new StringDataUTF32(buf, length);
        starfish->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}
}
