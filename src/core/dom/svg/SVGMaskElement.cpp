/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/svg/SVGMaskElement.h"

namespace Starfish {

void* SVGMaskElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGMaskElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGMaskElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMaskElement, m_x1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMaskElement, m_y1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMaskElement, m_x2));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMaskElement, m_y2));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGMaskElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGMaskElement::SVGMaskElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
{
}

void SVGMaskElement::didAttributeChanged(QualifiedName name,
                                         Optional<String*> old, String* value,
                                         bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_x1 == name) {
        attributeOfPaintServerLikeUpdated();
    } else if (ss->m_y1 == name) {
        attributeOfPaintServerLikeUpdated();
    } else if (ss->m_x2 == name) {
        attributeOfPaintServerLikeUpdated();
    } else if (ss->m_y2 == name) {
        attributeOfPaintServerLikeUpdated();
    }
}

void SVGMaskElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_x1 == name) {
        setAttribute(ss->m_x1, x1()->baseVal()->valueAsString());
    } else if (ss->m_y1 == name) {
        setAttribute(ss->m_y1, y1()->baseVal()->valueAsString());
    } else if (ss->m_x2 == name) {
        setAttribute(ss->m_x2, x2()->baseVal()->valueAsString());
    } else if (ss->m_y2 == name) {
        setAttribute(ss->m_y2, y2()->baseVal()->valueAsString());
    }
}

void SVGMaskElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
}

void SVGMaskElement::didNodeInserted(Node* parent, Node* newChild)
{
    SVGElement::didNodeInserted(parent, newChild);
    attributeOfPaintServerLikeUpdated();
}

void SVGMaskElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    SVGElement::didNodeRemoved(parent, oldChild);
    attributeOfPaintServerLikeUpdated();
}

} // namespace Starfish
