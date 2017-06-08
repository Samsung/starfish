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

namespace StarFish {

static bool isDigit(char c)
{
    if (c >= '0' && c <= '9') {
        return true;
    }
    return false;
}

static bool isAlpha(char c)
{
    if (c >= 'a' && c <= 'z') {
        return true;
    }
    return false;
}

static bool isNameChar(char c)
{
    if (isAlpha(c) || isDigit(c) || c == '_' || c == '-') {
        return true;
    }
    return false;
}

static bool isQuote(char c)
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

class CSSToken;
class CSSScanner;
class MediaQueryExp;

class MediaQueryData {
private:
    MediaQuery::RestrictorType m_restrictor;
    String* m_mediaType;
    GCVector<MediaQueryExp*> m_expressions;
    String* m_mediaFeature;
    GCVector<CSSToken*> m_valueList;
    bool m_mediaTypeSet;

public:
    MediaQueryData();
    void clear();
    bool addExpression();
    bool tryAddParserToken(CSSToken*);
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

class CSSMediaRule;
class CSSImportRule;
class CSSParser : public DocumentHoldable {
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

    CSSParser(Document* document)
        : DocumentHoldable(document)
    {
        m_error = String::emptyString;
        m_failedParsing = false;
    }

    void parseStyleSheet(String* sourceString, CSSStyleSheet* target);
    void parseRules(CSSToken* token, GCVector<CSSRule*>& rootRule,
                    RuleListType ruleListType);
    void parseStyleDeclaration(String* str, CSSStyleDeclaration* declaration);
    bool parseStyleRule(CSSToken* aToken, GCVector<CSSRule*>& rules,
                        AllowedRulesType allowedRules,
                        GCVector<GCDeque<CSSSelector*>*>* sList,
                        bool isQueryingSelector = false);
    CSSToken* makeToken(String* str);
    CSSMediaRule* parseMediaRule();
    MediaQuerySet* parseMediaQuery();
    CSSImportRule* parseImportRule();
    String* parseURLString();

protected:
    CSSToken* getToken(bool aSkipWS, bool aSkipComment, bool isURL = false);
    CSSToken* currentToken();
    void ungetToken();
    void preserveState();
    void restoreState();
    void forgetState();
    CSSToken* lookAhead(bool aSkipWS, bool aSkipComment);
    void parseSelector(GCVector<GCDeque<CSSSelector*>*>& list,
                       bool& validSelector);

    bool parseComplexSelectorList(GCVector<GCDeque<CSSSelector*>*>& sList);
    void parseComplexSelector(GCDeque<CSSSelector*>* selectorList);
    void parseCompoundSelector(GCDeque<CSSSelector*>* selectorList);
    CSSSelector::RelationType parseCombinator();
    bool parseName(String** name);
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
    CSSSelector::Type getAttributeMatch(CSSToken* token);
    CSSSelector::AttributeMatchType getAttributeFlags();
    String* getStringWithoutQuotationMarks(String* str);

    String* parseSimpleSelector(CSSToken* token, bool isFirstInChain,
                                bool canNegate, bool& validSelector);
    String* parseDefaultPropertyValue(CSSToken* token);
    void parseDeclaration(CSSToken* aToken, CSSStyleDeclaration* declaration);
    void addUnknownAtRule(String* aString);
    void reportError(const char* aMsg);
    bool parseCharsetRule(GCVector<CSSRule*>& rules);
    static String* combineAndTrimTokenValues(GCVector<CSSToken*>* list);
    bool m_preserveWS;
    bool m_preserveComments;
    GCVector<CSSToken*> m_preservedTokens;
    CSSScanner* m_scanner;
    CSSToken* m_lookAhead;
    CSSToken* m_token;
    String* m_error;
    bool m_failedParsing;

    // Media Query
    enum MediaQueryParserType {
        MediaQuerySetParser,
        MediaConditionParser,
    };

    void initParseMediaQuery(MediaQueryParserType parserType);

    void processToken(CSSToken* token);

    void readRestrictor(CSSToken*);
    void readMediaNot(CSSToken*);
    void readMediaType(CSSToken*);
    void readAnd(CSSToken*);
    void readFeatureStart(CSSToken*);
    void readFeature(CSSToken*);
    void readFeatureColon(CSSToken*);
    void readFeatureValue(CSSToken*);
    void readFeatureEnd(CSSToken*);
    void skipUntilComma(CSSToken*);
    void skipUntilBlockEnd(CSSToken*);
    void done(CSSToken*);

    using State = void (CSSParser::*)(CSSToken*);

    void setStateAndRestrict(State, MediaQuery::RestrictorType);
    void handleBlocks(CSSToken*);

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
                                        const GCVector<CSSToken*>&);
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
