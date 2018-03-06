/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishPseudoElement__
#define __StarFishPseudoElement__

#include "Element.h"
#include "core/style/ComputedStyle.h"
#include "core/style/Style.h"

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

    QualifiedName pseudoElementTagName(
        StyleResolver::PseudoElementType pseudoId);

protected:
    QualifiedName m_name;
    StyleResolver::PseudoElementType m_pseudoId;
};

inline bool pseudoElementFrameIsNeeded(ComputedStyle* style)
{
    if (!style || style->display() == NoneDisplayValue) {
        return false;
    }
    return true;
}

class FirstLetterPseudoElement : public PseudoElement {
public:
    FirstLetterPseudoElement(Document* document,
                             StyleResolver::PseudoElementType pseudoId)
        : PseudoElement(document, pseudoId)
    {
    }

    static size_t firstLetterLength(String* text);
    static Frame* firstLetterFrameText(Node* n);
};
}

#endif
