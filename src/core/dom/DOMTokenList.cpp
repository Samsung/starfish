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

#include "StarFishConfig.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMTokenList.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"

namespace StarFish {

ScriptBindingInstance* DOMTokenList::scriptBindingInstance()
{
    return m_element->document()->scriptBindingInstance();
}

void DOMTokenList::tokenize(String* src, GCVector<StringView>& tokens)
{
    auto accessData = src->bufferAccessData();
    size_t length = accessData.length;

    bool isWhiteSpaceState = true;

    size_t start = 0, end = 0;
    bool inQuotationMarks = false;
    bool inParenthesis = false;
    for (size_t i = 0; i < length; i++) {
        char32_t ch = accessData.charAt(i);
        if (ch == '"' || ch == '\'') {
            if (inQuotationMarks) {
                inQuotationMarks = false;
            } else {
                inQuotationMarks = true;
            }
        }
        if (ch == '(') {
            inParenthesis = true;
        } else if (ch == ')') {
            inParenthesis = false;
        }
        if (isWhiteSpaceState) {
            if (!String::isSpaceOrNewline(ch)) {
                isWhiteSpaceState = false;
                start = i;
                end = i + 1;
            } else {
                continue;
            }
        } else {
            if (String::isSpaceOrNewline(ch) && !inQuotationMarks) {
                if (!inParenthesis) {
                    isWhiteSpaceState = true;
                    tokens.emplace_back(src, start, end);
                    end = start = i;
                }
            } else {
                end++;
            }
        }
    }

    if (end - start) {
        tokens.emplace_back(src, start, end);
    }
}

void DOMTokenList::concatTokensInsideParentheses(GCVector<String*>* tokens)
{
    GCVector<String*> newTokens;
    String* combined = String::emptyString;
    unsigned combinedCount = 0;
    // unsigned parentheseDepth = 0; // Does not count parentheses depth
    for (auto token : *tokens) {
        // TODO: Should Consider brackets insize QuotationMarks
        if (combinedCount) {
            combined = combined->concat(token);
            combinedCount++;
            if (token->indexOf(')') != SIZE_MAX) {
                newTokens.push_back(combined);
                combined = String::emptyString;
                combinedCount = 0;
            }
        } else if (token->indexOf('(') != SIZE_MAX) {
            combined = combined->concat(token);
            combinedCount++;
        } else {
            newTokens.push_back(token);
        }
    }
    if (combinedCount) {
        newTokens.insert(newTokens.end(), tokens->end() - combinedCount,
                         tokens->end());
    }
    if (newTokens.size() != tokens->size()) {
        tokens->clear();
        tokens->assign(newTokens.begin(), newTokens.end());
    }
}

uint32_t DOMTokenList::length()
{
    Nullable<String*> src = m_element->getAttribute(m_localName);
    if (src.hasValue()) {
        GCVector<StringView> tokens;
        tokenize(src.getValue(), tokens);
        return tokens.size();
    }
    return 0;
}

Nullable<String*> DOMTokenList::item(unsigned long index)
{
    Nullable<String*> src = m_element->getAttribute(m_localName);
    if (src.hasValue()) {
        GCVector<StringView> tokens;
        tokenize(src.getValue(), tokens);
        if (index < tokens.size()) {
            return Nullable<String*>(new StringView(tokens[index]));
        }
    }
    return Nullable<String*>();
}

bool DOMTokenList::contains(String* token)
{
    validateToken(token);

    Nullable<String*> src = m_element->getAttribute(m_localName);
    if (src.hasValue()) {
        GCVector<StringView> tokens;
        tokenize(src.getValue(), tokens);
        for (unsigned i = 0; i < tokens.size(); i++) {
            if (tokens[i].equals(token)) {
                return true;
            }
        }
    }
    return false;
}

String* DOMTokenList::addSingleToken(String* src,
                                     const GCVector<StringView>& tokens,
                                     String* token)
{
    bool matched = false;
    for (unsigned j = 0; j < tokens.size(); j++) {
        if (token->equals(&tokens[j])) {
            matched = true;
            break;
        }
    }
    if (!matched) {
        if (src->length() > 0) {
            if (src->charAt(src->length() - 1) == ' ') {
                return src->concat(token);
            } else {
                return src->concat(String::spaceString)->concat(token);
            }
        } else {
            return src->concat(token);
        }
    }
    return src;
}

void DOMTokenList::add(GCVector<String*>& tokensToAdd)
{
    if (tokensToAdd.size() == 0) {
        return;
    }
    String* str = m_element->getAttributeOrEmpty(m_localName);
    GCVector<StringView> tokens;
    tokenize(str, tokens);
    for (unsigned i = 0; i < tokensToAdd.size(); i++) {
        validateToken(tokensToAdd[i]);
        str = addSingleToken(str, tokens, tokensToAdd[i]);
    }
    m_element->setAttribute(m_localName, str);
}

int DOMTokenList::checkMatchedTokens(bool* matchFlags,
                                     const GCVector<StringView>& tokens,
                                     String* token)
{
    int count = 0;
    for (unsigned i = 0; i < tokens.size(); i++) {
        if (tokens[i].equals(token)) {
            matchFlags[i] = true;
            count++;
        } else {
            matchFlags[i] = false;
        }
    }
    return count;
}

void DOMTokenList::remove(String* token)
{
    GCVector<String*> tokensToRemove;
    tokensToRemove.push_back(token);
    remove(tokensToRemove);
}

void DOMTokenList::remove(GCVector<String*>& tokensToRemove)
{
    if (tokensToRemove.size() == 0) {
        return;
    }
    Nullable<String*> old = m_element->getAttribute(m_localName);
    if (!old.hasValue()) {
        // Nothing to remove
        return;
    }
    String* src = old.getValue();
    String* dst = String::createASCIIString("");
    GCVector<StringView> tokens;
    tokenize(src, tokens);
    bool* matchFlags = new bool[tokens.size()];
    int matchCount = 0;
    for (unsigned i = 0; i < tokensToRemove.size(); i++) {
        validateToken(tokensToRemove[i]);
        matchCount += checkMatchedTokens(matchFlags, tokens, tokensToRemove[i]);
    }
    if (matchCount > 0) {
        bool isEmpty = true;
        for (unsigned i = 0; i < tokens.size(); i++) {
            if (!matchFlags[i]) {
                if (isEmpty) {
                    dst = new StringView(tokens[i]);
                    isEmpty = false;
                } else {
                    dst = dst->concat(String::spaceString)->concat(&tokens[i]);
                }
            }
        }
        m_element->setAttribute(m_localName, dst);
    }
    delete[] matchFlags;
}

bool DOMTokenList::toggle(String* token)
{
    return toggle(token, false, false);
}

bool DOMTokenList::toggle(String* token, bool forceValue)
{
    return toggle(token, true, forceValue);
}

bool DOMTokenList::toggle(String* token, bool isForced, bool forceValue)
{
    validateToken(token);
    String* str = m_element->getAttributeOrEmpty(m_localName);
    GCVector<StringView> tokens;
    tokenize(str, tokens);
    bool needAdd = false;
    if (isForced) {
        if (forceValue) {
            needAdd = true;
        }
    } else {
        bool* matchFlags = new bool[tokens.size()];
        int matchCount = checkMatchedTokens(matchFlags, tokens, token);
        if (matchCount == 0) {
            needAdd = true;
        }
        delete[] matchFlags;
    }
    if (needAdd) {
        str = addSingleToken(str, tokens, token);
        m_element->setAttribute(m_localName, str);
    } else {
        remove(token);
    }
    return needAdd;
}

bool DOMTokenList::replace(String* token, String* newToken)
{
    validateToken(token);
    validateToken(newToken);

    Nullable<String*> old = m_element->getAttribute(m_localName);
    if (!old.hasValue()) {
        // Nothing to replace
        return false;
    }
    String* src = old.getValue();
    String* dst = String::createASCIIString("");
    GCVector<StringView> tokens;
    tokenize(src, tokens);
    bool* matchFlags = new bool[tokens.size()];
    int matchCount = checkMatchedTokens(matchFlags, tokens, token);
    matchCount += checkMatchedTokens(matchFlags, tokens, newToken);
    bool isReplaced = false;

    if (matchCount > 0) {
        bool isEmpty = true;
        for (unsigned i = 0; i < tokens.size(); i++) {
            if (matchFlags[i]) {
                if (!isReplaced) {
                    if (isEmpty) {
                        dst = dst->concat(newToken);
                        isEmpty = false;
                    } else {
                        dst =
                            dst->concat(String::spaceString)->concat(newToken);
                    }
                    isReplaced = true;
                }
            } else {
                if (isEmpty) {
                    dst = dst->concat(&tokens[i]);
                    isEmpty = false;
                } else {
                    dst = dst->concat(String::spaceString)->concat(&tokens[i]);
                }
            }
        }
    }

    delete[] matchFlags;

    if (isReplaced) {
        m_element->setAttribute(m_localName, dst);
        return true;
    }

    return false;
}

bool DOMTokenList::supports(String* token)
{
    return validateTokenValue(token);
}

String* DOMTokenList::toString()
{
    return m_element->getAttributeOrEmpty(m_localName);
}

// Throw Exceptions
void DOMTokenList::validateToken(String* token)
{
    UTF8StringDataNonGCStd stdToken = token->toUTF8NonGCString();
    if (stdToken.length() == 0) {
        throw new DOMException(m_element->document(),
                               DOMException::Code::SYNTAX_ERR);
    }
    auto f = [](char c) { return std::isspace(static_cast<unsigned char>(c)); };
    if (std::find_if(stdToken.begin(), stdToken.end(), f) != stdToken.end()) {
        throw new DOMException(m_element->document(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    }
}

String* DOMTokenList::value() const
{
    return m_element->getAttributeOrEmpty(m_localName);
}

void DOMTokenList::setValue(String* value)
{
    m_element->setAttribute(m_localName, value);
}

bool DOMTokenList::validateTokenValue(String* token)
{
    String* lowerToken = token->toASCIILower();

    if (m_element->isHTMLAnchorElement() || m_element->isHTMLAreaElement()) {
        return supportedTokensOfAnchorAndArea(lowerToken);
    } else if (m_element->isHTMLLinkElement()) {
        return supportedTokensOfLink(lowerToken);
#ifdef STARFISH_ENABLE_MULTIMEDIA
    } else if (m_element->isHTMLMediaElement()) {
        return supportedTokensOfMedia(lowerToken);
#endif
    } else {
        throw new DOMException(m_element->document(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "DOMTokenList has no supported tokens.");
    }
    return false;
}

bool DOMTokenList::supportedTokensOfAnchorAndArea(String* token)
{
    if (token->equals("noreferrer") || token->equals("noopener")) {
        return true;
    }
    return false;
}

bool DOMTokenList::supportedTokensOfLink(String* token)
{
    // TODO check for module preload

    if (token->equals("preload") || token->equals("preconnect") ||
        token->equals("dns-prefetch") || token->equals("stylesheet") ||
        token->equals("import") || token->equals("icon") ||
        token->equals("alternate") || token->equals("prefetch") ||
        token->equals("prerender") || token->equals("next") ||
        token->equals("manifest") || token->equals("apple-touch-icon") ||
        token->equals("apple-touch-icon-precomposed") ||
        token->equals("canonical")) {
        return true;
    }
    return false;
}

bool DOMTokenList::supportedTokensOfMedia(String* token)
{
    if (token->equals("nodownload") || token->equals("nofullscreen") ||
        token->equals("noremoteplayback")) {
        return true;
    }
    return false;
}
}
