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
    template <unsigned int>
    friend class GatherableString;
    friend class QualifiedName;

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
    // only support bmp chars
    static AtomicString createAtomicString(StarFish* sf, const char16_t* str,
                                           size_t length);
    static AtomicString createAtomicString(StarFish* sf, const char32_t* str,
                                           size_t length);
    static AtomicString createAttrAtomicString(StarFish* sf, String* str);
    static AtomicString createAttrAtomicString(StarFish* sf, const char* str);
    static AtomicString createAttrAtomicString(StarFish* sf, const char* str,
                                               size_t length);
    // only support bmp chars
    static AtomicString createAttrAtomicString(StarFish* sf,
                                               const char16_t* str,
                                               size_t length);
    static AtomicString createAttrAtomicString(StarFish* sf,
                                               const char32_t* str,
                                               size_t length);
    static AtomicString createAttrAtomicString(StarFish* sf, char32_t str);
    static AtomicString emptyAtomicString()
    {
        return AtomicString(String::emptyString);
    }

    bool isEmptyAtomicString() const
    {
        return m_string == String::emptyString;
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

inline bool operator==(const AtomicString& a, const AtomicString& b)
{
    return a.string() == b.string();
}

inline bool operator!=(const AtomicString& a, const AtomicString& b)
{
    return a.string() != b.string();
}
}

namespace std {
template <>
struct hash<StarFish::AtomicString> {
    std::size_t operator()(const StarFish::AtomicString& s) const
    {
        return s.string()->hashValue();
    }
};

template <>
struct equal_to<StarFish::AtomicString> {
    bool operator()(const StarFish::AtomicString& s1,
                    const StarFish::AtomicString& s2) const
    {
        return s1 == s2;
    }
};
}
#endif
