/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "core/dom/Element.h"
#include "core/style/CSSVariableSyntaxTreeBuilder.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

static std::pair<const char*, size_t> trim(const char* src, size_t length)
{
    if (length) {
        size_t first = 0;
        size_t last = length - 1;

        for (size_t i = 0; i < length; i++) {
            if (!String::isSpaceOrNewline(src[i])) {
                first = i;
                break;
            }
        }

        do {
            if (!String::isSpaceOrNewline(src[last])) {
                break;
            }
        } while (last--);

        src = src + first;
        length = last - first + 1;
    }

    return std::make_pair(src, length);
}

class VariableTokenizer {
public:
    STARFISH_MAKE_STACK_ALLOCATED();
    enum TokenType {
        VARIABLE,
        VARIABLEBLOCKOPEN,
        VARIABLEBLOCKCLOSE,
        COMMA,
        RAWVALUE,
        EMPTY,
        END
    };

    struct VariableToken {
        VariableToken(TokenType type, const char* data, size_t length)
            : m_type(type)
            , m_data(data)
            , m_length(length)
        {
        }

        TokenType m_type;
        const char* m_data;
        size_t m_length;
    };

    VariableTokenizer(const char* data, size_t length)
        : m_cursorStart(0)
        , m_cursorEnd(0)
        , m_data(data)
        , m_length(length)
    {
    }

    VariableToken next()
    {
        VariableToken token(END, nullptr, 0);
        int parenthesisCount = 0;
        bool hadParenthesis = false;
        while (m_cursorEnd < m_length) {
            const char* subData = m_data + m_cursorStart;
            size_t subLength = m_cursorEnd - m_cursorStart + 1;

            if (subLength == 4 && subData[0] == 'v' && subData[1] == 'a' &&
                subData[2] == 'r' && subData[3] == '(') {
                m_cursorStart += 4;
                m_cursorEnd++;
                token.m_type = VARIABLEBLOCKOPEN;
                token.m_data = "var(";
                token.m_length = 4;
                break;
            } else if (subLength == 1 && subData[0] == ')' &&
                       parenthesisCount == 0) {
                token.m_type = VARIABLEBLOCKCLOSE;
                token.m_data = ")";
                token.m_length = 1;
                m_cursorStart = m_cursorEnd + 1;
                m_cursorEnd += 1;
                break;
            } else if (subLength >= 1 && subData[0] == ',') {
                auto r = trim(subData, subLength);
                if (r.second == 1 && r.first[0] == ',' &&
                    parenthesisCount == 0) {
                    token.m_type = COMMA;
                    token.m_data = ", ";
                    token.m_length = 2;
                    m_cursorStart = m_cursorEnd + 1;
                    m_cursorEnd += 1;
                    break;
                }
            }

            if (m_data[m_cursorEnd] == '(') {
                parenthesisCount++;
                hadParenthesis = true;
            } else if (m_data[m_cursorEnd] == ')' && parenthesisCount > 0) {
                parenthesisCount--;
            }

            if (m_data[m_cursorEnd] == ',' && parenthesisCount == 0) {
                token.m_type = VARIABLE;
                token.m_data = m_data + m_cursorStart;
                token.m_length = m_cursorEnd - m_cursorStart;
                m_cursorStart = m_cursorEnd + 1;
                m_cursorEnd += 2;
                break;
            }

            if (m_data[m_cursorEnd] == ')' && parenthesisCount == 0) {
                if (hadParenthesis) {
                    token.m_data = m_data + m_cursorStart;
                    token.m_length = m_cursorEnd - m_cursorStart + 1;
                    m_cursorStart = m_cursorEnd + 1;
                    m_cursorEnd++;
                } else {
                    token.m_data = m_data + m_cursorStart;
                    token.m_length = m_cursorEnd - m_cursorStart;
                    m_cursorStart = m_cursorEnd;
                }

                token.m_type = VARIABLE;
                break;
            }

            m_cursorEnd++;
        }

        return token;
    }

    size_t m_cursorStart;
    size_t m_cursorEnd;
    const char* m_data;
    size_t m_length;
};

void CSSVariableSyntaxTreeBuilder::build(const char* src, size_t length)
{
    bool pos = length >= 4 && src[0] == 'v' && src[1] == 'a' && src[2] == 'r' &&
               src[3] == '(';
    if (!pos) {
        m_valid = false;
        return;
    }

    size_t cursor = 0;
    int parenthesisCount = 0;
    size_t start = 0, end = 0;
    while (cursor < length) {
        if (cursor + 4 < length && src[cursor] == 'v' &&
            src[cursor + 1] == 'a' && src[cursor + 2] == 'r' &&
            src[cursor + 3] == '(') {
            if (!parenthesisCount) {
                // Store a start block position.
                start = cursor;
            }
            parenthesisCount++;
            cursor += 4;
        } else if (src[cursor] == '(') {
            parenthesisCount++;
            cursor++;
        } else if (src[cursor] == ')') {
            parenthesisCount--;

            if (parenthesisCount < 0) {
                m_valid = false;
                break;
            }

            if (!parenthesisCount) {
                // Make a block.
                end = cursor;
                VariableContainer variableContainer(start, end);
                m_variableContainers.push_back(variableContainer);
            }

            cursor++;
        } else {
            cursor++;
        }
    }

    if (!m_valid)
        return;

    if (!m_variableContainers.size()) {
        m_valid = false;
        return;
    }

    // Build a Syntax Tree.
    for (size_t i = 0; i < m_variableContainers.size(); i++) {
        VariableContainer* container = &m_variableContainers[i];
        size_t start = container->m_start;
        size_t end = container->m_end;
        buildTree(container, &src[start], end - start + 1);
    }
}

void CSSVariableSyntaxTreeBuilder::buildTree(VariableContainer* container,
                                             const char* str, size_t length)
{
    // Syntax of 'var()' = var( <custom-property-name> [, <declaration-value> ]?
    // )
    struct Context {
        Context(Block* b, size_t i)
            : block(b)
            , index(i)
        {
        }

        Block* block;
        size_t index;
    };

    GCVector<Context> contexts;

    container->m_root = new VariableBlock();
    VariableTokenizer tokenizer(str, length);
    VariableTokenizer::VariableToken token = tokenizer.next();

    if (token.m_type != VariableTokenizer::TokenType::VARIABLEBLOCKOPEN) {
        m_valid = false;
        return;
    }

    contexts.push_back(Context(container->m_root, 0));
    token = tokenizer.next();
    while (token.m_type != VariableTokenizer::TokenType::END) {
        Context* c = &contexts.back();
        VariableBlock* parent = (VariableBlock*)c->block;
        if (token.m_type == VariableTokenizer::TokenType::VARIABLE) {
            if (m_starfish) {
                auto trimmedValue = trim(token.m_data, token.m_length);
                parent->variables.push_back(new Variable(
                    AtomicString::createAtomicString(m_starfish.value(),
                                                     trimmedValue.first,
                                                     trimmedValue.second)));
            }
            c->index++;
        } else if (token.m_type ==
                   VariableTokenizer::TokenType::VARIABLEBLOCKOPEN) {
            if (c->index == 0) {
                m_valid = false;
                return;
            }
            VariableBlock* block = new VariableBlock();
            contexts.push_back(Context(block, 0));
            parent->variables.push_back(block);
            c->index++;
        } else if (token.m_type ==
                   VariableTokenizer::TokenType::VARIABLEBLOCKCLOSE) {
            contexts.pop_back();
        } else if (token.m_type == VariableTokenizer::TokenType::RAWVALUE) {
            // TODO : Implement a raw data such as #fff, yellow and 10px.
            STARFISH_UNSUPPORTED("css variable: raw data");
        }

        token = tokenizer.next();
    }

    if (contexts.size() != 0) {
        m_valid = false;
    }
}

static void appendString(CSSVariableSyntaxTreeBuilder::StyleString& str,
                         String* src)
{
    auto n = src->toOptionalUTF8String();
    for (size_t i = 0; i < n.m_bufferSize; i++) {
        str.push_back(n.m_buffer[i]);
    }
}

CSSVariableSyntaxTreeBuilder::StyleString
CSSVariableSyntaxTreeBuilder::generateStyle(
    Node* node, Optional<const MutablePropertyValueList*> cssCustomValues)
{
    struct Context {
        Context(Block* b, size_t i)
            : block(b)
            , index(i)
        {
        }

        Block* block;
        size_t index;
    };

    StyleString ret;
    for (size_t i = 0; i < m_variableContainers.size(); i++) {
        VariableContainer* container = &m_variableContainers[i];
        VariableBlock* parent = (VariableBlock*)container->m_root;
        GCVector<Context> contexts;
        contexts.push_back(Context(parent, 0));
        bool found = false;
        StyleString findValue;

        while (contexts.size() && !found) {
            Context* c = &contexts.back();
            VariableBlock* parent = (VariableBlock*)c->block;

            bool isFirst = true;
            for (size_t j = c->index; j < parent->variables.size() && !found;
                 j++) {
                Block* block = parent->variables[j];
                c->index++;
                if (block->isVariable()) {
                    Variable* variable = (Variable*)block;
                    if (isFirst) {
                        auto currentNode = node;
                        Optional<const MutablePropertyValueList*>
                            currentCustomValues = cssCustomValues;
                        while (currentNode && !found) {
                            if (currentCustomValues) {
                                for (size_t k = 0;
                                     k < currentCustomValues->values().size();
                                     k++) {
                                    MutablePropertyValue customProperty =
                                        currentCustomValues->values()[k];
                                    if (customProperty.name() ==
                                            variable->m_value &&
                                        // Prevent recursive behavior when
                                        // resolving a variable. This is a
                                        // temporary solution and should be
                                        // removed.
                                        !customProperty.value()->contains(
                                            customProperty.name().string())) {
                                        appendString(findValue,
                                                     customProperty.value());
                                        found = true;
                                    }
                                }
                            }

                            currentNode = currentNode->renderingParentElement();
                            if (currentNode) {
                                auto cp =
                                    currentNode->style()->customProperty();
                                if (cp) {
                                    currentCustomValues = cp.value();
                                } else {
                                    currentCustomValues = nullptr;
                                }
                            }
                        }
                    } else {
                        if (findValue.length() > 0) {
                            findValue.push_back(' ');
                            findValue.push_back(',');
                        }
                        appendString(findValue, variable->m_value.string());
                    }
                } else if (block->isRawValue()) {
                } else if (block->isVariableBlock()) {
                    VariableBlock* variableBlock = (VariableBlock*)block;
                    contexts.push_back(Context(variableBlock, 0));
                }
                isFirst = false;
            }

            if (c->index == parent->variables.size() &&
                parent == contexts.back().block) {
                contexts.pop_back();
            }
        }

        if (findValue.length() > 0) {
            // FIXME : To support the full style with variableContainers
            // such as 'rgb(100, var(--foo1), var(--foo2))'.
            // Before that, we should solve a issue(#1132).
            ret = std::move(findValue);
            break;
        }
    }

    return ret;
}

void CSSVariableSyntaxTreeBuilder::dump()
{
    struct Context {
        Context(Block* b, size_t i)
            : block(b)
            , index(i)
        {
        }

        Block* block;
        size_t index;
    };

    for (size_t i = 0; i < m_variableContainers.size(); i++) {
        VariableContainer* container = &m_variableContainers[i];
        VariableBlock* parent = (VariableBlock*)container->m_root;
        GCVector<Context> contexts;
        contexts.push_back(Context(parent, 0));
        size_t depth = 1;
        printf("[%p][VariableBlock]\n", parent);
        while (contexts.size()) {
            Context* c = &contexts.back();
            VariableBlock* parent = (VariableBlock*)c->block;

            for (size_t j = c->index; j < parent->variables.size(); j++) {
                Block* block = parent->variables[j];
                c->index++;
                for (size_t t = 0; t < depth; t++)
                    printf("\t");
                if (block->isVariable()) {
                    Variable* variable = (Variable*)block;
                    printf("[%p][Variable] [name : %s]\n", variable,
                           variable->m_value.string()
                               ->toUTF8NonGCString()
                               .c_str());
                } else if (block->isRawValue()) {
                } else if (block->isVariableBlock()) {
                    VariableBlock* variableBlock = (VariableBlock*)block;
                    printf("[%p][VariableBlock]\n", variableBlock);
                    contexts.push_back(Context(variableBlock, 0));
                    depth++;
                }
            }

            if (c->index == parent->variables.size() &&
                parent == contexts.back().block) {
                contexts.pop_back();
                depth--;
            }
        }
    }
}
} // namespace Starfish
