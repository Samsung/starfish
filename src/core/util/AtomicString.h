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

#ifndef __StarFishAtomicString__
#define __StarFishAtomicString__

namespace StarFish {

class StarFish;
class String;

typedef std::unordered_set<String*, std::hash<String*>, std::equal_to<String*>,
                           GCUtil::gc_malloc_ignore_off_page_allocator<String*>>
    AtomicStringMap;

class AtomicString {
    friend class StarFish;
    explicit AtomicString(String* str)
    {
        m_string = str;
    }

public:
    AtomicString();

    static AtomicString createAtomicString(StarFish* sf, String* str);
    static AtomicString createAtomicString(StarFish* sf, StringView str);
    static AtomicString createAtomicString(StarFish* sf, const char* str);
    static AtomicString createAtomicString(StarFish* sf, const char* str,
                                           size_t length);
    static AtomicString createAttrAtomicString(StarFish* sf, String* str);
    static AtomicString createAttrAtomicString(StarFish* sf, const char* str);
    static AtomicString createAttrAtomicString(StarFish* sf, const char* str,
                                               size_t length);
    static AtomicString createAttrAtomicString(StarFish* sf, char32_t str);
    static AtomicString emptyAtomicString();

    bool isEmptyAtomicString();

    bool operator==(const AtomicString& src) const
    {
        return m_string == src.m_string;
    }

    String* string() const
    {
        return m_string;
    }

    operator String*() const
    {
        return m_string;
    }

private:
    String* m_string;
};
}
#endif
