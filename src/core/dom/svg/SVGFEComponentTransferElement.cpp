/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGFEComponentTransferElement.h"

namespace Starfish {
SVGFEComponentTransferElement::SVGFEComponentTransferElement(
    Document* document, const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEComponentTransferElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEComponentTransferElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEComponentTransferElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEComponentTransferElement, m_in));
        descr = GC_make_descriptor(desc,
                                   GC_WORD_LEN(SVGFEComponentTransferElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEComponentTransferElement::didAttributeChanged(QualifiedName name,
                                                        Optional<String*> old,
                                                        String* value,
                                                        bool attributeCreated,
                                                        bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        in()->setBaseVal(value, true);
    }
}

void SVGFEComponentTransferElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        setAttribute(ss->m_in, in()->baseVal());
    }
}

void SVGFEComponentTransferElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
}

void SVGFEComponentTransferElement::didNodeInserted(Node* parent,
                                                    Node* newChild)
{
    SVGFilterPrimitiveStandardAttributes::didNodeInserted(parent, newChild);
    notifyAttributeOfPaintServerLikeUpdated(false);
}

void SVGFEComponentTransferElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    SVGFilterPrimitiveStandardAttributes::didNodeRemoved(parent, oldChild);
    notifyAttributeOfPaintServerLikeUpdated(false);
}

SVGAnimatedString* SVGFEComponentTransferElement::in()
{
    if (!m_in.hasValue()) {
        m_in = new SVGAnimatedString(this, starfish()->staticStrings()->m_in,
                                     String::emptyString, String::emptyString);
    }
    return m_in.getValue();
}

} // namespace Starfish
