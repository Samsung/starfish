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
    return createAtomicString(sf, str->toUTF32String());
}

AtomicString AtomicString::createAtomicString(StarFish* sf, const char* str)
{
    return createAtomicString(sf, new StringDataUTF32(str, strlen(str)));
}

AtomicString AtomicString::createAtomicString(StarFish* sf, UTF32String str)
{
    UTF32String data = str;
    auto& map = sf->m_atomicStringMap;
    auto iter = map.find(str);

    if (iter != map.end()) {
        return iter->second;
    }

    String* s = String::createUTF32String(data);
    AtomicString name(s);
    map.insert(std::make_pair(std::move(data), name));

    return name;
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, String* str)
{
    return createAttrAtomicString(sf, str->toUTF32String());
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, const char* str)
{
    return createAttrAtomicString(sf, new StringDataUTF32(str, strlen(str)));
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, UTF32String str)
{
    UTF32String data = str;
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    auto& map = sf->m_atomicStringMap;
    auto iter = map.find(data);

    if (iter != map.end()) {
        return iter->second;
    }

    String* s = String::createUTF32String(data);
    AtomicString name(s);
    map.insert(std::make_pair(std::move(data), name));

    return name;
}

AtomicString AtomicString::emptyAtomicString()
{
    return AtomicString(String::emptyString);
}
}
