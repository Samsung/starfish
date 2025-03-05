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
#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/dom/svg/SVGFilterElement.h"

namespace Starfish {

void* SVGFilterPrimitiveStandardAttributes::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFilterPrimitiveStandardAttributes));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFilterPrimitiveStandardAttributes)] = {
            0
        };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes, m_x));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes, m_y));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes,
                                        m_width));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes,
                                        m_height));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes,
                                        m_result));
        descr = GC_make_descriptor(
            desc, GC_WORD_LEN(SVGFilterPrimitiveStandardAttributes));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFilterPrimitiveStandardAttributes::didAttributeChanged(
    QualifiedName name, Optional<String*> old, String* value,
    bool attributeCreated, bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
}

void SVGFilterPrimitiveStandardAttributes::updateSVGAttributeNeeded(
    QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_x == name) {
        setAttribute(ss->m_x, x()->baseVal()->valueAsString());
    } else if (ss->m_y == name) {
        setAttribute(ss->m_y, y()->baseVal()->valueAsString());
    } else if (ss->m_width == name) {
        setAttribute(ss->m_width, width()->baseVal()->valueAsString());
    } else if (ss->m_height == name) {
        setAttribute(ss->m_height, height()->baseVal()->valueAsString());
    } else if (ss->m_result == name) {
        setAttribute(ss->m_result, result()->baseVal());
    }
}

void SVGFilterPrimitiveStandardAttributes::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
}

SVGAnimatedString* SVGFilterPrimitiveStandardAttributes::result()
{
    return m_result.getValue();
}

Optional<SVGFilterElement*>
SVGFilterPrimitiveStandardAttributes::filterElement()
{
    if (parentElement() && parentElement()->isSVGFilterElement()) {
        return parentElement()->asSVGFilterElement();
    }
    return nullptr;
}

void SVGFilterPrimitiveStandardAttributes::
    notifyAttributeOfPaintServerLikeUpdated()
{
    Optional<SVGFilterElement*> filterElement = this->filterElement();
    if (filterElement.hasValue()) {
        filterElement->attributeOfPaintServerLikeUpdated();
    }
}
} // namespace Starfish
