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

#ifndef __StarFishPseudoElement__
#define __StarFishPseudoElement__

#include "Element.h"

namespace StarFish {

class PseudoElement : public Element {
public:
    PseudoElement(Document* document, StyleResolver::PseudoElementType pseudoId)
        : Element(document)
        , m_name(pseudoElementTagName(pseudoId))
        , m_pseudoId(pseudoId)
    {
    }

    virtual QualifiedName name()
    {
        return m_name;
    }

    virtual String* localName()
    {
        return m_name.localName();
    }

    virtual String* nodeName()
    {
        return m_name.localName();
    }

    StyleResolver::PseudoElementType getPseudoId() const
    {
        return m_pseudoId;
    }

    QualifiedName pseudoElementTagName(StyleResolver::PseudoElementType pseudoId);

protected:
    QualifiedName m_name;
    StyleResolver::PseudoElementType m_pseudoId;
};

inline bool pseudoElementLayoutObjectIsNeeded(ComputedStyle* style)
{
    if (!style || style->display() == NoneDisplayValue) {
        return false;
    }
    if (style->pseudoType() == StyleResolver::PseudoElementType::PseudoElementFirstLetter) {
        return true;
    }
#if 0
    return style->contentData();
#else
    return false;
#endif
}

class FirstLetterPseudoElement : public PseudoElement {
public:
    FirstLetterPseudoElement(Document* document, StyleResolver::PseudoElementType pseudoId)
        : PseudoElement(document, pseudoId)
    {
    }

    static size_t firstLetterLength(String* text);
    static Frame* firstLetterFrameText(Node* n);
};

}

#endif
