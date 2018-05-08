/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2012 Apple Inc. All rights
 * reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
#include "core/page/Window.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/StyleRule.h"

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
const char* kUNKNOWN_AT_RULE = "Unknown @-rule";

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
constexpr size_t kLexTableSize = sizeof kLexTable / sizeof(int);

const char* unitTypeToString(UnitType type)
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

class CSSParser;

void* CSSToken::operator new(size_t size, CSSParser* parser)
{
    if (!parser) {
        return GC_MALLOC(sizeof(CSSToken));
    }
    if (parser->m_initialTokenMemoryPoolSize) {
        parser->m_initialTokenMemoryPoolSize--;
        return parser
            ->m_initialTokenMemoryPool[parser->m_initialTokenMemoryPoolSize];
    } else if (parser->m_tokenMemoryPool.size() == 0) {
        auto ret = (CSSToken*)GC_MALLOC(sizeof(CSSToken));
        return ret;
    } else {
        auto ret = parser->m_tokenMemoryPool.back();
        parser->m_tokenMemoryPool.pop_back();
        return ret;
    }
}

CSSToken::~CSSToken()
{
    if (m_parser && m_parser->m_isPoolEnabled) {
        if (m_parser->m_initialTokenMemoryPoolSize <
            CSSTOKEN_POOL_INITIAL_SIZE) {
            m_parser->m_initialTokenMemoryPool
                [m_parser->m_initialTokenMemoryPoolSize++] = this;
            return;
        }
        m_parser->m_tokenMemoryPool.push_back(this);
    }
}

class CSSScanner : public gc {
public:
    CSSScanner(CSSParser* parser, String* str)
        : m_parser(parser)
        , m_string(str)
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
            return m_stringBufferData.charAt(m_pos++);
        }
        return -1;
    }

    int peek()
    {
        if (LIKELY(m_pos < m_stringBufferData.length)) {
            return m_stringBufferData.charAt(m_pos);
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
               (aFirstChar == '-' && isIdentStart(aSecondChar)) ||
               (aFirstChar == '-' && aSecondChar == '-');
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

    CSSTokenString gatherIdent(int c)
    {
        CSSTokenString builder;
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
                    return CSSTokenString();
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

    RefPtr<CSSToken> parseIdent(int c)
    {
        CSSTokenString builder = gatherIdent(c);
        int nextChar = peek();
        if ((char32_t)nextChar == '(') {
            builder.appendChar((char32_t)read());
            builder.toLower();
            return CSSToken::createStringValueToken(
                m_parser, CSSToken::FUNCTION_TYPE, std::move(builder));
        }
        return CSSToken::createStringValueToken(m_parser, CSSToken::IDENT_TYPE,
                                                std::move(builder));
    }

    RefPtr<CSSToken> parseURL(int c)
    {
        CSSTokenString builder;
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
                        m_parser, CSSToken::STRING_TYPE, CSSTokenString());
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
        return CSSToken::createStringValueToken(m_parser, CSSToken::STRING_TYPE,
                                                std::move(builder));
    }

    bool isDigit(char32_t c)
    {
        return (c >= '0') && (c <= '9');
    }

    RefPtr<CSSToken> parseComment(int c)
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
        return CSSToken::createToken(m_parser, CSSToken::COMMENT_TYPE);
    }

    RefPtr<CSSToken> parseNumber(int c)
    {
        CSSTokenString s;
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
            CSSTokenString unit = gatherIdent(c);
            UnitType type;
            if (unit.hasASCIIContent()) {
                type = (UnitType)unit.peekASCIIBuffer(
                    [](const char* buf, size_t len, void* data) -> size_t {
                        return lookupUnitType(buf, len);
                    },
                    nullptr);
            } else {
                type = UnitType::UnknownType;
            }
            float f = 0;
            s.peekASCIIBuffer(
                [](const char* buf, size_t len, void* data) -> size_t {
                    *((float*)data) = atof(buf);
                    return 0;
                },
                &f);
            bool hasN = unit.equals("n");
            s.appendOther(unit);
            return CSSToken::createNumberValueToken(
                m_parser, CSSToken::DIMENSION_TYPE, f, std::move(s), type,
                foundDot, hasN);
        } else if (c == '%') {
            float f = 0;
            s.peekASCIIBuffer(
                [](const char* buf, size_t len, void* data) -> size_t {
                    *((float*)data) = atof(buf);
                    return 0;
                },
                &f);
            s.appendChar('%');
            return CSSToken::createNumberValueToken(
                m_parser, CSSToken::PERCENTAGE_TYPE, f, std::move(s),
                UnitType::Percentage, foundDot, false);
        } else if (c != -1) {
            pushback();
        }

        float f = 0;
        s.peekASCIIBuffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                *((float*)data) = atof(buf);
                return 0;
            },
            &f);
        return CSSToken::createNumberValueToken(
            m_parser, CSSToken::NUMBER_TYPE, f, std::move(s),
            UnitType::UnknownType, foundDot, false);
    }

    RefPtr<CSSToken> parseString(int aStop)
    {
        CSSTokenString s;
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
        return CSSToken::createStringValueToken(m_parser, CSSToken::STRING_TYPE,
                                                std::move(s));
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

    RefPtr<CSSToken> parseAtKeyword(int c)
    {
        auto s = gatherIdent(c);
        return CSSToken::createStringValueToken(m_parser, CSSToken::ATRULE_TYPE,
                                                std::move(s));
    }

    RefPtr<CSSToken> nextToken(bool isURL = false)
    {
        int c = read();
        if (c == -1) {
            return CSSToken::createNullToken(m_parser);
        }

        // url starts without \' nor \"
        if (isURL && c != '\'' && c != '"' && c != ')' && c != ' ') {
            return parseURL(c);
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
                            m_parser, CSSToken::SGML_COMMENT_TYPE);
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
                    return CSSToken::createToken(m_parser,
                                                 CSSToken::SGML_COMMENT_TYPE);
                }
                pushback();
            }
            pushback();
        }

        if (startsWithIdent(c, peek())) {
            return parseIdent(c);
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
            eatWhiteSpace(c);
            return CSSToken::createCharValueToken(
                m_parser, CSSToken::WHITESPACE_TYPE, ' ');
        }

        if (c == '|' || c == '~' || c == '^' || c == '$' || c == '*') {
            int nextChar = read();
            if (nextChar == '=') {
                switch (c) {
                case '~':
                    return CSSToken::createToken(m_parser,
                                                 CSSToken::INCLUDES_TYPE);
                case '|':
                    return CSSToken::createToken(m_parser,
                                                 CSSToken::DASHMATCH_TYPE);
                case '^':
                    return CSSToken::createToken(m_parser,
                                                 CSSToken::BEGINSMATCH_TYPE);
                case '$':
                    return CSSToken::createToken(m_parser,
                                                 CSSToken::ENDSMATCH_TYPE);
                case '*':
                    return CSSToken::createToken(m_parser,
                                                 CSSToken::CONTAINSMATCH_TYPE);
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

        return CSSToken::createCharValueToken(m_parser, CSSToken::SYMBOL_TYPE,
                                              (char32_t)c);
    }

protected:
    CSSParser* m_parser;
    String* m_string;
    StringBufferAccessData m_stringBufferData;
    size_t m_pos;
    GCAtomicVector<size_t> m_preservedPos;
};

bool CSSPropertyParser::stringIsIdent(String* v)
{
    CSSScanner scanner(nullptr, v);
    RefPtr<CSSToken> token = scanner.nextToken();
    return token->isIdent();
}

CSSParser::CSSParser(Document* document)
    : DocumentHoldable(document)
    , m_preserveWS(false)
    , m_preserveComments(false)
    , m_scanner(nullptr)
    , m_error(nullptr)
    , m_state()
    , m_parserType(MediaQuerySetParser)
    , m_querySet(nullptr)
    , m_blockLevel(0)
{
    m_error = String::emptyString;
    m_failedParsing = false;

    m_initialTokenMemoryPoolSize = CSSTOKEN_POOL_INITIAL_SIZE;
    CSSToken* ptr = (CSSToken*)m_tokenInnerPool;
    for (size_t i = 0; i < CSSTOKEN_POOL_INITIAL_SIZE; i++) {
        ptr[i].m_parser = this;
        m_initialTokenMemoryPool[i] = &ptr[i];
    }
    m_isPoolEnabled = true;
}

RefPtr<CSSToken> CSSParser::getToken(bool aSkipWS, bool aSkipComment,
                                     bool isURL)
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

RefPtr<CSSToken> CSSParser::currentToken()
{
    return m_token;
}

RefPtr<CSSToken> CSSParser::lookAhead(bool aSkipWS, bool aSkipComment)
{
    RefPtr<CSSToken> preservedToken = m_token;
    RefPtr<CSSToken> preservedLookAhead = m_lookAhead;
    m_scanner->preserveState();
    RefPtr<CSSToken> token = getToken(aSkipWS, aSkipComment);
    m_scanner->restoreState();
    m_token = preservedToken;
    m_lookAhead = preservedLookAhead;

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

void CSSParser::parseSelector(GCVector<CSSSelectorList*>& list,
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

    RefPtr<CSSToken> token = currentToken();
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
    if (prefix->equals("*")) {
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

    RefPtr<CSSToken> token = getToken(false, true);
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

    char32_t* buf =
        ALLOCA(token->value()->length() * sizeof(char32_t), char32_t);
    for (size_t i = 0; i < token->value()->length(); i++) {
        buf[i] = token->value()->charAt(i);
    }

    selector->updatePseudoType(starFish(),
                               token->value()->toAttrAtomicString(starFish()),
                               token->isFunction());

    if (token->isIdent()) {
        if (selector->pseudoType() == CSSSelector::PseudoNone) {
            return nullptr;
        }
        token = getToken(false, true);
        return selector;
    }

    if (selector->pseudoType() == CSSSelector::PseudoNone) {
        return nullptr;
    }

    getToken(true, true);

    switch (selector->pseudoType()) {
    case CSSSelector::PseudoNot: {
        CSSSelectorList selectorList;
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
    case CSSSelector::PseudoDir:
    case CSSSelector::PseudoLang: {
        RefPtr<CSSToken> token = currentToken();
        if (!token->isIdent()) {
            return nullptr;
        }

        selector->setArgument(token->value()->toString());
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
    RefPtr<CSSToken> token = currentToken();

    // in case of only number
    if (token->isNumber() && !token->hasSourceOfNumberValueDot()) {
        result = std::make_pair(0, (int)token->numericValue());
        return true;
    }

    // in case of string (odd and even)
    if (token->isIdent()) {
        if (token->value()->equalsIgnoreCase("odd")) {
            result = std::make_pair(2, 1);
            return true;
        }
        if (token->value()->equalsIgnoreCase("even")) {
            result = std::make_pair(2, 0);
            return true;
        }
    }

    String* nString = String::emptyString;

    // in case of 'an + b'
    if (token->isSymbol('+') && lookAhead(false, true)->isIdent()) { // +n
        result.first = 1;
        nString = getToken(false, true)->value()->toString();
    } else if (token->isDimension() &&
               !token->hasSourceOfNumberValueDot()) { // an+b
        result.first = token->numericValue();
        size_t pos = token->value()->indexOf('n');
        if (pos == SIZE_MAX) {
            return false;
        }
        nString = token->value()->toString()->substring(
            pos, token->value()->length() - pos);
    } else if (token->isIdent()) {              // -n or n
        if (token->value()->charAt(0) == '-') { // -n
            result.first = -1;
            nString = token->value()->toString()->substring(1, 1);
        } else { // n
            result.first = 1;
            nString = token->value()->toString();
        }
    }

    while (lookAhead(false, true)->isWhiteSpace()) {
        token = getToken(false, true);
    }

    if (nString->equals(String::emptyString) ||
        nString->toASCIILower()->charAt(0) != 'n') {
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
            RefPtr<CSSToken> ahead = lookAhead(false, true);
            if (ahead->hasStringValue() && (ahead->value()->charAt(0) == '+' ||
                                            ahead->value()->charAt(0) == '-')) {
                return false;
            }
        } else if (token->isSymbol('-')) {
            RefPtr<CSSToken> ahead = lookAhead(false, true);
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

    RefPtr<CSSToken> b = getToken(false, true);
    if (!b->isNumber() || (b->hasStringValue() && b->value()->contains('.'))) {
        return false;
    }
    /*
        if ((b.numericSign() == NoSign) == (sign == NoSign)) {
            return false;
        }
    */
    if (!b->isNumber()) {
        result.second = 0;
        if (b->hasStringValue()) {
            b->value()->peekASCIIBuffer(
                [](const char* buf, size_t len, void* data) -> size_t {
                    *((int*)data) = atoi(buf);
                    return 0;
                },
                &result.second);
        }
    } else
        result.second = b->numericValue();
    if (sign == MinusSign) {
        result.second = -result.second;
    }
    return true;
}

CSSSelector::Type CSSParser::getAttributeMatch(RefPtr<CSSToken> token)
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
    if (!lookAhead(true, true)->isIdent()) {
        return CSSSelector::CaseSensitive;
    }
    RefPtr<CSSToken> flag = getToken(true, true);
    if (flag->hasStringValue() && flag->value()->equalsIgnoreCase("i")) {
        return CSSSelector::CaseInsensitive;
    }
    m_failedParsing = true;
    return CSSSelector::CaseSensitive;
}

String* CSSParser::getStringWithoutQuotationMarks(const CSSTokenString& value)
{
    size_t curPos = 0;
    size_t endPos = curPos + value.length();

    while (curPos < endPos && String::isSpaceOrNewline(value.charAt(curPos))) {
        curPos++;
    }

    size_t len = 0;
    char32_t mark = '\0';
    if (value.charAt(curPos) == '\\') {
        curPos++;
    }
    if (value.charAt(curPos) == '"' || value.charAt(curPos) == '\'') {
        mark = value.charAt(curPos);
        curPos++;
    }
    size_t start = curPos;
    while (curPos < endPos && value.charAt(curPos) != mark) {
        curPos++;
        len++;
    }
    if (mark != '\0' && mark == value.charAt(curPos)) {
        if (value.charAt(curPos - 1) == '\\') {
            len--;
        }
        curPos++;
        while (curPos < endPos &&
               String::isSpaceOrNewline(value.charAt(curPos))) {
            curPos++;
        }
    }

    struct Sender {
        size_t start, len;
    } s;
    s.start = start;
    s.len = len;
    if (value.hasASCIIContent()) {
        return (String*)value.peekASCIIBuffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                Sender* sf = (Sender*)data;
                return (size_t) new StringDataASCII(buf + sf->start, sf->len);
            },
            &s);
    } else if (value.hasBMPContent()) {
        return (String*)value.peekBMPBuffer(
            [](const char16_t* buf, size_t len, void* data) -> size_t {
                Sender* sf = (Sender*)data;
                return (size_t) new StringDataBMP(buf + sf->start, sf->len);
            },
            &s);
    } else {
        return (String*)value.peekUTF32Buffer(
            [](const char32_t* buf, size_t len, void* data) -> size_t {
                Sender* sf = (Sender*)data;
                return (size_t) new StringDataUTF32(buf + sf->start, sf->len);
            },
            &s);
    }
}

CSSSelector* CSSParser::getAttributeSelector()
{
    RefPtr<CSSToken> token = getToken(true, true);

    CSSTokenString attributeName;
    if (!parseName(attributeName)) {
        return nullptr;
    }

    while (currentToken()->isWhiteSpace()) {
        getToken(false, true);
    }

    QualifiedName attrQualifiedName =
        QualifiedName(AtomicString::emptyAtomicString(),
                      attributeName.toAttrAtomicString(starFish()));

    if (currentToken()->isSymbol(']')) {
        getToken(false, false);
        return new CSSAttributeSelector(
            CSSSelector::Type::AttributeSet, attrQualifiedName,
            String::emptyString, CSSSelector::AttributeMatchType::CaseSensitive,
            CSSSelector::RelationType::SubSelector);
    }

    auto type = getAttributeMatch(currentToken());

    RefPtr<CSSToken> attributeValue = getToken(true, true);
    if (!attributeValue->isIdent() && !attributeValue->isString()) {
        return nullptr;
    }

    CSSSelector::AttributeMatchType flag = getAttributeFlags();

    token = getToken(true, false);
    getToken(false, false);

    if (!token->isSymbol(']')) {
        return nullptr;
    }

    return new CSSAttributeSelector(
        type, attrQualifiedName,
        getStringWithoutQuotationMarks(*attributeValue->value()), flag,
        CSSSelector::RelationType::SubSelector);
}

CSSSelector* CSSParser::getClassSelector()
{
    RefPtr<CSSToken> token = getToken(false, true);
    if (!token->isIdent()) {
        return nullptr;
    }

    CSSSelector* selector = new (PointerFreeGC)
        CSSSelector(CSSSelector::Type::Class, CSSSelector::SubSelector,
                    token->value()->toAtomicString(starFish()));
    getToken(false, true);

    return selector;
}

CSSSelector* CSSParser::getIdSelector()
{
    RefPtr<CSSToken> token = getToken(false, true);
    if (!token->isIdent()) {
        return nullptr;
    }

    CSSSelector* selector = new (PointerFreeGC)
        CSSSelector(CSSSelector::Type::Id, CSSSelector::SubSelector,
                    token->value()->toAtomicString(starFish()));
    getToken(false, true);

    return selector;
}

CSSSelector* CSSParser::getSimpleSelector()
{
    RefPtr<CSSToken> token = currentToken();
    CSSSelector* selector = nullptr;
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

bool CSSParser::parseName(CSSTokenString& name)
{
    RefPtr<CSSToken> firstToken = currentToken();
    if (firstToken->isIdent()) {
        name = *firstToken->value();
        getToken(false, true);
    } else if (firstToken->isSymbol('*')) {
        name.appendChar('*');
        getToken(false, true);
    } else if (firstToken->isSymbol('|')) {
    } else {
        return false;
    }

    if (!firstToken->isSymbol('|')) {
        return true;
    }

    name.clear();

    RefPtr<CSSToken> nameToken = getToken(true, true);
    if (nameToken->isIdent()) {
        name = *firstToken->value();
    } else if (nameToken->isSymbol('*')) {
        name.appendChar('*');
    } else {
        return false;
    }

    return true;
}

void CSSParser::parseCompoundSelector(CSSSelectorList* selectorList)
{
    CSSSelector* compoundSelector;

    CSSTokenString elementName;
    CSSSelector::PseudoType compoundPseudoElement = CSSSelector::PseudoNone;
    if (!parseName(elementName)) {
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

    if (elementName.length()) {
        bool isStar = elementName.equals("*");
        if (isStar && selectorList->size() > 0) {
            return;
        }

        CSSSelector* selector = new CSSSelector(
            isStar ? CSSSelector::Type::Universal : CSSSelector::Type::Tag,
            CSSSelector::RelationType::SubSelector,
            elementName.toAttrAtomicString(starFish()));
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

void CSSParser::parseComplexSelector(CSSSelectorList* selectorList)
{
    RefPtr<CSSToken> token = currentToken();
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

    while (CSSSelector::RelationType combinator = parseCombinator()) {
        CSSSelectorList secondSelectorList;

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
            end->setRelationIsAffectedByPseudoContent();
        }
        previousCompoundFlags = compoundFlags;
        selectorList->insert(selectorList->begin(), secondSelectorList.begin(),
                             secondSelectorList.end());
    }
}

bool CSSParser::parseComplexSelectorList(
    GCVector<CSSSelectorList*>& listOfSelectorList)
{
    CSSSelectorList* selectorList = new (GC) CSSSelectorList();
    parseComplexSelector(selectorList);

    if (selectorList->size() == 0) {
        return false;
    }

    listOfSelectorList.push_back(selectorList);

    RefPtr<CSSToken> token = currentToken();
    while (token->isNotNull() && token->isSymbol(',')) {
        do {
            token = getToken(false, true);
        } while (token->isSGMLComment() || token->isWhiteSpace());

        CSSSelectorList* nextSelectorList = new (GC) CSSSelectorList();
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

CSSTokenString CSSParser::parseDefaultPropertyValue(RefPtr<CSSToken> token)
{
    GCVector<RefPtr<CSSToken>> willBeConcat;
    GCVector<RefPtr<CSSToken>> blocks;
    // bool foundPriority = false;
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
        if (token->isSymbol('{') || token->isSymbol('(') ||
            token->isSymbol('[') || token->isFunction()) {
            if (token->isFunction() && token->value()->equals("url(")) {
                blocks.push_back(token);
                isURLFunc = true;
            } else {
                if (token->isFunction()) {
                    blocks.push_back(CSSToken::createCharValueToken(
                        this, CSSToken::SYMBOL_TYPE, '('));
                } else {
                    blocks.push_back(token);
                }
            }
        } else if (token->isSymbol('}') || token->isSymbol(')') ||
                   token->isSymbol(']')) {
            if (blocks.size()) {
                RefPtr<CSSToken> ontop = blocks.back();
                if ((token->isSymbol('}') && ontop->isSymbol('{')) ||
                    (token->isSymbol(')') && ontop->isSymbol('(')) ||
                    (token->isSymbol(']') && ontop->isSymbol('['))) {
                    blocks.pop_back();
                } else if (token->isSymbol(')') &&
                           ontop->value()->equalsIgnoreCase("url(")) {
                    blocks.pop_back();
                    if (urlTokens > 2) {
                        return CSSTokenString();
                    }
                    isURLFunc = false;
                    urlTokens = 0;
                } else {
                    return CSSTokenString();
                }
            } else {
                return combineAndTrimTokenValues(willBeConcat);
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
CSSTokenString CSSParser::combineAndTrimTokenValues(
    const GCVector<RefPtr<CSSToken>>& list)
{
    CSSTokenString result;
    for (RefPtr<CSSToken> item : list) {
        if (item->hasStringValue()) {
            auto s = item->value();
            for (size_t i = 0; i < s->length(); i++) {
                result.appendChar(s->charAt(i));
            }
        }
    }
    return result;
}

CSSParser::ParseResult CSSParser::parseDeclaration(
    RefPtr<CSSToken> aToken, CSSStyleDeclaration* declaration,
    bool allowSrcProperty)
{
    preserveState();
    GCVector<RefPtr<CSSToken>> blocks;
    if (aToken->isIdent()) {
        RefPtr<CSSToken> token = getToken(true, true);
        if (token->isSymbol(':')) {
            token = getToken(true, true);
            CSSTokenString value = parseDefaultPropertyValue(token);
            token = currentToken();
            if (value.length()) { // no error above
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
                            return ParseResult::Consumed;
                        }
                    } else {
                        return ParseResult::Consumed;
                    }
                }

                if (!aToken->value()->hasASCIIContent()) {
                    auto s = aToken->value()->toString()->toUTF8NonGCString();
                    STARFISH_LOG_ERROR("CSSParser: Unsupported property: %s\n",
                                       s.data());
                } else {
                    struct Sender {
                        bool priority;
                        bool allowSrcProperty;
                        CSSStyleValuePair::KeyKind kind;
                        String* key;
                        CSSTokenString* value;
                        CSSStyleDeclaration* declaration;
                    } sender;
                    sender.value = &value;
                    sender.allowSrcProperty = allowSrcProperty;
                    sender.priority = priority;
                    sender.declaration = declaration;
                    aToken->value()->peekASCIIBuffer(
                        [](const char* buf, size_t len, void* data) -> size_t {

                            // We can modify content of `buf`.
                            // peekASCIIBuffer function allocates new buffer for
                            // this function.
                            char* name = (char*)buf;
                            for (size_t i = 0; i < len; i++) {
                                name[i] = tolower(name[i]);
                            }
                            ((Sender*)data)->kind = lookupCSSStyle(name, len);

                            if (((Sender*)data)->kind ==
                                CSSStyleValuePair::KeyKind::CustomProperty) {
                                ((Sender*)data)->key =
                                    String::createASCIIString(name);
                            }
#ifndef NDEBUG
                            // ignore vendor prefix & CSS Custom Variables
                            if (((Sender*)data)->kind ==
                                    CSSStyleValuePair::KeyKind::Unknown &&
                                len && name[0] != '-') {
                                STARFISH_LOG_ERROR(
                                    "CSSParser: Unsupported property: %s\n",
                                    name);
                            }
#endif
                            if (((Sender*)data)->kind ==
                                    CSSStyleValuePair::KeyKind::Src &&
                                !((Sender*)data)->allowSrcProperty) {
                                return 0;
                            }

                            ((Sender*)data)
                                ->value->peekUTF8Buffer(
                                    [](const char* value, size_t len,
                                       void* data) -> size_t {
                                        bool priority =
                                            ((Sender*)data)->priority;
                                        CSSStyleDeclaration* declaration =
                                            ((Sender*)data)->declaration;
                                        CSSStyleValuePair::KeyKind kind =
                                            ((Sender*)data)->kind;

                                        if (kind == CSSStyleValuePair::KeyKind::
                                                        CustomProperty) {
                                            // https://www.w3.org/TR/css-variables-1/
                                            String* key = ((Sender*)data)->key;
                                            declaration->setCustomProperty(
                                                key, String::createASCIIString(
                                                         value),
                                                len);
                                        }
#define SET_ATTR(name, nameLower, nameCSSCase)         \
    else if (kind == CSSStyleValuePair::KeyKind::name) \
    {                                                  \
        declaration->set##name(value, len, priority);  \
    }
                                        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
                                        return 0;
                                    },
                                    data);
                            return 0;
                        },
                        &sender);
                }
                return ParseResult::Consumed;
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
        return ParseResult::Consumed;
    }

    // we have an error here, let's skip it
    restoreState();
    blocks.clear();
    RefPtr<CSSToken> token = aToken;
    bool isURLFunc = false;

    while (token->isNotNull()) {
        if (token->isSymbol(';') && blocks.size() == 0) {
            break;
        } else if (token->isSymbol('}') && blocks.size() == 0) {
            ungetToken();
            break;
        } else if (token->isSymbol('{') || token->isSymbol('(') ||
                   token->isSymbol('[') || token->isFunction()) {
            if (token->isFunction() &&
                token->value()->equalsIgnoreCase("url(")) {
                blocks.push_back(token);
                isURLFunc = true;
            } else {
                if (token->isFunction()) {
                    blocks.push_back(CSSToken::createCharValueToken(
                        this, CSSToken::SYMBOL_TYPE, '('));
                } else {
                    blocks.push_back(token);
                }
            }
        } else if (token->isSymbol('}') || token->isSymbol(')') ||
                   token->isSymbol(']')) {
            if (blocks.size()) {
                RefPtr<CSSToken> ontop = blocks.back();
                if ((token->isSymbol('}') && ontop->isSymbol('{')) ||
                    (token->isSymbol(')') && ontop->isSymbol('(')) ||
                    (token->isSymbol(']') && ontop->isSymbol('['))) {
                    blocks.pop_back();
                } else if (token->isSymbol(')') &&
                           ontop->value()->equalsIgnoreCase("url(")) {
                    blocks.pop_back();
                    isURLFunc = false;
                }
            }
        }
        if (isURLFunc) {
            token = getToken(true, false, true);
            if (token->isString()) {
                String* tokenStr = token->value()->toString();
                if (tokenStr->startsWith(String::fromUTF8("'")) ||
                    tokenStr->startsWith(String::fromUTF8("\""))) {
                    // https://drafts.csswg.org/css-values-3/#urls
                    return ParseResult::ErrorFounded;
                }
            }
        } else {
            token = getToken(false, false);
        }
    }
    return ParseResult::Consumed;
}

CSSParser::ParseResult CSSParser::parseStyleRule(
    RefPtr<CSSToken> aToken, GCVector<StyleRuleBase*>& rules,
    AllowedRulesType allowedRules, GCVector<CSSSelectorList*>* sList,
    bool isQueryingSelector)
{
    if (allowedRules > RegularRules) {
        return ParseResult::Failed;
    }

    // size_t currentLine = countLF(m_scanner->getAlreadyScanned());
    preserveState();
    // first let's see if we have a selector here...
    bool validSelector = true;

    GCVector<CSSSelectorList*> list;
    parseSelector(list, validSelector);

    bool valid = false;
    bool invalidDeclaration = false;
    CSSStyleDeclaration* declarations = new CSSStyleDeclaration(document());
    if (list.size()) {
        RefPtr<CSSToken> token = currentToken();
        if (token->isSymbol('{')) {
            RefPtr<CSSToken> token = getToken(true, false);
            while (true) {
                if (!token->isNotNull() || token->isSymbol('}')) {
                    valid = true;
                    break;
                } else {
                    if (parseDeclaration(token, declarations) ==
                        ParseResult::ErrorFounded) {
                        valid = true;
                        invalidDeclaration = true;
                        break;
                    }
                }
                token = getToken(true, false);
            }
        } else if (isQueryingSelector) {
            valid = true;
        }
    } else if (!validSelector) {
        if (isQueryingSelector) {
            forgetState();
            return ParseResult::Failed;
        } else {
            // selector is invalid so the whole rule is invalid with it
            RefPtr<CSSToken> token = getToken(true, true);
            while (!token->isSymbol('{') && token->isNotNull()) {
                token = getToken(true, false);
            }
            if (token->isSymbol('{')) {
                token = getToken(true, false);
            }
            while (true) {
                if (!token->isNotNull() || token->isSymbol('}')) {
                    forgetState();
                    return ParseResult::Failed;
                } else {
                    parseDeclaration(token, declarations);
                }
                token = getToken(true, false);
            }
        }
    }

    if (isQueryingSelector) {
        if (!getToken(true, false)->isNull()) {
            sList->clear();
            return ParseResult::Failed;
        }
    }

    if (valid) {
        if (isQueryingSelector) {
            sList->assign(list.begin(), list.end());
        } else {
            unsigned size = list.size();
            for (unsigned i = 0; i < size; ++i) {
                rules.push_back(
                    new StyleRule(std::move(*list[i]), declarations));
            }
        }
        forgetState();
        return invalidDeclaration ? ParseResult::ErrorFounded
                                  : ParseResult::Consumed;
    }
    restoreState();
    addUnknownAtRule();

    return ParseResult::Failed;
}

void CSSParser::addUnknownAtRule()
{
    GCVector<RefPtr<CSSToken>> blocks;
    RefPtr<CSSToken> token = getToken(true, false);
    while (token->isNotNull()) {
        if (token->isSymbol(';') && !blocks.size()) {
            break;
        } else if (token->isSymbol('{') || token->isSymbol('(') ||
                   token->isSymbol('[') || token->isFunction()) {
            if (token->isFunction()) {
                blocks.push_back(CSSToken::createCharValueToken(
                    this, CSSToken::SYMBOL_TYPE, '('));
            } else {
                blocks.push_back(token);
            }
        } else if (token->isSymbol('}') || token->isSymbol(')') ||
                   token->isSymbol(']')) {
            if (blocks.size()) {
                RefPtr<CSSToken> ontop = blocks.back();
                if ((token->isSymbol('}') && ontop->isSymbol('{')) ||
                    (token->isSymbol(')') && ontop->isSymbol('(')) ||
                    (token->isSymbol(']') && ontop->isSymbol('['))) {
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
    CSSParser::AllowedRulesType allowedRules, StyleRuleBase* rule)
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

bool CSSParser::parseCharsetRule(GCVector<StyleRuleBase*>& rules)
{
    RefPtr<CSSToken> token = getToken(false, false);
    StringBuilder s;
    if (token->isAtRule("@charset") &&
        token->value()->equals("@charset")) { // lowercase check
        s.appendString(token->value()->toString());
        token = getToken(false, false);
        s.appendString(token->value()->toString());
        if (token->isWhiteSpace(' ')) {
            token = getToken(false, false);
            s.appendString(token->value()->toString());
            if (token->isString()) {
                // String* encoding = token->m_value;
                token = getToken(false, false);
                s.appendString(token->value()->toString());
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

RefPtr<CSSToken> CSSParser::makeToken(String* str)
{
    m_lookAhead = nullptr;
    m_token = nullptr;
    m_preserveWS = false;
    m_preserveComments = false;
    m_scanner = new CSSScanner(this, str);

    return getToken(false, false);
}

void CSSParser::consumeComponentValue(RefPtr<CSSToken>& token)
{
    unsigned nestingLevel = 0;

    do {
        if (token->isFunction() || token->isSymbol('{') ||
            token->isSymbol('(') || token->isSymbol('[')) {
            nestingLevel++;
        } else if (token->isSymbol('}') || token->isSymbol(')') ||
                   token->isSymbol(']')) {
            nestingLevel--;
        }
        token = getToken(false, true);
    } while (nestingLevel && token->isNotNull());
}

StyleRuleMedia* CSSParser::parseMediaRule(bool isInsertedByUser)
{
    preserveState();
    RefPtr<CSSToken> token = getToken(true, true);

    while (token->isNotNull() && !token->isSymbol('{') &&
           !token->isSymbol(';')) {
        consumeComponentValue(token);
    }

    if (token->isSymbol(';')) {
        ungetToken();
        forgetState();
        return nullptr;
    }
    restoreState();

    preserveState();
    token = getToken(true, true);

    bool hasMediaRule = false;
    MediaQuerySet* mediaQuerySet;
    if (token->isNotNull()) {
        mediaQuerySet = parseMediaQuery();
        hasMediaRule = true;
    } else {
        forgetState();
        return nullptr;
    }

    token = currentToken();
    if (token->isSymbol('}') || token->isSymbol(';')) {
        forgetState();
        return nullptr;
    }

    GCVector<StyleRuleBase*> rootRule;
    if (token->isSymbol('{') && hasMediaRule) {
        parseRules(token, rootRule, RuleListType::RegularRuleList,
                   isInsertedByUser);
        forgetState();
        return new StyleRuleMedia(mediaQuerySet, rootRule);
    }

    forgetState();
    return nullptr;
}

StyleRuleImport* CSSParser::parseImportRule()
{
    preserveState();

    RefPtr<CSSToken> token = getToken(true, true);

    while (token->isNotNull() && !token->isSymbol('{') &&
           !token->isSymbol(';')) {
        consumeComponentValue(token);
    }

    if (!token->isSymbol(';')) {
        ungetToken();
        forgetState();
        return nullptr;
    }

    restoreState();

    Nullable<String*> url = parseURLString();
    if (!url.hasValue()) {
        return nullptr;
    }

    getToken(true, false);
    MediaQuerySet* mediaQuery = parseMediaQuery();
    return new StyleRuleImport(url.getValue(), mediaQuery);
}

static bool isFontRelatedKey(CSSStyleValuePair::KeyKind key)
{
    if ((key >= CSSStyleValuePair::KeyKind::FontSize &&
         key <= CSSStyleValuePair::KeyKind::FontKerning) ||
        key == CSSStyleValuePair::KeyKind::FontFamily ||
        key == CSSStyleValuePair::KeyKind::Src) {
        return true;
    }
    return false;
}

StyleRuleFontFace* CSSParser::parseFontFaceRule()
{
    preserveState();

    CSSStyleDeclaration* decl = new CSSStyleDeclaration(document());

    RefPtr<CSSToken> token = getToken(true, false);
    bool valid = false;
    if (token->isSymbol('{')) {
        RefPtr<CSSToken> token = getToken(true, false);
        while (true) {
            if (!token->isNotNull()) {
                valid = true;
                break;
            }
            if (token->isSymbol('}')) {
                valid = true;
                break;
            } else {
                parseDeclaration(token, decl, true);
            }
            token = getToken(true, false);
        }
    }

    if (valid) {
        forgetState();

        for (size_t i = 0; i < decl->cssValues().size(); i++) {
            auto keyKind = decl->cssValues()[i].keyKind();
            if (!isFontRelatedKey(keyKind)) {
                decl->removeCSSValuePair(keyKind);
                i--;
            }
        }

        if (decl->hasCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily)) {
            return new StyleRuleFontFace(decl);
        } else if (decl->hasCSSValuePair(
                       CSSStyleValuePair::KeyKind::FontSize) ||
                   decl->hasCSSValuePair(
                       CSSStyleValuePair::KeyKind::FontWeight) ||
                   decl->hasCSSValuePair(
                       CSSStyleValuePair::KeyKind::FontStyle) ||
                   decl->hasCSSValuePair(
                       CSSStyleValuePair::KeyKind::FontKerning) ||
                   decl->hasCSSValuePair(CSSStyleValuePair::KeyKind::Src)) {
            return new StyleRuleFontFace(new CSSStyleDeclaration(document()));
        } else {
            return nullptr;
        }
    }

    restoreState();
    addUnknownAtRule();

    return nullptr;
}

StyleRuleSupports* CSSParser::parseSupportsRule()
{
    // https://drafts.csswg.org/css-conditional-3/#at-supports
    preserveState();

    String* conditionText = String::emptyString;
    {
        preserveState();
        StringBuilder b;
        RefPtr<CSSToken> token;
        while ((token = getToken(false, true))->isNotNull()) {
            if (token->isSymbol('{') || token->isSymbol('}')) {
                break;
            }
            b.appendString(token->value()->toString());
        }
        conditionText = b.finalize()->trim();
        restoreState();
    }

    m_supportOperandStack.clear();
    m_supportOperatorStack.clear();
    if (!parseSupportsCondition()) {
        restoreState();
        return nullptr;
    }

    if (!parseGroupRuleBody()) {
        restoreState();
        return nullptr;
    }

    doLogicOperation();
    bool isSupported = m_supportOperandStack.back();
    GCVector<StyleRuleBase*> rules;
    StyleRuleSupports* supportsRule =
        new StyleRuleSupports(conditionText, isSupported, rules);

    forgetState();
    return supportsRule;
}

bool CSSParser::parseSupportsCondition()
{
    preserveState();

    if (parseSupportsNegation()) {
        forgetState();
        return true;
    }

    if (!parseSupportsConditionInParen()) {
        restoreState();
        return false;
    }

    // optional
    String* conjoiner = lookAhead(true, true)->value()->toString()->toLower();
    bool ok = true;
    if (conjoiner->equals("and") || conjoiner->equals("or")) {
        preserveState();

        while (lookAhead(true, true)
                   ->value()
                   ->toString()
                   ->equalsIgnoreCase(conjoiner)) {
            RefPtr<CSSToken> token = getToken(true, true);

            if (!parseSupportsConditionInParen()) {
                restoreState();
                ok = false;
                break;
            }
        }

        if (ok) {
            forgetState();
        }
    }

    if (ok) {
        forgetState();
        return true;
    }

    restoreState();
    return false;
}

bool CSSParser::parseGroupRuleBody()
{
    // TODO: parse group rules
    return true;
}

bool CSSParser::parseSupportsNegation()
{
    preserveState();

    RefPtr<CSSToken> token = getToken(true, true);
    if (!token->value()->toString()->equalsIgnoreCase("not")) {
        restoreState();
        return false;
    }

    m_supportOperandStack.push_back(Paren);
    if (!parseSupportsConditionInParen()) {
        restoreState();
        return false;
    }

    doLogicOperation();
    TruthOp last = m_supportOperandStack.back();
    if (last == True) {
        m_supportOperandStack.back() = False;
    } else {
        m_supportOperandStack.back() = True;
    }

    forgetState();
    return true;
}

void CSSParser::doLogicOperation()
{
    TruthOp result = False;
    while (m_supportOperatorStack.size() > 0) {
        LogicOp op = m_supportOperatorStack.back();
        m_supportOperatorStack.pop_back();

        STARFISH_ASSERT(m_supportOperandStack.size() >= 2);
        TruthOp rightOperand = m_supportOperandStack.back();
        m_supportOperandStack.pop_back();
        TruthOp leftOperand = m_supportOperandStack.back();
        m_supportOperandStack.pop_back();

        if (op == And) {
            result = (TruthOp)(leftOperand && rightOperand);
        } else {
            result = (TruthOp)(leftOperand || rightOperand);
        }

        if (m_supportOperandStack.back() == Paren) {
            m_supportOperandStack.pop_back();
            m_supportOperandStack.push_back(result);
            break;
        } else {
            m_supportOperandStack.push_back(result);
        }
    }
}

bool CSSParser::parseSupportsConnectives(String* conjoiner)
{
    preserveState();

    if (!parseSupportsConditionInParen()) {
        restoreState();
        return false;
    }

    int count = 0;
    while (lookAhead(true, true)
               ->value()
               ->toString()
               ->equalsIgnoreCase(conjoiner)) {
        RefPtr<CSSToken> token = getToken(true, true);
        if (!token->value()->toString()->equalsIgnoreCase(conjoiner)) {
            restoreState();
            return false;
        }

        if (!parseSupportsConditionInParen()) {
            restoreState();
            return false;
        }
        count++;
    };

    if (count >= 1) {
        if (conjoiner->equals("and")) {
            m_supportOperatorStack.push_back(And);
        } else {
            m_supportOperatorStack.push_back(Or);
        }

        forgetState();
        return true;
    }

    restoreState();
    return false;
}

bool CSSParser::parseSupportsConditionInParen()
{
    preserveState();

    if (parseSupportsConditionInParenSub()) {
        forgetState();
        return true;
    }

    if (parseSupportsDeclarationCondition()) {
        forgetState();
        return true;
    }

    if (parseGeneralEnclosed()) {
        forgetState();
        return true;
    }

    restoreState();
    return false;
}

bool CSSParser::parseSupportsConditionInParenSub()
{
    preserveState();

    if (!consumeToken(String::createASCIIString("("))) {
        restoreState();
        return false;
    }

    if (!parseSupportsCondition()) {
        restoreState();
        return false;
    }

    if (!consumeToken(String::createASCIIString(")"))) {
        restoreState();
        return false;
    }

    forgetState();
    return true;
}

bool CSSParser::parseSupportsDeclarationCondition()
{
    preserveState();

    if (!consumeToken(String::createASCIIString("("))) {
        restoreState();
        return false;
    }

    // NOTE: parseDeclaration() requires to read its key before calling the
    // function

    // Terms (that are part of the support grammar) cannot be CSS keys
    RefPtr<CSSToken> key = getToken(true, true);
    if (key->value()->toString()->equals("(") ||
        key->value()->toString()->equalsIgnoreCase("not")) {
        restoreState();
        return false;
    }

    {
        preserveState();
        if (!consumeToken(String::createASCIIString(":"))) {
            restoreState(); // for looking at ':' token
            restoreState(); // for terminating this function
            return false;
        }
        restoreState();
    }

    CSSStyleDeclaration* decl = new CSSStyleDeclaration(document());
    ParseResult r = parseDeclaration(key, decl);

    if (decl->cssText()->equals(String::emptyString)) {
        m_supportOperandStack.push_back(False);
    } else {
        m_supportOperandStack.push_back(True);
    }

    // NOTE: parseDeclaration() reads the next token.
    ungetToken();
    if (!consumeToken(String::createASCIIString(")"))) {
        restoreState();
        return false;
    }

    forgetState();
    return true;
}

bool CSSParser::parseGeneralEnclosed()
{
    preserveState();
    restoreState();
    return false;
}

bool CSSParser::consumeToken(String* token)
{
    RefPtr<CSSToken> next = lookAhead(true, true);
    if (next->value()->toString()->equals(token)) {
        getToken(true, true);
        return true;
    }

    return false;
}

StyleRuleCounterStyle* CSSParser::parseCounterStyleRule()
{
    // TODO: Parse @counter-style rules
    return nullptr;
}

StyleRuleNamespace* CSSParser::parseNamespaceRule()
{
    preserveState();
    RefPtr<CSSToken> token = getToken(true, true);

    while (token->isNotNull() && !token->isSymbol('{') &&
           !token->isSymbol(';')) {
        consumeComponentValue(token);
    }

    if (!token->isSymbol(';')) {
        ungetToken();
        forgetState();
        return nullptr;
    }
    restoreState();

    preserveState();
    token = getToken(true, true);

    String* prefix = String::emptyString;
    if (token->isIdent()) {
        prefix = token->value()->toString();
    } else {
        ungetToken();
    }

    Nullable<String*> namespaceURI = parseURLString();
    if (!namespaceURI.hasValue()) {
        ungetToken();
        forgetState();
        return nullptr;
    }

    token = currentToken();
    while (token->isNotNull() && !token->isSymbol(';')) {
        token = getToken(true, true);
    }

    forgetState();
    return new StyleRuleNamespace(namespaceURI.getValue(), prefix);
}

StyleRuleKeyframes* CSSParser::parseKeyframesRule()
{
    // TODO: Parse @keyframes CSS at-rule.
    // https://drafts.csswg.org/css-animations/#keyframes
    return nullptr;
}

Nullable<String*> CSSParser::parseURLString()
{
    RefPtr<CSSToken> token = getToken(true, true);

    CSSTokenString urlSource;
    if (token->isString()) {
        urlSource.appendChar('u');
        urlSource.appendChar('r');
        urlSource.appendChar('l');
        urlSource.appendChar('(');
        urlSource.appendOther(*token->value());
        urlSource.appendChar(')');
    } else if (token->isFunction() && token->value()->equals("url(")) {
        urlSource = *token->value();
        token = getToken(true, false);
        while (token->isNotNull() && !token->isSymbol(')')) {
            urlSource.appendOther(*(token->value()));
            token = getToken(true, false);
        }
        urlSource.appendOther(*(token->value()));
    } else {
        return nullptr;
    }

    String* ret = String::emptyString;
    urlSource.peekUTF8Buffer(
        [](const char* str, size_t len, void* data) -> size_t {
            String** ret = (String**)data;
            CSSStyleValuePair value;
            CSSPropertyParser::parseUrl(str, &value);
            *ret = value.value().m_stringValue;
            return 0;
        },
        &ret);
    return ret;
}

void CSSParser::parseStyleSheet(String* sourceString, CSSStyleSheet* target)
{
    // @charset can only appear at first char of the stylesheet
    RefPtr<CSSToken> token = makeToken(sourceString);
    if (!token->isNotNull()) {
        return;
    }

    GCVector<StyleRuleBase*> rules;
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

void CSSParser::parseRules(RefPtr<CSSToken> token,
                           GCVector<StyleRuleBase*>& rootRule,
                           RuleListType ruleListType, bool isInsertedByUser)
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

    unsigned nestingLevel = 0;
    while (true) {
        if (!token->isNotNull()) {
            break;
        }

        if (token->isSymbol('{')) {
            nestingLevel++;
            token = getToken(false, true);
            continue;
        } else if (token->isSymbol('}')) {
            if (--nestingLevel == 0) {
                break;
            }
        }

        if (token->isWhiteSpace()) {
        } else if (token->isComment()) {
        } else if (token->isAtRule()) {
            StyleRuleBase* rule = nullptr;

            if (allowedRules <= AllowImportRules &&
                token->isAtRule("@import")) {
                rule = parseImportRule();
            } else if (token->isAtRule("@media")) {
                rule = parseMediaRule(isInsertedByUser);
                if (isInsertedByUser && lookAhead(true, false)->isSymbol(';')) {
                    rule = nullptr;
                }
            } else if (token->isAtRule("@font-face")) {
                rule = parseFontFaceRule();
            } else if (token->isAtRule("@supports")) {
                rule = parseSupportsRule();
            } else if (token->isAtRule("@counter-style")) {
                rule = parseCounterStyleRule();
            } else if (token->isAtRule("@namespace")) {
                rule = parseNamespaceRule();
            } else if (token->isAtRule("@keyframes")) {
                rule = parseKeyframesRule();
            }

            if (rule) {
                allowedRules = computeNewAllowedRules(allowedRules, rule);
                rootRule.push_back(rule);
            } else {
                addUnknownAtRule();
            }
        } else {
            // plain style rules
            GCVector<StyleRuleBase*> rules;
            CSSParser::ParseResult res =
                parseStyleRule(token, rules, allowedRules, nullptr, false);
            if (res != ParseResult::Failed) {
                allowedRules = computeNewAllowedRules(allowedRules, rules[0]);
                rootRule.insert(rootRule.end(), rules.begin(), rules.end());
                if (res == ParseResult::ErrorFounded) {
                    // If quoted <string> 'url()' contains an error, we do not
                    // need to process the contents of the remaining stylesheet.
                    break;
                }
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
    m_scanner = new CSSScanner(this, str);
    RefPtr<CSSToken> token = getToken(true, false);
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

bool CSSParser::isBlockStart(RefPtr<CSSToken> token) const
{
    if ((token->isSymbol('{') || token->isSymbol('(') || token->isSymbol('[') ||
         token->isFunction())) {
        return true;
    }
    return false;
}

bool CSSParser::isBlockEnd(RefPtr<CSSToken> token) const
{
    if ((token->isSymbol('}') || token->isSymbol(')') ||
         token->isSymbol(']'))) {
        return true;
    }
    return false;
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
    m_blockLevel = 0;
    m_querySet = MediaQuerySet::create(m_document);
    if (parserType == MediaQuerySetParser)
        m_state = &CSSParser::readRestrictor;
    else // MediaConditionParser
        m_state = &CSSParser::readMediaNot;
}

void CSSParser::handleBlocks(RefPtr<CSSToken> token)
{
    if (isBlockStart(token)) {
        if (!token->isSymbol('(') || m_blockLevel) {
            m_state = SkipUntilBlockEnd;
        }
    }
}

void CSSParser::handleToken(RefPtr<CSSToken> token)
{
    if (isBlockStart(token)) {
        ++m_blockLevel;
    } else if (isBlockEnd(token)) {
        STARFISH_ASSERT(m_blockLevel);
        --m_blockLevel;
    }
}

void CSSParser::processToken(RefPtr<CSSToken> token)
{
    handleBlocks(token);
    handleToken(token);
    // Call the function that handles current state
    if (!token->isWhiteSpace()) {
        ((this)->*(m_state))(token);
    }
}

MediaQuerySet* CSSParser::parseMediaQuery()
{
    initParseMediaQuery(MediaQuerySetParser);

    RefPtr<CSSToken> token = currentToken();
    while (token->isNotNull() && !(token->isSymbol('{') && !m_blockLevel) &&
           m_state != Done) {
        processToken(token);
        token = getToken(false, true);
    }
    processToken(CSSToken::createNullToken(this));

    if (m_state != ReadAnd && m_state != ReadRestrictor && m_state != Done &&
        m_state != ReadMediaNot) {
        m_querySet->addMediaQuery(MediaQuery::createNotAll());
    } else if (m_mediaQueryData.currentMediaQueryChanged()) {
        m_querySet->addMediaQuery(m_mediaQueryData.mediaQuery());
    }

    return m_querySet;
}

void CSSParser::setStateAndRestrict(State state,
                                    MediaQuery::RestrictorType restrictor)
{
    m_mediaQueryData.setRestrictor(restrictor);
    m_state = state;
}

// State machine member functions start here
void CSSParser::readRestrictor(RefPtr<CSSToken> token)
{
    readMediaType(token);
}

void CSSParser::readMediaNot(RefPtr<CSSToken> token)
{
    if (token->isIdent() && token->value()->equalsIgnoreCase("not"))
        setStateAndRestrict(ReadFeatureStart, MediaQuery::Not);
    else
        readFeatureStart(token);
}

static bool isRestrictorOrLogicalOperator(RefPtr<CSSToken> token)
{
    STARFISH_ASSERT(token->isIdent());
    CSSTokenString* val = token->value();
    return val->equalsIgnoreCase("not") || val->equalsIgnoreCase("and") ||
           val->equalsIgnoreCase("or") || val->equalsIgnoreCase("only");
}

void CSSParser::readMediaType(RefPtr<CSSToken> token)
{
    if (token->isSymbol('(')) {
        if (m_mediaQueryData.restrictor() != MediaQuery::None)
            m_state = SkipUntilComma;
        else
            m_state = ReadFeature;
    } else if (token->isIdent()) {
        if (m_state == ReadRestrictor &&
            token->value()->equalsIgnoreCase("not")) {
            setStateAndRestrict(ReadMediaType, MediaQuery::Not);
        } else if (m_state == ReadRestrictor &&
                   token->value()->equalsIgnoreCase("only")) {
            setStateAndRestrict(ReadMediaType, MediaQuery::Only);
        } else if (m_mediaQueryData.restrictor() != MediaQuery::None &&
                   isRestrictorOrLogicalOperator(token)) {
            m_state = SkipUntilComma;
        } else {
            m_mediaQueryData.setMediaType(token->value()->toString());
            m_state = ReadAnd;
        }
    } else if ((token->isSymbol('}') || token->isSymbol(';') ||
                token->isNull()) &&
               (!m_querySet->queryVector().size() ||
                m_state != ReadRestrictor)) {
        m_state = Done;
    } else {
        m_state = SkipUntilComma;
        if (token->isSymbol(','))
            skipUntilComma(token);
    }
}

void CSSParser::readAnd(RefPtr<CSSToken> token)
{
    if (token->isIdent() && token->value()->equalsIgnoreCase("and")) {
        m_state = ReadFeatureStart;
    } else if (token->isSymbol(',') && m_parserType != MediaConditionParser) {
        m_querySet->addMediaQuery(m_mediaQueryData.mediaQuery());
        m_state = ReadRestrictor;
    } else if (token->isSymbol('}') || token->isSymbol(';') ||
               token->isNull()) {
        m_state = Done;
    } else {
        m_state = SkipUntilComma;
    }
}

void CSSParser::readFeatureStart(RefPtr<CSSToken> token)
{
    if (token->isSymbol('(')) {
        m_state = ReadFeature;
    } else {
        m_state = SkipUntilComma;
    }
}

void CSSParser::readFeature(RefPtr<CSSToken> token)
{
    if (token->isIdent()) {
        if (false) {
        }
#define SET_MEDIA_FEATURES(name, mediaFeatureName, ...)                     \
    else if (token->value()->equalsIgnoreCase(mediaFeatureName))            \
    {                                                                       \
        m_mediaQueryData.setMediaFeature(MediaFeature::MediaFeature##name); \
    }
        ENUM_MEDIA_FEATURES(SET_MEDIA_FEATURES)
#undef SET_MEDIA_FEATURES
        m_state = ReadFeatureColon;
    } else {
        m_state = SkipUntilComma;
    }
}

void CSSParser::readFeatureColon(RefPtr<CSSToken> token)
{
    if (token->isSymbol(':')) {
        m_state = ReadFeatureValue;
    } else if (token->isSymbol(')') || token->isSymbol('}') ||
               token->isSymbol(';') || token->isNull()) {
        readFeatureEnd(token);
    } else {
        m_state = SkipUntilBlockEnd;
    }
}

void CSSParser::readFeatureValue(RefPtr<CSSToken> token)
{
    if (token->isDimension() && token->unitType() == UnitType::UnknownType) {
        m_state = SkipUntilComma;
    } else {
        if (m_mediaQueryData.tryAddParserToken(token)) {
            m_state = ReadFeatureEnd;
        } else {
            m_state = SkipUntilBlockEnd;
        }
    }
}

void CSSParser::readFeatureEnd(RefPtr<CSSToken> token)
{
    if (token->isSymbol(')') || token->isSymbol('}') || token->isSymbol(';') ||
        token->isNull()) {
        if (m_mediaQueryData.addExpression()) {
            m_state = ReadAnd;
        } else {
            m_state = SkipUntilComma;
        }
    } else if (token->isSymbol('/')) {
        m_mediaQueryData.tryAddParserToken(token);
        m_state = ReadFeatureValue;
    } else {
        m_state = SkipUntilBlockEnd;
    }
}

void CSSParser::skipUntilComma(RefPtr<CSSToken> token)
{
    if ((token->isSymbol(',')) || token->isSymbol('}') ||
        token->isSymbol(';') || token->isNull()) {
        m_state = ReadRestrictor;
        m_mediaQueryData.clear();
        m_querySet->addMediaQuery(MediaQuery::createNotAll());
    }
}

void CSSParser::skipUntilBlockEnd(RefPtr<CSSToken> token)
{
    if (isBlockEnd(token) || token->isSymbol(';')) {
        m_state = SkipUntilComma;
    }
}

void CSSParser::done(RefPtr<CSSToken> token)
{
}

MediaQueryData::MediaQueryData()
    : m_restrictor(MediaQuery::None)
    , m_mediaType(String::createASCIIString("all"))
    , m_mediaFeature(MediaFeature::MediaFeatureNone)
    , m_mediaTypeSet(false)
{
}

void MediaQueryData::clear()
{
    m_restrictor = MediaQuery::None;
    m_mediaType = String::createASCIIString("all");
    m_mediaTypeSet = false;
    m_mediaFeature = MediaFeature::MediaFeatureNone;
    m_valueList.clear();
    m_expressions.clear();
}

bool MediaQueryData::tryAddParserToken(RefPtr<CSSToken> token)
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

static inline bool featureWithoutValue(MediaFeature mediaFeature)
{
    // Media features that are prefixed by min/max cannot be used without a
    // value.
    return mediaFeature == MediaFeature::MediaFeatureMonochrome ||
           mediaFeature == MediaFeature::MediaFeatureColor ||
           mediaFeature == MediaFeature::MediaFeatureColorIndex ||
           mediaFeature == MediaFeature::MediaFeatureGrid ||
           mediaFeature == MediaFeature::MediaFeatureHeight ||
           mediaFeature == MediaFeature::MediaFeatureWidth ||
           mediaFeature == MediaFeature::MediaFeatureDeviceHeight ||
           mediaFeature == MediaFeature::MediaFeatureDeviceWidth ||
           mediaFeature == MediaFeature::MediaFeatureOrientation ||
           mediaFeature == MediaFeature::MediaFeatureAspectRatio ||
           mediaFeature == MediaFeature::MediaFeatureDeviceAspectRatio ||
           mediaFeature == MediaFeature::MediaFeatureResolution ||
           mediaFeature == MediaFeature::MediaFeatureScan ||
           mediaFeature == MediaFeature::MediaFeatureHover ||
           mediaFeature == MediaFeature::MediaFeatureAnyHover ||
           mediaFeature == MediaFeature::MediaFeaturePointer ||
           mediaFeature == MediaFeature::MediaFeatureAnyPointer ||
           mediaFeature == MediaFeature::MediaFeatureScripting ||
           mediaFeature == MediaFeature::MediaFeatureUpdate ||
           mediaFeature == MediaFeature::MediaFeatureOverflowBlock ||
           mediaFeature == MediaFeature::MediaFeatureOverflowInline ||
           mediaFeature == MediaFeature::MediaFeatureDisplayMode;
}

static inline bool featureWithValidIdent(MediaFeature mediaFeature,
                                         const String* ident)
{
    switch (mediaFeature) {
    case MediaFeature::MediaFeatureOrientation:
        return ident->equalsIgnoreCase("portrait") ||
               ident->equalsIgnoreCase("landscape");
    case MediaFeature::MediaFeatureScan:
        return ident->equalsIgnoreCase("interlace") ||
               ident->equalsIgnoreCase("progressive");
    case MediaFeature::MediaFeatureHover:
    case MediaFeature::MediaFeatureAnyHover:
        return ident->equalsIgnoreCase("none") ||
               ident->equalsIgnoreCase("hover");
    case MediaFeature::MediaFeaturePointer:
    case MediaFeature::MediaFeatureAnyPointer:
        return ident->equalsIgnoreCase("none") ||
               ident->equalsIgnoreCase("coarse") ||
               ident->equalsIgnoreCase("fine");
    case MediaFeature::MediaFeatureScripting:
        return ident->equalsIgnoreCase("none") ||
               ident->equalsIgnoreCase("initial-only") ||
               ident->equalsIgnoreCase("enabled");
    case MediaFeature::MediaFeatureUpdate:
        return ident->equalsIgnoreCase("none") ||
               ident->equalsIgnoreCase("slow") ||
               ident->equalsIgnoreCase("fast");
    case MediaFeature::MediaFeatureOverflowBlock:
        return ident->equalsIgnoreCase("none") ||
               ident->equalsIgnoreCase("scroll") ||
               ident->equalsIgnoreCase("optional-paged") ||
               ident->equalsIgnoreCase("paged");
    case MediaFeature::MediaFeatureOverflowInline:
        return ident->equalsIgnoreCase("none") ||
               ident->equalsIgnoreCase("scroll");
    case MediaFeature::MediaFeatureDisplayMode:
        return ident->equalsIgnoreCase("fullscreen") ||
               ident->equalsIgnoreCase("standalone") ||
               ident->equalsIgnoreCase("minimal-ui") ||
               ident->equalsIgnoreCase("browser");
    default:
        return false;
    }
}

static inline bool featureWithValidPositiveLength(MediaFeature mediaFeature,
                                                  RefPtr<CSSToken> token)
{
    if (!token->isLength() ||
        (token->isNumber() && token->numericValue() == 0) ||
        token->numericValue() < 0) {
        return false;
    }

    return mediaFeature == MediaFeature::MediaFeatureHeight ||
           mediaFeature == MediaFeature::MediaFeatureMaxHeight ||
           mediaFeature == MediaFeature::MediaFeatureMinHeight ||
           mediaFeature == MediaFeature::MediaFeatureWidth ||
           mediaFeature == MediaFeature::MediaFeatureMaxWidth ||
           mediaFeature == MediaFeature::MediaFeatureMinWidth ||
           mediaFeature == MediaFeature::MediaFeatureDeviceHeight ||
           mediaFeature == MediaFeature::MediaFeatureMaxDeviceHeight ||
           mediaFeature == MediaFeature::MediaFeatureMinDeviceHeight ||
           mediaFeature == MediaFeature::MediaFeatureDeviceWidth ||
           mediaFeature == MediaFeature::MediaFeatureMinDeviceWidth ||
           mediaFeature == MediaFeature::MediaFeatureMaxDeviceWidth;
}

static inline bool featureWithValidDensity(MediaFeature mediaFeature,
                                           RefPtr<CSSToken> token)
{
    if (token->unitType() != UnitType::DotsPerPixel &&
        token->unitType() != UnitType::DotsPerInch &&
        token->unitType() != UnitType::DotsPerCentimeter) {
        return false;
    }

    return mediaFeature == MediaFeature::MediaFeatureResolution ||
           mediaFeature == MediaFeature::MediaFeatureMinResolution ||
           mediaFeature == MediaFeature::MediaFeatureMaxResolution;
}

static inline bool featureWithPositiveInteger(MediaFeature mediaFeature,
                                              RefPtr<CSSToken> token)
{
    if (token->value()->toString()->contains(".") ||
        token->numericValue() < 0) {
        return false;
    }

    return mediaFeature == MediaFeature::MediaFeatureColor ||
           mediaFeature == MediaFeature::MediaFeatureMaxColor ||
           mediaFeature == MediaFeature::MediaFeatureMinColor ||
           mediaFeature == MediaFeature::MediaFeatureColorIndex ||
           mediaFeature == MediaFeature::MediaFeatureMaxColorIndex ||
           mediaFeature == MediaFeature::MediaFeatureMinColorIndex ||
           mediaFeature == MediaFeature::MediaFeatureMonochrome ||
           mediaFeature == MediaFeature::MediaFeatureMaxMonochrome ||
           mediaFeature == MediaFeature::MediaFeatureMinMonochrome;
}

static inline bool featureWithZeroOrOne(MediaFeature mediaFeature,
                                        RefPtr<CSSToken> token)
{
    if (token->value()->toString()->contains(".") ||
        !(token->numericValue() == 1 || token->numericValue() == 0)) {
        return false;
    }

    return mediaFeature == MediaFeature::MediaFeatureGrid;
}

static inline bool featureWithAspectRatio(MediaFeature mediaFeature)
{
    return mediaFeature == MediaFeature::MediaFeatureAspectRatio ||
           mediaFeature == MediaFeature::MediaFeatureDeviceAspectRatio ||
           mediaFeature == MediaFeature::MediaFeatureMinAspectRatio ||
           mediaFeature == MediaFeature::MediaFeatureMaxAspectRatio ||
           mediaFeature == MediaFeature::MediaFeatureMinDeviceAspectRatio ||
           mediaFeature == MediaFeature::MediaFeatureMaxDeviceAspectRatio;
}

MediaQueryExp::MediaQueryExp(MediaQueryExp& other)
    : m_mediaFeature(other.mediaFeature())
    , m_expValue(other.expValue())
{
}

MediaQueryExp::MediaQueryExp(MediaFeature mediaFeature,
                             MediaQueryExpValue expValue)
    : m_mediaFeature(mediaFeature)
    , m_expValue(expValue)
{
}

MediaQueryExp* MediaQueryExp::createIfValid(
    MediaFeature mediaFeature, const GCVector<RefPtr<CSSToken>>& tokenList)
{
    MediaQueryExpValue expValue;

    // Create value for media query expression that must have 1 or more values.
    if (tokenList.size() == 0 && featureWithoutValue(mediaFeature)) {
        // Valid, creates a MediaQueryExp with an 'invalid' MediaQueryExpValue
    } else if (tokenList.size() == 1) {
        RefPtr<CSSToken> token = tokenList.front();

        if (token->isIdent()) {
            String* ident = token->value()->toString();
            if (!featureWithValidIdent(mediaFeature, ident)) {
                return nullptr;
            }
            expValue.id = ident;
            expValue.unit = UnitType::ValueID;
            expValue.isID = true;
        } else if (token->isNumber() || token->isPercentage() ||
                   token->isDimension()) {
            // Check for numeric token types since it is only safe for these
            // types to call numericValue.
            if (featureWithValidDensity(mediaFeature, token) ||
                featureWithValidPositiveLength(mediaFeature, token)) {
                // Media features that must have non-negative <density>, ie.
                // dppx, dpi or dpcm,
                // or Media features that must have non-negative <length> or
                // number value.
                expValue.value = token->numericValue();
                expValue.unit = token->unitType();
                expValue.isValue = true;
            } else if (featureWithPositiveInteger(mediaFeature, token) ||
                       featureWithZeroOrOne(mediaFeature, token)) {
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
    } else if (tokenList.size() == 3 && featureWithAspectRatio(mediaFeature)) {
        // <ratio> is supposed to allow whitespace around the '/'
        // Applicable to device-aspect-ratio and aspect-ratio.
        RefPtr<CSSToken> numerator = tokenList[0];
        RefPtr<CSSToken> delimiter = tokenList[1];
        RefPtr<CSSToken> denominator = tokenList[2];
        if (!delimiter->isSymbol('/')) {
            return nullptr;
        }
        if (!numerator->isNumber() || numerator->numericValue() <= 0 ||
            numerator->hasSourceOfNumberValueDot()) {
            return nullptr;
        }
        if (!denominator->isNumber() || denominator->numericValue() <= 0 ||
            denominator->hasSourceOfNumberValueDot()) {
            return nullptr;
        }

        expValue.numerator = (unsigned)numerator->numericValue();
        expValue.denominator = (unsigned)denominator->numericValue();
        expValue.isRatio = true;
    } else {
        return nullptr;
    }

    return new MediaQueryExp(mediaFeature, expValue);
}

bool MediaQueryExp::isViewportDependent() const
{
    return m_mediaFeature >= MediaFeatureViewportDependentStart &&
           m_mediaFeature <= MediaFeatureViewportDependentEnd;
}

bool MediaQueryExp::isDeviceDependent() const
{
    return m_mediaFeature >= MediaFeatureDeviceDependentStart &&
           m_mediaFeature <= MediaFeatureDeviceDependentEnd;
}

String* MediaQueryExp::serialize() const
{
    StringBuilder result;
    result.appendChar('(');
    switch (m_mediaFeature) {
#define SERIALIZE_MEDIA_FEATURE(name, mediaFeatureName, ...) \
    case MediaFeature##name:                                 \
        result.appendString(mediaFeatureName);               \
        break;
        ENUM_MEDIA_FEATURES(SERIALIZE_MEDIA_FEATURE)
#undef SERIALIZE_MEDIA_FEATURE
    default:
        break;
    }
    if (m_expValue.isValid()) {
        result.appendString(": ");
        result.appendString(m_expValue.cssText());
    }
    result.appendChar(')');

    return result.finalize();
}

String* MediaQueryExpValue::cssText() const
{
    StringBuilder output;
    if (isValue) {
        output.appendString(String::fromFloat(value));
        output.appendString(unitTypeToString(unit));
    } else if (isRatio) {
        output.appendString(String::fromFloat(numerator));
        output.appendChar('/');
        output.appendString(String::fromFloat(denominator));
    } else if (isID) {
        output.appendString(id);
    }

    return output.finalize();
}
}
