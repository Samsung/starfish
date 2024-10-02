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

#ifndef __StarfishCSSVariableSyntaxTreeBuilder__
#define __StarfishCSSVariableSyntaxTreeBuilder__

#include "Starfish.h"

#include "core/style/Style.h"

namespace Starfish {

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
        Variable(AtomicString value)
            : m_value(value)
        {
        }
        AtomicString m_value;
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
        VariableContainer(size_t start = 0, size_t end = 0)
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
    STARFISH_MAKE_STACK_ALLOCATED();
    CSSVariableSyntaxTreeBuilder(Optional<Starfish*> sf)
        : m_valid(true)
        , m_starfish(sf)
    {
    }

    void build(const char* str, size_t length);

    void buildTree(VariableContainer*, const char* str, size_t length);

    typedef VectorWithInlineStorage<256, char, std::allocator<char>>
        StyleString;
    StyleString generateStyle(Element* element,
                              Optional<const MutablePropertyValueList*>);

    void dump();

    bool isValid()
    {
        return m_valid;
    }

    void reset()
    {
        m_valid = true;
        m_variableContainers.clear();
    }

private:
    bool m_valid;
    Optional<Starfish*> m_starfish; // if this value is null, there is no actual
                                    // Variable will build.
    VectorWithInlineStorage<12, VariableContainer,
                            GCUtil::gc_malloc_allocator<VariableContainer>>
        m_variableContainers;
};
} // namespace Starfish

#endif /* __StarfishCSSVariableSyntaxTreeBuilder__ */
