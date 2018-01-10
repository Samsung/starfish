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

#ifndef __StarFishAncestorSelectorFilter__
#define __StarFishAncestorSelectorFilter__

#include "core/util/BloomFilter.h"
#include "core/style/Style.h"
#include "core/style/StyleRule.h"

namespace StarFish {

class StyleRule;

class AncestorSelectorFilter {
public:
    AncestorSelectorFilter()
    {
    }
    ~AncestorSelectorFilter()
    {
    }

    void pushElement(Element* e);
    void popElement();

    static void computeIdentifierHash(StyleRule* rule);
    ALWAYS_INLINE bool canUseAncestorSelectorFilter(Element* e)
    {
        if (!m_parentStack.size() ||
            (Node*)m_parentStack.back().m_element != e->parentNode()) {
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
        AncestorStackFrame(Element* element)
            : m_element(element)
        {
        }
        Element* m_element;
        std::vector<unsigned> m_identifierHashes;
    };
    std::vector<AncestorStackFrame> m_parentStack;

    BloomFilter<12> m_bloomFilter;
};
}

#endif
