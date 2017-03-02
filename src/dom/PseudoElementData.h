/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "style/Style.h"

namespace StarFish {

class PseudoElementData : public gc {
public:
    PseudoElementData()
        : m_hasFirstLine(false),
          m_hasFirstLetter(false),
          m_hasBefore(false),
          m_hasAfter(false)
    {
    }

    bool hasPseudoElements() const
    {
        return m_hasFirstLine || m_hasFirstLetter || m_hasBefore || m_hasAfter;
    }

    bool hasPseudoElement(StyleResolver::PseudoElementType type) const
    {
        switch (type) {
        case StyleResolver::PseudoElementType::PseudoElementFirstLine:
            return m_hasFirstLine;
        case StyleResolver::PseudoElementType::PseudoElementFirstLetter:
            return m_hasFirstLetter;
        case StyleResolver::PseudoElementType::PseudoElementBefore:
            return m_hasBefore;
        case StyleResolver::PseudoElementType::PseudoElementAfter:
            return m_hasAfter;
        default:
            return false;
        }
    }

    void setPseudoElement(StyleResolver::PseudoElementType type)
    {
        switch (type) {
        case StyleResolver::PseudoElementType::PseudoElementFirstLine:
            m_hasFirstLine = true;
            break;
        case StyleResolver::PseudoElementType::PseudoElementFirstLetter:
            m_hasFirstLetter = true;
            break;
        case StyleResolver::PseudoElementType::PseudoElementBefore:
            m_hasBefore = true;
            break;
        case StyleResolver::PseudoElementType::PseudoElementAfter:
            m_hasAfter = true;
            break;
        case StyleResolver::PseudoElementType::PseudoElementNone:
            m_hasFirstLine = false;
            m_hasFirstLetter = false;
            m_hasBefore = false;
            m_hasAfter = false;
            break;
        default:
            break;
        }
    }

private:
    bool m_hasFirstLine;
    bool m_hasFirstLetter;
    bool m_hasBefore;
    bool m_hasAfter;
};
}

#endif
