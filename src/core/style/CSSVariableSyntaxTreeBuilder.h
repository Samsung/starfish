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

#ifndef __StarFishCSSVariableSyntaxTreeBuilder__
#define __StarFishCSSVariableSyntaxTreeBuilder__

#include "StarFish.h"

#include "core/style/Style.h"

namespace StarFish {

class CSSVariableSyntaxTreeBuilder : public gc {
    class Block : public gc {
    public:
        virtual ~Block()
        {
        }
        virtual bool isVariableBlock() = 0;
        virtual bool isVariable() = 0;
        virtual bool isRawValue() = 0;
    };

    class VariableBlock : public Block {
    public:
        virtual bool isVariable()
        {
            return false;
        }
        virtual bool isRawValue()
        {
            return false;
        }
        virtual bool isVariableBlock()
        {
            return true;
        }
        GCVector<Block*> variables;
    };

    class Variable : public Block {
    public:
        virtual bool isVariable()
        {
            return true;
        }
        virtual bool isRawValue()
        {
            return false;
        }
        virtual bool isVariableBlock()
        {
            return false;
        }
        Variable(CSSTokenValue value)
            : m_value(value)
        {
        }
        CSSTokenValue m_value;
    };

    class RawValue : public Block {
    public:
        virtual bool isVariable()
        {
            return false;
        }
        virtual bool isRawValue()
        {
            return true;
        }
        virtual bool isVariableBlock()
        {
            return false;
        }
    };

    class VariableContainer {
    public:
        VariableContainer(size_t start, size_t end)
            : m_start(start)
            , m_end(end)
            , m_root(nullptr)
        {
        }
        size_t m_start;
        size_t m_end;
        Block* m_root;
    };

public:
    CSSVariableSyntaxTreeBuilder()
        : m_valid(true)
    {
        m_variableContainers.clear();
    }

    void build(const CSSTokenValue&);

    void buildTree(VariableContainer*, CSSTokenValue&);

    CSSTokenValue generateStyle(GCVector<MutablePropertyValue>&);

    void dump();

    bool isValid()
    {
        return m_valid;
    }

private:
    bool m_valid;
    GCVector<VariableContainer> m_variableContainers;
};
}

#endif /* __StarFishCSSVariableSyntaxTreeBuilder__ */
