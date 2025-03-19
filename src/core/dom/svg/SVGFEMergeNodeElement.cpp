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

#include "core/dom/svg/SVGFEMergeNodeElement.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGAnimatedString.h"
#include "core/dom/svg/SVGAnimatedNumberList.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {

void SVGFEMergeNodeElement::didAttributeChanged(QualifiedName name,
                                                Optional<String*> old,
                                                String* value,
                                                bool attributeCreated,
                                                bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_in == name) {
        in()->setBaseVal(value, true);
        notifyAttributeOfPaintServerLikeUpdated();
    }
}

void SVGFEMergeNodeElement::updateSVGAttributeNeeded(QualifiedName name)
{
    SVGElement::updateSVGAttributeNeeded(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        setAttribute(ss->m_in, in()->baseVal());
    }
}

void SVGFEMergeNodeElement::notifyAttributeOfPaintServerLikeUpdated()
{
    if (parentElement() && parentElement()->isSVGFEMergeElement() &&
        parentElement()->parentElement() &&
        parentElement()->parentElement()->isSVGFilterElement()) {
        parentElement()
            ->parentElement()
            ->asSVGFilterElement()
            ->attributeOfPaintServerLikeUpdated(false);
    }
}

SVGAnimatedString* SVGFEMergeNodeElement::in()
{
    if (!m_in.hasValue()) {
        m_in = new SVGAnimatedString(this, starfish()->staticStrings()->m_in,
                                     String::emptyString, String::emptyString);
    }
    return m_in.getValue();
}

void* SVGFEMergeNodeElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEMergeNodeElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEMergeNodeElement)] = { 0 };
        fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFEMergeNodeElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

} // namespace Starfish
