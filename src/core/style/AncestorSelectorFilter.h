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

#ifndef __StarfishAncestorSelectorFilter__
#define __StarfishAncestorSelectorFilter__

#include "core/util/BloomFilter.h"
#include "core/style/Style.h"
#include "core/style/StyleRule.h"

namespace Starfish {

class StyleRule;

class AncestorSelectorFilter {
public:
    AncestorSelectorFilter()
    {
        m_parentStack.reserve(32);
    }
    ~AncestorSelectorFilter()
    {
    }

    void pushNode(Node* n);
    void popNode();

    static void computeIdentifierHash(StyleRule* rule);
    ALWAYS_INLINE bool canUseAncestorSelectorFilter(Element* e)
    {
        if (!m_parentStack.size() ||
            (Node*)m_parentStack.back().m_node != e->parentNode()) {
            return false;
        }
        return true;
    }

    ALWAYS_INLINE bool canIgnoreSelector(StyleRule* rule, Element* e)
    {
        STARFISH_ASSERT(canUseAncestorSelectorFilter(e));
        unsigned* identifierHashes = rule->m_identifierHashes;
        for (unsigned n = 0;
             n < StyleRule::maximumIdentifierCount && identifierHashes[n];
             ++n) {
            if (!m_bloomFilter.mayContain(identifierHashes[n])) {
                return true;
            }
        }

        return false;
    }

private:
    struct AncestorStackFrame {
        AncestorStackFrame(Node* node)
            : m_node(node)
        {
        }

        AncestorStackFrame(AncestorStackFrame&& frame)
        {
            m_node = frame.m_node;
            m_identifierHashes = std::move(frame.m_identifierHashes);
        }

        Node* m_node;
        VectorWithInlineStorage<16, unsigned, std::allocator<unsigned>>
            m_identifierHashes;
    };
    std::vector<AncestorStackFrame> m_parentStack;

    BloomFilter<12> m_bloomFilter;
};
} // namespace Starfish

#endif
