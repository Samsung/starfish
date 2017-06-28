/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishPseudoElementData__
#define __StarFishPseudoElementData__

#include "core/style/Style.h"

namespace StarFish {

class PseudoElement;

class PseudoElementData : public gc {
public:
    PseudoElementData()
        : m_pseudoElementType(
              StyleResolver::PseudoElementType::PseudoElementNone)
        , m_firstLine(nullptr)
        , m_firstLetter(nullptr)
        , m_before(nullptr)
        , m_after(nullptr)
        , m_firstLineInherited(nullptr)
    {
    }

    bool hasPseudoElements() const
    {
        return m_pseudoElementType !=
               StyleResolver::PseudoElementType::PseudoElementNone;
    }

    bool hasPseudoElement(StyleResolver::PseudoElementType type) const
    {
        return m_pseudoElementType & type;
    }

    PseudoElement* pseudoElement(StyleResolver::PseudoElementType type) const
    {
        switch (type) {
        case StyleResolver::PseudoElementType::PseudoElementFirstLine:
            return m_firstLine;
        case StyleResolver::PseudoElementType::PseudoElementFirstLetter:
            return m_firstLetter;
        case StyleResolver::PseudoElementType::PseudoElementBefore:
            return m_before;
        case StyleResolver::PseudoElementType::PseudoElementAfter:
            return m_after;
        case StyleResolver::PseudoElementType::PseudoElementFirstLineInherited:
            return m_firstLineInherited;
        default:
            return nullptr;
        }
    }

    void setPseudoElement(StyleResolver::PseudoElementType type,
                          PseudoElement* pseudoElement)
    {
        m_pseudoElementType |= type;

        if (!pseudoElement) {
            return;
        }

        switch (type) {
        case StyleResolver::PseudoElementType::PseudoElementFirstLine:
            m_firstLine = pseudoElement;
            break;
        case StyleResolver::PseudoElementType::PseudoElementFirstLetter:
            m_firstLetter = pseudoElement;
            break;
        case StyleResolver::PseudoElementType::PseudoElementBefore:
            m_before = pseudoElement;
            break;
        case StyleResolver::PseudoElementType::PseudoElementAfter:
            m_after = pseudoElement;
            break;
        case StyleResolver::PseudoElementType::PseudoElementFirstLineInherited:
            m_firstLineInherited = pseudoElement;
            break;
        default:
            break;
        }
    }

    void clearPseudoElements()
    {
        m_pseudoElementType =
            StyleResolver::PseudoElementType::PseudoElementNone;
        m_firstLine = nullptr;
        m_firstLetter = nullptr;
        m_before = nullptr;
        m_after = nullptr;
        m_firstLineInherited = nullptr;
    }

private:
    int m_pseudoElementType;
    PseudoElement* m_firstLine;
    PseudoElement* m_firstLetter;
    PseudoElement* m_before;
    PseudoElement* m_after;
    PseudoElement* m_firstLineInherited;
};
}

#endif
