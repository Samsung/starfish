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

#ifndef __StarFishCSSParser__
#define __StarFishCSSParser__

#include "style/Style.h"

namespace StarFish {

static bool isDigit(char c)
{
    if (c >= '0' && c <= '9')
        return true;
    return false;
}

static bool isAlpha(char c)
{
    if (c >= 'a' && c <= 'z')
        return true;
    return false;
}

static bool isNameChar(char c)
{
    if (isAlpha(c) || isDigit(c) || c == '_' || c == '-')
        return true;
    return false;
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
        if (str->equals("px") || str->equals("em") || str->equals("ex")
            || str->equals("in") || str->equals("cm") || str->equals("mm")
            || str->equals("pt") || str->equals("pc"))
            return true;
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
        if (cur == m_curPos && *cur != '.') // number can just start with '.' without '0'
            return false;

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
        if (!sign)
            res *= (-1);
        m_parsedNumber = res;
        return true;
    }

    bool consumeNumber()
    {
        bool t;
        return consumeNumber(&t);
    }

    float parsedNumber() { return m_parsedNumber; }

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

        if (cur != m_endPos)
            return false;

        m_curPos = cur;
        if (!sign)
            res *= (-1);
        m_parsedInt32 = res;
        return true;
    }

    int32_t parsedInt32() { return m_parsedInt32; }

    // a-z | 0-9 | - | _ | %
    bool consumeString()
    {
        int len = 0;
        for (char* cur = m_curPos; cur < m_endPos; cur++, len++) {
            if (!(isNameChar(*cur) || *cur == '%'))
                break;
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
                if (*(m_curPos - 1) == '\\')
                    len--;
                m_curPos++;
                consumeWhitespaces();
                break;
            }
        }
        if (*m_curPos != ')')
            return false;
        m_parsedUrl = String::fromUTF8(start, len);
        m_curPos++;
        return true;
    }

    String* parsedString() { return m_parsedString; }
    String* parsedUrl() { return m_parsedUrl; }

    bool isEnd()
    {
        return (m_curPos == m_endPos);
    }

    static bool parseUrl(String* token, String** ret)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token->utf8Data());
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
            if (!allowNegative && t < 0)
                return false;
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
            if (!allowNegative && t < 0)
                return false;
            *val = t;
            return parser->isEnd();
        }
        return false;
    }
    static bool parseLength(const char* token, bool allowNegative, CSSLength* ret)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0)
            return false;
        parser->consumeString();
        String* str = parser->parsedString();
        if ((str->length() == 0 && num == 0) || isLengthUnit(str)) {
            *ret = CSSLength(str, num);
            return parser->isEnd();
        }
        return false;
    }

    static bool parseLengthOrPercent(const char* token, bool allowNegative, CSSStyleValuePair* pair)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0)
            return false;
        parser->consumeString();
        String* str = parser->parsedString();
        if (str->equals("%")) {
            pair->setPercentageValue(num / 100.f);
            return parser->isEnd();
        } else if ((str->length() == 0 && num == 0)
            || isLengthUnit(str)) {
            pair->setLengthValue(CSSLength(str, num));
            return parser->isEnd();
        }
        return false;
    }

    static bool parseColorFunctionPart(String* s, bool isAlpha, unsigned char* ret, bool* isPercent)
    {
        const char* piece = s->trim()->utf8Data();
        CSSPropertyParser* parser = new CSSPropertyParser((char*)piece);

        bool hasPoint = false;
        parser->consumeWhitespaces();
        if (!parser->consumeNumber(&hasPoint))
            return false;

        float number = parser->parsedNumber();
        bool percent = false;
        if (parser->consumeIfNext('%'))
            percent = true;

        // NOTE: decimal-point is disallowed for rgb value.
        if (!isAlpha && !percent && hasPoint)
            return false;

        parser->consumeWhitespaces();
        if (!parser->isEnd())
            return false;

        number = number < 0 ? 0 : number;
        if (percent) {
            if (number > 100)
                number = 100;
            number = 255 * number / 100;
        } else if (isAlpha) {
            if (number > 1)
                number = 1;
            number = number * 255;
        } else if (number > 255) {
            number = 255;
        }
        *ret = number;
        *isPercent = percent;
        return true;
    }

    static bool parseNonNamedColor(String* str, Color* ret)
    {
        bool maybeRGBA = str->startsWith("rgba(");
        bool maybeRGB = str->startsWith("rgb(");
        bool maybeCode = str->startsWith("#");

        if (maybeRGBA || maybeRGB) {
            size_t s1 = str->indexOf('(');
            size_t s2 = str->indexOf(')');
            if (s1 == SIZE_MAX || s2 != str->length() - 1 || s1 >= s2)
                return false;

            String* sub = str->substring(s1 + 1, s2 - s1 - 1);
            String::Vector v;
            sub->split(',', v);
            size_t size = v.size();
            if (!(maybeRGBA && size == 4) && !(maybeRGB && size == 3))
                return false;

            bool isPercent = false, shouldPercent;
            unsigned char parsed[4];
            for (size_t i = 0; i < size; i++) {
                if (!parseColorFunctionPart(v[i], (i == 3), &parsed[i], &isPercent))
                    return false;
                if (i == 0)
                    shouldPercent = isPercent;
                else if (shouldPercent != isPercent)
                    return false;
            }
            *ret = Color(parsed[0], parsed[1], parsed[2], maybeRGBA ? parsed[3] : 255);
        } else if (maybeCode) {
            const char* s = str->utf8Data();
            const unsigned len = strlen(s);
            if (!(len == 7 || len == 4))
                return false;
            for (unsigned i = 1; i < len; i++) {
                if (!(s[i] >= '0' && s[i] <= '9') && !(s[i] >= 'A' && s[i] <= 'F') && !(s[i] >= 'a' && s[i] <= 'f'))
                    return false;
            }
            if (len == 7) {
                unsigned int r, g, b;
                sscanf(s, "#%02x%02x%02x", &r, &g, &b);
                *ret = Color(r, g, b, 255);
            } else if (len == 4) {
                unsigned int r, g, b;
                sscanf(s, "#%01x%01x%01x", &r, &g, &b);
                *ret = Color(r * 17, g * 17, b * 17, 255);
            }
        } else if (str->equals("transparent")) {
            *ret = Color(0, 0, 0, 0);
        } else {
            return false;
        }
        return true;
    }

    static bool parseNamedColor(String* str, NamedColorValue* ret)
    {
        if (str->equals(String::fromUTF8("currentcolor"))) {
            *ret = NamedColorValue::currentColor;
            return true;
        }
        return ::parseNamedColor(str->utf8Data(), str->length(), *ret);
    }

    char* m_startPos;
    char* m_endPos;
    char* m_curPos;

    float m_parsedNumber;
    int32_t m_parsedInt32;
    String* m_parsedString;
    String* m_parsedUrl;
};


class CSSToken;
class CSSScanner;
class CSSParser {
public:
    CSSParser(Document* document)
        : m_document(document)
    {
        m_error = String::emptyString;
    }

    void parseStyleSheet(String* sourceString, CSSStyleSheet* target);
    void parseStyleDeclaration(String* str, CSSStyleDeclaration* declaration);
protected:
    CSSToken* getToken(bool aSkipWS, bool aSkipComment, bool isURL = false);
    CSSToken* currentToken();
    void ungetToken();
    void preserveState();
    void restoreState();
    void forgetState();
    CSSToken* lookAhead(bool aSkipWS, bool aSkipComment);
    void parseStyleRule(CSSToken* aToken, CSSStyleSheet* aOwner, bool aIsInsideMediaRule);
    String* parseSelector(CSSToken* aToken, bool aParseSelectorOnly, bool& validSelector);
    String* parseSimpleSelector(CSSToken* token, bool isFirstInChain, bool canNegate, bool& validSelector);
    String* parseDefaultPropertyValue(CSSToken* token);
    void parseDeclaration(CSSToken* aToken, CSSStyleDeclaration* declaration);
    void addUnknownAtRule(CSSStyleSheet* aSheet, String* aString);
    void reportError(const char *aMsg);
    bool parseCharsetRule(CSSStyleSheet* aSheet);
    static String* combineAndTrimTokenValues(std::vector<CSSToken*, gc_allocator<CSSToken*>>* list);
    Document* m_document;
    bool m_preserveWS;
    bool m_preserveComments;
    std::vector<CSSToken*, gc_allocator<CSSToken*> > m_preservedTokens;
    CSSScanner* m_scanner;
    CSSToken* m_lookAhead;
    CSSToken* m_token;
    String* m_error;
};

}

#endif
