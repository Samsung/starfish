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

#ifndef __StarfishAtomicString__
#define __StarfishAtomicString__

namespace Starfish {

class Starfish;
class String;

typedef std::unordered_set<String*, std::hash<String*>, std::equal_to<String*>,
                           GCUtil::gc_malloc_allocator<String*>>
    AtomicStringMap;

class AtomicString {
    friend class Starfish;
    template <unsigned int>
    friend class GatherableString;
    friend class QualifiedName;

    explicit AtomicString(String* str)
    {
        m_string = str;
    }

public:
    AtomicString();

    static AtomicString createAtomicString(Starfish* starfish, String* str);
    static AtomicString createAtomicString(Starfish* starfish, StringView str);
    static AtomicString createAtomicString(Starfish* starfish, const char* str);
    static AtomicString createAtomicString(Starfish* starfish, const char* str,
                                           size_t length);
    // only support bmp chars
    static AtomicString createAtomicString(Starfish* starfish,
                                           const char16_t* str, size_t length);
    static AtomicString createAtomicString(Starfish* starfish,
                                           const char32_t* str, size_t length);
    static AtomicString createAttrAtomicString(Starfish* starfish, String* str);
    static AtomicString createAttrAtomicString(Starfish* starfish,
                                               const char* str);
    static AtomicString createAttrAtomicString(Starfish* starfish,
                                               const char* str, size_t length);
    // only support bmp chars
    static AtomicString createAttrAtomicString(Starfish* starfish,
                                               const char16_t* str,
                                               size_t length);
    static AtomicString createAttrAtomicString(Starfish* starfish,
                                               const char32_t* str,
                                               size_t length);
    static AtomicString createAttrAtomicString(Starfish* starfish,
                                               char32_t str);
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
struct hash<Starfish::AtomicString> {
    std::size_t operator()(const Starfish::AtomicString& s) const
    {
        return s.string()->hashValue();
    }
};

template <>
struct equal_to<Starfish::AtomicString> {
    bool operator()(const Starfish::AtomicString& s1,
                    const Starfish::AtomicString& s2) const
    {
        return s1 == s2;
    }
};
}
#endif
