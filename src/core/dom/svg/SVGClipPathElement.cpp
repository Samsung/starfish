/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "core/dom/ShadowRoot.h"
#include "core/dom/svg/SVGClipPathElement.h"
#include "binding/DocumentHoldable.h"

namespace Starfish {

void* SVGClipPathElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGClipPathElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGClipPathElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGClipPathElement, m_clipPathUnits));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGClipPathElement));

        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGClipPathElement::SVGClipPathElement(Document* document,
                                       const QualifiedName& qname)
    : SVGElement(document, qname)
{
    m_clipPathUnits = new SVGAnimatedEnumeration(
        this, QualifiedName(staticStrings()->m_clipPathUnits),
        SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE,
        SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
}

void SVGClipPathElement::didNodeInserted(Node* parent, Node* newChild)
{
    SVGElement::didNodeInserted(parent, newChild);
    attributeOfPaintServerLikeUpdated();
}

void SVGClipPathElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    SVGElement::didNodeRemoved(parent, oldChild);
    attributeOfPaintServerLikeUpdated();
}

SVGAnimatedEnumeration* SVGClipPathElement::clipPathUnits()
{
    return m_clipPathUnits;
}
} // namespace Starfish
