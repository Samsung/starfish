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

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   emk <VYV03354@nifty.ne.jp>
 *   Daniel Glazman <glazman@netscape.com>
 *   L. David Baron <dbaron@dbaron.org>
 *   Boris Zbarsky <bzbarsky@mit.edu>
 *   Mats Palmgren <mats.palmgren@bredband.net>
 *   Christian Biesinger <cbiesinger@web.de>
 *   Jeff Walden <jwalden+code@mit.edu>
 *   Jonathon Jongsma <jonathon.jongsma@collabora.co.uk>, Collabora Ltd.
 *   Siraj Razick <siraj.razick@collabora.co.uk>, Collabora Ltd.
 *   Daniel Glazman <daniel.glazman@disruptive-innovations.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/modules/window/Window.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/Style.h"

namespace StarFish {

const char* kCHARSET_RULE_MISSING_SEMICOLON =
    "Missing semicolon at the end of @charset rule";
const char* kCHARSET_RULE_CHARSET_IS_STRING =
    "The charset in the @charset rule should be a string";
const char* kCHARSET_RULE_MISSING_WS =
    "Missing mandatory whitespace after @charset";
const char* kIMPORT_RULE_MISSING_URL = "Missing URL in @import rule";
const char* kURL_EOF = "Unexpected end of stylesheet";
const char* kURL_WS_INSIDE = "Multiple tokens inside a url() notation";
const char* kVARIABLES_RULE_POSITION =
    "@variables rule invalid at this position in the stylesheet";
const char* kIMPORT_RULE_POSITION =
    "@import rule invalid at this position in the stylesheet";
const char* kNAMESPACE_RULE_POSITION =
    "@namespace rule invalid at this position in the stylesheet";
const char* kCHARSET_RULE_CHARSET_SOF =
    "@charset rule invalid at this position in the stylesheet";
const char* kUNKNOWN_AT_RULE = "Unknow @-rule";

unsigned char CSS_ESCAPE = '\\';

char IS_HEX_DIGIT = 1;
char START_IDENT = 2;
char IS_IDENT = 4;
char IS_WHITESPACE = 8;

char W = IS_WHITESPACE;
char I = IS_IDENT;
char S = START_IDENT;
char SI = IS_IDENT | START_IDENT;
char XI = IS_IDENT | IS_HEX_DIGIT;
char XSI = IS_IDENT | START_IDENT | IS_HEX_DIGIT;

size_t countLF(String* s)
{
    size_t cnt = 1;
    for (size_t i = 0; i < s->length(); i++) {
        if (s->charAt(i) == '\n') {
            cnt++;
        }
    }
    return cnt;
}

char kLexTable[] = {
    0,  0,   0,   0,   0,   0,   0,   0,  0,  W,  W,  0,  W,  W,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,
    W,  0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0,  I,  0,  0,
    XI, XI,  XI,  XI,  XI,  XI,  XI,  XI, XI, XI, 0,  0,  0,  0,  0,  0,
    0,  XSI, XSI, XSI, XSI, XSI, XSI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, 0,  S,  0,  0,  SI,
    0,  XSI, XSI, XSI, XSI, XSI, XSI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, 0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI,  SI,  SI,  SI,  SI,  SI,  SI, SI, SI, SI, SI, SI, SI, SI, SI
};
size_t kLexTableSize = sizeof kLexTable / sizeof(int);

class CSSToken : public gc {
    CSSToken(char type)
    {
        m_type = type;
        m_stringValue = String::emptyString;
        m_unitType = UnitType::UnknownType;
        m_hasStringValue = false;
        m_hasCharValue = false;
        m_hasNumberValue = false;
        m_hasAlphabetNInUnit = false;
        m_hasSourceOfNumberValueDot = false;
    }

    CSSToken(char type, String* value)
    {
        m_type = type;
        m_stringValue = value;
        m_unitType = UnitType::UnknownType;
        m_hasStringValue = true;
        m_hasCharValue = false;
        m_hasNumberValue = false;
        m_hasAlphabetNInUnit = false;
        m_hasSourceOfNumberValueDot = false;
    }

    CSSToken(char type, char32_t value)
    {
        m_type = type;
        m_charValue = value;
        m_unitType = UnitType::UnknownType;
        m_hasStringValue = false;
        m_hasCharValue = true;
        m_hasNumberValue = false;
        m_hasAlphabetNInUnit = false;
        m_hasSourceOfNumberValueDot = false;
    }

    CSSToken(char type, float number, String* source, UnitType u,
             bool hasSourceOfNumberValueDot, bool hasAlphabetNInUnit)
    {
        m_type = type;
        m_numericValue = number;
        m_numericValueSource = source;
        m_unitType = u;
        m_hasStringValue = false;
        m_hasCharValue = false;
        m_hasNumberValue = true;
        m_hasAlphabetNInUnit = hasAlphabetNInUnit;
        m_hasSourceOfNumberValueDot = hasSourceOfNumberValueDot;
    }

public:
    static CSSToken* createNullToken()
    {
        return new CSSToken(CSSToken::NULL_TYPE);
    }

    static CSSToken* createToken(char type)
    {
        return new CSSToken(type);
    }

    static CSSToken* createStringValueToken(char type, String* value)
    {
        return new CSSToken(type, value);
    }

    static CSSToken* createCharValueToken(char type, char32_t ch)
    {
        return new CSSToken(type, ch);
    }

    static CSSToken* createNumberValueToken(char type, float number,
                                            String* source, UnitType u,
                                            bool hasSourceOfNumberValueDot,
                                            bool hasAlphabetNInUnit)
    {
        return new CSSToken(type, number, source, u, hasSourceOfNumberValueDot,
                            hasAlphabetNInUnit);
    }

    bool hasStringValue()
    {
        return m_hasStringValue;
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
        }
        return false;
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

    String* value()
    {
        STARFISH_ASSERT(m_hasStringValue);
        return m_stringValue;
    }

    char32_t charValue()
    {
        STARFISH_ASSERT(m_hasCharValue);
        return m_charValue;
    }

    AtomicString toAttrAtomicString(StarFish* sf)
    {
        STARFISH_ASSERT(m_hasCharValue || m_hasStringValue);
        if (m_hasCharValue) {
            return AtomicString::createAttrAtomicString(sf, charValue());
        } else {
            return AtomicString::createAttrAtomicString(sf, value());
        }
    }

    String* toStringValue()
    {
        if (m_hasCharValue) {
            return String::createUTF32String(charValue());
        } else if (m_hasNumberValue) {
            return m_numericValueSource;
        } else if (m_hasStringValue) {
            return value();
        }
        return String::emptyString;
    }

    float numericValue()
    {
        STARFISH_ASSERT(m_hasNumberValue);
        return m_numericValue;
    }

    String* numericValueSource()
    {
        STARFISH_ASSERT(m_hasNumberValue);
        return m_numericValueSource;
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
    bool m_hasStringValue : 1;
    bool m_hasCharValue : 1;
    bool m_hasNumberValue : 1;
    bool m_hasSourceOfNumberValueDot : 1;
    bool m_hasAlphabetNInUnit : 1;

    union {
        String* m_stringValue;
        char32_t m_charValue;
        struct {
            float m_numericValue;
            String* m_numericValueSource;
        };
    };
};

class CSSScanner : public gc {
public:
    CSSScanner(String* str)
        : m_string(str)
        , m_stringBufferData(str->bufferAccessData())
    {
        m_string = str;
        m_pos = 0;
    }

    size_t getCurrentPos()
    {
        return m_pos;
    }

    String* getAlreadyScanned()
    {
        return m_string->substring(0, m_pos);
    }

    void preserveState()
    {
        m_preservedPos.push_back(m_pos);
    }

    void restoreState()
    {
        if (m_preservedPos.size()) {
            m_pos = m_preservedPos.back();
            m_preservedPos.pop_back();
        }
    }

    void forgetState()
    {
        if (m_preservedPos.size()) {
            m_preservedPos.pop_back();
        }
    }

    int read()
    {
        if (LIKELY(m_pos < m_stringBufferData.length)) {
            if (m_stringBufferData.hasASCIIContent) {
                return m_stringBufferData.asciiData()[m_pos++];
            } else {
                return m_stringBufferData.utf32Data()[m_pos++];
            }
        }
        return -1;
    }

    int peek()
    {
        if (LIKELY(m_pos < m_stringBufferData.length)) {
            if (m_stringBufferData.hasASCIIContent) {
                return m_stringBufferData.asciiData()[m_pos];
            } else {
                return m_stringBufferData.utf32Data()[m_pos];
            }
        }
        return -1;
    }

    bool isHexDigit(char32_t code)
    {
        return (code < 256 && (kLexTable[code] & IS_HEX_DIGIT) != 0);
    }

    bool isIdentStart(char32_t code)
    {
        return (code >= 256 || (kLexTable[code] & START_IDENT) != 0);
    }

    bool startsWithIdent(char32_t aFirstChar, char32_t aSecondChar)
    {
        return isIdentStart(aFirstChar) ||
               (aFirstChar == '-' && isIdentStart(aSecondChar));
    }

    bool isIdent(char32_t code)
    {
        return (code >= 256 || (kLexTable[code] & IS_IDENT) != 0);
    }

    void pushback()
    {
        m_pos--;
    }

    /*
    // unused method
    CSSToken* nextHexValue()
    {
        int c = read();
        if (c == -1 || !isHexDigit((char32_t)c)) {
            return CSSToken::createNullToken();
        }
        String* s = String::createUTF32String((char32_t)c);
        c = read();
        while (c != -1 && isHexDigit((char32_t)c)) {
            s = s->concat(String::createUTF32String((char32_t)c));
            c = read();
        }
        if (c != -1)
            pushback();
        return new CSSToken(CSSToken::HEX_TYPE, s);
    }*/

    // returns char32_t code
    int gatherEscape()
    {
        int c = peek();
        if (c == -1) {
            return -1;
        }
        if (isHexDigit((char32_t)c)) {
            int code = 0;
            size_t i;
            for (i = 0; i < 6; i++) {
                c = read();
                if (isDigit((char32_t)c)) {
                    code = code * 16 + (c - '0');
                } else if (isHexDigit((char32_t)c)) {
                    code = code * 16 + (towlower(c) - 'a' + 10);
                } else if (!isHexDigit((char32_t)c) &&
                           !isWhiteSpace((char32_t)c)) {
                    pushback();
                    break;
                } else {
                    break;
                }
            }
            if (i == 6) {
                c = peek();
                if (isWhiteSpace((char32_t)c))
                    read();
            }
            return code;
        }
        c = read();
        if (c != '\n') {
            return c;
        }
        return -1;
    }

    StringBuilder gatherIdent(int c)
    {
        StringBuilder builder;
        if (c == CSS_ESCAPE) {
            int code = gatherEscape();
            if (code != -1)
                builder.appendChar((char32_t)code);
        } else {
            builder.appendChar((char32_t)c);
        }
        c = read();
        while (c != -1 && (isIdent(c) || c == CSS_ESCAPE)) {
            if (c == CSS_ESCAPE) {
                int code = gatherEscape();
                if (code == -1) {
                    return StringBuilder();
                } else {
                    builder.appendChar((char32_t)code);
                }
            } else {
                builder.appendChar((char32_t)c);
            }
            c = read();
        }
        if (c != -1) {
            pushback();
        }
        return builder;
    }

    CSSToken* parseIdent(int c)
    {
        StringBuilder builder = gatherIdent(c);
        int nextChar = peek();
        if ((char32_t)nextChar == '(') {
            builder.appendChar((char32_t)read());
            return CSSToken::createStringValueToken(
                CSSToken::FUNCTION_TYPE, builder.finalize()->toLower());
        }
        return CSSToken::createStringValueToken(CSSToken::IDENT_TYPE,
                                                builder.finalize());
    }

    CSSToken* parseURL(int c)
    {
        StringBuilder builder;
        if (c == CSS_ESCAPE) {
            builder.appendChar((char32_t)gatherEscape());
        } else {
            builder.appendChar((char32_t)c);
        }
        c = read();
        while (c != -1 && c != ' ' && c != ')') {
            if (c == CSS_ESCAPE) {
                int code = gatherEscape();
                if (code == -1) {
                    return CSSToken::createStringValueToken(
                        CSSToken::STRING_TYPE, String::emptyString);
                } else {
                    builder.appendChar((char32_t)code);
                }
            } else {
                builder.appendChar((char32_t)c);
            }
            c = read();
        }
        if (c != -1) {
            pushback();
        }
        return CSSToken::createStringValueToken(CSSToken::STRING_TYPE,
                                                builder.finalize());
    }

    bool isDigit(char32_t c)
    {
        return (c >= '0') && (c <= '9');
    }

    CSSToken* parseComment(int c)
    {
        // StringBuilder s;
        // s.appendChar((char32_t)c);
        while ((c = read()) != -1) {
            // s.appendChar((char32_t)c);
            if (c == '*') {
                c = read();
                if (c == -1) {
                    break;
                }
                if (c == '/') {
                    // s.appendChar((char32_t)c);
                    break;
                }
                pushback();
            }
        }
        return CSSToken::createToken(CSSToken::COMMENT_TYPE);
    }

    CSSToken* parseNumber(int c)
    {
        StringBuilder s;
        s.appendChar((char32_t)c);
        bool foundDot = false;
        while ((c = read()) != -1) {
            if (c == '.') {
                if (foundDot) {
                    break;
                } else {
                    s.appendChar((char32_t)c);
                    foundDot = true;
                }
            } else if (isDigit(c)) {
                s.appendChar((char32_t)c);
            } else {
                break;
            }
        }

        if (c != -1 && startsWithIdent(c, peek())) { // DIMENSION
            String* unit = gatherIdent(c).finalize();
            String* ss = s.finalize();
            float f = String::parseFloat(ss);
            UnitType type = (UnitType)unit->peekUTF8Buffer(
                [](const char* buf, size_t len, void* data) -> size_t {
                    return lookupUnitType(buf, len);
                },
                nullptr);
            return CSSToken::createNumberValueToken(
                CSSToken::DIMENSION_TYPE, f, ss->concat(unit), type, foundDot,
                unit->equals("n"));
        } else if (c == '%') {
            String* ss = s.finalize();
            float f = String::parseFloat(ss);
            return CSSToken::createNumberValueToken(
                CSSToken::PERCENTAGE_TYPE, f, ss->concat("%"),
                UnitType::Percentage, foundDot, false);
        } else if (c != -1) {
            pushback();
        }

        String* ss = s.finalize();
        float f = String::parseFloat(ss);
        return CSSToken::createNumberValueToken(CSSToken::NUMBER_TYPE, f, ss,
                                                UnitType::UnknownType, foundDot,
                                                false);
    }

    CSSToken* parseString(int aStop)
    {
        StringBuilder s;
        s.appendChar((char32_t)aStop);
        int previousChar = aStop;
        int c;
        while ((c = read()) != -1) {
            if (c == aStop && previousChar != CSS_ESCAPE) {
                s.appendChar((char32_t)c);
                break;
            } else if (c == CSS_ESCAPE) {
                c = peek();
                if (c == -1) {
                    break;
                } else if (c == '\n' || c == '\r' || c == '\f') {
                    int d = c;
                    c = read();
                    // special for Opera that preserves \r\n...
                    if (d == '\r') {
                        c = peek();
                        if (c == '\n') {
                            c = read();
                        }
                    }
                } else {
                    s.appendChar((char32_t)gatherEscape());
                    c = peek();
                }
            } else if (c == '\n' || c == '\r' || c == '\f') {
                break;
            } else {
                s.appendChar((char32_t)c);
            }
            previousChar = c;
        }
        return CSSToken::createStringValueToken(CSSToken::STRING_TYPE,
                                                s.finalize());
    }

    bool isWhiteSpace(char32_t c)
    {
        char32_t code = c;
        return code < 256 && (kLexTable[code] & IS_WHITESPACE) != 0;
    }

    bool eatWhiteSpace(int c)
    {
        bool solo = true;
        while ((c = read()) != -1) {
            if (!isWhiteSpace(c)) {
                break;
            }
            solo = false;
        }
        if (c != -1) {
            pushback();
        }
        return solo;
    }

    CSSToken* parseAtKeyword(int c)
    {
        return CSSToken::createStringValueToken(CSSToken::ATRULE_TYPE,
                                                gatherIdent(c).finalize());
    }

    CSSToken* nextToken(bool isURL = false)
    {
        int c = read();
        if (c == -1) {
            return CSSToken::createNullToken();
        }

        // url starts without \' nor \"
        if (isURL && c != '\'' && c != '"' && c != ')' && c != ' ') {
            return parseURL(c);
        }

        if (startsWithIdent(c, peek())) {
            return parseIdent(c);
        }

        if (c == '@') {
            int nextChar = read();
            if (nextChar != -1) {
                int followingChar = peek();
                pushback();
                if (startsWithIdent(nextChar, followingChar)) {
                    return parseAtKeyword(c);
                }
            }
        }

        if (c == '<') {
            if (read() == '!') {
                if (read() == '-') {
                    if (read() == '-') {
                        return CSSToken::createToken(
                            CSSToken::SGML_COMMENT_TYPE);
                    }
                    pushback();
                }
                pushback();
            }
            pushback();
        }

        if (c == '-') {
            if (read() == '-') {
                if (read() == '>') {
                    return CSSToken::createToken(CSSToken::SGML_COMMENT_TYPE);
                }
                pushback();
            }
            pushback();
        }

        if (c == '.' || c == '+' || c == '-') {
            int nextChar = peek();
            if (isDigit(nextChar)) {
                return parseNumber(c);
            } else if (nextChar == '.' && c != '.') {
                // int firstChar = read();
                read();
                int secondChar = peek();
                pushback();
                if (isDigit(secondChar)) {
                    return parseNumber(c);
                }
            }
        }
        if (isDigit(c)) {
            return parseNumber(c);
        }

        if (c == '\'' || c == '"') {
            return parseString(c);
        }

        if (isWhiteSpace(c)) {
            bool solo = eatWhiteSpace(c);
            char32_t ch = solo ? ' ' : '\0';
            return CSSToken::createCharValueToken(CSSToken::WHITESPACE_TYPE,
                                                  ch);
        }

        if (c == '|' || c == '~' || c == '^' || c == '$' || c == '*') {
            int nextChar = read();
            if (nextChar == '=') {
                switch (c) {
                case '~':
                    return CSSToken::createToken(CSSToken::INCLUDES_TYPE);
                case '|':
                    return CSSToken::createToken(CSSToken::DASHMATCH_TYPE);
                case '^':
                    return CSSToken::createToken(CSSToken::BEGINSMATCH_TYPE);
                case '$':
                    return CSSToken::createToken(CSSToken::ENDSMATCH_TYPE);
                case '*':
                    return CSSToken::createToken(CSSToken::CONTAINSMATCH_TYPE);
                default:
                    break;
                }
            } else if (nextChar != -1) {
                pushback();
            }
        }

        if (c == '/' && peek() == '*') {
            return parseComment(c);
        }

        return CSSToken::createCharValueToken(CSSToken::SYMBOL_TYPE,
                                              (char32_t)c);
    }

protected:
    String* m_string;
    StringBufferAccessData m_stringBufferData;
    size_t m_pos;
    GCAtomicVector<size_t> m_preservedPos;
};

CSSToken* CSSParser::getToken(bool aSkipWS, bool aSkipComment, bool isURL)
{
    if (m_lookAhead) {
        m_token = m_lookAhead;
        m_lookAhead = nullptr;
        return m_token;
    }

    m_token = m_scanner->nextToken(isURL);
    while (m_token && ((aSkipWS && m_token->isWhiteSpace()) ||
                       (aSkipComment && m_token->isComment()))) {
        m_token = m_scanner->nextToken(isURL);
    }
    return m_token;
}

CSSToken* CSSParser::currentToken()
{
    return m_token;
}

CSSToken* CSSParser::lookAhead(bool aSkipWS, bool aSkipComment)
{
    CSSToken* preservedToken = m_token;
    m_scanner->preserveState();
    CSSToken* token = getToken(aSkipWS, aSkipComment);
    m_scanner->restoreState();
    m_token = preservedToken;

    return token;
}

void CSSParser::ungetToken()
{
    m_lookAhead = m_token;
}

void CSSParser::preserveState()
{
    m_preservedTokens.push_back(currentToken());
    m_scanner->preserveState();
}

void CSSParser::restoreState()
{
    if (m_preservedTokens.size()) {
        m_scanner->restoreState();
        m_token = m_preservedTokens.back();
        m_preservedTokens.pop_back();
    }
}

void CSSParser::forgetState()
{
    if (m_preservedTokens.size()) {
        m_scanner->forgetState();
        m_preservedTokens.pop_back();
    }
}

void CSSParser::parseSelector(GCVector<GCDeque<CSSSelector*>*>& list,
                              bool& validSelector)
{
    m_failedParsing = false;
    validSelector = parseComplexSelectorList(list);
    if (!validSelector) {
        list.clear();
    }
}

CSSSelector::RelationType CSSParser::parseCombinator()
{
    CSSSelector::RelationType fallbackResult =
        CSSSelector::RelationType::SubSelector;

    CSSToken* token = currentToken();
    while (token->isWhiteSpace()) {
        token = getToken(true, true);
        fallbackResult = CSSSelector::RelationType::Descendant;
    }

    if (token->isSymbol('+')) {
        token = getToken(true, true);
        return CSSSelector::RelationType::AdjacentSibling;
    } else if (token->isSymbol('~')) {
        token = getToken(true, true);
        return CSSSelector::RelationType::GeneralSibling;
    } else if (token->isSymbol('>')) {
        token = getToken(true, true);
        return CSSSelector::RelationType::Child;
    } else {
        return fallbackResult;
    }
}

String* CSSParser::determineNamespace(String* prefix)
{
    if (prefix == nullptr) {
        return String::emptyString;
    }
    if (prefix->equals(String::emptyString)) {
        return String::emptyString; // No namespace. If an element/attribute has
                                    // a namespace, we won't match it.
    }
    if (prefix->equals(String::fromUTF8("*"))) {
        return String::fromUTF8("*"); // We'll match any namespace.
    }

    if (m_document->styleResolver().sheets().size() == 0) {
        return nullptr; // Cannot resolve prefix to namespace without a
                        // stylesheet, syntax error.
    }

    // TODO: Implement logic for getting namespace uri from prefix in stylesheet
    // return m_styleSheet->namespaceURIFromPrefix(prefix);
    return String::emptyString;
}

CSSSelector* CSSParser::getPseudoSelector()
{
    int colons = 1;

    CSSToken* token = getToken(false, true);
    if (token->isSymbol(':')) {
        token = getToken(false, true);
        colons++;
    }

    if (!token->isIdent() && !token->isFunction()) {
        return nullptr;
    }

    if (token->isIdent() && token->value()->indexOf('(') != SIZE_MAX) {
        return nullptr;
    }

    auto type = colons == 1 ? CSSSelector::Type::PseudoClass
                            : CSSSelector::Type::PseudoElement;
    auto relType = CSSSelector::RelationType::SubSelector;
    CSSPseudoSelector* selector = new CSSPseudoSelector(type, relType);

    selector->updatePseudoType(
        starFish(), token->toAttrAtomicString(starFish()), token->isFunction());

    if (token->isIdent()) {
        if (selector->pseudoType() == CSSSelector::PseudoNone) {
            return nullptr;
        }
        token = getToken(true, true);
        return selector;
    }

    if (selector->pseudoType() == CSSSelector::PseudoNone) {
        return nullptr;
    }

    getToken(true, true);

    switch (selector->pseudoType()) {
    case CSSSelector::PseudoNot: {
        GCDeque<CSSSelector*> selectorList;
        parseCompoundSelector(&selectorList);

        if (selectorList.size() != 1) {
            return nullptr;
        }

        CSSSelector* innerSelector = selectorList[0];
        if ((innerSelector->isPseudoSelector() &&
             innerSelector->asCSSPseudoSelector()
                 ->pseudoSelectorList()
                 .size()) ||
            innerSelector->type() == CSSSelector::PseudoElement) {
            return nullptr;
        }

        selector->setPseudoSelectorList(innerSelector);
        getToken(false, true);

        return selector;
    }
    case CSSSelector::PseudoLang: {
        CSSToken* token = currentToken();
        if (!token->isIdent()) {
            return nullptr;
        }

        selector->setArgument(token->toStringValue());
        token = getToken(true, true);
        if (!token->isSymbol(')')) {
            return nullptr;
        }
        getToken(false, true);

        return selector;
    }
    case CSSSelector::PseudoNthChild:
    case CSSSelector::PseudoNthLastChild:
    case CSSSelector::PseudoNthOfType:
    case CSSSelector::PseudoNthLastOfType: {
        std::pair<int, int> ab;

        if (!getANPlusB(ab)) {
            return nullptr;
        }
        token = getToken(true, true);
        if (!token->isSymbol(')')) {
            return nullptr;
        }
        getToken(false, true);

        selector->setNth(ab.first, ab.second);

        return selector;
    }
    default:
        break;
    }

    return nullptr;
}

bool CSSParser::getANPlusB(std::pair<int, int>& result)
{
    CSSToken* token = currentToken();

    // in case of only number
    if (token->isNumber() && !token->hasSourceOfNumberValueDot()) {
        result = std::make_pair(0, (int)token->numericValue());
        return true;
    }

    // in case of string (odd and even)
    if (token->isIdent()) {
        if (token->value()->equalsWithoutCase("odd")) {
            result = std::make_pair(2, 1);
            return true;
        }
        if (token->value()->equalsWithoutCase("even")) {
            result = std::make_pair(2, 0);
            return true;
        }
    }

    String* nString = String::emptyString;

    // in case of 'an + b'
    if (token->isSymbol('+') && lookAhead(false, true)->isIdent()) { // +n
        result.first = 1;
        nString = getToken(false, true)->toStringValue();
    } else if (token->isDimension() &&
               !token->hasSourceOfNumberValueDot()) { // an+b
        result.first = token->numericValue();
        size_t pos = token->numericValueSource()->find("n");
        if (pos < 0) {
            return false;
        }
        nString = token->numericValueSource()->substring(
            pos, token->numericValueSource()->length() - pos);
    } else if (token->isIdent()) {              // -n or n
        if (token->value()->charAt(0) == '-') { // -n
            result.first = -1;
            nString = token->value()->substring(1, 1);
        } else { // n
            result.first = 1;
            nString = token->value();
        }
    }

    while (lookAhead(false, true)->isWhiteSpace()) {
        token = getToken(false, true);
    }

    if (nString->equals(String::emptyString) ||
        nString->toLower()->charAt(0) != 'n') {
        return false;
    }
    if (nString->length() > 1 && nString->charAt(1) != '-') {
        return false;
    }
    if (nString->length() > 2) {
        // TODO: return result after checking whether nString is valid.
        result.second =
            String::parseInt(nString->substring(1, nString->length() - 1));
        return true;
    }

    NumericSign sign = nString->length() == 1 ? NoSign : MinusSign;
    if (sign == NoSign && lookAhead(false, true)->isSymbol() &&
        !lookAhead(false, true)->isSymbol(')')) {
        token = getToken(true, true);
        if (token->isSymbol('+')) {
            sign = PlusSign;
            CSSToken* ahead = lookAhead(false, true);
            if (ahead->hasStringValue() && (ahead->value()->charAt(0) == '+' ||
                                            ahead->value()->charAt(0) == '-')) {
                return false;
            }
        } else if (token->isSymbol('-')) {
            CSSToken* ahead = lookAhead(false, true);
            if (ahead->hasStringValue() && (ahead->value()->charAt(0) == '+' ||
                                            ahead->value()->charAt(0) == '-')) {
                return false;
            }
            sign = MinusSign;
        } else {
            return false;
        }
        while (lookAhead(false, true)->isWhiteSpace()) {
            token = getToken(false, true);
        }
    }

    if (sign == NoSign && !lookAhead(false, true)->isNumber()) {
        result.second = 0;
        return true;
    }

    CSSToken* b = getToken(false, true);
    if (!b->isNumber() || b->toStringValue()->contains(".")) {
        return false;
    }
    /*
        if ((b.numericSign() == NoSign) == (sign == NoSign)) {
            return false;
        }
    */
    if (!b->isNumber())
        result.second = String::parseInt(b->toStringValue());
    else
        result.second = b->numericValue();
    if (sign == MinusSign) {
        result.second = -result.second;
    }
    return true;
}

CSSSelector::Type CSSParser::getAttributeMatch(CSSToken* token)
{
    if (token->isIncludes()) {
        return CSSSelector::AttributeList;
    } else if (token->isDashmatch()) {
        return CSSSelector::AttributeHyphen;
    } else if (token->isBeginsmatch()) {
        return CSSSelector::AttributeBegin;
    } else if (token->isEndsmatch()) {
        return CSSSelector::AttributeEnd;
    } else if (token->isContainsmatch()) {
        return CSSSelector::AttributeContain;
    } else if (token->isSymbol('=')) {
        return CSSSelector::AttributeExact;
    } else {
        m_failedParsing = true;
        return CSSSelector::AttributeExact;
    }
}

CSSSelector::AttributeMatchType CSSParser::getAttributeFlags()
{
    if (!lookAhead(false, true)->isIdent()) {
        return CSSSelector::CaseSensitive;
    }
    CSSToken* flag = getToken(true, true);
    if (flag->toStringValue()->equalsWithoutCase("i")) {
        return CSSSelector::CaseInsensitive;
    }
    m_failedParsing = true;
    return CSSSelector::CaseSensitive;
}

String* CSSParser::getStringWithoutQuotationMarks(String* value)
{
    const char* curPos = value->utf8Data();
    const char* endPos = curPos + value->length();

    while (String::isSpaceOrNewline(*curPos) && curPos < endPos) {
        curPos++;
    }

    int len = 0;
    char mark = '\0';
    if (*curPos == '\\') {
        curPos++;
    }
    if (*curPos == '"' || *curPos == '\'') {
        mark = *curPos;
        curPos++;
    }
    const char* start = curPos;
    while (*curPos != mark && curPos < endPos) {
        curPos++;
        len++;
    }
    if (mark != '\0' && mark == *curPos) {
        if (*(curPos - 1) == '\\') {
            len--;
        }
        curPos++;
        while (String::isSpaceOrNewline(*curPos) && curPos < endPos) {
            curPos++;
        }
    }

    return String::fromUTF8(start, len);
}

CSSSelector* CSSParser::getAttributeSelector()
{
    CSSToken* token = getToken(true, true);

    String* attributeName = nullptr;
    if (!parseName(&attributeName)) {
        return nullptr;
    }

    while (currentToken()->isWhiteSpace()) {
        getToken(false, true);
    }

    QualifiedName attrQualifiedName = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAttrAtomicString(starFish(), attributeName));

    if (currentToken()->isSymbol(']')) {
        getToken(true, false);
        return new CSSAttributeSelector(
            CSSSelector::Type::AttributeSet, attrQualifiedName,
            String::emptyString, CSSSelector::AttributeMatchType::CaseSensitive,
            CSSSelector::RelationType::SubSelector);
    }

    auto type = getAttributeMatch(currentToken());

    CSSToken* attributeValue = getToken(true, true);
    if (!attributeValue->isIdent() && !attributeValue->isString()) {
        return nullptr;
    }

    token = getToken(true, false);
    getToken(false, false);

    if (!token->isSymbol(']')) {
        return nullptr;
    }

    return new CSSAttributeSelector(
        type, attrQualifiedName,
        getStringWithoutQuotationMarks(attributeValue->toStringValue()),
        getAttributeFlags(), CSSSelector::RelationType::SubSelector);
}

CSSSelector* CSSParser::getClassSelector()
{
    CSSToken* token = getToken(false, true);
    if (!token->isIdent()) {
        return nullptr;
    }

    CSSSelector* selector = new (PointerFreeGC) CSSSelector(
        CSSSelector::Type::Class, CSSSelector::SubSelector,
        AtomicString::createAtomicString(starFish(), token->value()));
    getToken(false, true);

    return selector;
}

CSSSelector* CSSParser::getIdSelector()
{
    CSSToken* token = getToken(false, true);
    if (!token->isIdent()) {
        return nullptr;
    }

    CSSSelector* selector = new (PointerFreeGC) CSSSelector(
        CSSSelector::Type::Id, CSSSelector::SubSelector,
        AtomicString::createAtomicString(starFish(), token->value()));
    getToken(false, true);

    return selector;
}

CSSSelector* CSSParser::getSimpleSelector()
{
    CSSToken* token = currentToken();
    CSSSelector* selector;
    if (token->isSymbol('#')) {
        selector = getIdSelector();
    } else if (token->isSymbol('.')) {
        selector = getClassSelector();
    } else if (token->isSymbol('[')) {
        selector = getAttributeSelector();
    } else if (token->isSymbol(':')) {
        selector = getPseudoSelector();
    } else {
        return nullptr;
    }

    if (!selector) {
        m_failedParsing = true;
    }

    return selector;
}

bool CSSParser::parseName(String** name)
{
    CSSToken* firstToken = currentToken();
    if (firstToken->isIdent()) {
        *name = firstToken->value();
        getToken(false, true);
    } else if (firstToken->isSymbol('*')) {
        *name = String::fromUTF8("*");
        getToken(false, true);
    } else if (firstToken->isSymbol('|')) {
        *name = String::emptyString;
    } else {
        return false;
    }

    if (!firstToken->isSymbol('|')) {
        return true;
    }

    CSSToken* nameToken = getToken(true, true);
    if (nameToken->isIdent()) {
        *name = nameToken->value();
    } else if (nameToken->isSymbol('*')) {
        *name = String::fromUTF8("*");
    } else {
        *name = nullptr;
        return false;
    }

    return true;
}

void CSSParser::parseCompoundSelector(GCDeque<CSSSelector*>* selectorList)
{
    CSSSelector* compoundSelector;

    String* elementName = nullptr;
    CSSSelector::PseudoType compoundPseudoElement = CSSSelector::PseudoNone;
    if (!parseName(&elementName)) {
        compoundSelector = getSimpleSelector();

        if (!compoundSelector) {
            return;
        }
        if (compoundSelector->type() == CSSSelector::PseudoElement) {
            compoundPseudoElement =
                compoundSelector->asCSSPseudoSelector()->pseudoType();
        }

        selectorList->push_back(compoundSelector);
    }

    while (CSSSelector* simpleSelector = getSimpleSelector()) {
        if (compoundPseudoElement != CSSSelector::PseudoNone) {
            m_failedParsing = true;
            return;
        }

        if (simpleSelector->type() == CSSSelector::PseudoElement) {
            compoundPseudoElement =
                simpleSelector->asCSSPseudoSelector()->pseudoType();
        }

        selectorList->push_back(simpleSelector);
    }

    if (selectorList->size() > 0) {
        (*selectorList)[selectorList->size() - 1]->updateRelation(
            CSSSelector::None);
    }

    if (elementName) {
        bool isStar = elementName->equals("*");
        if (isStar && selectorList->size() > 0) {
            return;
        }

        CSSSelector* selector = new CSSSelector(
            isStar ? CSSSelector::Type::Universal : CSSSelector::Type::Tag,
            CSSSelector::RelationType::SubSelector,
            AtomicString::createAttrAtomicString(starFish(), elementName));
        if (selectorList->size() == 0) {
            selector->updateRelation(CSSSelector::None);
        }

        selectorList->push_front(selector);
    }
}

enum CompoundSelectorFlags {
    HasPseudoElementForRightmostCompound = 1 << 0,
    HasContentPseudoElement = 1 << 1
};

unsigned CSSParser::extractCompoundFlags(CSSSelector* simpleSelector)
{
    if (simpleSelector->type() != CSSSelector::PseudoElement) {
        return 0;
    }
    return HasPseudoElementForRightmostCompound;
}

void CSSParser::parseComplexSelector(GCDeque<CSSSelector*>* selectorList)
{
    CSSToken* token = currentToken();
    while (token->isSGMLComment() || token->isWhiteSpace()) {
        token = getToken(false, true);
    }

    parseCompoundSelector(selectorList);

    unsigned selectorSize = selectorList->size();
    if (selectorSize == 0) {
        return;
    }

    unsigned previousCompoundFlags = 0;
    for (unsigned i = 0; i < selectorSize; i++) {
        previousCompoundFlags |= extractCompoundFlags((*selectorList)[i]);
        if (previousCompoundFlags) {
            break;
        }
    }

    if (m_failedParsing) {
        return;
    }

    GCDeque<CSSSelector*> secondSelectorList;

    while (CSSSelector::RelationType combinator = parseCombinator()) {
        secondSelectorList.clear();
        parseCompoundSelector(&secondSelectorList);

        if (secondSelectorList.size() == 0) {
            return;
        }

        if (previousCompoundFlags & HasPseudoElementForRightmostCompound) {
            m_failedParsing = true;
        }
        if (m_failedParsing) {
            return;
        }

        unsigned i = 0;
        CSSSelector* end = secondSelectorList[i];
        unsigned compoundFlags = extractCompoundFlags(end);
        selectorSize = secondSelectorList.size();

        while (++i < selectorSize) {
            end = secondSelectorList[i];
            compoundFlags |= extractCompoundFlags(end);
        }
        end->updateRelation(combinator);

        if (previousCompoundFlags & HasContentPseudoElement) {
            end->relationIsAffectedByPseudoContent();
        }
        previousCompoundFlags = compoundFlags;
        selectorList->insert(selectorList->begin(), secondSelectorList.begin(),
                             secondSelectorList.end());
    }
}

bool CSSParser::parseComplexSelectorList(
    GCVector<GCDeque<CSSSelector*>*>& listOfSelectorList)
{
    GCDeque<CSSSelector*>* selectorList = new (GC) GCDeque<CSSSelector*>();
    parseComplexSelector(selectorList);

    if (selectorList->size() == 0) {
        return false;
    }

    listOfSelectorList.push_back(selectorList);

    CSSToken* token = currentToken();
    while (token->isNotNull() && token->isSymbol(',')) {
        do {
            token = getToken(false, true);
        } while (token->isSGMLComment() || token->isWhiteSpace());

        GCDeque<CSSSelector*>* nextSelectorList =
            new (GC) GCDeque<CSSSelector*>();
        parseComplexSelector(nextSelectorList);
        if (nextSelectorList->size() == 0) {
            return false;
        }

        listOfSelectorList.push_back(nextSelectorList);
        token = currentToken();
    }

    if (m_failedParsing) {
        return false;
    }

    return true;
}

String* CSSParser::parseDefaultPropertyValue(CSSToken* token)
{
    GCVector<CSSToken*> willBeConcat;
    GCVector<String*> blocks;
    // bool foundPriority = false;
    GCVector<String*> values;
    bool isURLFunc = false;
    int urlTokens = 0;
    while (token->isNotNull()) {
        if ((token->isSymbol(';') || token->isSymbol('}') ||
             token->isSymbol('!')) &&
            !blocks.size()) {
            if (token->isSymbol('}') && willBeConcat.size() > 0) {
                ungetToken();
            }
            break;
        }
        if (token->isIdent("inherit")) {
            /*
            if (values.size()) {
                return;
            } else {
                valueText = String::createASCIIString("inherit");
                // var value = new jscsspVariable(kJscsspINHERIT_VALUE, aSheet);
                // values.push_back(value);
                values.push_back(valueText);
                token = getToken(true, true);
                break;
            }*/
            if (willBeConcat.size() > 0) {
                return combineAndTrimTokenValues(willBeConcat);
            } else {
                willBeConcat.clear();
                willBeConcat.push_back(token);
                token = getToken(true, true);
                break;
            }
        } else if (token->isSymbol('{') || token->isSymbol('(') ||
                   token->isSymbol('[') || token->isFunction()) {
            if (token->isFunction() && token->value()->equals("url(")) {
                blocks.push_back(token->toStringValue());
                isURLFunc = true;
            } else {
                blocks.push_back(token->isFunction()
                                     ? String::createASCIIString("(")
                                     : token->toStringValue());
            }
        } else if (token->isSymbol('}') || token->isSymbol(')') ||
                   token->isSymbol(']')) {
            if (blocks.size()) {
                String* ontop = blocks[blocks.size() - 1];
                if ((token->isSymbol('}') && ontop->equals("{")) ||
                    (token->isSymbol(')') && ontop->equals("(")) ||
                    (token->isSymbol(']') && ontop->equals("["))) {
                    blocks.pop_back();
                } else if (token->isSymbol(')') &&
                           ontop->equalsWithoutCase("url(")) {
                    blocks.pop_back();
                    if (urlTokens > 2) {
                        return String::emptyString;
                    }
                    isURLFunc = false;
                    urlTokens = 0;
                } else {
                    return String::emptyString;
                }
            } else {
                return String::emptyString;
            }
        }

        willBeConcat.push_back(token);
        if (isURLFunc) {
            token = getToken(true, false, true);
            urlTokens++;
        } else {
            token = getToken(false, false);
        }
    }
    /*
    if (values.length && valueText) {
        this.forgetState();
        aDecl.push(this._createJscsspDeclarationFromValuesArray(descriptor,
                   values, valueText));
        return valueText;
    }*/
    if (willBeConcat.size() > 0) {
        forgetState();
    }
    return combineAndTrimTokenValues(willBeConcat);
}

// Remove comments from both sides of a tokenList & Concat
String* CSSParser::combineAndTrimTokenValues(const GCVector<CSSToken*>& list)
{
    StringBuilder result;
    for (CSSToken* item : list) {
        result.appendString(item->toStringValue());
    }
    return result.finalize();
}

void CSSParser::parseDeclaration(CSSToken* aToken,
                                 CSSStyleDeclaration* declaration)
{
    preserveState();
    GCVector<String*> blocks;
    if (aToken->isIdent()) {
        CSSToken* token = getToken(true, true);
        if (token->isSymbol(':')) {
            token = getToken(true, true);
            String* value = String::emptyString;
            value = parseDefaultPropertyValue(token);
            token = currentToken();
            if (value->length()) { // no error above
                bool priority = false;
                if (token->isSymbol('!')) {
                    token = getToken(true, true);
                    if (token->isIdent("important")) {
                        priority = true;
                        token = getToken(true, true);
                        if (token->isSymbol(';') || token->isSymbol('}') ||
                            token->type() == CSSToken::NULL_TYPE) {
                            if (token->isSymbol('}')) {
                                ungetToken();
                            }
                        } else {
                            return;
                        }
                    } else {
                        return;
                    }
                } else if (token->isNotNull() && !token->isSymbol(';') &&
                           !token->isSymbol('}')) {
                    return;
                }
                // use decls
                /*
                for (size_t i = 0; i < declarations.length; i++) {
                  declarations[i].priority = priority;
                  aDecl.push(declarations[i]);
                }
                return descriptor + ": " + value + ";";
                */

                String* descriptor = aToken->value();
                struct Sender {
                    bool priority;
                    String* value;
                    CSSStyleDeclaration* declaration;
                } sender;
                sender.value = value;
                sender.priority = priority;
                sender.declaration = declaration;
                descriptor->peekUTF8Buffer(
                    [](const char* buf, size_t len, void* data) -> size_t {
                        String* value = ((Sender*)data)->value;
                        bool priority = ((Sender*)data)->priority;
                        CSSStyleDeclaration* declaration =
                            ((Sender*)data)->declaration;
                        char* name = ALLOCA(len, char);
                        for (size_t i = 0; i < len; i++) {
                            name[i] = tolower(buf[i]);
                        }
                        CSSStyleKind kind = lookupCSSStyle(name, len);

                        if (false) {
                        }
#define SET_ATTR(name, nameLower, nameCSSCase)   \
    else if (kind == CSSStyleKind::name)         \
    {                                            \
        declaration->set##name(value, priority); \
    }
                        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
                        else
                        {
                            /*
                            STARFISH_LOG_ERROR(
                                "unsupported property name(CSSParser) -> %s\n",
                                buf);
                                */
                        }
                        return 0;
                    },
                    &sender);
                return;
            }
        }
    } else if (aToken->isComment()) {
        /*
        if (this.mPreserveComments) {
            this.forgetState();
            var comment = new jscsspComment();
            comment.parsedCssText = aToken.value;
            aDecl.push(comment);
        }
        return aToken.value;
        */
        return;
    }

    // we have an error here, let's skip it
    restoreState();
    blocks.clear();
    CSSToken* token = aToken;
    bool isURLFunc = false;

    while (token->isNotNull()) {
        if (token->isSymbol(';')) {
            break;
        } else if (token->isSymbol('}') && !blocks.size()) {
            ungetToken();
            break;
        } else if (token->isSymbol('{') || token->isSymbol('(') ||
                   token->isSymbol('[') || token->isFunction()) {
            if (token->isFunction() &&
                token->value()->equalsWithoutCase("url(")) {
                blocks.push_back(token->value());
                isURLFunc = true;
            } else {
                blocks.push_back(token->isFunction()
                                     ? String::createASCIIString("(")
                                     : token->toStringValue());
            }
        } else if (token->isSymbol('}') || token->isSymbol(')') ||
                   token->isSymbol(']')) {
            if (blocks.size()) {
                String* ontop = blocks[blocks.size() - 1];
                if ((token->isSymbol('}') && ontop->equals("{")) ||
                    (token->isSymbol(')') && ontop->equals("(")) ||
                    (token->isSymbol(']') && ontop->equals("["))) {
                    blocks.pop_back();
                } else if (token->isSymbol(')') &&
                           ontop->equalsWithoutCase("url(")) {
                    blocks.pop_back();
                    isURLFunc = false;
                }
            }
        }
        if (isURLFunc) {
            token = getToken(true, false, true);
        } else {
            token = getToken(false, false);
        }
    }
    return;
}

bool CSSParser::parseStyleRule(CSSToken* aToken, GCVector<CSSRule*>& rules,
                               AllowedRulesType allowedRules,
                               GCVector<GCDeque<CSSSelector*>*>* sList,
                               bool isQueryingSelector)
{
    if (allowedRules > RegularRules) {
        return false;
    }

    // size_t currentLine = countLF(m_scanner->getAlreadyScanned());
    preserveState();
    // first let's see if we have a selector here...
    bool validSelector = true;

    GCVector<GCDeque<CSSSelector*>*> list;
    parseSelector(list, validSelector);

    bool valid = false;
    CSSStyleDeclaration* declarations = new CSSStyleDeclaration();
    if (list.size()) {
        CSSToken* token = currentToken();
        if (token->isSymbol('{')) {
            CSSToken* token = getToken(true, false);
            while (true) {
                if (!token->isNotNull()) {
                    valid = true;
                    break;
                }
                if (token->isSymbol('}')) {
                    valid = true;
                    break;
                } else {
                    parseDeclaration(token, declarations);
                }
                token = getToken(true, false);
            }
        } else if (isQueryingSelector) {
            valid = true;
        }
    } else if (!validSelector) {
        if (isQueryingSelector) {
            return false;
        } else {
            // selector is invalid so the whole rule is invalid with it
            CSSToken* token = getToken(true, true);
            while (!token->isSymbol('{') && token->isNotNull()) {
                token = getToken(true, false);
            }
            if (token->isSymbol('{')) {
                token = getToken(true, false);
            }
            while (true) {
                if (!token->isNotNull()) {
                    return false;
                }
                if (token->isSymbol('}')) {
                    return false;
                } else {
                    parseDeclaration(token, declarations);
                }
                token = getToken(true, false);
            }
        }
    }

    if (valid) {
        if (isQueryingSelector) {
            sList->assign(list.begin(), list.end());
        } else {
            unsigned size = list.size();
            for (unsigned i = 0; i < size; ++i) {
                rules.push_back(new CSSStyleRule(list[i], declarations));
            }
        }
        return true;
    }
    restoreState();
    addUnknownAtRule();

    return false;
}

void CSSParser::addUnknownAtRule()
{
    GCVector<String*> blocks;
    CSSToken* token = getToken(false, false);
    while (token->isNotNull()) {
        if (token->isSymbol(';') && !blocks.size()) {
            break;
        } else if (token->isSymbol('{') || token->isSymbol('(') ||
                   token->isSymbol('[') || token->isFunction()) {
            blocks.push_back(token->isFunction()
                                 ? String::createASCIIString("(")
                                 : token->toStringValue());
        } else if (token->isSymbol('}') || token->isSymbol(')') ||
                   token->isSymbol(']')) {
            if (blocks.size()) {
                String* ontop = blocks[blocks.size() - 1];
                if ((token->isSymbol('}') && ontop->equals("{")) ||
                    (token->isSymbol(')') && ontop->equals("(")) ||
                    (token->isSymbol(']') && ontop->equals("["))) {
                    blocks.pop_back();
                    if (!blocks.size() && token->isSymbol('}')) {
                        break;
                    }
                }
            }
        }
        token = getToken(false, false);
    }
}

void CSSParser::reportError(const char* aMsg)
{
    m_error = String::createASCIIString(aMsg);
}

static CSSParser::AllowedRulesType computeNewAllowedRules(
    CSSParser::AllowedRulesType allowedRules, CSSRule* rule)
{
    if (!rule || allowedRules == CSSParser::KeyframeRules ||
        allowedRules == CSSParser::NoRules) {
        return allowedRules;
    }
    STARFISH_ASSERT(allowedRules <= CSSParser::RegularRules);
    if (rule->isCharsetRule() || rule->isImportRule()) {
        return CSSParser::AllowImportRules;
    }
    if (rule->isNamespaceRule()) {
        return CSSParser::AllowNamespaceRules;
    }
    return CSSParser::RegularRules;
}

bool CSSParser::parseCharsetRule(GCVector<CSSRule*>& rules)
{
    CSSToken* token = getToken(false, false);
    StringBuilder s;
    if (token->isAtRule("@charset") &&
        token->value()->equals("@charset")) { // lowercase check
        s.appendString(token->value());
        token = getToken(false, false);
        s.appendString(token->toStringValue());
        if (token->isWhiteSpace(' ')) {
            token = getToken(false, false);
            s.appendString(token->toStringValue());
            if (token->isString()) {
                // String* encoding = token->m_value;
                token = getToken(false, false);
                s.appendString(token->toStringValue());
                if (token->isSymbol(';')) {
                    // var rule = new jscsspCharsetRule();
                    // rule.encoding = encoding;
                    // rule.parsedCssText = s;
                    // rule.parentStyleSheet = aSheet;
                    // aSheet.cssRules.push(rule);
                    return true;
                } else {
                    reportError(kCHARSET_RULE_MISSING_SEMICOLON);
                }
            } else {
                reportError(kCHARSET_RULE_CHARSET_IS_STRING);
            }
        } else {
            reportError(kCHARSET_RULE_MISSING_WS);
        }
    }

    addUnknownAtRule();
    return false;
}

CSSToken* CSSParser::makeToken(String* str)
{
    m_lookAhead = nullptr;
    m_token = nullptr;
    m_preserveWS = false;
    m_preserveComments = false;
    m_scanner = new CSSScanner(str);

    return getToken(false, false);
}

CSSMediaRule* CSSParser::parseMediaRule()
{
    preserveState();

    CSSToken* token = getToken(true, true);

    bool hasMediaRule = false;
    MediaQuerySet* mediaQuerySet;
    if (token->isNotNull()) {
        mediaQuerySet = parseMediaQuery();
        hasMediaRule = true;
    } else {
        return nullptr;
    }

    token = currentToken();
    if (token->isSymbol('}') || token->isSymbol(';')) {
        return nullptr;
    }

    bool valid = false;
    GCVector<CSSRule*> rootRule;
    if (token->isSymbol('{') && hasMediaRule) {
        token = getToken(true, false);
        if (token->isNotNull()) {
            parseRules(token, rootRule, RuleListType::RegularRuleList);
            valid = rootRule.size() > 0;
        } else {
            return nullptr;
        }
    }

    if (valid) {
        forgetState();
        return new CSSMediaRule(mediaQuerySet, rootRule);
    }
    restoreState();
    return nullptr;
}

CSSImportRule* CSSParser::parseImportRule()
{
    String* url = parseURLString();
    if (url->equals(String::emptyString)) {
        return nullptr;
    }

    getToken(true, false);
    MediaQuerySet* mediaQuery = parseMediaQuery();

    return new CSSImportRule(url, mediaQuery);
}

String* CSSParser::parseURLString()
{
    CSSToken* token = getToken(true, false);

    String* url = String::emptyString;
    if (token->isString()) {
        String* str = token->value();
        if (str->charAt(0) != str->charAt(str->length() - 1)) {
            return String::emptyString;
        }
        url = String::createASCIIString("url(");
        url = url->concat(str);
        url = url->concat(String::createASCIIString(")"));
    } else if (token->isFunction() && token->value()->equals("url(")) {
        url = token->value();
        url = url->concat(getToken(true, false)->value());
        url = url->concat(getToken(true, false)->value());
    } else {
        return String::emptyString;
    }

    String* ret = String::emptyString;
    CSSPropertyParser::parseUrl(url, &(ret));
    return ret;
}

void CSSParser::parseStyleSheet(String* sourceString, CSSStyleSheet* target)
{
    // @charset can only appear at first char of the stylesheet
    CSSToken* token = makeToken(sourceString);
    if (!token->isNotNull()) {
        return;
    }

    GCVector<CSSRule*> rules;
    if (token->isAtRule("@charset")) {
        ungetToken();
        parseCharsetRule(rules);
        token = getToken(false, false);
    }
    parseRules(token, rules, RuleListType::TopLevelRuleList);

    for (size_t i = 0; i < rules.size(); ++i) {
        target->addRule(rules[i]);
    }
}

void CSSParser::parseRules(CSSToken* token, GCVector<CSSRule*>& rootRule,
                           RuleListType ruleListType)
{
    AllowedRulesType allowedRules = AllowedRulesType::RegularRules;
    switch (ruleListType) {
    case TopLevelRuleList:
        allowedRules = AllowCharsetRules;
        break;
    case RegularRuleList:
        allowedRules = RegularRules;
        break;
    case KeyframesRuleList:
        allowedRules = KeyframeRules;
        break;
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }

    unsigned nestingLevel = 1;
    while (true) {
        if (!token->isNotNull()) {
            break;
        }

        if (token->isSymbol('{')) {
            nestingLevel++;
        } else if (token->isSymbol('}')) {
            if (--nestingLevel == 0) {
                break;
            }
        }

        if (token->isWhiteSpace()) {
        } else if (token->isComment()) {
        } else if (token->isAtRule()) {
            CSSRule* rule = nullptr;
            if (allowedRules <= AllowImportRules &&
                token->isAtRule("@import")) {
                rule = parseImportRule();
            } else if (token->isAtRule("@media")) {
                rule = parseMediaRule();
            }
            /*
             else if (token.isAtRule("@variables")) {
            } else if (token.isAtRule("@namespace")) {
            } else if (token.isAtRule("@font-face")) {
            } else if (token.isAtRule("@page")) {
            } else if (token.isAtRule("@keyframes")) {
            } else if (token.isAtRule("@charset")) {
            }*/

            if (rule) {
                allowedRules = computeNewAllowedRules(allowedRules, rule);
                rootRule.push_back(rule);
            } else {
                addUnknownAtRule();
            }
        } else {
            // plain style rules
            GCVector<CSSRule*> rules;
            if (parseStyleRule(token, rules, allowedRules, nullptr, false)) {
                allowedRules = computeNewAllowedRules(allowedRules, rules[0]);
                rootRule.insert(rootRule.end(), rules.begin(), rules.end());
            }
        }

        token = getToken(false, false);
    }
}

void CSSParser::parseStyleDeclaration(String* str,
                                      CSSStyleDeclaration* declarations)
{
    m_lookAhead = nullptr;
    m_token = nullptr;
    m_preserveWS = false;
    m_preserveComments = false;
    m_scanner = new CSSScanner(str);
    CSSToken* token = getToken(true, false);
    bool valid = false;
    while (true) {
        if (!token->isNotNull()) {
            valid = true;
            break;
        }
        parseDeclaration(token, declarations);
        token = getToken(true, false);
    }
}

const CSSParser::State CSSParser::ReadRestrictor = &CSSParser::readRestrictor;
const CSSParser::State CSSParser::ReadMediaNot = &CSSParser::readMediaNot;
const CSSParser::State CSSParser::ReadMediaType = &CSSParser::readMediaType;
const CSSParser::State CSSParser::ReadAnd = &CSSParser::readAnd;
const CSSParser::State CSSParser::ReadFeatureStart =
    &CSSParser::readFeatureStart;
const CSSParser::State CSSParser::ReadFeature = &CSSParser::readFeature;
const CSSParser::State CSSParser::ReadFeatureColon =
    &CSSParser::readFeatureColon;
const CSSParser::State CSSParser::ReadFeatureValue =
    &CSSParser::readFeatureValue;
const CSSParser::State CSSParser::ReadFeatureEnd = &CSSParser::readFeatureEnd;
const CSSParser::State CSSParser::SkipUntilComma = &CSSParser::skipUntilComma;
const CSSParser::State CSSParser::SkipUntilBlockEnd =
    &CSSParser::skipUntilBlockEnd;
const CSSParser::State CSSParser::Done = &CSSParser::done;

void CSSParser::initParseMediaQuery(MediaQueryParserType parserType)
{
    m_parserType = parserType;
    m_querySet = MediaQuerySet::create();
    if (parserType == MediaQuerySetParser)
        m_state = &CSSParser::readRestrictor;
    else // MediaConditionParser
        m_state = &CSSParser::readMediaNot;
}

void CSSParser::handleBlocks(CSSToken* token)
{
    if (!token->isSymbol('('))
        m_state = SkipUntilBlockEnd;
}

void CSSParser::processToken(CSSToken* token)
{
    // Call the function that handles current state
    if (!token->isWhiteSpace()) {
        ((this)->*(m_state))(token);
    }
}

MediaQuerySet* CSSParser::parseMediaQuery()
{
    initParseMediaQuery(MediaQuerySetParser);

    CSSToken* token = currentToken();
    while (token->isNotNull() && !token->isSymbol('{') && m_state != Done) {
        processToken(token);
        token = getToken(false, true);
    }

    if (m_state != ReadAnd && m_state != ReadRestrictor && m_state != Done &&
        m_state != ReadMediaNot)
        m_querySet->addMediaQuery(MediaQuery::createNotAll());
    else if (m_mediaQueryData.currentMediaQueryChanged())
        m_querySet->addMediaQuery(m_mediaQueryData.mediaQuery());

    return m_querySet;
}

void CSSParser::setStateAndRestrict(State state,
                                    MediaQuery::RestrictorType restrictor)
{
    m_mediaQueryData.setRestrictor(restrictor);
    m_state = state;
}

// State machine member functions start here
void CSSParser::readRestrictor(CSSToken* token)
{
    readMediaType(token);
}

void CSSParser::readMediaNot(CSSToken* token)
{
    if (token->isIdent() && token->value()->equalsWithoutCase("not"))
        setStateAndRestrict(ReadFeatureStart, MediaQuery::Not);
    else
        readFeatureStart(token);
}

static bool isRestrictorOrLogicalOperator(CSSToken* token)
{
    STARFISH_ASSERT(token->isIdent());
    String* val = token->value();
    return val->equalsWithoutCase("not") || val->equalsWithoutCase("and") ||
           val->equalsWithoutCase("or") || val->equalsWithoutCase("only");
}

void CSSParser::readMediaType(CSSToken* token)
{
    if (token->isSymbol('(')) {
        if (m_mediaQueryData.restrictor() != MediaQuery::None)
            m_state = SkipUntilComma;
        else
            m_state = ReadFeature;
    } else if (token->isIdent()) {
        if (m_state == ReadRestrictor &&
            token->value()->equalsWithoutCase("not")) {
            setStateAndRestrict(ReadMediaType, MediaQuery::Not);
        } else if (m_state == ReadRestrictor &&
                   token->value()->equalsWithoutCase("only")) {
            setStateAndRestrict(ReadMediaType, MediaQuery::Only);
        } else if (m_mediaQueryData.restrictor() != MediaQuery::None &&
                   isRestrictorOrLogicalOperator(token)) {
            m_state = SkipUntilComma;
        } else {
            m_mediaQueryData.setMediaType(token->value());
            m_state = ReadAnd;
        }
    } else if ((token->isSymbol('}') || token->isSymbol(';')) &&
               (!m_querySet->queryVector().size() ||
                m_state != ReadRestrictor)) {
        m_state = Done;
    } else {
        m_state = SkipUntilComma;
        if (token->isSymbol(','))
            skipUntilComma(token);
    }
}

void CSSParser::readAnd(CSSToken* token)
{
    if (token->isIdent() && token->value()->equalsWithoutCase("and")) {
        m_state = ReadFeatureStart;
    } else if (token->isSymbol(',') && m_parserType != MediaConditionParser) {
        m_querySet->addMediaQuery(m_mediaQueryData.mediaQuery());
        m_state = ReadRestrictor;
    } else if (token->isSymbol('}') || token->isSymbol(';')) {
        m_state = Done;
    } else {
        m_state = SkipUntilComma;
    }
}

void CSSParser::readFeatureStart(CSSToken* token)
{
    if (token->isSymbol('('))
        m_state = ReadFeature;
    else
        m_state = SkipUntilComma;
}

void CSSParser::readFeature(CSSToken* token)
{
    if (token->isIdent()) {
        m_mediaQueryData.setMediaFeature(token->value());
        m_state = ReadFeatureColon;
    } else {
        m_state = SkipUntilComma;
    }
}

void CSSParser::readFeatureColon(CSSToken* token)
{
    if (token->isSymbol(':'))
        m_state = ReadFeatureValue;
    else if (token->isSymbol(')') || token->isSymbol('}') ||
             token->isSymbol(';'))
        readFeatureEnd(token);
    else
        m_state = SkipUntilBlockEnd;
}

void CSSParser::readFeatureValue(CSSToken* token)
{
    if (token->isDimension() && token->unitType() == UnitType::UnknownType) {
        m_state = SkipUntilComma;
    } else {
        if (m_mediaQueryData.tryAddParserToken(token))
            m_state = ReadFeatureEnd;
        else
            m_state = SkipUntilBlockEnd;
    }
}

void CSSParser::readFeatureEnd(CSSToken* token)
{
    if (token->isSymbol(')') || token->isSymbol('}') || token->isSymbol(';')) {
        if (m_mediaQueryData.addExpression())
            m_state = ReadAnd;
        else
            m_state = SkipUntilComma;
    } else if (token->isSymbol('/')) {
        m_mediaQueryData.tryAddParserToken(token);
        m_state = ReadFeatureValue;
    } else {
        m_state = SkipUntilBlockEnd;
    }
}

void CSSParser::skipUntilComma(CSSToken* token)
{
    if ((token->isSymbol(',')) || token->isSymbol('}') ||
        token->isSymbol(';')) {
        m_state = ReadRestrictor;
        m_mediaQueryData.clear();
        m_querySet->addMediaQuery(MediaQuery::createNotAll());
    }
}

void CSSParser::skipUntilBlockEnd(CSSToken* token)
{
    if (token->isSymbol('}') || token->isSymbol(';'))
        m_state = SkipUntilComma;
}

void CSSParser::done(CSSToken* token)
{
}

MediaQueryData::MediaQueryData()
    : m_restrictor(MediaQuery::None)
    , m_mediaType(String::createASCIIString("all"))
    , m_mediaFeature(String::emptyString)
    , m_mediaTypeSet(false)
{
}

void MediaQueryData::clear()
{
    m_restrictor = MediaQuery::None;
    m_mediaType = String::createASCIIString("all");
    m_mediaTypeSet = false;
    m_mediaFeature = String::emptyString;
    m_valueList.clear();
    m_expressions.clear();
}

bool MediaQueryData::tryAddParserToken(CSSToken* token)
{
    if (token->isNumber() || token->isPercentage() || token->isDimension() ||
        token->isSymbol() || token->isIdent()) {
        m_valueList.push_back(token);
        return true;
    }

    return false;
}

void MediaQueryData::setMediaType(String* mediaType)
{
    m_mediaType = mediaType;
    m_mediaTypeSet = true;
}

MediaQuery* MediaQueryData::mediaQuery()
{
    MediaQuery* mediaQuery =
        MediaQuery::create(m_restrictor, m_mediaType, std::move(m_expressions));
    clear();
    return mediaQuery;
}

bool MediaQueryData::addExpression()
{
    MediaQueryExp* expression =
        MediaQueryExp::createIfValid(m_mediaFeature, m_valueList);
    bool isValid = !!expression;
    m_expressions.push_back(expression);
    m_valueList.clear();
    return isValid;
}

const String* devicePixelRatioMediaFeature =
    String::createASCIIStringWithNoGC("-webkit-device-pixel-ratio");
const String* maxDevicePixelRatioMediaFeature =
    String::createASCIIStringWithNoGC("-webkit-max-device-pixel-ratio");
const String* minDevicePixelRatioMediaFeature =
    String::createASCIIStringWithNoGC("-webkit-min-device-pixel-ratio");
const String* transform3dMediaFeature =
    String::createASCIIStringWithNoGC("-webkit-transform-3d");
const String* aspectRatioMediaFeature =
    String::createASCIIStringWithNoGC("aspect-ratio");
const String* colorMediaFeature = String::createASCIIStringWithNoGC("color");
const String* colorIndexMediaFeature =
    String::createASCIIStringWithNoGC("color-index");
const String* deviceAspectRatioMediaFeature =
    String::createASCIIStringWithNoGC("device-aspect-ratio");
const String* deviceHeightMediaFeature =
    String::createASCIIStringWithNoGC("device-height");
const String* deviceWidthMediaFeature =
    String::createASCIIStringWithNoGC("device-width");
const String* displayModeMediaFeature =
    String::createASCIIStringWithNoGC("display-mode");
const String* gridMediaFeature = String::createASCIIStringWithNoGC("grid");
const String* heightMediaFeature = String::createASCIIStringWithNoGC("height");
const String* maxAspectRatioMediaFeature =
    String::createASCIIStringWithNoGC("max-aspect-ratio");
const String* maxColorMediaFeature =
    String::createASCIIStringWithNoGC("max-color");
const String* maxColorIndexMediaFeature =
    String::createASCIIStringWithNoGC("max-color-index");
const String* maxDeviceAspectRatioMediaFeature =
    String::createASCIIStringWithNoGC("max-device-aspect-ratio");
const String* maxDeviceHeightMediaFeature =
    String::createASCIIStringWithNoGC("max-device-height");
const String* maxDeviceWidthMediaFeature =
    String::createASCIIStringWithNoGC("max-device-width");
const String* maxHeightMediaFeature =
    String::createASCIIStringWithNoGC("max-height");
const String* maxMonochromeMediaFeature =
    String::createASCIIStringWithNoGC("max-monochrome");
const String* maxResolutionMediaFeature =
    String::createASCIIStringWithNoGC("max-resolution");
const String* maxWidthMediaFeature =
    String::createASCIIStringWithNoGC("max-width");
const String* minAspectRatioMediaFeature =
    String::createASCIIStringWithNoGC("min-aspect-ratio");
const String* minColorMediaFeature =
    String::createASCIIStringWithNoGC("min-color");
const String* minColorIndexMediaFeature =
    String::createASCIIStringWithNoGC("min-color-index");
const String* minDeviceAspectRatioMediaFeature =
    String::createASCIIStringWithNoGC("min-device-aspect-ratio");
const String* minDeviceHeightMediaFeature =
    String::createASCIIStringWithNoGC("min-device-height");
const String* minDeviceWidthMediaFeature =
    String::createASCIIStringWithNoGC("min-device-width");
const String* minHeightMediaFeature =
    String::createASCIIStringWithNoGC("min-height");
const String* minMonochromeMediaFeature =
    String::createASCIIStringWithNoGC("min-monochrome");
const String* minResolutionMediaFeature =
    String::createASCIIStringWithNoGC("min-resolution");
const String* minWidthMediaFeature =
    String::createASCIIStringWithNoGC("min-width");
const String* monochromeMediaFeature =
    String::createASCIIStringWithNoGC("monochrome");
const String* orientationMediaFeature =
    String::createASCIIStringWithNoGC("orientation");
const String* resolutionMediaFeature =
    String::createASCIIStringWithNoGC("resolution");
const String* scanMediaFeature = String::createASCIIStringWithNoGC("scan");
const String* widthMediaFeature = String::createASCIIStringWithNoGC("width");

static inline bool featureWithoutValue(String* mediaFeature)
{
    // Media features that are prefixed by min/max cannot be used without a
    // value.
    return mediaFeature->equals(monochromeMediaFeature) ||
           mediaFeature->equals(colorMediaFeature) ||
           mediaFeature->equals(colorIndexMediaFeature) ||
           mediaFeature->equals(gridMediaFeature) ||
           mediaFeature->equals(heightMediaFeature) ||
           mediaFeature->equals(widthMediaFeature) ||
           mediaFeature->equals(deviceHeightMediaFeature) ||
           mediaFeature->equals(deviceWidthMediaFeature) ||
           mediaFeature->equals(orientationMediaFeature) ||
           mediaFeature->equals(aspectRatioMediaFeature) ||
           mediaFeature->equals(deviceAspectRatioMediaFeature) ||
           mediaFeature->equals(transform3dMediaFeature) ||
           mediaFeature->equals(devicePixelRatioMediaFeature) ||
           mediaFeature->equals(resolutionMediaFeature) ||
           mediaFeature->equals(displayModeMediaFeature) ||
           mediaFeature->equals(scanMediaFeature);
}

static inline bool featureWithValidIdent(const String* mediaFeature,
                                         const String* ident)
{
    if (mediaFeature->equals(displayModeMediaFeature)) {
        return ident->equalsWithoutCase(
                   String::createASCIIString("fullscreen")) ||
               ident->equalsWithoutCase(
                   String::createASCIIString("standalone")) ||
               ident->equalsWithoutCase(
                   String::createASCIIString("minimalui")) ||
               ident->equalsWithoutCase(String::createASCIIString("browser"));
    }

    if (mediaFeature->equals(orientationMediaFeature)) {
        return ident->equalsWithoutCase(
                   String::createASCIIString("portrait")) ||
               ident->equalsWithoutCase(String::createASCIIString("landscape"));
    }

    if (mediaFeature->equals(scanMediaFeature)) {
        return ident->equalsWithoutCase(
                   String::createASCIIString("interlace")) ||
               ident->equalsWithoutCase(
                   String::createASCIIString("progressive"));
    }

    return false;
}

static inline bool featureWithValidPositiveLength(String* mediaFeature,
                                                  CSSToken* token)
{
    if (!token->isLength() ||
        (token->isNumber() && token->numericValue() == 0) ||
        token->numericValue() < 0) {
        return false;
    }

    return mediaFeature->equals(heightMediaFeature) ||
           mediaFeature->equals(maxHeightMediaFeature) ||
           mediaFeature->equals(minHeightMediaFeature) ||
           mediaFeature->equals(widthMediaFeature) ||
           mediaFeature->equals(maxWidthMediaFeature) ||
           mediaFeature->equals(minWidthMediaFeature) ||
           mediaFeature->equals(deviceHeightMediaFeature) ||
           mediaFeature->equals(maxDeviceHeightMediaFeature) ||
           mediaFeature->equals(minDeviceHeightMediaFeature) ||
           mediaFeature->equals(deviceWidthMediaFeature) ||
           mediaFeature->equals(minDeviceWidthMediaFeature) ||
           mediaFeature->equals(maxDeviceWidthMediaFeature);
}

static inline bool featureWithValidDensity(const String* mediaFeature,
                                           CSSToken* token)
{
    if (token->unitType() != UnitType::DotsPerPixel &&
        token->unitType() != UnitType::DotsPerInch &&
        token->unitType() != UnitType::DotsPerCentimeter) {
        return false;
    }

    return mediaFeature->equals(resolutionMediaFeature) ||
           mediaFeature->equals(minResolutionMediaFeature) ||
           mediaFeature->equals(maxResolutionMediaFeature);
}

static inline bool featureWithPositiveInteger(const String* mediaFeature,
                                              CSSToken* token)
{
    if (token->toStringValue()->contains(".") || token->numericValue() < 0) {
        return false;
    }

    return mediaFeature->equals(colorMediaFeature) ||
           mediaFeature->equals(maxColorMediaFeature) ||
           mediaFeature->equals(minColorMediaFeature) ||
           mediaFeature->equals(colorIndexMediaFeature) ||
           mediaFeature->equals(maxColorIndexMediaFeature) ||
           mediaFeature->equals(minColorIndexMediaFeature) ||
           mediaFeature->equals(monochromeMediaFeature) ||
           mediaFeature->equals(maxMonochromeMediaFeature) ||
           mediaFeature->equals(minMonochromeMediaFeature);
}

static inline bool featureWithPositiveNumber(const String* mediaFeature,
                                             CSSToken* token)
{
    if (!token->isNumber() || token->numericValue() < 0) {
        return false;
    }

    return mediaFeature->equals(transform3dMediaFeature) ||
           mediaFeature->equals(devicePixelRatioMediaFeature) ||
           mediaFeature->equals(maxDevicePixelRatioMediaFeature) ||
           mediaFeature->equals(minDevicePixelRatioMediaFeature);
}

static inline bool featureWithZeroOrOne(const String* mediaFeature,
                                        CSSToken* token)
{
    if (token->value()->contains(".") ||
        !(token->numericValue() == 1 || token->numericValue() == 0)) {
        return false;
    }

    return mediaFeature->equals(gridMediaFeature);
}

static inline bool featureWithAspectRatio(const String* mediaFeature)
{
    return mediaFeature->equals(aspectRatioMediaFeature) ||
           mediaFeature->equals(deviceAspectRatioMediaFeature) ||
           mediaFeature->equals(minAspectRatioMediaFeature) ||
           mediaFeature->equals(maxAspectRatioMediaFeature) ||
           mediaFeature->equals(minDeviceAspectRatioMediaFeature) ||
           mediaFeature->equals(maxDeviceAspectRatioMediaFeature);
}

MediaQueryExp::MediaQueryExp(MediaQueryExp& other)
    : m_mediaFeature(other.mediaFeature())
    , m_expValue(other.expValue())
{
}

MediaQueryExp::MediaQueryExp(String* mediaFeature, MediaQueryExpValue expValue)
    : m_mediaFeature(mediaFeature)
    , m_expValue(expValue)
{
}

MediaQueryExp* MediaQueryExp::createIfValid(
    String* mediaFeature, const GCVector<CSSToken*>& tokenList)
{
    STARFISH_ASSERT(mediaFeature);

    MediaQueryExpValue expValue;
    String* lowerMediaFeature = mediaFeature->toLower();

    // Create value for media query expression that must have 1 or more values.
    if (tokenList.size() == 0 && featureWithoutValue(lowerMediaFeature)) {
        // Valid, creates a MediaQueryExp with an 'invalid' MediaQueryExpValue
    } else if (tokenList.size() == 1) {
        CSSToken* token = tokenList.front();

        if (token->isIdent()) {
            String* ident = token->value();
            if (!featureWithValidIdent(lowerMediaFeature, ident)) {
                return nullptr;
            }
            expValue.id = ident;
            expValue.unit = UnitType::ValueID;
            expValue.isID = true;
        } else if (token->isNumber() || token->isPercentage() ||
                   token->isDimension()) {
            // Check for numeric token types since it is only safe for these
            // types to call numericValue.
            if (featureWithValidDensity(lowerMediaFeature, token) ||
                featureWithValidPositiveLength(lowerMediaFeature, token)) {
                // Media features that must have non-negative <density>, ie.
                // dppx, dpi or dpcm,
                // or Media features that must have non-negative <length> or
                // number value.
                expValue.value = token->numericValue();
                expValue.unit = token->unitType();
                expValue.isValue = true;
            } else if (featureWithPositiveInteger(lowerMediaFeature, token) ||
                       featureWithPositiveNumber(lowerMediaFeature, token) ||
                       featureWithZeroOrOne(lowerMediaFeature, token)) {
                // Media features that must have non-negative integer value,
                // or media features that must have non-negative number value,
                // or media features that must have (0|1) value.
                expValue.value = token->numericValue();
                expValue.unit = UnitType::Number;
                expValue.isValue = true;
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    } else if (tokenList.size() == 3 &&
               featureWithAspectRatio(lowerMediaFeature)) {
        // <ratio> is supposed to allow whitespace around the '/'
        // Applicable to device-aspect-ratio and aspect-ratio.
        CSSToken* numerator = tokenList[0];
        CSSToken* delimiter = tokenList[1];
        CSSToken* denominator = tokenList[2];
        if (!delimiter->isSymbol() || !delimiter->isSymbol('/')) {
            return nullptr;
        }
        if (!numerator->isNumber() || numerator->numericValue() <= 0 ||
            numerator->toStringValue()->contains(".")) {
            return nullptr;
        }
        if (!denominator->isNumber() || denominator->numericValue() <= 0 ||
            denominator->toStringValue()->contains(".")) {
            return nullptr;
        }

        expValue.numerator = (unsigned)numerator->numericValue();
        expValue.denominator = (unsigned)denominator->numericValue();
        expValue.isRatio = true;
    } else {
        return nullptr;
    }

    return new MediaQueryExp(lowerMediaFeature, expValue);
}

String* MediaQueryExp::serialize() const
{
    String* result = String::emptyString;
    result->concat(String::createASCIIString("("));
    result->concat(m_mediaFeature->toLower());
    if (m_expValue.isValid()) {
        result->concat(String::createASCIIString(": "));
        result->concat(m_expValue.cssText());
    }
    result->concat(String::createASCIIString(")"));

    return result;
}

String* MediaQueryExpValue::cssText() const
{
    String* output = String::emptyString;
    if (isValue) {
        output->concat(String::fromFloat(value));
        output->concat(String::fromUTF8(unitTypeToString(unit)));
    } else if (isRatio) {
        output->concat(String::fromFloat(numerator));
        output->concat(String::createASCIIString("/"));
        output->concat(String::fromFloat(denominator));
    } else if (isID) {
        output->concat(id);
    }

    return output;
}
}
