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

GCVector<StringView> DOMTokenList::tokenize(String* src)
{
    GCVector<StringView> tokens;

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
                    tokens.push_back(new StringView(src, start, end));
                    end = start = i;
                }
            } else {
                end++;
            }
        }
    }

    if (end - start) {
        tokens.push_back(new StringView(src, start, end));
    }

    return tokens;
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
            if (strstr(token->utf8Data(), ")")) {
                newTokens.push_back(combined);
                combined = String::emptyString;
                combinedCount = 0;
            }
        } else if (strstr(token->utf8Data(), "(")) {
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
        GCVector<StringView> tokens = tokenize(src.getValue());
        return tokens.size();
    }
    return 0;
}

Nullable<String*> DOMTokenList::item(unsigned long index)
{
    Nullable<String*> src = m_element->getAttribute(m_localName);
    if (src.hasValue()) {
        GCVector<StringView> tokens = tokenize(src.getValue());
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
        GCVector<StringView> tokens = tokenize(src.getValue());
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
    GCVector<StringView> tokens = tokenize(str);
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
    GCVector<StringView> tokens = tokenize(src);
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
    GCVector<StringView> tokens = tokenize(str);
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

String* DOMTokenList::toString()
{
    return m_element->getAttributeOrEmpty(m_localName);
}

// Throw Exceptions
void DOMTokenList::validateToken(String* token)
{
    std::string stdToken = token->utf8Data();
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

void DOMTokenList::setValue(String* value)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
