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
#include "core/dom/svg/SVGFEMergeElement.h"

namespace Starfish {
SVGFEMergeElement::SVGFEMergeElement(Document* document,
                                     const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEMergeElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEMergeElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEMergeElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFEMergeElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEMergeElement::didNodeInserted(Node* parent, Node* newChild)
{
    SVGFilterPrimitiveStandardAttributes::didNodeInserted(parent, newChild);
    notifyAttributeOfPaintServerLikeUpdated(false);
}

void SVGFEMergeElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    SVGFilterPrimitiveStandardAttributes::didNodeRemoved(parent, oldChild);
    notifyAttributeOfPaintServerLikeUpdated(false);
}

} // namespace Starfish
