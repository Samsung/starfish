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
#include "AtomicString.h"
#include "StarFish.h"

namespace StarFish {

AtomicString AtomicString::createAtomicString(StarFish* sf, String* str)
{
    return createAtomicString(sf, str->utf8Data());
}

AtomicString AtomicString::createAtomicString(StarFish* sf, const char* str)
{
    std::string data = str;
    std::unordered_map<std::string, AtomicString,
        std::hash<std::string>, std::equal_to<std::string>, gc_allocator<std::pair<std::string, AtomicString> > >& map = sf->m_atomicStringMap;
    auto iter = map.find(data);

    if (iter != map.end()) {
        return iter->second;
    }

    String* s = String::fromUTF8(data.c_str());
    AtomicString name(s);
    map.insert(std::make_pair(data, name));

    return name;
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, String* str)
{
    return createAttrAtomicString(sf, str->utf8Data());
}

AtomicString AtomicString::createAttrAtomicString(StarFish* sf, const char* str)
{
    std::string data = str;
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    std::unordered_map<std::string, AtomicString,
        std::hash<std::string>, std::equal_to<std::string>, gc_allocator<std::pair<std::string, AtomicString> > >& map = sf->m_atomicStringMap;
    auto iter = map.find(data);

    if (iter != map.end()) {
        return iter->second;
    }

    String* s = String::fromUTF8(data.c_str());
    AtomicString name(s);
    map.insert(std::make_pair(data, name));

    return name;
}


}
