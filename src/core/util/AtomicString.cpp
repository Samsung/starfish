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
#include "AtomicString.h"
#include "StarFish.h"

namespace StarFish {

AtomicString::AtomicString()
{
    m_string = String::emptyString;
}

AtomicString AtomicString::createAtomicString(StarFish* sf, String* str)
{
    auto iter = sf->m_atomicStringMap.find(str);
    if (sf->m_atomicStringMap.end() == iter) {
        sf->m_atomicStringMap.insert(str);
        return AtomicString(str);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(StarFish* sf, StringView str)
{
    auto iter = sf->m_atomicStringMap.find(&str);
    if (sf->m_atomicStringMap.end() == iter) {
        auto sv = new StringView(str);
        sf->m_atomicStringMap.insert(sv);
        return AtomicString(sv);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(StarFish* sf, const char* str)
{
    return createAtomicString(sf, str, strlen(str));
}

AtomicString AtomicString::createAtomicString(StarFish* sf, const char* cStr,
                                              size_t length)
{
    StringDataOnStackASCII str(cStr, length);
    auto iter = sf->m_atomicStringMap.find(&str);
    if (sf->m_atomicStringMap.end() == iter) {
        String* string = new StringDataASCII(cStr, length);
        sf->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(StarFish* sf,
                                              const char16_t* cStr,
                                              size_t length)
{
    StringDataOnStackBMP str(cStr, length);
    auto iter = sf->m_atomicStringMap.find(&str);
    if (sf->m_atomicStringMap.end() == iter) {
        String* string = new StringDataBMP(cStr, length);
        sf->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAtomicString(StarFish* sf,
                                              const char32_t* cStr,
                                              size_t length)
{
    StringDataOnStackUTF32 str(cStr, length);
    auto iter = sf->m_atomicStringMap.find(&str);
    if (sf->m_atomicStringMap.end() == iter) {
        String* string = new StringDataUTF32(cStr, length);
        sf->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, String* str)
{
    auto data = str->bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        char* buf = (char*)alloca(data.length + 1);
        buf[data.length] = 0;
        for (size_t i = 0; i < data.length; i++) {
            buf[i] = ::tolower(data.asciiData()[i]);
        }
        StringDataOnStackASCII str(buf, data.length);

        auto iter = sf->m_atomicStringMap.find(&str);
        if (sf->m_atomicStringMap.end() == iter) {
            String* string = new StringDataASCII(buf, data.length);
            sf->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        char16_t* buf = (char16_t*)alloca((data.length + 1) * sizeof(char16_t));
        buf[data.length] = 0;
        for (size_t i = 0; i < data.length; i++) {
            buf[i] = ::tolower(data.utf16Data()[i]);
        }
        StringDataOnStackBMP str(buf, data.length);

        auto iter = sf->m_atomicStringMap.find(&str);
        if (sf->m_atomicStringMap.end() == iter) {
            String* string =
                new StringDataBMP(std::move(BMPString(buf, data.length)));
            sf->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    } else {
        char32_t* buf = (char32_t*)alloca((data.length + 1) * sizeof(char32_t));
        buf[data.length] = 0;
        for (size_t i = 0; i < data.length; i++) {
            buf[i] = ::tolower(data.utf32Data()[i]);
        }
        StringDataOnStackUTF32 str(buf, data.length);

        auto iter = sf->m_atomicStringMap.find(&str);
        if (sf->m_atomicStringMap.end() == iter) {
            String* string =
                new StringDataUTF32(std::move(UTF32String(buf, data.length)));
            sf->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    }
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, char32_t str)
{
    if (str < 128) {
        char* buf = (char*)alloca(2);
        buf[0] = (char)str;
        buf[1] = 0;
        StringDataOnStackASCII str(buf, 1);

        auto iter = sf->m_atomicStringMap.find(&str);
        if (sf->m_atomicStringMap.end() == iter) {
            String* string = new StringDataASCII(buf, 1);
            sf->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    } else {
        char32_t* buf = (char32_t*)alloca(sizeof(char32_t) * 2);
        buf[0] = str;
        buf[1] = 0;
        StringDataOnStackUTF32 str(buf, 1);

        auto iter = sf->m_atomicStringMap.find(&str);
        if (sf->m_atomicStringMap.end() == iter) {
            String* string = new StringDataUTF32(buf);
            sf->m_atomicStringMap.insert(string);
            return AtomicString(string);
        } else {
            return AtomicString(iter.operator*());
        }
    }
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, const char* str)
{
    return AtomicString::createAttrAtomicString(sf, str, strlen(str));
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, const char* str,
                                                  size_t length)
{
    char* buf = (char*)alloca(length + 1);
    buf[length] = 0;
    for (size_t i = 0; i < length; i++) {
        buf[i] = ::tolower(str[i]);
    }
    StringDataOnStackASCII newStr(buf, length);

    auto iter = sf->m_atomicStringMap.find(&newStr);
    if (sf->m_atomicStringMap.end() == iter) {
        String* string = new StringDataASCII(buf, length);
        sf->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf,
                                                  const char16_t* str,
                                                  size_t length)
{
    char16_t* buf = (char16_t*)alloca((length + 1) * sizeof(char16_t));
    buf[length] = 0;
    for (size_t i = 0; i < length; i++) {
        buf[i] = ::tolower(str[i]);
    }
    StringDataOnStackBMP newStr(buf, length);

    auto iter = sf->m_atomicStringMap.find(&newStr);
    if (sf->m_atomicStringMap.end() == iter) {
        String* string = new StringDataBMP(buf, length);
        sf->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf,
                                                  const char32_t* str,
                                                  size_t length)
{
    char32_t* buf = (char32_t*)alloca((length + 1) * sizeof(char32_t));
    buf[length] = 0;
    for (size_t i = 0; i < length; i++) {
        buf[i] = ::tolower(str[i]);
    }
    StringDataOnStackUTF32 newStr(buf, length);

    auto iter = sf->m_atomicStringMap.find(&newStr);
    if (sf->m_atomicStringMap.end() == iter) {
        String* string = new StringDataUTF32(buf, length);
        sf->m_atomicStringMap.insert(string);
        return AtomicString(string);
    } else {
        return AtomicString(iter.operator*());
    }
}
}
