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
#include "CSSParser.h"

#include "dom/Element.h"
#include "dom/Document.h"

#include "CSSStyleLookupTrie.h"

#include "Style.h"

namespace StarFish {

const char* kCHARSET_RULE_MISSING_SEMICOLON = "Missing semicolon at the end of @charset rule";
const char* kCHARSET_RULE_CHARSET_IS_STRING = "The charset in the @charset rule should be a string";
const char* kCHARSET_RULE_MISSING_WS = "Missing mandatory whitespace after @charset";
const char* kIMPORT_RULE_MISSING_URL = "Missing URL in @import rule";
const char* kURL_EOF = "Unexpected end of stylesheet";
const char* kURL_WS_INSIDE = "Multiple tokens inside a url() notation";
const char* kVARIABLES_RULE_POSITION = "@variables rule invalid at this position in the stylesheet";
const char* kIMPORT_RULE_POSITION = "@import rule invalid at this position in the stylesheet";
const char* kNAMESPACE_RULE_POSITION = "@namespace rule invalid at this position in the stylesheet";
const char* kCHARSET_RULE_CHARSET_SOF = "@charset rule invalid at this position in the stylesheet";
const char* kUNKNOWN_AT_RULE = "Unknow @-rule";

unsigned char CSS_ESCAPE  = '\\';

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
    for (size_t i = 0; i < s->length(); i ++) {
        if (s->charAt(i) == '\n') {
            cnt++;
        }
    }
    return cnt;
}

char kLexTable[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, W, W, 0, W, W, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    W, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, I, 0, 0,
    XI, XI, XI, XI, XI, XI, XI, XI, XI, XI, 0, 0, 0, 0, 0, 0,
    0, XSI, XSI, XSI, XSI, XSI, XSI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, 0, S, 0, 0, SI,
    0, XSI, XSI, XSI, XSI, XSI, XSI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI,
    SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI, SI
};
size_t kLexTableSize = sizeof kLexTable / sizeof(int);

class CSSToken : public gc {
public:
    CSSToken(char type, String* value = String::emptyString, String* unit = String::emptyString)
    {
        m_type = type;
        m_value = value;
        m_unit = unit;
    }

    bool isNotNull()
    {
        return m_type;
    }

    bool isOfType(char aType, String* aValue = nullptr)
    {
        return (m_type == aType && (!aValue || m_value->equalsWithoutCase(aValue)));
    }

    bool isWhiteSpace(String* w = nullptr)
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

    bool isNumber(String* n = nullptr)
    {
        return isOfType(CSSToken::NUMBER_TYPE, n);
    }

    bool isIdent(String* i = nullptr)
    {
        return isOfType(CSSToken::IDENT_TYPE, i);
    }

    bool isFunction(String* f = nullptr)
    {
        return isOfType(CSSToken::FUNCTION_TYPE, f);
    }

    bool isAtRule(String* a = nullptr)
    {
        return isOfType(CSSToken::ATRULE_TYPE, a);
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

    bool isSymbol(char32_t c)
    {
        return (m_type == CSSToken::SYMBOL_TYPE && ((m_value->length() == 1) && (m_value->charAt(0) == c)));
    }

    bool isSymbol(String* c = nullptr)
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

    bool isDimensionOfUnit(const char* aUnit)
    {
        return (isDimension() && m_unit->equals(aUnit));
    }

    bool isLength()
    {
        return (isPercentage()
            || isDimensionOfUnit("cm")
            || isDimensionOfUnit("mm")
            || isDimensionOfUnit("in")
            || isDimensionOfUnit("pc")
            || isDimensionOfUnit("px")
            || isDimensionOfUnit("em")
            || isDimensionOfUnit("ex")
            || isDimensionOfUnit("pt"));
    }

    bool isAngle()
    {
        return (isDimensionOfUnit("deg")
            || isDimensionOfUnit("rad")
            || isDimensionOfUnit("grad"));
    }

    char m_type;
    String* m_value;
    String* m_unit;

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
};



class CSSScanner : public gc {
public:
    CSSScanner(String* str)
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
        if (m_pos < m_string->length())
            return m_string->charAt(m_pos++);
        return -1;
    }

    int peek()
    {
        if (m_pos < m_string->length())
            return m_string->charAt(m_pos);
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
        return isIdentStart(aFirstChar) || (aFirstChar == '-' && isIdentStart(aSecondChar));
    }

    bool isIdent(char32_t code)
    {
        return (code >= 256 || (kLexTable[code] & IS_IDENT) != 0);
    }

    void pushback()
    {
        m_pos--;
    }

    CSSToken* nextHexValue()
    {
        int c = read();
        if (c == -1 || !isHexDigit((char32_t)c))
            return new CSSToken(CSSToken::NULL_TYPE, String::emptyString);
        String* s = String::createUTF32String((char32_t)c);
        c = read();
        while (c != -1 && isHexDigit((char32_t)c)) {
            s = s->concat(String::createUTF32String((char32_t)c));
            c = read();
        }
        if (c != -1)
            pushback();
        return new CSSToken(CSSToken::HEX_TYPE, s);
    }

    String* gatherEscape()
    {
        int c = peek();
        if (c == -1)
            return String::emptyString;
        if (isHexDigit((char32_t)c)) {
            int code = 0;
            size_t i;
            for (i = 0; i < 6; i++) {
                c = read();
                if (isDigit((char32_t)c))
                    code = code * 16 + (c - '0');
                else if (isHexDigit((char32_t)c))
                    code = code * 16 + (towlower(c) - 'a' + 10);
                else if (!isHexDigit((char32_t)c) && !isWhiteSpace((char32_t)c)) {
                    pushback();
                    break;
                } else
                    break;
            }
            if (i == 6) {
                c = peek();
                if (isWhiteSpace((char32_t)c))
                    read();
            }
            return String::createUTF32String((char32_t)code);
        }
        c = read();
        if (c != '\n')
            return String::createUTF32String((char32_t)c);
        return String::emptyString;
    }

    String* gatherIdent(int c)
    {
        String* s = String::emptyString;
        if (c == CSS_ESCAPE)
            s = s->concat(gatherEscape());
        else
            s = s->concat(String::createUTF32String((char32_t)c));
        c = read();
        while (c != -1 && (isIdent(c) || c == CSS_ESCAPE)) {
            if (c == CSS_ESCAPE) {
                String* tmp = gatherEscape();
                if (!tmp->length())
                    return String::emptyString;
                else
                    s = s->concat(tmp);
            } else
                s = s->concat(String::createUTF32String((char32_t)c));
            c = read();
        }
        if (c != -1)
            pushback();
        return s;
    }

    CSSToken* parseIdent(int c)
    {
        String* value = gatherIdent(c);
        int nextChar = peek();
        if ((char32_t)nextChar == '(') {
            value = value->concat(String::createUTF32String(read()));
            return new CSSToken(CSSToken::FUNCTION_TYPE, value);
        }
        return new CSSToken(CSSToken::IDENT_TYPE, value);
    }

    CSSToken* parseURL(int c)
    {
        String* value = String::emptyString;
        if (c == CSS_ESCAPE)
            value = value->concat(gatherEscape());
        else
            value = value->concat(String::createUTF32String((char32_t)c));
        c = read();
        while (c != -1 && c != ' ' && c != ')') {
            if (c == CSS_ESCAPE) {
                String* tmp = gatherEscape();
                if (!tmp->length())
                    return new CSSToken(CSSToken::STRING_TYPE, String::emptyString);
                else
                    value = value->concat(tmp);
            } else
                value = value->concat(String::createUTF32String((char32_t)c));
            c = read();
        }
        if (c != -1)
            pushback();
        return new CSSToken(CSSToken::STRING_TYPE, value);
    }

    bool isDigit(char32_t c)
    {
        return (c >= '0') && (c <= '9');
    }

    CSSToken* parseComment(int c)
    {
        String* s = String::createUTF32String((char32_t)c);
        while ((c = read()) != -1) {
            s = s->concat(String::createUTF32String((char32_t)c));
            if (c == '*') {
                c = read();
                if (c == -1)
                    break;
                if (c == '/') {
                    s = s->concat(String::createUTF32String((char32_t)c));
                    break;
                }
                pushback();
            }
        }
        return new CSSToken(CSSToken::COMMENT_TYPE, s);
    }

    CSSToken* parseNumber(int c)
    {
        String* s = String::createUTF32String((char32_t)c);
        bool foundDot = false;
        while ((c = read()) != -1) {
            if (c == '.') {
                if (foundDot)
                    break;
                else {
                    s = s->concat(String::createUTF32String((char32_t)c));
                    foundDot = true;
                }
            } else if (isDigit(c))
                s = s->concat(String::createUTF32String((char32_t)c));
            else
                break;
        }

        if (c != -1 && startsWithIdent(c, peek())) { // DIMENSION
            String* unit = gatherIdent(c);
            s = s->concat(unit);
            return new CSSToken(CSSToken::DIMENSION_TYPE, s, unit);
        } else if (c == '%') {
            s = s->concat(String::createUTF32String('%'));
            return new CSSToken(CSSToken::PERCENTAGE_TYPE, s);
        } else if (c != -1)
            pushback();
        return new CSSToken(CSSToken::NUMBER_TYPE, s);
    }

    CSSToken* parseString(int aStop)
    {
        String* s = String::createUTF32String((char32_t)aStop);
        int previousChar = aStop;
        int c;
        while ((c = read()) != -1) {
            if (c == aStop && previousChar != CSS_ESCAPE) {
                s = s->concat(String::createUTF32String((char32_t)c));
                break;
            } else if (c == CSS_ESCAPE) {
                c = peek();
                if (c == -1)
                    break;
                else if (c == '\n' || c == '\r' || c == '\f') {
                    int d = c;
                    c = read();
                    // special for Opera that preserves \r\n...
                    if (d == '\r') {
                        c = peek();
                        if (c == '\n')
                            c = read();
                    }
                } else {
                    s = s->concat(gatherEscape());
                    c = peek();
                }
            } else if (c == '\n' || c == '\r' || c == '\f') {
                break;
            } else
                s = s->concat(String::createUTF32String((char32_t)c));
            previousChar = c;
        }
        return new CSSToken(CSSToken::STRING_TYPE, s);
    }

    bool isWhiteSpace(char32_t c)
    {
        char32_t code = c;
        return code < 256 && (kLexTable[code] & IS_WHITESPACE) != 0;
    }

    String* eatWhiteSpace(int c)
    {
        String* s = String::createUTF32String((char32_t)c);
        while ((c = read()) != -1) {
            if (!isWhiteSpace(c))
                break;
            s = s->concat(String::createUTF32String((char32_t)c));
        }
        if (c != -1)
            pushback();
        return s;
    }

    CSSToken* parseAtKeyword(int c)
    {
        return new CSSToken(CSSToken::ATRULE_TYPE, gatherIdent(c));
    }

    CSSToken* nextToken(bool isURL = false)
    {
        int c = read();
        if (c == -1)
            return new CSSToken(CSSToken::NULL_TYPE, String::emptyString);

        if (isURL && c != '\'' && c != '"' && c != ')' && c != ' ') // url starts without \' nor \"
            return parseURL(c);

        if (startsWithIdent(c, peek()))
            return parseIdent(c);

        if (c == '@') {
            int nextChar = read();
            if (nextChar != -1) {
                int followingChar = peek();
                pushback();
                if (startsWithIdent(nextChar, followingChar))
                    return parseAtKeyword(c);
            }
        }

        if (c == '<') {
            if (read() == '!') {
                if (read() == '-') {
                    if (read() == '-') {
                        return new CSSToken(CSSToken::SGML_COMMENT_TYPE, String::createASCIIString("<!--"));
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
                    return new CSSToken(CSSToken::SGML_COMMENT_TYPE, String::createASCIIString("-->"));
                }
                pushback();
            }
            pushback();
        }

        if (c == '.' || c == '+' || c == '-') {
            int nextChar = peek();
            if (isDigit(nextChar))
                return parseNumber(c);
            else if (nextChar == '.' && c != '.') {
                // int firstChar = read();
                read();
                int secondChar = peek();
                pushback();
                if (isDigit(secondChar))
                    return parseNumber(c);
            }
        }
        if (isDigit(c)) {
            return parseNumber(c);
        }

        if (c == '\'' || c == '"')
            return parseString(c);

        if (isWhiteSpace(c)) {
            String* s = eatWhiteSpace(c);
            return new CSSToken(CSSToken::WHITESPACE_TYPE, s);
        }

        if (c == '|' || c == '~' || c == '^' || c == '$' || c == '*') {
            int nextChar = read();
            if (nextChar == '=') {
                switch (c) {
                case '~':
                    return new CSSToken(CSSToken::INCLUDES_TYPE, String::createASCIIString("~="));
                case '|':
                    return new CSSToken(CSSToken::DASHMATCH_TYPE, String::createASCIIString("|="));
                case '^':
                    return new CSSToken(CSSToken::BEGINSMATCH_TYPE, String::createASCIIString("^="));
                case '$':
                    return new CSSToken(CSSToken::ENDSMATCH_TYPE, String::createASCIIString("$="));
                case '*':
                    return new CSSToken(CSSToken::CONTAINSMATCH_TYPE, String::createASCIIString("*="));
                default:
                    break;
                }
            } else if (nextChar != -1)
                pushback();
        }

        if (c == '/' && peek() == '*')
            return parseComment(c);

        return new CSSToken(CSSToken::SYMBOL_TYPE, String::createUTF32String((char32_t)c));
    }
protected:
    String* m_string;
    size_t m_pos;
    std::vector<size_t, gc_allocator_ignore_off_page<size_t> > m_preservedPos;

};

CSSToken* CSSParser::getToken(bool aSkipWS, bool aSkipComment, bool isURL)
{
    if (m_lookAhead) {
        m_token = m_lookAhead;
        m_lookAhead = nullptr;
        return m_token;
    }

    m_token = m_scanner->nextToken(isURL);
    while (m_token && ((aSkipWS && m_token->isWhiteSpace()) || (aSkipComment && m_token->isComment())))
        m_token = m_scanner->nextToken(isURL);
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

String* CSSParser::parseSimpleSelector(CSSToken* token, bool isFirstInChain, bool canNegate, bool& validSelector)
{
    String* s = String::emptyString;
    // var specificity = {a: 0, b: 0, c: 0, d: 0}; // CSS 2.1 section 6.4.3

    if (isFirstInChain
        && (token->isSymbol('*') || token->isSymbol('|') || token->isIdent())) {
        // type or universal selector
        if (token->isSymbol('*') || token->isIdent()) {
            // we don't know yet if it's a prefix or a universal
            // selector
            s = s->concat(token->m_value);
            // bool isIdent = token->isIdent();
            token = getToken(false, true);
            if (token->isSymbol('|')) {
                // it's a prefix
                s = s->concat(token->m_value);
                token = getToken(false, true);
                if (token->isIdent() || token->isSymbol('*')) {
                    // ok we now have a type element or universal
                    // selector
                    s = s->concat(token->m_value);
                    /*
                    if (token->isIdent())
                        specificity.d++;
                     */
                } else // oops that's an error...
                    return String::emptyString;
            } else {
                ungetToken();
                /*
                if (isIdent)
                    specificity.d++;
                */
            }
        } else if (token->isSymbol('|')) {
            s = s->concat(token->m_value);
            token = getToken(false, true);
            if (token->isIdent() || token->isSymbol('*')) {
                s = s->concat(token->m_value);
                /*
                if (token->isIdent())
                    specificity.d++;
                */
            } else // oops that's an error
                return String::emptyString;
        }
    } else if (token->isSymbol('.') || token->isSymbol('#')) {
        // bool isClass = token->isSymbol('.');
        s = s->concat(token->m_value);
        token = getToken(false, true);
        if (token->isIdent()) {
            if (token->m_value->length())
                s = s->concat(token->m_value);
            else {
                validSelector = false;
                return String::emptyString;
            }
            /*
            if (isClass)
                specificity.c++;
            else
                specificity.b++;
            */
        } else
            return String::emptyString;
    } else if (token->isSymbol(':')) {
        s = s->concat(token->m_value);
        token = getToken(false, true);
        if (token->isSymbol(':')) {
            s = s->concat(token->m_value);
            token = getToken(false, true);
        }
        if (token->isIdent()) {
            s = s->concat(token->m_value);
            /*
            if (isPseudoElement(token->value))
                specificity.d++;
            else
                specificity.c++;
            */
        } else if (token->isFunction()) {
            s = s->concat(token->m_value);
            if (token->isFunction(String::createASCIIString(":not("))) {
                if (!canNegate)
                    return String::emptyString;
                token = getToken(true, true);
                String* simpleSelector = parseSimpleSelector(token, isFirstInChain, false, validSelector);
                if (simpleSelector->length() == 0)
                    return String::emptyString;
                else {
                    // s += simpleSelector.selector;
                    s = s->concat(simpleSelector);
                    token = getToken(true, true);
                    if (token->isSymbol(')'))
                        s = s->concat(String::createASCIIString(")"));
                    else
                        return String::emptyString;
                }
                // specificity.c++;
            } else {
                while (true) {
                    token = getToken(false, true);
                    if (token->isSymbol(')')) {
                        s = s->concat(String::createASCIIString(")"));
                        break;
                    } else
                        s = s->concat(token->m_value);
                }
                // specificity.c++;
            }
        } else
            return String::emptyString;
    } else if (token->isSymbol('[')) {
        s = s->concat(String::createASCIIString("["));
        token = getToken(true, true);
        if (token->isIdent() || token->isSymbol('*')) {
            s = s->concat(token->m_value);
            CSSToken* nextToken = getToken(true, true);
            if (nextToken->isSymbol('|')) {
                s = s->concat(String::createASCIIString("|"));
                token = getToken(true, true);
                if (token->isIdent())
                    s = s->concat(token->m_value);
                else
                    return String::emptyString;
            } else
                ungetToken();
        } else if (token->isSymbol('|')) {
            s = s->concat(String::createASCIIString("|"));
            token = getToken(true, true);
            if (token->isIdent())
                s = s->concat(token->m_value);
            else
                return String::emptyString;
        } else
            return String::emptyString;

        // nothing, =, *=, $=, ^=, |=
        token = getToken(true, true);
        if (token->isIncludes()
            || token->isDashmatch()
            || token->isBeginsmatch()
            || token->isEndsmatch()
            || token->isContainsmatch()
            || token->isSymbol('=')) {
            s = s->concat(token->m_value);
            token = getToken(true, true);
            if (token->isString() || token->isIdent()) {
                s = s->concat(token->m_value);
                token = getToken(true, true);
            } else
                return String::emptyString;

            if (token->isSymbol(']')) {
                s = s->concat(token->m_value);
                // specificity.c++;
            } else
                return String::emptyString;
        } else if (token->isSymbol(']')) {
            s = s->concat(token->m_value);
            // specificity.c++;
        } else
            return String::emptyString;
    } else if (token->isWhiteSpace()) {
        CSSToken* t = lookAhead(true, true);
        if (t->isSymbol('{'))
            return String::emptyString;
    }
    if (s->length())
        return s;
    return String::emptyString;
}

void CSSParser::parseSelector(std::vector<CSSSelectorList *, gc_allocator_ignore_off_page<CSSSelectorList *>>& list, bool& validSelector)
{
    validSelector = parseComplexSelectorList(list);
    if (!validSelector)
        list.clear();
}

CSSSelector::RelationType CSSParser::parseCombinator()
{
    CSSSelector::RelationType fallbackResult = CSSSelector::SubSelector;

    CSSToken* token = currentToken();
    while (token->isWhiteSpace()) {
        token = getToken(false, true);
        fallbackResult = CSSSelector::Descendant;
    }

    if (fallbackResult == CSSSelector::Descendant)
        return fallbackResult;

    if (token->isSymbol('+'))
        return CSSSelector::DirectAdjacent;
    else if (token->isSymbol('~'))
        return CSSSelector::IndirectAdjacent;
    else if (token->isSymbol('>'))
        return CSSSelector::Child;

    return fallbackResult;
}

String* CSSParser::determineNamespace(String* prefix)
{
    if (prefix == nullptr)
        return String::emptyString;
    if (prefix->equals(String::emptyString))
        return String::emptyString; // No namespace. If an element/attribute has a namespace, we won't match it.
    if (prefix->equals(String::fromUTF8("*")))
        return String::fromUTF8("*"); // We'll match any namespace.

    if (m_document->styleResolver()->sheets().size() == 0)
        return nullptr; // Cannot resolve prefix to namespace without a stylesheet, syntax error.

    // TODO: Implement logic for getting namespace uri from prefix in stylesheet
    // return m_styleSheet->namespaceURIFromPrefix(prefix);
    return String::emptyString;
}

CSSSelector* CSSParser::getPseudoSelector()
{
    int colons = 1;

    CSSToken* token = getToken(false, true);
    if (token->isSymbol(':'))
        colons++;

    token = currentToken();
    if (!token->isIdent() && !token->isFunction())
        return nullptr;

    CSSSelector* selector = new CSSSelector();
    selector->setType(colons == 1 ? CSSSelector::Type::PseudoClass: CSSSelector::Type::PseudoElement);
    selector->setRelation(CSSSelector::RelationType::SubSelector);
    selector->updatePseudoType(token->m_value->toLower(), token->isFunction());

    if (token->isIdent()) {
        token = getToken(false, true);
        if (selector->pseudoType() == CSSSelector::PseudoNone)
            return nullptr;
        return selector;
    }

    // TODO: handle pseudo-* selectors of function type
    // For examples, not(), lang(), nth-*() ans so on.

    return nullptr;
}

CSSSelector* CSSParser::getAttributeSelector()
{
    // TODO: implement logic for getting attribute selector
    return nullptr;
}

CSSSelector* CSSParser::getClassSelector()
{
    CSSToken* token = getToken(false, true);
    if (!token->isIdent())
        return nullptr;

    CSSSelector* selector = new CSSSelector(CSSSelector::Type::Class, CSSSelector::SubSelector, CSSSelector::PseudoType::PseudoNone, token->m_value);
    getToken(false, true);

    return selector;
}

CSSSelector* CSSParser::getIdSelector()
{
    CSSToken* token = getToken(false, true);
    if (!token->isIdent())
        return nullptr;

    CSSSelector* selector = new CSSSelector(CSSSelector::Type::Id, CSSSelector::SubSelector, CSSSelector::PseudoType::PseudoNone, token->m_value);
    getToken(false, true);

    return selector;
}

CSSSelector* CSSParser::getSimpleSelector()
{
    CSSToken* token = currentToken();
    CSSSelector* selector;
    if (token->isSymbol('#'))
        selector = getIdSelector();
    else if (token->isSymbol('.'))
        selector = getClassSelector();
    else if (token->isSymbol('['))
        selector = getAttributeSelector();
    else if (token->isSymbol(':'))
        selector = getPseudoSelector();
    else
        return nullptr;

    if (!selector)
        m_failedParsing = true;

    return selector;
}

bool CSSParser::parseName(String** name)
{
    CSSToken* firstToken = currentToken();
    if (firstToken->isIdent()) {
        *name = firstToken->m_value;
        getToken(false, true);
    } else if (firstToken->isSymbol('*')) {
        *name = String::fromUTF8("*");
        getToken(false, true);
    } else if (firstToken->isSymbol('|')) {
        *name = String::emptyString;
    } else {
        return false;
    }

    if (!firstToken->isSymbol('|'))
        return true;

    CSSToken* nameToken = getToken(true, true);
    if (nameToken->isIdent()) {
        *name = nameToken->m_value;
    } else if (nameToken->isSymbol('*')) {
        *name = String::fromUTF8("*");
    } else {
        *name = nullptr;
        return false;
    }

    return true;
}

void CSSParser::parseCompoundSelector(CSSSelectorList* selectorList)
{
    CSSSelector* compoundSelector;

    String* elementName = nullptr;
    CSSSelector::PseudoType compoundPseudoElement = CSSSelector::PseudoNone;
    if (!parseName(&elementName)) {
        compoundSelector = getSimpleSelector();

        if (!compoundSelector)
            return;
        if (compoundSelector->type() == CSSSelector::PseudoElement)
            compoundPseudoElement = compoundSelector->pseudoType();

        selectorList->pushBack(compoundSelector);
    }

    while (CSSSelector* simpleSelector = getSimpleSelector()) {
        if (simpleSelector->type() == CSSSelector::PseudoElement)
            compoundPseudoElement = simpleSelector->pseudoType();

        selectorList->pushBack(simpleSelector);
    }

    if (selectorList->size() > 0)
        selectorList->at(selectorList->size() - 1)->setRelation(CSSSelector::None);

    if (elementName) {
        CSSSelector* selector = new CSSSelector();
        selector->setSelectorText(elementName->toLower());

        if (elementName->equals(String::fromUTF8("*")))
            selector->setType(CSSSelector::Type::Universal);
        else
            selector->setType(CSSSelector::Type::Tag);

        if (selectorList->size() > 0)
            selector->setRelation(CSSSelector::SubSelector);
        else
            selector->setRelation(CSSSelector::None);

        selector->setPseudoType(CSSSelector::PseudoType::PseudoNone);

        selectorList->insertFront(selector);
    }
}

enum CompoundSelectorFlags {
    HasPseudoElementForRightmostCompound = 1 << 0,
    HasContentPseudoElement = 1 << 1
};

unsigned CSSParser::extractCompoundFlags(CSSSelector* simpleSelector)
{
    if (simpleSelector->type() != CSSSelector::PseudoElement)
        return 0;
//    if (simpleSelector->pseudoType() == CSSSelector::PseudoContent)
//        return HasContentPseudoElement;

    return HasPseudoElementForRightmostCompound;
}

void CSSParser::parseComplexSelector(CSSSelectorList* selectorList)
{
    CSSToken* token = currentToken();
    while (token->isSGMLComment() || token->isWhiteSpace())
        token = getToken(false, true);

    parseCompoundSelector(selectorList);

    if (selectorList->size() == 0)
        return;

    unsigned selectorSize = selectorList->size();

    unsigned previousCompoundFlags = 0;
    for (unsigned i = 0; i < selectorSize; i++) {
        CSSSelector* simple = selectorList->at(i);
        if (simple && !previousCompoundFlags)
            break;

        previousCompoundFlags |= extractCompoundFlags(simple);
    }

    CSSSelectorList* secondSelectorList = new CSSSelectorList();

    while (CSSSelector::RelationType combinator = parseCombinator()) {
        secondSelectorList->clear();
        parseCompoundSelector(secondSelectorList);

        if (secondSelectorList->size() == 0)
            return;
        if (previousCompoundFlags & HasPseudoElementForRightmostCompound)
            return;

        unsigned i = 0;
        CSSSelector* end = secondSelectorList->at(i);
        unsigned compoundFlags = extractCompoundFlags(end);
        selectorSize = secondSelectorList->size();

        while (++i < selectorSize) {
            end = secondSelectorList->at(i);
            compoundFlags |= extractCompoundFlags(end);
        }
        end->setRelation(combinator);

        if (previousCompoundFlags & HasContentPseudoElement)
            end->relationIsAffectedByPseudoContent();
        previousCompoundFlags = compoundFlags;
        selectorList->selectors().insert(selectorList->selectors().begin(),
            secondSelectorList->selectors().begin(),
            secondSelectorList->selectors().end());
    }
}

bool CSSParser::parseComplexSelectorList(std::vector<CSSSelectorList*, gc_allocator_ignore_off_page<CSSSelectorList*>>& listOfSelectorList)
{
    CSSSelectorList* selectorList = new CSSSelectorList();
    parseComplexSelector(selectorList);

    if (selectorList->size() == 0)
        return false;

    listOfSelectorList.push_back(selectorList);

    CSSToken* token = currentToken();
    while (token->isNotNull() && token->isSymbol(',')) {
        do {
            token = getToken(false, true);
        } while (token->isSGMLComment() || token->isWhiteSpace());

        CSSSelectorList* nextSelectorList = new CSSSelectorList();
        parseComplexSelector(nextSelectorList);
        if (nextSelectorList->size() == 0)
            return false;

        listOfSelectorList.push_back(nextSelectorList);
        token = currentToken();
    }

    if (m_failedParsing)
        return false;

    return true;
}

String* CSSParser::parseDefaultPropertyValue(CSSToken* token)
{
    std::vector<CSSToken*, gc_allocator_ignore_off_page<CSSToken*>> willBeConcat;
    std::vector<String*, gc_allocator_ignore_off_page<String*>> blocks;
    // bool foundPriority = false;
    std::vector<String*, gc_allocator_ignore_off_page<String*>> values;
    bool isURLFunc = false;
    int urlTokens = 0;
    while (token->isNotNull()) {
        if ((token->isSymbol(';')
            || token->isSymbol('}')
            || token->isSymbol('!'))
            && !blocks.size()) {
            if (token->isSymbol('}') && willBeConcat.size() > 0)
                ungetToken();
            break;
        }
        if (token->isIdent(String::inheritString)) {
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
                return combineAndTrimTokenValues(&willBeConcat);
            } else {
                willBeConcat.clear();
                willBeConcat.push_back(token);
                token = getToken(true, true);
                break;
            }
        } else if (token->isSymbol('{') || token->isSymbol('(') || token->isSymbol('[') || token->isFunction()) {
            if (token->isFunction() && token->m_value->toLower()->equals("url(")) {
                blocks.push_back(String::createASCIIString("url("));
                isURLFunc = true;
            } else
                blocks.push_back(token->isFunction() ? String::createASCIIString("(") : token->m_value);
        } else if (token->isSymbol('}') || token->isSymbol(')') || token->isSymbol(']')) {
            if (blocks.size()) {
                String* ontop = blocks[blocks.size() - 1];
                if ((token->isSymbol('}') && ontop->equals("{"))
                    || (token->isSymbol(')') && ontop->equals("("))
                    || (token->isSymbol(']') && ontop->equals("["))) {
                    blocks.pop_back();
                } else if (token->isSymbol(')') && ontop->equals("url(")) {
                    blocks.pop_back();
                    if (urlTokens > 2)
                        return String::emptyString;
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
        } else
            token = getToken(false, false);
    }
    /*
    if (values.length && valueText) {
        this.forgetState();
        aDecl.push(this._createJscsspDeclarationFromValuesArray(descriptor, values, valueText));
        return valueText;
    }*/
    if (willBeConcat.size() > 0) {
        forgetState();
    }
    return combineAndTrimTokenValues(&willBeConcat);
}

// Remove comments from both sides of a tokenList & Concat
String* CSSParser::combineAndTrimTokenValues(std::vector<CSSToken*, gc_allocator_ignore_off_page<CSSToken*>>* list)
{
    String* result = String::emptyString;
    if (list != nullptr) {
        std::vector<CSSToken*, gc_allocator_ignore_off_page<CSSToken*>> stashed;
        bool seenNoneComment = false;
        for (CSSToken* item : *list) {
            if (seenNoneComment && item->isComment()) {
                stashed.push_back(item);
            } else {
                seenNoneComment = true;
                if (stashed.size()) {
                    for (CSSToken* commentItem : stashed) {
                        result = result->concat(commentItem->m_value);
                    }
                    stashed.clear();
                }
                result = result->concat(item->m_value);
            }
        }
    }
    return result;
}

void CSSParser::parseDeclaration(CSSToken* aToken, CSSStyleDeclaration* declaration)
{
    preserveState();
    std::vector<String*, gc_allocator_ignore_off_page<String*>> blocks;
    if (aToken->isIdent()) {
        String* descriptor = aToken->m_value->toLower();
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("+++style:%s\n", descriptor->utf8Data());
#endif
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
                    if (token->isIdent(String::createASCIIString("important"))) {
                        priority = true;
                        token = getToken(true, true);
                        if (token->isSymbol(';') || token->isSymbol('}')) {
                            if (token->isSymbol('}'))
                                ungetToken();
                        } else
                            return;
                    } else
                        return;
                } else if (token->isNotNull() && !token->isSymbol(';') && !token->isSymbol('}'))
                    return;
                // use decls
                /*
                for (size_t i = 0; i < declarations.length; i++) {
                  declarations[i].priority = priority;
                  aDecl.push(declarations[i]);
                }
                return descriptor + ": " + value + ";";
                */
                if (!descriptor) {
                    return;
                }

                const char* name = descriptor->toLower()->utf8Data();
                CSSStyleKind kind = lookupCSSStyle(name, strlen(name));

                if (false) {

                }
#define SET_ATTR(name, nameLower, nameCSSCase) \
                else if (kind == CSSStyleKind::name) { \
                    declaration->set##name(value);\
                }
                FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
                else {
                    STARFISH_LOG_ERROR("unsupported property name(CSSParser) -> %s\n", descriptor->utf8Data());
                }
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
        } else if (token->isSymbol('{') || token->isSymbol('(') || token->isSymbol('[') || token->isFunction()) {
            if (token->isFunction() && token->m_value->toLower()->equals("url(")) {
                blocks.push_back(String::createASCIIString("url("));
                isURLFunc = true;
            } else
                blocks.push_back(token->isFunction() ? String::createASCIIString("(") : token->m_value);
        } else if (token->isSymbol('}') || token->isSymbol(')') || token->isSymbol(']')) {
            if (blocks.size()) {
                String* ontop = blocks[blocks.size() - 1];
                if ((token->isSymbol('}') && ontop->equals("{")) || (token->isSymbol(')') && ontop->equals("(")) || (token->isSymbol(']') && ontop->equals("["))) {
                    blocks.pop_back();
                } else if (token->isSymbol(')') && ontop->equals("url(")) {
                    blocks.pop_back();
                    isURLFunc = false;
                }
            }
        }
        if (isURLFunc)
            token = getToken(true, false, true);
        else
            token = getToken(false, false);
    }
    return;
}

void CSSParser::parseStyleRule(CSSToken* aToken, CSSStyleSheet* aOwner, bool aIsInsideMediaRule, std::vector<CSSSelectorList*, gc_allocator_ignore_off_page<CSSSelectorList*>>* sList, bool isQueryingSelector)
{
    // size_t currentLine = countLF(m_scanner->getAlreadyScanned());
    preserveState();
    // first let's see if we have a selector here...
    bool validSelector = true;

    std::vector<CSSSelectorList*, gc_allocator_ignore_off_page<CSSSelectorList*>> list;
    parseSelector(list, validSelector);


    bool valid = false;
    CSSStyleDeclaration* declarations = new CSSStyleDeclaration(m_document);
    // var declarations = [];
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
        // selector is invalid so the whole rule is invalid with it
        CSSToken* token = getToken(true, true);
        while (!token->isSymbol('{') && token->isNotNull())
            token = getToken(true, false);
        if (token->isSymbol('{'))
            token = getToken(true, false);
        while (true) {
            if (!token->isNotNull()) {
                return;
            }
            if (token->isSymbol('}')) {
                return;
            } else {
                parseDeclaration(token, declarations);
            }
            token = getToken(true, false);
        }
    }

    if (valid) {
        if (isQueryingSelector) {
            sList->assign(list.begin(), list.end());
        } else {
            unsigned size = list.size();
            for (unsigned i = 0; i < size; ++i) {
                CSSStyleRule* rule = new CSSStyleRule(list.at(i), m_document, declarations);
                aOwner->addRule(rule);
            }
        }

        return;
    }
    restoreState();
    String* s = currentToken()->m_value;
    addUnknownAtRule(aOwner, s);
}

void CSSParser::addUnknownAtRule(CSSStyleSheet* aSheet, String* aString)
{
    // size_t currentLine = countLF(m_scanner->getAlreadyScanned());
    std::vector<String*, gc_allocator_ignore_off_page<String*>> blocks;
    CSSToken* token = getToken(false, false);
    while (token->isNotNull()) {
        aString = aString->concat(token->m_value);
        if (token->isSymbol(';') && !blocks.size())
            break;
        else if (token->isSymbol('{')
            || token->isSymbol('(')
            || token->isSymbol('[')
            || token->m_type == CSSToken::FUNCTION_TYPE) {
            blocks.push_back(token->isFunction() ? String::createASCIIString("(") : token->m_value);
        } else if (token->isSymbol('}')
            || token->isSymbol(')')
            || token->isSymbol(']')) {
            if (blocks.size()) {
                String* ontop = blocks[blocks.size() - 1];
                if ((token->isSymbol('}') && ontop->equals("{"))
                    || (token->isSymbol(')') && ontop->equals("("))
                    || (token->isSymbol(']') && ontop->equals("["))) {
                    blocks.pop_back();
                    if (!blocks.size() && token->isSymbol('}'))
                        break;
                }
            }
        }
        token = getToken(false, false);
    }

    // addUnknownRule(aSheet, aString, currentLine);
}

/*
void CSSParser::addUnknownRule(CSSStyleSheet* aSheet, String* aString, size_t aCurrentLine)
{
    String* errorMsg = consumeError();
    // var rule = new jscsspErrorRule(errorMsg);
    // rule.currentLine = aCurrentLine;
    // rule.parsedCssText = aString;
    // rule.parentStyleSheet = aSheet;
    // aSheet.cssRules.push(rule);
}
*/

void CSSParser::reportError(const char *aMsg)
{
    m_error = String::createASCIIString(aMsg);
}

bool CSSParser::parseCharsetRule(CSSStyleSheet* aSheet)
{
    CSSToken* token = getToken(false, false);
    String* s = String::emptyString;
    if (token->isAtRule(String::createASCIIString("@charset")) && token->m_value->equals("@charset")) { // lowercase check
        s = token->m_value;
        token = getToken(false, false);
        s = s->concat(token->m_value);
        if (token->isWhiteSpace(String::createASCIIString(" "))) {
            token = getToken(false, false);
            s = s->concat(token->m_value);
            if (token->isString()) {
                // String* encoding = token->m_value;
                token = getToken(false, false);
                s = s->concat(token->m_value);
                if (token->isSymbol(';')) {
                    // var rule = new jscsspCharsetRule();
                    // rule.encoding = encoding;
                    // rule.parsedCssText = s;
                    // rule.parentStyleSheet = aSheet;
                    // aSheet.cssRules.push(rule);
                    return true;
                } else
                    reportError(kCHARSET_RULE_MISSING_SEMICOLON);
            } else
                reportError(kCHARSET_RULE_CHARSET_IS_STRING);
        } else
            reportError(kCHARSET_RULE_MISSING_WS);
    }

    addUnknownAtRule(aSheet, s);
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

void CSSParser::parseStyleSheet(String* sourceString, CSSStyleSheet* target)
{
    /* m_lookAhead = nullptr;
    m_token = nullptr;
    m_preserveWS = false;
    m_preserveComments = false;
    m_scanner = new CSSScanner(sourceString);

    CSSToken* token = getToken(false, false);
    */
    // @charset can only appear at first char of the stylesheet
    CSSToken* token = makeToken(sourceString);
    if (!token->isNotNull())
        return;
    if (token->isAtRule(String::createASCIIString("@charset"))) {
        ungetToken();
        parseCharsetRule(target);
        token = getToken(false, false);
    }

    // bool foundStyleRules = false;
    // bool foundImportRules = false;
    // bool foundNameSpaceRules = false;

    while (true) {
        if (!token->isNotNull())
            break;
        if (token->isWhiteSpace()) {
            // if (aTryToPreserveWhitespaces)
            //     this.addWhitespace(sheet, token.value);
        } else if (token->isComment()) {
            // if (this.mPreserveComments)
            //     this.addComment(sheet, token.value);
        } else if (token->isAtRule()) {
            addUnknownAtRule(target, token->m_value);
            /*
            if (token.isAtRule("@variables")) {
              if (!foundImportRules && !foundStyleRules)
                this.parseVariablesRule(token, sheet);
              else {
                this.reportError(kVARIABLES_RULE_POSITION);
                this.addUnknownAtRule(sheet, token.value);
              }
            }
            else if (token.isAtRule("@import")) {
              // @import rules MUST occur before all style and namespace
              // rules
              if (!foundStyleRules && !foundNameSpaceRules)
                foundImportRules = this.parseImportRule(token, sheet);
              else {
                this.reportError(kIMPORT_RULE_POSITION);
                this.addUnknownAtRule(sheet, token.value);
              }
            }
            else if (token.isAtRule("@namespace")) {
              // @namespace rules MUST occur before all style rule and
              // after all @import rules
              if (!foundStyleRules)
                foundNameSpaceRules = this.parseNamespaceRule(token, sheet);
              else {
                this.reportError(kNAMESPACE_RULE_POSITION);
                this.addUnknownAtRule(sheet, token.value);
              }
            }
            else if (token.isAtRule("@font-face")) {
              if (this.parseFontFaceRule(token, sheet))
                foundStyleRules = true;
              else
                this.addUnknownAtRule(sheet, token.value);
            }
            else if (token.isAtRule("@page")) {
              if (this.parsePageRule(token, sheet))
                foundStyleRules = true;
              else
                this.addUnknownAtRule(sheet, token.value);
            }
            else if (token.isAtRule("@media")) {
              if (this.parseMediaRule(token, sheet))
                foundStyleRules = true;
              else
                this.addUnknownAtRule(sheet, token.value);
            }
            else if (token.isAtRule("@keyframes")) {
              if (!this.parseKeyframesRule(token, sheet))
                this.addUnknownAtRule(sheet, token.value);
            }
            else if (token.isAtRule("@charset")) {
              this.reportError(kCHARSET_RULE_CHARSET_SOF);
              this.addUnknownAtRule(sheet, token.value);
            }
            else {
              this.reportError(kUNKNOWN_AT_RULE);
              this.addUnknownAtRule(sheet, token.value);
            } */
        } else {
            // plain style rules
            parseStyleRule(token, target, false, nullptr, false);
            // String* ruleText = parseStyleRule(token, sheet, false);
            // if (ruleText)
            //     foundStyleRules = true;
        }
        token = getToken(false, false);
    }
}

void CSSParser::parseStyleDeclaration(String* str, CSSStyleDeclaration* declarations)
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


}
