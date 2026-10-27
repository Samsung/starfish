/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishWebFont__
#define __StarfishWebFont__

#include "core/style/Style.h"
#include "platform/loader/FontResource.h"

namespace Starfish {

// CSS Fonts 4 unicode-range descriptor: inclusive code point intervals.
class UnicodeRangeList : public GCAtomicVector<std::pair<char32_t, char32_t>> {
public:
    // Parses a <urange># list; returns nullptr when it is invalid.
    static UnicodeRangeList* parse(const char* data, size_t length);

    bool contains(char32_t ch) const
    {
        for (const auto& range : *this) {
            if (range.first <= ch && ch <= range.second) {
                return true;
            }
        }
        return false;
    }

    bool equals(const UnicodeRangeList* other) const
    {
        if (!other || size() != other->size()) {
            return false;
        }
        for (size_t i = 0; i < size(); i++) {
            if ((*this)[i] != (*other)[i]) {
                return false;
            }
        }
        return true;
    }
};

class WebFont : public gc {
public:
    WebFont(bool isFontStyleSpecified, bool isFontWeightSpecified,
            String* familyName, FontStyleValue fontStyleValue,
            char fontWeightValue, FontResource* fontResource,
            UnicodeRangeList* unicodeRanges = nullptr)
    {
        m_unicodeRanges = unicodeRanges;
        m_fromLocal = false;
        m_isFontStyleSpecified = isFontStyleSpecified;
        m_isFontWeightSpecified = isFontWeightSpecified;
        m_familyName = familyName;
        m_fontStyleValue = fontStyleValue;
        m_fontWeightValue = fontWeightValue;
        m_fontResource = fontResource;
    }

    WebFont(bool isFontStyleSpecified, bool isFontWeightSpecified,
            String* familyName, FontStyleValue fontStyleValue,
            char fontWeightValue, String* localFontName,
            UnicodeRangeList* unicodeRanges = nullptr)
    {
        m_unicodeRanges = unicodeRanges;
        m_fromLocal = true;
        m_isFontStyleSpecified = isFontStyleSpecified;
        m_isFontWeightSpecified = isFontWeightSpecified;
        m_familyName = familyName;
        m_fontStyleValue = fontStyleValue;
        m_fontWeightValue = fontWeightValue;
        m_localFontName = localFontName;
    }

    bool isFontStyleSpecified()
    {
        return m_isFontStyleSpecified;
    }

    bool isFontWeightSpecified()
    {
        return m_isFontWeightSpecified;
    }

    bool fromLocal()
    {
        return m_fromLocal;
    }

    String* familyName()
    {
        return m_familyName;
    }

    FontStyleValue fontStyleValue()
    {
        return m_fontStyleValue;
    }

    char fontWeightValue()
    {
        return m_fontWeightValue;
    }

    FontResource* fontResource()
    {
        STARFISH_ASSERT(!m_fromLocal);
        return m_fontResource;
    }

    String* localFontName()
    {
        STARFISH_ASSERT(m_fromLocal);
        return m_localFontName;
    }

    // nullptr when the face covers every code point.
    UnicodeRangeList* unicodeRanges()
    {
        return m_unicodeRanges;
    }

    bool operator==(const WebFont& src)
    {
        bool sameRanges = m_unicodeRanges
                              ? m_unicodeRanges->equals(src.m_unicodeRanges)
                              : !src.m_unicodeRanges;
        bool e = sameRanges && m_fromLocal == src.m_fromLocal &&
                 m_isFontStyleSpecified == src.m_isFontStyleSpecified &&
                 m_isFontWeightSpecified == src.m_isFontWeightSpecified &&
                 m_fontStyleValue == src.m_fontStyleValue &&
                 m_fontWeightValue == src.m_fontWeightValue;
        if (e) {
            if (m_familyName->equals(src.m_familyName)) {
                if (m_fromLocal) {
                    return m_localFontName->equals(src.m_localFontName);
                } else {
                    return m_fontResource == src.m_fontResource;
                }
            }
        }
        return false;
    }

private:
    bool m_fromLocal;
    bool m_isFontStyleSpecified;
    bool m_isFontWeightSpecified;
    FontStyleValue m_fontStyleValue;
    char m_fontWeightValue;

    String* m_familyName;
    UnicodeRangeList* m_unicodeRanges;
    union {
        FontResource* m_fontResource;
        String* m_localFontName;
    };
};

} /* namespace Starfish */

#endif /* __StarfishWebFont__ */
