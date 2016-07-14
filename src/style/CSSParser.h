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
#include "style/NamedColors.h"

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

    static String* parseUrl(String* token)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token->utf8Data());
        if (parser->consumeString()) {
            String* name = parser->parsedString();
            if (name->equals("url") && parser->consumeIfNext('(')) {
                if (parser->consumeUrl())
                    return String::fromUTF8(parser->parsedUrl()->utf8Data());
            }
        }
        return String::emptyString;
    }

    static bool assureUrl(const char* str)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)str);
        if (parser->consumeString()) {
            String* name = parser->parsedString();
            if (name->equals("url") && parser->consumeIfNext('(')) {

                return parser->consumeUrl() && parser->isEnd();
            }
        }
        return false;
    }

    static bool assureUrlOrNone(const char* token)
    {
        if (strcmp(token, "none") == 0)
            return true;
        return CSSPropertyParser::assureUrl(token);
    }

    static float parseNumber(const char* token)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (parser->consumeNumber()) {
            return parser->parsedNumber();
        }
        return 0;
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

    static int32_t parseInt32(const char* token)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (parser->consumeInt32()) {
            return parser->parsedInt32();
        }
        return 0;
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

    static String* parseNumberAndUnit(const char* token, float* result)
    {
        // NOTE(example): token("10px") -> result=10.0 + return "px"
        *result = 0;
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (parser->consumeNumber()) {
            float num = parser->parsedNumber();
            *result = num;
        }
        parser->consumeString();
        return parser->parsedString();
    }

    static bool assureLengthOrPercent(const char* token, bool allowNegative)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0)
            return false;
        if (parser->consumeString()) {
            String* str = parser->parsedString();
            if ((str->length() == 0 && num == 0)
                || isLengthUnit(str)
                || str->equals("%"))
                return parser->isEnd();
            return false;
        } else {
            // After a zero length, the unit identifier is optional
            if (num == 0)
                return true;
            return false;
        }
        return true;
    }

    static bool assureLengthOrPercentOrAuto(const char* token, bool allowNegative)
    {
        // <length> | <percentage> | auto
        if (assureLengthOrPercent(token, allowNegative) || strcmp(token, "auto") == 0)
            return true;
        return false;
    }

    static bool assureLengthOrPercentList(const char* token, bool allowNegative, int minSize, int maxSize)
    {
        // NOTE: Allowed form - [space*][Length | Percent][space*] , ...
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        int cnt = 0;
        do {
            parser->consumeWhitespaces();
            if (!parser->consumeNumber())
                return false;
            float num = parser->parsedNumber();
            if (!allowNegative && num < 0)
                return false;
            if (parser->consumeString()) {
                String* str = parser->parsedString();
                if (!((str->length() == 0 && num == 0)
                    || isLengthUnit(str)
                    || str->equals("%")))
                    return false;
            } else {
                // After a zero length, the unit identifier is optional
                if (num != 0)
                    return false;
            }
            cnt++;
            parser->consumeWhitespaces();
        } while (parser->consumeIfNext(','));
        return parser->isEnd() && minSize <= cnt && cnt <= maxSize;
    }

    static bool assureLength(const char* token, bool allowNegative)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0)
            return false;
        if (parser->consumeString()) {
            String* str = parser->parsedString();
            if ((str->length() == 0 && num == 0)
                || isLengthUnit(str))
                return parser->isEnd();
            return false;
        } else {
            // After a zero length, the unit identifier is optional
            if (num == 0)
                return true;
            return false;
        }
        return true;
    }

    static bool assureAngle(const char* token)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (parser->consumeString()) {
            String* str = parser->parsedString();
            if ((str->length() == 0 && num == 0)
                || str->equals("deg")
                || str->equals("grad")
                || str->equals("rad")
                || str->equals("turn"))
                return parser->isEnd();
            return false;
        } else {
            // After a zero length, the unit identifier is optional
            if (num == 0)
                return true;
            return false;
        }
        return true;
    }

    // comma-seperated list
    static bool assureAngleList(const char* token, int minSize, int maxSize)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        int cnt = 0;
        do {
            parser->consumeWhitespaces();
            if (!parser->consumeNumber())
                return false;
            float num = parser->parsedNumber();
            if (parser->consumeString()) {
                String* str = parser->parsedString();
                if ((str->length() == 0 && num == 0)
                    || str->equals("deg")
                    || str->equals("grad")
                    || str->equals("rad")
                    || str->equals("turn"))
                    cnt++;
                else
                    return false;
            } else {
                // After a zero length, the unit identifier is optional
                if (num == 0)
                    cnt++;
                else
                    return false;
            }
            parser->consumeWhitespaces();
        } while (parser->consumeIfNext(','));
        return parser->isEnd() && minSize <= cnt && cnt <= maxSize;
    }

    static bool assurePercent(const char* token, bool allowNegative)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0)
            return false;
        if (parser->consumeString()) {
            String* str = parser->parsedString();
            if (str->equals("%"))
                return parser->isEnd();
            return false;
        } else {
            // After a zero length, the unit identifier is optional
            if (num == 0)
                return true;
            return false;
        }
        return true;
    }

    static bool assureNumber(const char* token, bool allowNegative)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (!allowNegative && num < 0)
            return false;
        return parser->isEnd();
    }

    static bool assureNumberList(const char* token, bool allowNegative, int minSize, int maxSize) // comma-seperated list
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        int cnt = 0;
        do {
            parser->consumeWhitespaces();
            if (!parser->consumeNumber())
                return false;
            if (!allowNegative && parser->parsedNumber() < 0)
                return false;
            cnt++;
            parser->consumeWhitespaces();
        } while (parser->consumeIfNext(','));
        return parser->isEnd() && minSize <= cnt && cnt <= maxSize;
    }

    static bool assureInteger(const char* token, bool allowNegative)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        parser->consumeNumber();
        float num = parser->parsedNumber();
        if (num != std::floor(num))
            return false;
        return parser->isEnd();
    }

    static bool assureColor(const char* token)
    {
        if (strcmp("currentcolor", token) == 0) {
            return true;
        } else if (token[0] == '#') {
            if (!(strlen(token) == 9 || strlen(token) == 7 || strlen(token) == 4)) {
                return false;
            }

            for (unsigned i = 1; i < strlen(token); i++) {
                if ((token[i] >= '0' && token[i] <= '9') || (token[i] >= 'A' && token[i] <= 'F') || (token[i] >= 'a' && token[i] <= 'f')) {
                } else {
                    return false;
                }
            }
            return true;
        }
#define PARSE_COLOR(name, value)        \
    else if (strcmp(#name, token) == 0) \
    {                                   \
        return true;                    \
    }

        NAMED_COLOR_FOR_EACH(PARSE_COLOR)
#undef PARSE_COLOR

        else if (strstr(token, "rgb") == token) {
            CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
            if (parser->consumeString()) {
                String* str = parser->parsedString();
                int numcnt;
                if (str->equals("rgb") && parser->consumeIfNext('(')) {
                    numcnt = 3;
                } else if (str->equals("rgba") && parser->consumeIfNext('(')) {
                    numcnt = 4;
                } else {
                    return false;
                }
                bool isPercent = false;
                for (int i = 0; i < numcnt; i++) {
                    bool hasPoint = false;
                    parser->consumeWhitespaces();
                    if (!parser->consumeNumber(&hasPoint))
                        return false;

                    if (i == 0 && parser->consumeIfNext('%'))
                        isPercent = true;
                    else if (isPercent && !parser->consumeIfNext('%'))
                        return false;

                    // NOTE: decimal-point is disallowed for rgb value.
                    if (i < 3 && !isPercent && hasPoint)
                        return false;

                    parser->consumeWhitespaces();
                    if (i == numcnt - 1)
                        parser->consumeIfNext(')');
                    else
                        parser->consumeIfNext(',');
                }
                return parser->isEnd();
            }
        }
        return false;
    }

    static bool assureEssential(const char* token)
    {
        // initial || inherit
        if (strcmp(token, "initial") == 0 || strcmp(token, "inherit") == 0)
            return true;
        return false;
    }

    static bool assureBorderWidth(const char* token)
    {
        // border-width(thin | <medium> | thick) | length
        if (strcmp(token, "thin") == 0 || strcmp(token, "medium") == 0 || strcmp(token, "thick") == 0 || assureLength(token, false))
            return true;
        return false;
    }

    static bool assureBorderStyle(const char* token)
    {
        // border-style(<none> | solid) | inherit
        if (strcmp(token, "none") == 0 || strcmp(token, "solid") == 0)
            return true;
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
