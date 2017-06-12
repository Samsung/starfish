/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, String* str)
{
    auto data = str->bufferAccessData();
    if (data.hasASCIIContent) {
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
    } else {
        char32_t* buf = (char32_t*)alloca((data.length + 1) * sizeof(char32_t));
        buf[data.length] = 0;
        for (size_t i = 0; i < data.length; i++) {
            buf[i] = ::tolower(data.utf32Data()[i]);
        }
        StringDataOnStackUTF32 str(buf, data.length);

        auto iter = sf->m_atomicStringMap.find(&str);
        if (sf->m_atomicStringMap.end() == iter) {
            String* string = new StringDataUTF32(
                std::move(TightUTF32String(buf, data.length)));
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
    char* buf = (char*)alloca(length);
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

AtomicString AtomicString::emptyAtomicString()
{
    return AtomicString(String::emptyString);
}

bool AtomicString::isEmptyAtomicString()
{
    return m_string == String::emptyString;
}
}
