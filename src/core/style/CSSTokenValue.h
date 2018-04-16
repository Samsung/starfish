/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCSSTokenValue__
#define __StarFishCSSTokenValue__

namespace StarFish {

class CSSTokenValue : public std::string {
public:
    CSSTokenValue()
        : std::string()
    {
    }

    CSSTokenValue(const char* str)
        : std::string(str)
    {
    }

    CSSTokenValue(const char* str, size_t len)
        : std::string(str, len)
    {
    }

    CSSTokenValue(std::string&& str)
        : std::string(std::move(str))
    {
    }

    bool startsWith(const char* str) const
    {
        if (std::string::find(str) == 0) {
            return true;
        }
        return false;
    }

    size_t indexOf(char ch) const
    {
        return std::string::find(ch);
    }

    bool equals(const char* str) const
    {
        size_t srcLen = strlen(str);
        if (length() != srcLen) {
            return false;
        }
        for (size_t i = 0; i < length(); i++) {
            char c = str[i];
            if (c != std::string::operator[](i)) {
                return false;
            }
        }
        return true;
    }

    CSSTokenValue substring(size_t pos, size_t len) const
    {
        return std::string::substr(pos, len);
    }

    void split(const char delim, std::vector<CSSTokenValue>& tokens) const
    {
        size_t prev_pos = 0, pos = 0;
        while ((pos = find(delim, pos)) != SIZE_MAX) {
            tokens.push_back(
                CSSTokenValue(&std::string::data()[prev_pos], pos - prev_pos));
            prev_pos = ++pos;
        }

        if (pos == SIZE_MAX)
            pos = length();

        tokens.push_back(CSSTokenValue(&std::string::data()[prev_pos],
                                       pos - prev_pos)); // Last word
    }

    char charAt(size_t i) const
    {
        return std::string::operator[](i);
    }

    CSSTokenValue trim() const
    {
        size_t first = 0;
        size_t last = 0;
        if (length()) {
            last = length() - 1;

            for (size_t i = 0; i < length(); i++) {
                if (!String::isSpaceOrNewline(charAt(i))) {
                    first = i;
                    break;
                }
            }

            do {
                if (!String::isSpaceOrNewline(charAt(last))) {
                    break;
                }
            } while (last--);
        } else {
            return *this;
        }

        if (first == 0 && ((last + 1) == length())) {
            return *this;
        }

        return substring(first, (last - first + 1));
    }
};
}

#endif
