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

#ifndef __StarFishCSSParser__
#define __StarFishCSSParser__

#include "binding/DocumentHoldable.h"
#include "core/style/Style.h"
#include "core/style/MediaQuerySet.h"
#include "core/util/RefPtr.h"

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
inline bool isNameChar(CharType c)
{
    if (isAlpha(c) || isDigit(c) || c == '_' || c == '-') {
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

static const char* unitTypeToString(UnitType type)
{
    switch (type) {
    case UnitType::Number:
    case UnitType::Integer:
    case UnitType::UserUnits:
        return "";
    case UnitType::Percentage:
        return "%";
    case UnitType::Ems:
        return "em";
    case UnitType::Exs:
        return "ex";
    case UnitType::Rems:
        return "rem";
    case UnitType::Chs:
        return "ch";
    case UnitType::Pixels:
        return "px";
    case UnitType::Centimeters:
        return "cm";
    case UnitType::DotsPerPixel:
        return "dppx";
    case UnitType::DotsPerInch:
        return "dpi";
    case UnitType::DotsPerCentimeter:
        return "dpcm";
    case UnitType::Millimeters:
        return "mm";
    case UnitType::Inches:
        return "in";
    case UnitType::Points:
        return "pt";
    case UnitType::Picas:
        return "pc";
    case UnitType::Degrees:
        return "deg";
    case UnitType::Radians:
        return "rad";
    case UnitType::Gradians:
        return "grad";
    case UnitType::Milliseconds:
        return "ms";
    case UnitType::Seconds:
        return "s";
    case UnitType::Hertz:
        return "hz";
    case UnitType::Kilohertz:
        return "khz";
    case UnitType::Turns:
        return "turn";
    case UnitType::Fraction:
        return "fr";
    case UnitType::ViewportWidth:
        return "vw";
    case UnitType::ViewportHeight:
        return "vh";
    case UnitType::ViewportMin:
        return "vmin";
    case UnitType::ViewportMax:
        return "vmax";
    case UnitType::UnknownType:
    case UnitType::ValueID:
    case UnitType::Calc:
    case UnitType::CalcPercentageWithNumber:
    case UnitType::CalcPercentageWithLength:
        break;
    };
    STARFISH_ASSERT_NOT_REACHED();
    return "";
}

class CSSPropertyParser : public gc {
public:
    CSSPropertyParser(char* value)
        : m_startPos(value)
        , m_endPos(value + strlen(value))
        , m_curPos(value)
    {
    }

    static bool isLengthUnit(String* str)
    {
        if (str->equals("px") || str->equals("em") || str->equals("ex") ||
            str->equals("in") || str->equals("cm") || str->equals("mm") ||
            str->equals("pt") || str->equals("pc")) {
            return true;
        }
        return false;
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
        while (isDigit(*cur) && cur < m_endPos) {
            res = res * 10 + (*cur - '0');
            cur++;
        }
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
        }

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
    bool consumeString()
    {
        int len = 0;
        for (char *cur = m_curPos; cur < m_endPos; cur++, len++) {
            if (!(isNameChar(*cur) || *cur == '%')) {
                break;
            }
        }
        m_parsedString = String::fromUTF8(m_curPos, len);
        m_curPos += len;
        return true;
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

    bool consumeUrl()
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
        m_parsedUrl = String::fromUTF8(start, len);
        m_curPos++;
        return true;
    }

    bool consumeContentString()
    {
        if (isQuote(*m_curPos++) && isQuote(*--m_endPos)) {
            m_parsedString = String::fromUTF8(m_curPos, m_endPos - m_curPos);
            return true;
        } else {
            return false;
        }
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
        m_parsedString = String::fromUTF8(start, len);
        m_curPos++;
        return true;
    }

    String* parsedString()
    {
        return m_parsedString;
    }
    String* parsedUrl()
    {
        return m_parsedUrl;
    }

    bool isEnd()
    {
        return (m_curPos == m_endPos);
    }

    static bool parseUrl(String* token, String** ret)
    {
        CSSPropertyParser* parser =
            new CSSPropertyParser((char*)token->utf8Data());
        if (parser->consumeString()) {
            String* name = parser->parsedString();
            if (name->equals("url") && parser->consumeIfNext('(')) {
                if (parser->consumeUrl() && parser->isEnd()) {
                    *ret = parser->parsedUrl();
                    return true;
                }
            }
        }
        return false;
    }

    static bool parseNumber(const char* token, bool allowNegative, float* val)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (parser->consumeNumber()) {
            float t = parser->parsedNumber();
            if (!allowNegative && t < 0) {
                return false;
            }
            *val = t;
            return parser->isEnd();
        }
        return false;
    }

    static bool parseInt32(const char* token, bool allowNegative, int32_t* val)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (parser->consumeInt32()) {
            int32_t t = parser->parsedInt32();
            if (!allowNegative && t < 0) {
                return false;
            }
            *val = t;
            return parser->isEnd();
        }
        return false;
    }
    static bool parseLength(const char* token, bool allowNegative,
                            CSSLength* ret)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber()) {
            return false;
        }
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0) {
            return false;
        }
        parser->consumeString();
        String* str = parser->parsedString();
        if ((str->length() == 0 && num == 0) || isLengthUnit(str)) {
            *ret = CSSLength(str, num);
            return parser->isEnd();
        }
        return false;
    }

    static bool parseLengthOrPercent(const char* token, bool allowNegative,
                                     CSSStyleValuePair* pair)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber()) {
            return false;
        }
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0) {
            return false;
        }
        parser->consumeString();
        String* str = parser->parsedString();
        if (str->equals("%")) {
            pair->setPercentageValue(num / 100.f);
            return parser->isEnd();
        } else if ((str->length() == 0 && num == 0) || isLengthUnit(str)) {
            pair->setLengthValue(CSSLength(str, num));
            return parser->isEnd();
        }
        return false;
    }

    static bool parseTime(const char* token, bool allowNegative, CSSTime* ret)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber()) {
            return false;
        }
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0) {
            return false;
        }
        parser->consumeString();
        String* str = parser->parsedString();

        if (str->equalsWithoutCase("s") ||
            str->equalsWithoutCase(String::emptyString)) {
            *ret = CSSTime(CSSTime::Kind::S, num);
            return parser->isEnd();
        } else if (str->equalsWithoutCase("ms")) {
            *ret = CSSTime(CSSTime::Kind::MS, num);
            return parser->isEnd();
        }
        return false;
    }

    static bool parseColorFunctionPart(String* s, bool isAlpha,
                                       unsigned char* ret, bool* isPercent)
    {
        const char* piece = s->trim()->utf8Data();
        CSSPropertyParser* parser = new CSSPropertyParser((char*)piece);

        bool hasPoint = false;
        parser->consumeWhitespaces();
        if (!parser->consumeNumber(&hasPoint)) {
            return false;
        }

        float number = parser->parsedNumber();
        bool percent = false;
        if (parser->consumeIfNext('%')) {
            percent = true;
        }

        // NOTE: decimal-point is disallowed for rgb value.
        if (!isAlpha && !percent && hasPoint) {
            return false;
        }

        parser->consumeWhitespaces();
        if (!parser->isEnd()) {
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

    static bool parseNonNamedColor(String* str, Unit::Color* ret)
    {
        bool maybeRGBA = str->startsWith("rgba(");
        bool maybeRGB = str->startsWith("rgb(");
        bool maybeCode = str->startsWith("#");

        if (maybeRGBA || maybeRGB) {
            size_t s1 = str->indexOf('(');
            size_t s2 = str->indexOf(')');
            if (s1 == SIZE_MAX || s2 != str->length() - 1 || s1 >= s2) {
                return false;
            }

            String* sub = str->substring(s1 + 1, s2 - s1 - 1);
            GCVector<String*> v;
            sub->split(',', v);
            size_t size = v.size();
            if (!(maybeRGBA && size == 4) && !(maybeRGB && size == 3)) {
                return false;
            }

            bool isPercent = false, shouldPercent = false;
            unsigned char parsed[4];
            for (size_t i = 0; i < size; i++) {
                if (!parseColorFunctionPart(v[i], (i == 3), &parsed[i],
                                            &isPercent)) {
                    return false;
                }
                if (i == 0) {
                    shouldPercent = isPercent;
                } else if (shouldPercent != isPercent) {
                    return false;
                }
            }
            *ret = Unit::Color(parsed[0], parsed[1], parsed[2],
                               maybeRGBA ? parsed[3] : 255);
        } else if (maybeCode) {
            const char* s = str->utf8Data();
            const unsigned len = strlen(s);
            if (!(len == 7 || len == 4)) {
                return false;
            }
            for (unsigned i = 1; i < len; i++) {
                if (!(s[i] >= '0' && s[i] <= '9') &&
                    !(s[i] >= 'A' && s[i] <= 'F') &&
                    !(s[i] >= 'a' && s[i] <= 'f')) {
                    return false;
                }
            }
            if (len == 7) {
                unsigned int r, g, b;
                sscanf(s, "#%02x%02x%02x", &r, &g, &b);
                *ret = Unit::Color(r, g, b, 255);
            } else if (len == 4) {
                unsigned int r, g, b;
                sscanf(s, "#%01x%01x%01x", &r, &g, &b);
                *ret = Unit::Color(r * 17, g * 17, b * 17, 255);
            }
        } else if (str->equals("transparent")) {
            *ret = Unit::Color(0, 0, 0, 0);
        } else {
            return false;
        }
        return true;
    }

    static bool parseNamedColor(String* str, NamedColor::NamedColorValue* ret)
    {
        if (str->equals(String::fromUTF8("currentcolor"))) {
            *ret = NamedColor::NamedColorValue::currentColor;
            return true;
        }
        return NamedColor::parseNamedColor(str->utf8Data(), str->length(),
                                           *ret);
    }

    static bool parseContentString(String* str, String** ret)
    {
        CSSPropertyParser* parser =
            new CSSPropertyParser((char*)str->utf8Data());
        if (parser->consumeContentString()) {
            *ret = parser->parsedString();
            return true;
        }
        return false;
    }

    static bool parseAttr(String* str, String** ret)
    {
        CSSPropertyParser* parser =
            new CSSPropertyParser((char*)str->utf8Data());
        if (parser->consumeString()) {
            String* name = parser->parsedString();
            if (name->equals("attr") && parser->consumeIfNext('(')) {
                if (parser->consumeAttr() && parser->isEnd()) {
                    *ret = parser->parsedString();
                    return true;
                }
            }
        }
        return false;
    }

    char* m_startPos;
    char* m_endPos;
    char* m_curPos;

    float m_parsedNumber;
    int32_t m_parsedInt32;
    String* m_parsedString;
    String* m_parsedUrl;
};

class CSSParser;

#ifndef CSSTOKENSTRING_BUILTIN_BUFFER_SIZE
#define CSSTOKENSTRING_BUILTIN_BUFFER_SIZE 24
#endif

class CSSTokenString : public gc {
public:
    CSSTokenString()
    {
        m_hasASCIIContent = true;
        m_length = 0;
        m_externalString = nullptr;
    }

    CSSTokenString(const CSSTokenString& src)
    {
        operator=(src);
    }

    void operator=(const CSSTokenString& src)
    {
        m_hasASCIIContent = src.m_hasASCIIContent;
        m_length = src.m_length;
        memcpy(
            m_builtInBuffer, src.m_builtInBuffer,
            sizeof(char32_t) *
                std::min((size_t)CSSTOKENSTRING_BUILTIN_BUFFER_SIZE, m_length));
        if (src.m_externalString) {
            m_externalString = new UTF32String(*src.m_externalString);
        } else {
            m_externalString = nullptr;
        }
    }

    CSSTokenString(CSSTokenString&& src)
    {
        m_hasASCIIContent = src.m_hasASCIIContent;
        m_length = src.m_length;
        memcpy(
            m_builtInBuffer, src.m_builtInBuffer,
            sizeof(char32_t) *
                std::min((size_t)CSSTOKENSTRING_BUILTIN_BUFFER_SIZE, m_length));
        m_externalString = src.m_externalString;

        src.m_hasASCIIContent = true;
        src.m_length = 0;
        src.m_externalString = nullptr;
    }

    void clear()
    {
        m_length = 0;
        m_hasASCIIContent = true;
        m_externalString = nullptr;
    }

    void appendChar(char32_t ch)
    {
        if (ch > 127) {
            m_hasASCIIContent = false;
        }
        if (m_length < CSSTOKENSTRING_BUILTIN_BUFFER_SIZE) {
            m_builtInBuffer[m_length++] = ch;
        } else {
            if (!m_externalString)
                m_externalString = new UTF32String();
            m_externalString->pushBack(ch);
            m_length++;
        }
    }

    size_t indexOf(const char32_t& ch)
    {
        for (size_t i = 0; i < length(); i++) {
            if (ch == charAt(i)) {
                return i;
            }
        }
        return SIZE_MAX;
    }

    bool contains(const char32_t& ch)
    {
        return indexOf(ch) != SIZE_MAX;
    }

    size_t length() const
    {
        return m_length;
    }

    char32_t charAt(const size_t& i) const
    {
        if (i < CSSTOKENSTRING_BUILTIN_BUFFER_SIZE) {
            return m_builtInBuffer[i];
        } else {
            return (*m_externalString)[i - CSSTOKENSTRING_BUILTIN_BUFFER_SIZE];
        }
    }

    bool hasASCIIContent() const
    {
        return m_hasASCIIContent;
    }

    void appendOther(const CSSTokenString& src)
    {
        for (size_t i = 0; i < src.length(); i++) {
            appendChar(src.charAt(i));
        }
    }

    bool equals(const char* src) const;
    bool equalsWithoutCase(const char* src) const;
    void toLower();
    String* toString() const;
    size_t peekASCIIBuffer(size_t (*cb)(const char* buffer, size_t len,
                                        void* data),
                           void* data) const;
    size_t peekUTF32Buffer(size_t (*cb)(const char32_t* buffer, size_t len,
                                        void* data),
                           void* data) const;
    size_t peekUTF8Buffer(size_t (*cb)(const char* buffer, size_t len,
                                       void* data),
                          void* data) const;
    AtomicString toAtomicString(StarFish* sf);
    AtomicString toAttrAtomicString(StarFish* sf);

protected:
    bool m_hasASCIIContent;
    size_t m_length;
    char32_t m_builtInBuffer[CSSTOKENSTRING_BUILTIN_BUFFER_SIZE];
    UTF32String* m_externalString;
};

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
                (!aValue || value()->equalsWithoutCase(aValue)));
    }

    bool isOfType(char aType)
    {
        return m_type == aType;
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
        return isOfType(CSSToken::IDENT_TYPE) && value()->equalsWithoutCase(s);
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
    char m_type : 8;
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

class CSSScanner;
class MediaQueryExp;

class MediaQueryData {
private:
    MediaQuery::RestrictorType m_restrictor;
    String* m_mediaType;
    GCVector<MediaQueryExp*> m_expressions;
    String* m_mediaFeature;
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
    inline void setMediaFeature(String* str)
    {
        m_mediaFeature = str;
    }
};

class StyleRuleMedia;
class StyleRuleImport;

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

    CSSParser(Document* document);
    inline ~CSSParser()
    {
        m_isPoolEnabled = false;
    }

    void parseStyleSheet(String* sourceString, CSSStyleSheet* target);
    void parseRules(RefPtr<CSSToken> token, GCVector<StyleRuleBase*>& rootRule,
                    RuleListType ruleListType);
    void parseStyleDeclaration(String* str, CSSStyleDeclaration* declaration);
    bool parseStyleRule(RefPtr<CSSToken> aToken,
                        GCVector<StyleRuleBase*>& rules,
                        AllowedRulesType allowedRules,
                        GCVector<CSSSelctorList*>* sList,
                        bool isQueryingSelector = false);
    RefPtr<CSSToken> makeToken(String* str);
    StyleRuleMedia* parseMediaRule();
    MediaQuerySet* parseMediaQuery();
    StyleRuleImport* parseImportRule();
    String* parseURLString();

protected:
    RefPtr<CSSToken> getToken(bool aSkipWS, bool aSkipComment,
                              bool isURL = false);
    RefPtr<CSSToken> currentToken();
    void ungetToken();
    void preserveState();
    void restoreState();
    void forgetState();
    RefPtr<CSSToken> lookAhead(bool aSkipWS, bool aSkipComment);
    void parseSelector(GCVector<CSSSelctorList*>& list, bool& validSelector);

    bool parseComplexSelectorList(GCVector<CSSSelctorList*>& sList);
    void parseComplexSelector(CSSSelctorList* selectorList);
    void parseCompoundSelector(CSSSelctorList* selectorList);
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
    void parseDeclaration(RefPtr<CSSToken> aToken,
                          CSSStyleDeclaration* declaration);
    void addUnknownAtRule();
    void reportError(const char* aMsg);
    bool parseCharsetRule(GCVector<StyleRuleBase*>& rules);
    static CSSTokenString combineAndTrimTokenValues(
        const GCVector<RefPtr<CSSToken>>& list);
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
    void handleBlocks(RefPtr<CSSToken>);

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
    static MediaQueryExp* createIfValid(String* mediaFeature,
                                        const GCVector<RefPtr<CSSToken>>&);
    ~MediaQueryExp();

    String* mediaFeature()
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
    MediaQueryExp(String*, MediaQueryExpValue);

    String* m_mediaFeature;
    MediaQueryExpValue m_expValue;
};
}

#endif
