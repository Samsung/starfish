/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCSSVariableSyntaxTreeBuilder__
#define __StarFishCSSVariableSyntaxTreeBuilder__

#include "StarFish.h"

#include "core/style/Style.h"

namespace StarFish {

class CSSVariableSyntaxTreeBuilder : public gc {
    class Block : public gc {
    public:
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
    }

    void build(const CSSTokenValue&);

    void buildTree(VariableContainer*, CSSTokenValue&);

    void generateStyle()
    {
        // TODO: Generate new style with the variable map and syntax trees.
    }

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
