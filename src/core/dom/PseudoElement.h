/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPseudoElement__
#define __StarfishPseudoElement__

#include "Element.h"
#include "core/style/ComputedStyle.h"
#include "core/style/Style.h"

namespace Starfish {

class PseudoElement : public Element {
public:
    QualifiedName pseudoElementTagName(Document* document,
                                       PseudoElementType pseudoId);

    PseudoElement(Document* document, Element* originElement,
                  PseudoElementType pseudoId)
        : Element(document, pseudoElementTagName(document, pseudoId))
        , m_originElement(originElement)
        , m_pseudoId(pseudoId)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual QualifiedName name()
    {
        return m_name;
    }

    virtual String* localName()
    {
        return name().localName();
    }

    virtual String* nodeName()
    {
        return name().localName();
    }

    Element* originElement() const
    {
        return m_originElement;
    }

    PseudoElementType getPseudoId() const
    {
        return m_pseudoId;
    }

protected:
    Element* m_originElement;
    PseudoElementType m_pseudoId;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        Element::fillGCDescriptor(desc);
    }
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
    FirstLetterPseudoElement(Document* document, Element* originElement,
                             PseudoElementType pseudoId)
        : PseudoElement(document, originElement, pseudoId)
    {
    }

    static size_t firstLetterLength(String* text);
    static Frame* firstLetterFrameText(Node* n);
};
}

#endif
