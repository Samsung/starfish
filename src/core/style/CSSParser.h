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

#ifndef __StarFishCSSParser__
#define __StarFishCSSParser__

#include "binding/DocumentHoldable.h"
#include "core/style/Style.h"
#include "core/style/MediaQuery.h"
#include "core/style/MediaQuerySet.h"
#include "core/util/RefPtr.h"
#include "core/util/GatherableString.h"

namespace StarFish {

template <typename CharType>
inline bool isDigit(CharType c)
{
    if (c >= '0' && c <= '9') {
        return true;
    }
    return false;
}

template <typename CharType>
inline bool isAlpha(CharType c)
{
    if (c >= 'a' && c <= 'z') {
        return true;
    }
    return false;
}

template <typename CharType>
inline bool isQuote(CharType c)
{
    if (c == '"' || c == '\'') {
        return true;
    }
    return false;
}

const char* unitTypeToString(UnitType type);

class CSSPropertyParser {
public:
    enum ParserOption ENSURE_ENUM_UNSIGNED {
        AllowNegative = 1 << 0,
        AllowPercent = 1 << 1,
        AllowAuto = 1 << 2,
        AllowWithoutUnit = 1 << 3,
        AllowNone = 1 << 4,
        AllowDot = 1 << 5,
        AllowUnderline = 1 << 6,
        AllowPlus = 1 << 7,
        AllowSharp = 1 << 8,
    };

    STARFISH_MAKE_STACK_ALLOCATED();
    CSSPropertyParser(char* value)
        : m_startPos(value)
        , m_endPos(value + strlen(value))
        , m_curPos(value)
    {
        m_parsedNumber = 0;
        m_parsedInt32 = 0;
    }

    CSSPropertyParser(char* value, size_t len)
        : m_startPos(value)
        , m_endPos(value + len)
        , m_curPos(value)
    {
        m_parsedNumber = 0;
        m_parsedInt32 = 0;
    }

    CSSPropertyParser(const CSSTokenValue& src)
        : m_startPos((char*)src.data())
        , m_endPos((char*)src.data() + src.length())
        , m_curPos((char*)src.data())
    {
        m_parsedNumber = 0;
        m_parsedInt32 = 0;
    }

    static bool isLengthUnit(String* str)
    {
        return isLengthUnitImpl(str);
    }

    static bool isLengthUnit(const CSSTokenValue& str)
    {
        return isLengthUnitImpl(&str);
    }

    template <typename T>
    static bool isLengthUnitImpl(T str)
    {
        size_t len = str->length();

        if (len == 2) {
            char32_t c0 = str->operator[](0);
            char32_t c1 = str->operator[](1);

            switch (c0) {
            case 'p':
                if (c1 == 'x') {
                    return true;
                }
                if (c1 == 't') {
                    return true;
                }
                if (c1 == 'c') {
                    return true;
                }
                break;
            case 'e':
                if (c1 == 'm') {
                    return true;
                }
                if (c1 == 'x') {
                    return true;
                }
                break;
            case 'i':
                if (c1 == 'n') {
                    return true;
                }
                break;
            case 'c':
                if (c1 == 'm') {
                    return true;
                }
                if (c1 == 'h') {
                    return true;
                }
                break;
            case 'm':
                if (c1 == 'm') {
                    return true;
                }
                break;
            case 'v':
                if (c1 == 'w') {
                    return true;
                }
                if (c1 == 'h') {
                    return true;
                }
                break;
            default:
                break;
            }
        } else if (len == 3) {
            char32_t c0 = str->operator[](0);
            char32_t c1 = str->operator[](1);
            char32_t c2 = str->operator[](2);

            switch (c0) {
            case 'r':
                if (c1 == 'e' && c2 == 'm') {
                    return true;
                }
                break;
            default:
                break;
            }
        } else if (len == 4) {
            char32_t c0 = str->operator[](0);
            char32_t c1 = str->operator[](1);
            char32_t c2 = str->operator[](2);
            char32_t c3 = str->operator[](3);

            switch (c0) {
            case 'v':
                if (c1 == 'm' && c2 == 'i' && c3 == 'n') {
                    return true;
                }
                if (c1 == 'm' && c2 == 'a' && c3 == 'x') {
                    return true;
                }
                break;
            default:
                break;
            }
        }
        return false;
    }

    static bool isAngleUnit(String* str)
    {
        return isAngleUnitImpl(str);
    }

    static bool isAngleUnit(const CSSTokenValue& str)
    {
        return isAngleUnitImpl(&str);
    }

    template <typename T>
    static bool isAngleUnitImpl(T str)
    {
        size_t len = str->length();
        if (len == 3) {
            char32_t c0 = str->operator[](0);
            char32_t c1 = str->operator[](1);
            char32_t c2 = str->operator[](2);
            if (c0 == 'd' && c1 == 'e' && c2 == 'g') {
                return true;
            } else if (c0 == 'r' && c1 == 'a' && c2 == 'd') {
                return true;
            }
        } else if (len == 4) {
            char32_t c0 = str->operator[](0);
            char32_t c1 = str->operator[](1);
            char32_t c2 = str->operator[](2);
            char32_t c3 = str->operator[](3);

            if (c0 == 'g' && c1 == 'r' && c2 == 'a' && c3 == 'd') {
                return true;
            } else if (c0 == 't' && c1 == 'u' && c2 == 'r' && c3 == 'n') {
                return true;
            }
        }
        return false;
    }

    static bool isTimeUnit(String* str)
    {
        return isTimeUnitImpl(str);
    }

    static bool isTimeUnit(const CSSTokenValue& str)
    {
        return isTimeUnitImpl(&str);
    }

    template <typename T>
    static bool isTimeUnitImpl(T str)
    {
        size_t len = str->length();
        if (len == 1) {
            char32_t c0 = str->charAt(0);
            if (c0 == 's') {
                return true;
            }
        } else if (len == 2) {
            char32_t c0 = str->charAt(0);
            char32_t c1 = str->charAt(1);

            if (c0 == 'm' && c1 == 's') {
                return true;
            }
        }
        return false;
    }

    static bool stringIsIdent(String* v);

    char* curPos() const
    {
        return m_curPos;
    }

    void swap(char* pos)
    {
        m_curPos = pos;
    }

    bool consumeNumber(bool* hasPoint)
    {
        float res = 0;
        bool sign = true; // +
        char* cur = m_curPos;
        if (*cur == '-') {
            sign = false;
            cur++;
        } else if (*cur == '+') {
            sign = true;
            cur++;
        }

        char* numberStart = cur;

        while (isDigit(*cur) && cur < m_endPos) {
            res = res * 10 + (*cur - '0');
            cur++;
        }

#define TRY_PARSE_SCIENTIFIC_NOTATION()                                        \
    if ((*cur == 'e' || *cur == 'E') && (cur + 1 < m_endPos) &&                \
        (isDigit(*(cur + 1)) || *(cur + 1) == '-' || *(cur + 1) == '+')) {     \
        char* numberEnd = cur + 1;                                             \
        while (                                                                \
            (isDigit(*numberEnd) || *numberEnd == '-' || *numberEnd == '+') && \
            numberEnd < m_endPos) {                                            \
            numberEnd++;                                                       \
        }                                                                      \
        size_t l = (size_t)numberEnd - (size_t)numberStart;                    \
        char* buf = (char*)alloca(sizeof(char) * l + 1);                       \
        memcpy(buf, numberStart, l);                                           \
        buf[l] = 0;                                                            \
        double result;                                                         \
        m_curPos = numberEnd;                                                  \
        bool r = sscanf(buf, "%lf", &result) == 1;                             \
        m_parsedNumber = result;                                               \
        if (!sign) {                                                           \
            m_parsedNumber *= (-1);                                            \
        }                                                                      \
        return r;                                                              \
    }

        TRY_PARSE_SCIENTIFIC_NOTATION();

        // number can just start with '.' without '0'
        if (cur == m_curPos && *cur != '.') {
            return false;
        }

        if (*cur == '.' && cur < m_endPos) {
            *hasPoint = true;
            cur++;
            int pt = 10;
            while (isDigit(*cur) && cur < m_endPos) {
                res += (float)(*cur - '0') / pt;
                pt *= 10;
                cur++;
            }
            TRY_PARSE_SCIENTIFIC_NOTATION();
        }

#undef TRY_PARSE_SCIENTIFIC_NOTATION

        m_curPos = cur;
        if (!sign) {
            res *= (-1);
        }
        m_parsedNumber = res;
        return true;
    }

    bool consumeNumber()
    {
        bool t;
        return consumeNumber(&t);
    }

    float parsedNumber()
    {
        return m_parsedNumber;
    }

    bool consumeInt32()
    {
        int32_t res = 0;
        bool sign = true; // +
        char* cur = m_curPos;
        if (*cur == '-') {
            sign = false;
            cur++;
        } else if (*cur == '+') {
            sign = true;
            cur++;
        }

        while (isDigit(*cur) && cur < m_endPos) {
            res = res * 10 + (*cur - '0');
            cur++;
        }

        if (cur != m_endPos) {
            return false;
        }

        m_curPos = cur;
        if (!sign) {
            res *= (-1);
        }
        m_parsedInt32 = res;
        return true;
    }

    int32_t parsedInt32()
    {
        return m_parsedInt32;
    }

    // a-z | 0-9 | - | _ | %
    bool consumeString(uint32_t option)
    {
        bool allowNegative = option & AllowNegative;
        bool allowPercent = option & AllowPercent;
        bool allowUnderline = option & AllowUnderline;
        bool allowDot = option & AllowDot;
        bool allowPlus = option & AllowPlus;
        bool allowSharp = option & AllowSharp;

        int len = 0;
        for (char *cur = m_curPos; cur < m_endPos; cur++, len++) {
            if (isAlpha(*cur) || isDigit(*cur)) {
                continue;
            } else if (allowPercent && *cur == '%') {
                continue;
            } else if (allowPlus && *cur == '+') {
                continue;
            } else if (allowNegative && *cur == '-') {
                continue;
            } else if (allowUnderline && *cur == '_') {
                continue;
            } else if (allowDot && *cur == '.') {
                continue;
            } else if (allowSharp && *cur == '#') {
                continue;
            } else {
                break;
            }
        }
        m_parsedString = CSSTokenValue(m_curPos, len);
        m_curPos += len;
        return (len > 0) ? true : false;
    }

    bool consumeIfNext(char c)
    {
        if (*m_curPos == c) {
            m_curPos++;
            return true;
        }
        return false;
    }

    bool consumeWhitespaces()
    {
        while (String::isSpaceOrNewline(*m_curPos) && m_curPos < m_endPos) {
            m_curPos++;
        }
        return true;
    }

    bool consumeParenthesisBlock()
    {
        char* backup = m_curPos;
        if (!consumeIfNext('(')) {
            m_curPos = backup;
            return false;
        }
        int len = 0;
        char* start = m_curPos;
        while (*m_curPos != ')' && m_curPos < m_endPos) {
            m_curPos++;
            len++;
        }
        if (*m_curPos != ')') {
            m_curPos = backup;
            return false;
        }
        m_parsedString = CSSTokenValue(start, len);
        m_curPos++;
        return true;
    }

    bool consumeQuoteBlock()
    {
        char* backup = m_curPos;
        consumeIfNext('\\');
        char mark = '\0';
        if (*m_curPos == '"' || *m_curPos == '\'') {
            mark = *m_curPos;
            m_curPos++;
        }
        int len = 0;
        char* start = m_curPos;
        while (*m_curPos != mark && m_curPos < m_endPos) {
            m_curPos++;
            len++;
        }
        if (m_curPos == m_endPos) {
            m_curPos = backup;
            return false;
        }
        if (*(m_curPos - 1) == '\\') {
            len--;
        }
        m_parsedString = CSSTokenValue(start, len);
        m_curPos++;
        return true;
    }

    bool consumeFunctionContent()
    {
        consumeWhitespaces();
        int len = 0;
        char mark = '\0';
        consumeIfNext('\\');
        if (*m_curPos == '"' || *m_curPos == '\'') {
            mark = *m_curPos;
            m_curPos++;
        }
        char* start = m_curPos;
        while (*m_curPos != ')' && m_curPos < m_endPos) {
            m_curPos++;
            len++;
            if (mark != '\0' && mark == *m_curPos) {
                if (*(m_curPos - 1) == '\\') {
                    len--;
                }
                m_curPos++;
                consumeWhitespaces();
                break;
            }
        }
        if (*m_curPos != ')') {
            return false;
        }
        m_parsedFunctionContent = CSSTokenValue(start, len);
        m_curPos++;
        return true;
    }

    bool consumeContentStringAll()
    {
        if (isQuote(*m_curPos) && *m_curPos++ == (*--m_endPos)) {
            m_parsedString = CSSTokenValue(m_curPos, m_endPos - m_curPos);
            return true;
        } else {
            return false;
        }
    }

    bool consumeContentString()
    {
        char* backup = m_curPos;
        consumeWhitespaces();
        char openingQuote = *m_curPos;
        if (!consumeIfNext('"') && !consumeIfNext('\'')) {
            m_curPos = backup;
            return false;
        }
        char* start = m_curPos;
        while (m_curPos < m_endPos) {
            if (*m_curPos == openingQuote) {
                m_parsedString = CSSTokenValue(start, m_curPos - start);
                m_curPos++;
                return true;
            }
            m_curPos++;
        }
        m_curPos = backup;
        return false;
    }

    bool consumeAttr()
    {
        consumeWhitespaces();
        size_t len = 0;
        char* start = m_curPos;
        while (*m_curPos != ')' && m_curPos < m_endPos) {
            if (String::isASCIISpace(*m_curPos)) {
                consumeWhitespaces();
                if (*m_curPos != ')') {
                    return false;
                }
            } else {
                m_curPos++;
                len++;
            }
        }
        if (*m_curPos != ')') {
            return false;
        }
        m_parsedString = CSSTokenValue(start, len);
        m_curPos++;
        return true;
    }

    bool consumeParenthesis()
    {
        consumeWhitespaces();
        if (*m_curPos != '(') {
            return false;
        }

        size_t len = 0;
        char* start = m_curPos;
        for (char *cur = m_curPos; cur < m_endPos && *cur != ')';
             cur++, len++) {
        }

        if (*(m_curPos + len) != ')') {
            return false;
        }
        m_parsedString = CSSTokenValue(start, len + 1);
        m_curPos += len + 1;
        return true;
    }

    bool consumeLayerBlock()
    {
        consumeWhitespaces();
        char* start = m_curPos;
        size_t openParenthesis = 0;
        while (m_curPos < m_endPos) {
            if (*m_curPos == '(') {
                openParenthesis++;
            } else if (*m_curPos == ')' && openParenthesis) {
                openParenthesis--;
            } else if (*m_curPos == ',') {
                if (m_curPos <= start) {
                    return false;
                }
                if (!openParenthesis) {
                    m_parsedString = CSSTokenValue(start, m_curPos - start);
                    m_curPos++;
                    return true;
                }
            }
            m_curPos++;
        }
        if (m_curPos > start) {
            m_parsedString = CSSTokenValue(start, m_curPos - start);
            return true;
        }
        return false;
    }

    const CSSTokenValue& parsedString()
    {
        return m_parsedString;
    }

    String* parsedStringToGCString()
    {
        return String::fromUTF8(m_parsedString.data(), m_parsedString.size());
    }

    const CSSTokenValue& parsedFunctionContent()
    {
        return m_parsedFunctionContent;
    }

    String* parsedFunctionContentToGCString()
    {
        return String::fromUTF8(m_parsedFunctionContent.data(),
                                m_parsedFunctionContent.size());
    }

    bool isEnd()
    {
        return (m_curPos == m_endPos);
    }

    static bool parseUrl(const char* token, CSSStyleValuePair* pair)
    {
        CSSPropertyParser parser((char*)token);
        if (parser.consumeString(0)) {
            const CSSTokenValue& name = parser.parsedString();
            if (name == "url" && parser.consumeIfNext('(')) {
                if (parser.consumeFunctionContent() && parser.isEnd()) {
                    pair->setUrlValue(parser.parsedFunctionContentToGCString());
                    return true;
                }
            }
        }
        return false;
    }

    static Nullable<CSSTokenValue> parseFunctionBlock(const char* token,
                                                      const char* functionName)
    {
        CSSPropertyParser parser((char*)token);
        parser.consumeWhitespaces();
        uint32_t options = 0;
        options |= AllowNegative;
        options |= AllowUnderline;
        if (parser.consumeString(options)) {
            const CSSTokenValue& name = parser.parsedString();
            parser.consumeWhitespaces();
            if (name == functionName && parser.consumeParenthesisBlock()) {
                parser.consumeWhitespaces();
                if (parser.isEnd()) {
                    return parser.parsedString();
                }
            }
        }
        return Nullable<CSSTokenValue>();
    }

    static Nullable<CSSTokenValue> parseQuoteBlock(const char* token)
    {
        CSSPropertyParser parser((char*)token);
        parser.consumeWhitespaces();
        if (parser.consumeQuoteBlock()) {
            parser.consumeWhitespaces();
            if (parser.isEnd()) {
                return parser.parsedString();
            }
        }
        return Nullable<CSSTokenValue>();
    }

    static bool parseLayers(const char* token, size_t length,
                            CSSTokenVector& layers)
    {
        CSSPropertyParser parser((char*)token, length);
        while (!parser.isEnd()) {
            if (!parser.consumeLayerBlock()) {
                return false;
            }
            CSSTokenValue layer(parser.parsedString().trim());
            if (!layer.length()) {
                return false;
            }
            layers.push_back(layer);
        }
        return !(length && token[length - 1] == ',') && layers.size();
    }

    static bool parseNumber(const char* token, uint32_t option, float* val)
    {
        bool allowNegative = option & AllowNegative;
        CSSPropertyParser parser((char*)token);
        if (parser.consumeNumber()) {
            float t = parser.parsedNumber();
            if (!allowNegative && t < 0) {
                return false;
            }
            *val = t;
            return parser.isEnd();
        }
        return false;
    }

    static bool parseInt32(const char* token, uint32_t option, int32_t& result)
    {
        bool allowNegative = option & AllowNegative;
        CSSPropertyParser parser((char*)token);
        if (parser.consumeInt32()) {
            int32_t t = parser.parsedInt32();
            if (!allowNegative && t < 0) {
                return false;
            }
            result = t;
            return parser.isEnd();
        }
        return false;
    }

    static bool parseInt32(const char* token, uint32_t option,
                           CSSStyleValuePair* pair)
    {
        int32_t result;
        if (!parseInt32(token, option, result)) {
            return false;
        }
        pair->setInt32Value(result);
        return true;
    }

    static bool parseLength(const char* token, uint32_t option,
                            CSSStyleValuePair* pair)
    {
        bool allowWithoutUnit = option & AllowWithoutUnit;
        bool allowNegative = option & AllowNegative;
        bool allowPercent = option & AllowPercent;

        CSSPropertyParser parser((char*)token);
        if (!parser.consumeNumber()) {
            return false;
        }
        float num = parser.parsedNumber();
        if (!allowNegative && num < 0) {
            return false;
        }
        parser.consumeString(option & AllowPercent);
        const CSSTokenValue& str = parser.parsedString();
        if (allowWithoutUnit && str.length() == 0) {
            pair->setLengthValue(CSSLength(num));
            return parser.isEnd();
        } else if ((str.length() == 0 && num == 0) || isLengthUnit(str)) {
            pair->setLengthValue(CSSLength(str, num));
            return parser.isEnd();
        } else if (allowPercent && str == "%") {
            pair->setPercentageValue(num / 100.f);
            return parser.isEnd();
        }

        return false;
    }

    static bool parseTime(const char* token, uint32_t option,
                          CSSStyleValuePair* pair)
    {
        bool allowWithoutUnit = option & AllowWithoutUnit;
        bool allowNegative = option & AllowNegative;

        CSSPropertyParser parser((char*)token);
        if (!parser.consumeNumber()) {
            return false;
        }
        float num = parser.parsedNumber();
        if (!allowNegative && num < 0) {
            return false;
        }
        parser.consumeString(0);
        const CSSTokenValue& str = parser.parsedString();

        if (allowWithoutUnit && str.length() == 0) {
            pair->setTimeValue(CSSTime(num));
            return parser.isEnd();
        } else if ((str.length() == 0 && num == 0) || isTimeUnit(str)) {
            pair->setTimeValue(CSSTime(str, num));
            return parser.isEnd();
        }

        return false;
    }

    static bool parseAngle(const char* token, uint32_t option,
                           CSSStyleValuePair* pair)
    {
        bool allowWithoutUnit = option & AllowWithoutUnit;
        bool allowNegative = option & AllowNegative;

        CSSPropertyParser parser((char*)token);
        if (!parser.consumeNumber()) {
            return false;
        }
        float num = parser.parsedNumber();
        if (!allowNegative && num < 0) {
            return false;
        }
        parser.consumeString(0);
        const CSSTokenValue& str = parser.parsedString();

        if (allowWithoutUnit && str.length() == 0) {
            pair->setAngleValue(CSSAngle(num));
            return parser.isEnd();
        } else if ((str.length() == 0 && num == 0) || isAngleUnit(str)) {
            pair->setAngleValue(CSSAngle(str, num));
            return parser.isEnd();
        }

        return false;
    }

    static bool parseColorFunctionPart(const CSSTokenValue& s, bool isAlpha,
                                       unsigned char* ret, bool* isPercent)
    {
        auto ss = s.trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());

        bool hasPoint = false;
        parser.consumeWhitespaces();
        if (!parser.consumeNumber(&hasPoint)) {
            return false;
        }

        float number = parser.parsedNumber();
        bool percent = false;
        if (parser.consumeIfNext('%')) {
            percent = true;
        }

        // NOTE: decimal-point is disallowed for rgb value.
        if (!isAlpha && !percent && hasPoint) {
            return false;
        }

        parser.consumeWhitespaces();
        if (!parser.isEnd()) {
            return false;
        }

        number = number < 0 ? 0 : number;
        if (percent) {
            if (number > 100) {
                number = 100;
            }
            number = 255 * number / 100;
        } else if (isAlpha) {
            if (number > 1) {
                number = 1;
            }
            number = number * 255;
        } else if (number > 255) {
            number = 255;
        }
        *ret = number;
        *isPercent = percent;
        return true;
    }

    static bool parsePercent(const CSSTokenValue& s, double* ret)
    {
        auto ss = s.trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());

        bool hasPoint = false;
        parser.consumeWhitespaces();
        if (!parser.consumeNumber(&hasPoint)) {
            return false;
        }

        float number = parser.parsedNumber();
        bool percent = false;
        if (parser.consumeIfNext('%')) {
            percent = true;
        }
        if (!percent) {
            return false;
        }

        parser.consumeWhitespaces();
        if (!parser.isEnd()) {
            return false;
        }

        number = number < 0 ? 0 : number;
        number = number > 100 ? 100 : number;

        *ret = number;
        return true;
    }

    static bool parseNonNamedColor(const CSSTokenValue& str,
                                   CSSStyleValuePair* pair)
    {
        bool maybeRGBA = str.startsWith("rgba(");
        bool maybeRGB = str.startsWith("rgb(");
        bool maybeCode = str.startsWith("#");
        bool maybeHSL = str.startsWith("hsl(");
        bool maybeHSLA = str.startsWith("hsla(");

        if (maybeRGBA || maybeRGB || maybeHSL || maybeHSLA) {
            size_t s1 = str.indexOf('(');
            size_t s2 = str.indexOf(')');
            if (s1 == SIZE_MAX || s2 != str.length() - 1 || s1 >= s2) {
                return false;
            }

            CSSTokenValue sub = str.substring(s1 + 1, s2 - s1 - 1).trim();
            size_t commaPos = sub.indexOf(',');
            size_t slashPos = sub.indexOf('/');

            std::vector<CSSTokenValue> v;
            if ((0 < commaPos) && (commaPos < sub.size())) {
                // functional syntax
                sub.split(',', v);
            } else if ((0 < slashPos) && (slashPos < sub.size())) {
                // whitespace syntax
                std::vector<CSSTokenValue> s;
                std::vector<CSSTokenValue> rgbVals;
                sub.split('/', s);
                s[0].trim().split(' ', rgbVals);
                for (size_t i = 0; i < rgbVals.size(); i++) {
                    v.push_back(rgbVals[i]);
                }
                CSSTokenValue alpha = s[1].trim();
                if (alpha.size() > 0) {
                    v.push_back(alpha);
                }
            } else {
                return false;
            }

            size_t size = v.size();
            if (!((maybeRGBA && (size == 4)) ||
                  (maybeRGB && (size == 3 || size == 4)) ||
                  (maybeHSL && (size == 3 || size == 4)) ||
                  (maybeHSLA && (size == 4)))) {
                return false;
            }

            bool hasAlpha = (size == 4);
            if (maybeRGB || maybeRGBA) {
                unsigned char parsed[4];
                bool isPercent = false, shouldPercent = false;

                // parse rgb
                for (size_t i = 0; i < 3; i++) {
                    if (!parseColorFunctionPart(v[i], false, &parsed[i],
                                                &isPercent)) {
                        return false;
                    }
                    if (i == 0) {
                        shouldPercent = isPercent;
                    } else if (shouldPercent != isPercent) {
                        return false;
                    }
                }

                // parse alpha if exists
                if (hasAlpha &&
                    !parseColorFunctionPart(v[3], true, &parsed[3],
                                            &isPercent)) {
                    return false;
                }

                pair->setColorValue(Unit::Color(parsed[0], parsed[1], parsed[2],
                                                hasAlpha ? parsed[3] : 255));
            } else { // HSL or HSLA
                // parse hue in angle. <number> | <angle>
                double hue = 0;
                CSSStyleValuePair p;
                p.setValueKind(CSSStyleValuePair::ValueKind::Angle);
                uint32_t option = 0;
                option |= CSSPropertyParser::AllowNegative;
                option |= CSSPropertyParser::AllowWithoutUnit;
                if (!parseAngle(v[0].data(), option, &p)) {
                    return false;
                }
                hue = p.angleValue().toDegreeValue();
                hue = (((((int)round(hue)) % 360) + 360) % 360);

                // parse saturation and luminance in percentage
                double saturation = 0, luminance = 0;
                if (!parsePercent(v[1], &saturation)) {
                    return false;
                }
                if (!parsePercent(v[2], &luminance)) {
                    return false;
                }

                // parse alpha if exists
                unsigned char alpha = 0;
                bool _isPercent;
                if (hasAlpha &&
                    !parseColorFunctionPart(v[3], true, &alpha, &_isPercent)) {
                    return false;
                }

                unsigned char r, g, b;
                // floats are rounded to int if given
                pair->setColorValue(Unit::Color::fromHsla(
                    hue / 360, round(saturation) / 100, round(luminance) / 100,
                    hasAlpha ? alpha : 255));
            }
        } else if (maybeCode) {
            const char* s = str.data();
            const unsigned len = str.length();

            if (!(len == 9 || len == 7 || len == 5 || len == 4)) {
                return false;
            }
            for (unsigned i = 1; i < len; i++) {
                if (!(s[i] >= '0' && s[i] <= '9') &&
                    !(s[i] >= 'A' && s[i] <= 'F') &&
                    !(s[i] >= 'a' && s[i] <= 'f')) {
                    return false;
                }
            }

            if (len == 9) {
                unsigned int r, g, b, a;
                sscanf(s, "#%02x%02x%02x%02x", &r, &g, &b, &a);
                pair->setColorValue(Unit::Color(r, g, b, a));
            } else if (len == 7) {
                unsigned int r, g, b;
                sscanf(s, "#%02x%02x%02x", &r, &g, &b);
                pair->setColorValue(Unit::Color(r, g, b, 255));
            } else if (len == 5) {
                unsigned int r, g, b, a;
                sscanf(s, "#%01x%01x%01x%01x", &r, &g, &b, &a);
                pair->setColorValue(
                    Unit::Color(r * 17, g * 17, b * 17, a * 17));
            } else if (len == 4) {
                unsigned int r, g, b;
                sscanf(s, "#%01x%01x%01x", &r, &g, &b);
                pair->setColorValue(Unit::Color(r * 17, g * 17, b * 17, 255));
            }
        } else if (str.equals("transparent")) {
            pair->setColorValue(Unit::Color(0, 0, 0, 0));
        } else {
            return false;
        }
        return true;
    }

    static bool parseNamedColor(const CSSTokenValue& str,
                                CSSStyleValuePair* pair)
    {
        if (str.equals("currentcolor")) {
            pair->setNamedColorValue(NamedColor::NamedColorValue::currentColor);
            return true;
        }
        NamedColor::NamedColorValue ret;
        if (NamedColor::parseNamedColor(str.data(), str.length(), ret)) {
            pair->setNamedColorValue(ret);
            return true;
        }

        return false;
    }

    static bool parseContentString(const char* str, size_t len, String** ret)
    {
        CSSPropertyParser parser((char*)str, len);
        if (parser.consumeContentStringAll()) {
            *ret = parser.parsedStringToGCString();
            return true;
        }
        return false;
    }

    static bool parseAttr(const char* str, size_t len, String** ret)
    {
        CSSPropertyParser parser((char*)str, len);
        if (parser.consumeString(0)) {
            const CSSTokenValue& name = parser.parsedString();
            if (name == "attr" && parser.consumeIfNext('(')) {
                if (parser.consumeAttr() && parser.isEnd()) {
                    *ret = parser.parsedStringToGCString();
                    return true;
                }
            }
        }
        return false;
    }

    static bool parseCustomIdent(const char* str, size_t len, String** ret)
    {
        String* ident = String::fromUTF8(str, len);
        if (stringIsIdent(ident)) {
            *ret = ident;
            return true;
        }
        return false;
    }

    char* m_startPos;
    char* m_endPos;
    char* m_curPos;

    float m_parsedNumber;
    int32_t m_parsedInt32;
    CSSTokenValue m_parsedString;
    CSSTokenValue m_parsedFunctionContent;
};

class CSSParser;

#ifndef CSSTOKENSTRING_BUILTIN_BUFFER_SIZE
#define CSSTOKENSTRING_BUILTIN_BUFFER_SIZE 24
#endif

typedef GatherableString<CSSTOKENSTRING_BUILTIN_BUFFER_SIZE> CSSTokenString;

class CSSToken : public RefCounted<CSSToken>, public gc {
    friend class CSSParser;

    CSSToken(CSSParser* parser, char type)
    {
        m_parser = parser;
        m_type = type;
        m_unitType = UnitType::UnknownType;
        m_hasCharValue = false;
        m_hasNumberValue = false;
        m_hasStringValue = false;
        m_hasAlphabetNInUnit = false;
        m_hasSourceOfNumberValueDot = false;
    }

    CSSToken(CSSParser* parser, char type, CSSTokenString&& value)
        : m_stringValue(std::move(value))
    {
        m_parser = parser;
        m_type = type;
        m_unitType = UnitType::UnknownType;
        m_hasCharValue = false;
        m_hasNumberValue = false;
        m_hasStringValue = true;
        m_hasAlphabetNInUnit = false;
        m_hasSourceOfNumberValueDot = false;
    }

    CSSToken(CSSParser* parser, char type, char32_t value)
    {
        m_parser = parser;
        m_type = type;
        m_charValue = value;
        m_unitType = UnitType::UnknownType;
        m_stringValue.appendChar(value);
        m_hasStringValue = true;
        m_hasCharValue = true;
        m_hasNumberValue = false;
        m_hasAlphabetNInUnit = false;
        m_hasSourceOfNumberValueDot = false;
    }

    CSSToken(CSSParser* parser, char type, float number, CSSTokenString&& value,
             UnitType u, bool hasSourceOfNumberValueDot,
             bool hasAlphabetNInUnit)
        : m_stringValue(std::move(value))
    {
        m_parser = parser;
        m_type = type;
        m_numericValue = number;
        m_unitType = u;
        m_hasStringValue = true;
        m_hasCharValue = false;
        m_hasNumberValue = true;
        m_hasAlphabetNInUnit = hasAlphabetNInUnit;
        m_hasSourceOfNumberValueDot = hasSourceOfNumberValueDot;
    }

public:
    void* operator new(size_t size, CSSParser* parser);
    inline void operator delete(void* obj)
    {
    }
    inline void operator delete(void*, void*)
    {
    }
    ~CSSToken();

    static RefPtr<CSSToken> createNullToken(CSSParser* parser)
    {
        return adoptRef(new (parser) CSSToken(parser, CSSToken::NULL_TYPE));
    }

    static RefPtr<CSSToken> createToken(CSSParser* parser, char type)
    {
        return adoptRef(new (parser) CSSToken(parser, type));
    }

    static RefPtr<CSSToken> createStringValueToken(CSSParser* parser, char type,
                                                   CSSTokenString&& value)
    {
        return adoptRef(new (parser) CSSToken(parser, type, std::move(value)));
    }

    static RefPtr<CSSToken> createCharValueToken(CSSParser* parser, char type,
                                                 char32_t ch)
    {
        return adoptRef(new (parser) CSSToken(parser, type, ch));
    }

    static RefPtr<CSSToken> createNumberValueToken(
        CSSParser* parser, char type, float number, CSSTokenString&& source,
        UnitType u, bool hasSourceOfNumberValueDot, bool hasAlphabetNInUnit)
    {
        return adoptRef(new (parser) CSSToken(
            parser, type, number, std::move(source), u,
            hasSourceOfNumberValueDot, hasAlphabetNInUnit));
    }

    bool isNotNull()
    {
        return m_type;
    }

    bool isOfType(char aType, char32_t aValue)
    {
        return (m_type == aType && (!aValue || charValue() == aValue));
    }

    bool isOfType(char aType, const char* aValue)
    {
        return (m_type == aType &&
                (!aValue || value()->equalsIgnoreCase(aValue)));
    }

    bool isOfType(char aType)
    {
        return m_type == aType;
    }

    bool isNull()
    {
        return isOfType(CSSToken::NULL_TYPE);
    }

    bool isWhiteSpace(char32_t w = 0)
    {
        return isOfType(CSSToken::WHITESPACE_TYPE, w);
    }

    bool isString()
    {
        return isOfType(CSSToken::STRING_TYPE);
    }

    bool isComment()
    {
        return isOfType(CSSToken::COMMENT_TYPE);
    }

    bool isSGMLComment()
    {
        return isOfType(CSSToken::SGML_COMMENT_TYPE);
    }

    bool isNumber()
    {
        return isOfType(CSSToken::NUMBER_TYPE);
    }

    bool hasStringValue()
    {
        return m_hasStringValue;
    }

    bool hasSourceOfNumberValueDot()
    {
        return m_hasSourceOfNumberValueDot;
    }

    bool hasAlphabetNInUnit()
    {
        return m_hasAlphabetNInUnit;
    }

    bool isIdent()
    {
        return isOfType(CSSToken::IDENT_TYPE);
    }

    bool isIdent(char c)
    {
        char s[2] = { c, '\0' };
        return isOfType(CSSToken::IDENT_TYPE) && value()->equals(s);
    }

    bool isIdent(const char* s)
    {
        return isOfType(CSSToken::IDENT_TYPE) && value()->equalsIgnoreCase(s);
    }

    bool isFunction(const char* f = nullptr)
    {
        return isOfType(CSSToken::FUNCTION_TYPE, f);
    }

    bool isAtRule(const char* f = nullptr)
    {
        return isOfType(CSSToken::ATRULE_TYPE, f);
    }

    bool isIncludes()
    {
        return isOfType(CSSToken::INCLUDES_TYPE);
    }

    bool isDashmatch()
    {
        return isOfType(CSSToken::DASHMATCH_TYPE);
    }

    bool isBeginsmatch()
    {
        return isOfType(CSSToken::BEGINSMATCH_TYPE);
    }

    bool isEndsmatch()
    {
        return isOfType(CSSToken::ENDSMATCH_TYPE);
    }

    bool isContainsmatch()
    {
        return isOfType(CSSToken::CONTAINSMATCH_TYPE);
    }

    bool isSymbol(char32_t c = 0)
    {
        return isOfType(CSSToken::SYMBOL_TYPE, c);
    }

    bool isDimension()
    {
        return isOfType(CSSToken::DIMENSION_TYPE);
    }

    bool isPercentage()
    {
        return isOfType(CSSToken::PERCENTAGE_TYPE);
    }

    bool isHex()
    {
        return isOfType(CSSToken::HEX_TYPE);
    }

    bool isDimensionOfUnit(UnitType aUnit)
    {
        return (isDimension() && m_unitType == aUnit);
    }

    bool isLength()
    {
        STARFISH_ASSERT(m_hasNumberValue);
        switch (m_unitType) {
        case UnitType::Centimeters:
        case UnitType::Millimeters:
        case UnitType::Inches:
        case UnitType::Picas:
        case UnitType::Pixels:
        case UnitType::Ems:
        case UnitType::Exs:
        case UnitType::Points:
        case UnitType::Rems:
            return true;
        default:
            return false;
        }
    }

    bool isAngle()
    {
        STARFISH_ASSERT(m_hasNumberValue);
        switch (m_unitType) {
        case UnitType::Degrees:
        case UnitType::Radians:
        case UnitType::Gradians:
            return true;
        default:
            return false;
        }
    }

    char type()
    {
        return m_type;
    }

    CSSTokenString* value()
    {
        return &m_stringValue;
    }

    float numericValue()
    {
        STARFISH_ASSERT(m_hasNumberValue);
        return m_numericValue;
    }

    char32_t charValue()
    {
        STARFISH_ASSERT(m_hasCharValue);
        return m_charValue;
    }

    UnitType unitType()
    {
        STARFISH_ASSERT(m_hasNumberValue);
        return m_unitType;
    }

    static const char NULL_TYPE = 0;
    static const char WHITESPACE_TYPE = 1;
    static const char STRING_TYPE = 2;
    static const char COMMENT_TYPE = 3;
    static const char NUMBER_TYPE = 4;
    static const char IDENT_TYPE = 5;
    static const char FUNCTION_TYPE = 6;
    static const char ATRULE_TYPE = 7;
    static const char INCLUDES_TYPE = 8;
    static const char DASHMATCH_TYPE = 9;
    static const char BEGINSMATCH_TYPE = 10;
    static const char ENDSMATCH_TYPE = 11;
    static const char CONTAINSMATCH_TYPE = 12;
    static const char SYMBOL_TYPE = 13;
    static const char DIMENSION_TYPE = 14;
    static const char PERCENTAGE_TYPE = 15;
    static const char HEX_TYPE = 16;
    static const char SGML_COMMENT_TYPE = 17;

protected:
    unsigned char m_type : 8;
    UnitType m_unitType : 8;
    bool m_hasNumberValue : 1;
    bool m_hasCharValue : 1;
    bool m_hasStringValue : 1;
    bool m_hasSourceOfNumberValueDot : 1;
    bool m_hasAlphabetNInUnit : 1;

    CSSParser* m_parser;
    CSSTokenString m_stringValue;
    union {
        float m_numericValue;
        char32_t m_charValue;
    };
};

#define ENUM_MEDIA_FEATURES(F)                                               \
    F(AspectRatio, "aspect-ratio", aspectRatio, NoPrefix)                    \
    F(MaxAspectRatio, "max-aspect-ratio", aspectRatio, MaxPrefix)            \
    F(MinAspectRatio, "min-aspect-ratio", aspectRatio, MinPrefix)            \
    F(Height, "height", height, NoPrefix)                                    \
    F(MaxHeight, "max-height", height, MaxPrefix)                            \
    F(MinHeight, "min-height", height, MinPrefix)                            \
    F(Orientation, "orientation", orientation, NoPrefix)                     \
    F(Resolution, "resolution", resolution, NoPrefix)                        \
    F(MaxResolution, "max-resolution", resolution, MaxPrefix)                \
    F(MinResolution, "min-resolution", resolution, MinPrefix)                \
    F(Width, "width", width, NoPrefix)                                       \
    F(MaxWidth, "max-width", width, MaxPrefix)                               \
    F(MinWidth, "min-width", width, MinPrefix)                               \
    F(DeviceAspectRatio, "device-aspect-ratio", deviceAspectRatio, NoPrefix) \
    F(MaxDeviceAspectRatio, "max-device-aspect-ratio", deviceAspectRatio,    \
      MaxPrefix)                                                             \
    F(MinDeviceAspectRatio, "min-device-aspect-ratio", deviceAspectRatio,    \
      MinPrefix)                                                             \
    F(DeviceHeight, "device-height", deviceHeight, NoPrefix)                 \
    F(MaxDeviceHeight, "max-device-height", deviceHeight, MaxPrefix)         \
    F(MinDeviceHeight, "min-device-height", deviceHeight, MinPrefix)         \
    F(DeviceWidth, "device-width", deviceWidth, NoPrefix)                    \
    F(MaxDeviceWidth, "max-device-width", deviceWidth, MaxPrefix)            \
    F(MinDeviceWidth, "min-device-width", deviceWidth, MinPrefix)            \
    F(AnyHover, "any-hover", hover, NoPrefix)                                \
    F(AnyPointer, "any-pointer", pointer, NoPrefix)                          \
    F(Color, "color", color, NoPrefix)                                       \
    F(DisplayMode, "display-mode", displayMode, NoPrefix)                    \
    F(MaxColor, "max-color", color, MaxPrefix)                               \
    F(MinColor, "min-color", color, MinPrefix)                               \
    F(ColorIndex, "color-index", colorIndex, NoPrefix)                       \
    F(MaxColorIndex, "max-color-index", colorIndex, MaxPrefix)               \
    F(MinColorIndex, "min-color-index", colorIndex, MinPrefix)               \
    F(Grid, "grid", grid, NoPrefix)                                          \
    F(Hover, "hover", hover, NoPrefix)                                       \
    F(Monochrome, "monochrome", monochrome, NoPrefix)                        \
    F(MaxMonochrome, "max-monochrome", monochrome, MaxPrefix)                \
    F(MinMonochrome, "min-monochrome", monochrome, MinPrefix)                \
    F(OverflowBlock, "overflow-block", overflowBlock, NoPrefix)              \
    F(OverflowInline, "overflow-inline", overflowInline, NoPrefix)           \
    F(Pointer, "pointer", pointer, NoPrefix)                                 \
    F(Scan, "scan", scan, NoPrefix)                                          \
    F(Scripting, "scripting", scripting, NoPrefix)                           \
    F(Update, "update", update, NoPrefix)

enum MediaFeature {
    MediaFeatureNone,
#define ADD_MEDIA_FEATURE(name, ...) MediaFeature##name,
    ENUM_MEDIA_FEATURES(ADD_MEDIA_FEATURE)
#undef ADD_MEDIA_FEATURE
        MediaFeatureViewportDependentStart = MediaFeatureAspectRatio,
    MediaFeatureViewportDependentEnd = MediaFeatureMinWidth,
    MediaFeatureDeviceDependentStart = MediaFeatureDeviceAspectRatio,
    MediaFeatureDeviceDependentEnd = MediaFeatureMinDeviceWidth
};

class CSSScanner;
class MediaQueryExp;

class MediaQueryData {
private:
    MediaQuery::RestrictorType m_restrictor;
    String* m_mediaType;
    GCVector<MediaQueryExp*> m_expressions;
    MediaFeature m_mediaFeature;
    GCVector<RefPtr<CSSToken>> m_valueList;
    bool m_mediaTypeSet;

public:
    MediaQueryData();
    void clear();
    bool addExpression();
    bool tryAddParserToken(RefPtr<CSSToken>);
    void setMediaType(String*);
    MediaQuery* mediaQuery();

    inline bool currentMediaQueryChanged() const
    {
        return (m_restrictor != MediaQuery::None || m_mediaTypeSet ||
                m_expressions.size() > 0);
    }

    inline MediaQuery::RestrictorType restrictor()
    {
        return m_restrictor;
    }
    inline void setRestrictor(MediaQuery::RestrictorType restrictor)
    {
        m_restrictor = restrictor;
    }
    inline void setMediaFeature(MediaFeature mediaFeature)
    {
        m_mediaFeature = mediaFeature;
    }
};

class StyleRuleMedia;
class StyleRuleImport;
class StyleRuleFontFace;
class StyleRuleSupports;
class StyleRuleCounterStyle;
class StyleRuleNamespace;
class StyleRuleKeyframes;

#ifndef CSSTOKEN_POOL_INITIAL_SIZE
#define CSSTOKEN_POOL_INITIAL_SIZE 24
#endif

class CSSParser : public DocumentHoldable {
    friend class CSSToken;

public:
    enum NumericSign {
        NoSign,
        PlusSign,
        MinusSign,
    };

    enum LogicOp {
        And,
        Or,
        Not,
    };

    enum TruthOp {
        False = 0,
        True = 1,
        Paren = 3,
    };

    enum AllowedRulesType {
        // As per css-syntax, css-cascade and css-namespaces, @charset rules
        // must come first, followed by @import then @namespace.
        // AllowImportRules actually means we allow @import and any rules they
        // may follow it, i.e. @namespace rules and regular rules.
        // AllowCharsetRules and AllowNamespaceRules behave similarly.
        AllowCharsetRules,
        AllowImportRules,
        AllowNamespaceRules,
        RegularRules,
        KeyframeRules,
        ApplyRules, // For @apply inside style rules
        NoRules,    // For parsing at-rules inside declaration lists
    };

    enum RuleListType { TopLevelRuleList, RegularRuleList, KeyframesRuleList };

    enum ParseResult { Consumed, ErrorFounded, Failed };

    CSSParser(Document* document);
    inline ~CSSParser()
    {
        m_isPoolEnabled = false;
    }

    void parseStyleSheet(String* sourceString, CSSStyleSheet* target);
    bool parseSupportCondition(String* str);
    void parseRules(RefPtr<CSSToken> token, GCVector<StyleRuleBase*>& rootRule,
                    RuleListType ruleListType, bool isInsertedByUser = false);
    void parseStyleDeclaration(String* str, CSSStyleDeclaration* declaration);
    ParseResult parseStyleRule(RefPtr<CSSToken> aToken,
                               GCVector<StyleRuleBase*>& rules,
                               AllowedRulesType allowedRules,
                               GCVector<CSSSelectorList*>* sList,
                               bool isQueryingSelector = false);
    RefPtr<CSSToken> makeToken(String* str);
    StyleRuleMedia* parseMediaRule(bool isInsertedByUser);
    MediaQuerySet* parseMediaQuery();
    StyleRuleImport* parseImportRule();
    StyleRuleFontFace* parseFontFaceRule();
    StyleRuleSupports* parseSupportsRule();
    StyleRuleCounterStyle* parseCounterStyleRule();
    StyleRuleNamespace* parseNamespaceRule();
    StyleRuleKeyframes* parseKeyframesRule();
    Nullable<String*> parseURLString();
    void consumeComponentValue(RefPtr<CSSToken>& token);
    void parseSelector(GCVector<CSSSelectorList*>& list, bool& validSelector);

    bool parseSupportsCondition(); // for supports rule

protected:
    RefPtr<CSSToken> getToken(bool aSkipWS, bool aSkipComment,
                              bool isURL = false);
    RefPtr<CSSToken> currentToken();
    void ungetToken();
    void preserveState();
    void restoreState();
    void forgetState();
    RefPtr<CSSToken> lookAhead(bool aSkipWS, bool aSkipComment);

    bool parseComplexSelectorList(GCVector<CSSSelectorList*>& sList);
    void parseComplexSelector(CSSSelectorList* selectorList);
    void parseCompoundSelector(CSSSelectorList* selectorList);
    CSSSelector::RelationType parseCombinator();
    bool parseName(CSSTokenString& name);
    CSSSelector* getSimpleSelector();
    CSSSelector* getIdSelector();
    CSSSelector* getClassSelector();
    CSSSelector* getAttributeSelector();
    CSSSelector* getPseudoSelector();
    String* determineNamespace(String* prefix);
    void prependTypeSelectorIfNeeded(String* namespacePrefix,
                                     String* elementName,
                                     CSSSelector* compoundSelector);
    unsigned extractCompoundFlags(CSSSelector* simpleSelector);
    bool getANPlusB(std::pair<int, int>& result);
    CSSSelector::Type getAttributeMatch(RefPtr<CSSToken> token);
    CSSSelector::AttributeMatchType getAttributeFlags();
    String* getStringWithoutQuotationMarks(const CSSTokenString& value);

    CSSTokenString parseDefaultPropertyValue(RefPtr<CSSToken> token);
    ParseResult parseDeclaration(RefPtr<CSSToken> aToken,
                                 CSSStyleDeclaration* declaration,
                                 bool allowSrcProperty = false);
    void addUnknownAtRule();
    void reportError(const char* aMsg);
    bool parseCharsetRule(GCVector<StyleRuleBase*>& rules);
    static CSSTokenString combineAndTrimTokenValues(
        const GCVector<RefPtr<CSSToken>>& list);
    bool isBlockStart(RefPtr<CSSToken> token) const;
    bool isBlockEnd(RefPtr<CSSToken> token) const;

    bool m_preserveWS;
    bool m_preserveComments;
    GCVector<RefPtr<CSSToken>> m_preservedTokens;
    CSSScanner* m_scanner;
    RefPtr<CSSToken> m_lookAhead;
    RefPtr<CSSToken> m_token;
    String* m_error;
    bool m_failedParsing;

    // Media Query
    enum MediaQueryParserType {
        MediaQuerySetParser,
        MediaConditionParser,
    };

    void initParseMediaQuery(MediaQueryParserType parserType);

    void processToken(RefPtr<CSSToken> token);

    void readRestrictor(RefPtr<CSSToken>);
    void readMediaNot(RefPtr<CSSToken>);
    void readMediaType(RefPtr<CSSToken>);
    void readAnd(RefPtr<CSSToken>);
    void readFeatureStart(RefPtr<CSSToken>);
    void readFeature(RefPtr<CSSToken>);
    void readFeatureColon(RefPtr<CSSToken>);
    void readFeatureValue(RefPtr<CSSToken>);
    void readFeatureEnd(RefPtr<CSSToken>);
    void skipUntilComma(RefPtr<CSSToken>);
    void skipUntilBlockEnd(RefPtr<CSSToken>);
    void done(RefPtr<CSSToken>);

    using State = void (CSSParser::*)(RefPtr<CSSToken>);

    void setStateAndRestrict(State, MediaQuery::RestrictorType);
    void handleBlocks(RefPtr<CSSToken> token);
    void handleToken(RefPtr<CSSToken> token);

    State m_state;
    MediaQueryParserType m_parserType;
    MediaQueryData m_mediaQueryData;
    MediaQuerySet* m_querySet;

    const static State ReadRestrictor;
    const static State ReadMediaNot;
    const static State ReadMediaType;
    const static State ReadAnd;
    const static State ReadFeatureStart;
    const static State ReadFeature;
    const static State ReadFeatureColon;
    const static State ReadFeatureValue;
    const static State ReadFeatureEnd;
    const static State SkipUntilComma;
    const static State SkipUntilBlockEnd;
    const static State Done;

    // Token memory pool variables
    bool m_isPoolEnabled;
    CSSToken* m_initialTokenMemoryPool[CSSTOKEN_POOL_INITIAL_SIZE];
    size_t m_initialTokenMemoryPoolSize;
    GCVector<CSSToken*> m_tokenMemoryPool;
    char m_tokenInnerPool[CSSTOKEN_POOL_INITIAL_SIZE * sizeof(CSSToken)];
    unsigned m_blockLevel;

private:
    // for supports rule
    bool parseSupportsNegation();
    bool parseSupportsConnectives(String* conjoiner);
    bool parseGroupRuleBody(GCVector<StyleRuleBase*>& rules);
    bool parseSupportsConditionInParen();
    bool parseSupportsConditionInParenSub();
    bool parseSupportsDeclarationCondition();
    bool parseGeneralEnclosed();
    bool doLogicOperation();
    GCVector<TruthOp> m_supportOperandStack;
    GCVector<LogicOp> m_supportOperatorStack;
};

struct MediaQueryExpValue {
    String* id;
    double value;
    UnitType unit;
    unsigned numerator;
    unsigned denominator;

    bool isID;
    bool isValue;
    bool isRatio;

    MediaQueryExpValue()
        : id(String::emptyString)
        , value(0)
        , unit(UnitType::UnknownType)
        , numerator(0)
        , denominator(1)
        , isID(false)
        , isValue(false)
        , isRatio(false)
    {
    }

    bool isValid() const
    {
        return (isID || isValue || isRatio);
    }
    String* cssText() const;
    bool equals(const MediaQueryExpValue& expValue) const
    {
        if (isID) {
            return (id->equals(expValue.id));
        }
        if (isValue) {
            return (value == expValue.value);
        }
        if (isRatio) {
            return (numerator == expValue.numerator &&
                    denominator == expValue.denominator);
        }
        return !expValue.isValid();
    }
};

class MediaQueryExp : public gc {
public:
    static MediaQueryExp* createIfValid(MediaFeature mediaFeature,
                                        const GCVector<RefPtr<CSSToken>>&);
    ~MediaQueryExp();

    MediaFeature mediaFeature()
    {
        return m_mediaFeature;
    }

    MediaQueryExpValue expValue()
    {
        return m_expValue;
    }

    bool operator==(const MediaQueryExp& other) const;

    bool isViewportDependent() const;

    bool isDeviceDependent() const;

    MediaQueryExp(MediaQueryExp& other);

    String* serialize() const;

protected:
    MediaQueryExp(MediaFeature mediaFeature, MediaQueryExpValue expValue);

    MediaFeature m_mediaFeature;
    MediaQueryExpValue m_expValue;
};
}

#endif
