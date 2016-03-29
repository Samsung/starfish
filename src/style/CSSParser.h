#ifndef __StarFishCSSParser__
#define __StarFishCSSParser__

#include "style/Style.h"
#include "style/NamedColors.h"

namespace StarFish {

class CSSTokenizer : public gc {
    // <ident-token>, <function-token>, <at-keyword-token>, <hash-token>, <string-token>, <bad-string-token>, <url-token>, <bad-url-token>, <delim-token>, <number-token>, <percentage-token>, <dimension-token>, <unicode-range-token>, <include-match-token>, <dash-match-token>, <prefix-match-token>, <suffix-match-token>, <substring-match-token>, <column-token>, <whitespace-token>, <CDO-token>, <CDC-token>, <colon-token>, <semicolon-token>, <comma-token>, <[-token>, <]-token>, <(-token>, <)-token>, <{-token>, and <}-token>
    enum TokenType {
    };

public:
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

    static bool isNameStart(char c)
    {
        if (isAlpha(c))
            return true;
        if (c == '_')
            return true;
        // TODO
        return false;
    }

    static bool isNameChar(char c)
    {
        if (isNameStart(c))
            return true;
        if (isDigit(c))
            return true;
        if (c == '-')
            return true;
        return false;
    }

    static bool isUnitChar(char c)
    {
        if (c == '%')
            return true;
        return false;
    }
};

class CSSPropertyParser : public gc {
public:
    CSSPropertyParser(char* value)
        : m_startPos(value)
        , m_endPos(value + strlen(value))
        , m_curPos(value)
    {
    }

    bool consumeNumber()
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
        while (CSSTokenizer::isDigit(*cur) && cur < m_endPos) {
            res = res * 10 + (*cur - '0');
            cur++;
        }
        if (cur == m_curPos)
            return false;
        if (*cur == '.' && cur < m_endPos) {
            cur++;
            int pt = 10;
            while (CSSTokenizer::isDigit(*cur) && cur < m_endPos) {
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

    float parsedNumber() { return m_parsedNumber; }

    bool consumeString()
    {
        int len = 0;
        for (char* cur = m_curPos; cur < m_endPos; cur++, len++) {
            if (!(CSSTokenizer::isNameChar(*cur) || CSSTokenizer::isUnitChar(*cur)))
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
        while (*m_curPos == ' ' && m_curPos < m_endPos) {
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

    static String* parseUrl(std::vector<String*, gc_allocator<String*> >* tokens, unsigned start, unsigned end)
    {
        String* str = String::emptyString;
        for (unsigned i = start; i < end; i++) {
            str = str->concat(tokens->at(i));
        }
        CSSPropertyParser* parser = new CSSPropertyParser((char*)str->utf8Data());
        if (parser->consumeString()) {
            String* name = parser->parsedString();
            if (name->toLower()->equals("url") && parser->consumeIfNext('(')) {
                if (parser->consumeUrl())
                    return String::fromUTF8(parser->parsedUrl()->utf8Data());
            }
        }
        return String::emptyString;
    }

    static bool assureUrl(std::vector<String*, gc_allocator<String*> >* tokens, unsigned start, unsigned end)
    {
        String* str = String::emptyString;
        for (unsigned i = start; i < end; i++) {
            str = str->concat(tokens->at(i));
        }
        CSSPropertyParser* parser = new CSSPropertyParser((char*)str->utf8Data());
        if (parser->consumeString()) {
            String* name = parser->parsedString();
            if (name->toLower()->equals("url") && parser->consumeIfNext('(')) {

                return parser->consumeUrl();
            }
        }
        return false;
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
                || str->equals("px")
                || str->equals("em")
                || str->equals("ex")
                || str->equals("in")
                || str->equals("cm")
                || str->equals("mm")
                || str->equals("pt")
                || str->equals("pc")
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

    static bool assureLengthOrPercentList(const char* token, bool allowNegative, int minSize, int maxSize)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        int cnt = 0;
        do {
            if (!parser->consumeNumber())
                return false;
            float num = parser->parsedNumber();
            if (!allowNegative && num < 0)
                return false;
            if (parser->consumeString()) {
                String* str = parser->parsedString();
                if (!((str->length() == 0 && num == 0)
                    || str->equals("px")
                    || str->equals("em")
                    || str->equals("ex")
                    || str->equals("in")
                    || str->equals("cm")
                    || str->equals("mm")
                    || str->equals("pt")
                    || str->equals("pc")
                    || str->equals("%")))
                    return false;
            } else {
                // After a zero length, the unit identifier is optional
                if (num != 0)
                    return false;
            }
            cnt++;
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
                || str->equals("px")
                || str->equals("em")
                || str->equals("ex")
                || str->equals("in")
                || str->equals("cm")
                || str->equals("mm")
                || str->equals("pt")
                || str->equals("pc"))
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
            if (!parser->consumeNumber())
                return false;
            if (!allowNegative && parser->parsedNumber() < 0)
                return false;
            cnt++;
        } while (parser->consumeIfNext(','));
        return parser->isEnd() && minSize <= cnt && cnt <= maxSize;
    }

    static bool assureInteger(const char* token, bool allowNegative)
    {
        CSSPropertyParser* parser = new CSSPropertyParser((char*)token);
        if (!parser->consumeNumber())
            return false;
        float num = parser->parsedNumber();
        if (num != std::floor(num))
            return false;
        return parser->isEnd();
    }

    static bool assureColor(const char* token)
    {
        if (strcmp("transparent", token) == 0) {
            return true;
        } else if (strlen(token) == 7 && token[0] == '#') {
            for (int i = 1; i < 7; i++) {
                if ((token[i] >= '0' && token[i] <= '9') || (token[i] >= 'a' && token[i] <= 'f')) {
                } else {
                    return false;
                }
            }
            return true;
        } else if (strlen(token) == 4 && token[0] == '#') {
            for (int i = 1; i < 4; i++) {
                if ((token[i] >= '0' && token[i] <= '9') || (token[i] >= 'a' && token[i] <= 'f')) {
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
                    if (!parser->consumeNumber())
                        return false;

                    if (i == 0 && parser->consumeIfNext('%'))
                        isPercent = true;
                    else if (isPercent && !parser->consumeIfNext('%'))
                        return false;

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
        // initial || inherit || empty string
        if (strcmp(token, "initial") == 0 || strcmp(token, "inherit") == 0 || strcmp(token, "") == 0)
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

    static bool assureBorderColor(const char* token)
    {
        // color | transparent | inherit
        if (assureColor(token))
            return true;
        return false;
    }

    static char* getNextSingleValue(char* str)
    {
        char* next = NULL;
        while ((next = strchr(str, ' '))) {
            if (next == str) {
                str++;
                continue;
            }
            *next = '\0';
            return str;
        }
        return str;
    }

    bool findNextValueKind(char separator, CSSStyleValuePair::ValueKind* kind)
    {
        if (m_endPos <= m_curPos)
            return false;
        char* nextSep = strchr(m_curPos, separator);
        if (nextSep == NULL)
            nextSep = m_curPos + strlen(m_curPos);
        if (strncmp(m_curPos, "auto", 4) == 0) {
            *kind = CSSStyleValuePair::ValueKind::Auto;
        } else if (strncmp(nextSep - 1, "%", 1) == 0) {
            *kind = CSSStyleValuePair::ValueKind::Percentage;
            float f;
            sscanf(m_curPos, "%f%%", &f);
            f = f / 100.f;
            m_parsedFloatValue = f;
        } else if (strncmp(nextSep - 2, "px", 1) == 0) {
            *kind = CSSStyleValuePair::ValueKind::Length;
            float f;
            sscanf(m_curPos, "%fpx", &f);
            m_parsedFloatValue = f;
        }
        m_curPos = nextSep + 1;
        return true;
    }

    float parsedFloatValue()
    {
        return m_parsedFloatValue;
    }

    char* m_startPos;
    char* m_endPos;
    char* m_curPos;
    float m_parsedFloatValue;

    float m_parsedNumber;
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
    }
    CSSStyleSheet* parseStyleSheet(String* str);
protected:
    CSSToken* getToken(bool aSkipWS, bool aSkipComment);
    CSSToken* currentToken();
    void ungetToken();
    void preserveState();
    void restoreState();
    void forgetState();
    CSSToken* lookAhead(bool aSkipWS, bool aSkipComment);
    void parseStyleRule(CSSToken* aToken, CSSStyleSheet* aOwner, bool aIsInsideMediaRule);
    String* parseSelector(CSSToken* aToken, bool aParseSelectorOnly);
    String* parseSimpleSelector(CSSToken* token, bool isFirstInChain, bool canNegate);
    String* parseDefaultPropertyValue(CSSToken* token);
    void parseDeclaration(CSSToken* aToken, CSSStyleDeclaration* declaration);

    Document* m_document;
    bool m_preserveWS;
    bool m_preserveComments;
    std::vector<CSSToken*, gc_allocator<CSSToken*> > m_preservedTokens;
    CSSScanner* m_scanner;
    CSSToken* m_lookAhead;
    CSSToken* m_token;
};

}

#endif
