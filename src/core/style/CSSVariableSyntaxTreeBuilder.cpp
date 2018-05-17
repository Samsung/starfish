/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/style/CSSVariableSyntaxTreeBuilder.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

class VariableTokenizer {
public:
    enum TokenType {
        VARIABLE,
        VARIABLEBLOCK,
        VARIABLEBLOCKCLOSE,
        RAWVALUE,
        EMPTY,
        END
    };

    struct VariableToken {
        VariableToken(TokenType type, CSSTokenValue value)
            : m_type(type)
            , m_value(value)
        {
        }
        TokenType m_type;
        CSSTokenValue m_value;
    };

    VariableTokenizer(CSSTokenValue token)
        : m_cursorStart(0)
        , m_cursorEnd(0)
        , m_data(token)
    {
    }

    VariableToken next()
    {
        VariableToken token(END, "");
        while (m_cursorEnd <= m_data.size()) {
            CSSTokenValue sub = m_data.substring(
                m_cursorStart, m_cursorEnd - m_cursorStart + 1);
            if (sub.equals("var(")) {
                m_cursorStart += 4;
                m_cursorEnd++;
                token.m_type = VARIABLEBLOCK;
                token.m_value = "var(";
                break;
            }

            if (sub.equals(")")) {
                token.m_type = VARIABLEBLOCKCLOSE;
                token.m_value = ")";
                m_cursorStart = m_cursorEnd + 1;
                m_cursorEnd += 1;
                break;
            }

            if (m_data[m_cursorEnd] == ',' && m_data[m_cursorEnd - 1] != ')') {
                CSSTokenValue var = m_data.substring(
                    m_cursorStart, m_cursorEnd - m_cursorStart);
                token.m_type = VARIABLE;
                token.m_value = var;
                m_cursorStart = m_cursorEnd + 1;
                m_cursorEnd += 2;
                break;
            }

            if (m_data[m_cursorEnd] == ')' && m_data[m_cursorEnd - 1] != ')') {
                CSSTokenValue var = m_data.substring(
                    m_cursorStart, m_cursorEnd - m_cursorStart);
                token.m_type = VARIABLE;
                token.m_value = var;

                m_cursorStart = m_cursorEnd;
                break;
            }

            m_cursorEnd++;
        }

        return token;
    }

    size_t m_cursorStart;
    size_t m_cursorEnd;
    CSSTokenValue m_data;
};

void CSSVariableSyntaxTreeBuilder::build(const CSSTokenValue& src)
{
    bool pos = src.startsWith("var(");
    if (!pos) {
        m_valid = false;
        return;
    }

    size_t cursor = 0;
    int parenthesisCount = 0;
    size_t start = 0, end = 0;
    while (cursor < src.size()) {
        if (cursor + 4 < src.size() && src[cursor] == 'v' &&
            src[cursor + 1] == 'a' && src[cursor + 2] == 'r' &&
            src[cursor + 3] == '(') {
            if (!parenthesisCount) {
                // Store a start block position.
                start = cursor;
            }
            parenthesisCount++;
            cursor += 4;
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
        CSSTokenValue range = src.substring(start, end - start + 1);
        buildTree(container, range);
    }
}

void CSSVariableSyntaxTreeBuilder::buildTree(VariableContainer* container,
                                             CSSTokenValue& str)
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
    VariableTokenizer tokenizer(str);
    VariableTokenizer::VariableToken token = tokenizer.next();

    if (token.m_type != VariableTokenizer::TokenType::VARIABLEBLOCK) {
        m_valid = false;
        return;
    }

    contexts.push_back(Context(container->m_root, 0));
    token = tokenizer.next();
    while (token.m_type != VariableTokenizer::TokenType::END) {
        Context* c = &contexts.back();
        VariableBlock* parent = (VariableBlock*)c->block;
        if (token.m_type == VariableTokenizer::TokenType::VARIABLE) {
            parent->variables.push_back(new Variable(token.m_value));
            c->index++;
        } else if (token.m_type ==
                   VariableTokenizer::TokenType::VARIABLEBLOCK) {
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
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }

        token = tokenizer.next();
    }

    if (contexts.size() != 0) {
        m_valid = false;
    }
}

CSSTokenValue CSSVariableSyntaxTreeBuilder::generateStyle(
    GCVector<MutablePropertyValue>& cssCustomValues)
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

    CSSTokenValue ret;
    for (size_t i = 0; i < m_variableContainers.size(); i++) {
        VariableContainer* container = &m_variableContainers[i];
        VariableBlock* parent = (VariableBlock*)container->m_root;
        GCVector<Context> contexts;
        contexts.push_back(Context(parent, 0));
        bool isFind = false;
        String* findValue = nullptr;

        while (contexts.size()) {
            Context* c = &contexts.back();
            VariableBlock* parent = (VariableBlock*)c->block;

            for (size_t j = c->index; j < parent->variables.size(); j++) {
                Block* block = parent->variables[j];
                c->index++;
                if (block->isVariable()) {
                    Variable* variable = (Variable*)block;
                    String* key =
                        String::createASCIIString(variable->m_value.c_str());

                    for (size_t k = 0; k < cssCustomValues.size(); k++) {
                        MutablePropertyValue customProperty =
                            cssCustomValues[k];
                        if (customProperty.name()->equals(key)) {
                            findValue = customProperty.value();
                            isFind = true;
                        }
                    }

                    if (isFind) {
                        break;
                    }

                } else if (block->isRawValue()) {
                } else if (block->isVariableBlock()) {
                    VariableBlock* variableBlock = (VariableBlock*)block;
                    contexts.push_back(Context(variableBlock, 0));
                }
            }

            if (isFind) {
                break;
            }

            if (c->index == parent->variables.size() &&
                parent == contexts.back().block) {
                contexts.pop_back();
            }
        }

        if (findValue) {
            // FIXME : To support the full style with variableContainers
            // such as 'rgb(100, var(--foo1), var(--foo2))'.
            // Before that, we should solve a issue(#1132).
            ret = CSSTokenValue(findValue->toUTF8NonGCString().data());
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
                           variable->m_value.c_str());
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
}
