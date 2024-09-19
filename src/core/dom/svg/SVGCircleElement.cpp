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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/style/CSSParser.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGCircleElement.h"
#include "core/dom/svg/SVGAnimatedTransformList.h"

namespace Starfish {

void SVGCircleElement::didAttributeChanged(QualifiedName name,
                                           Nullable<String*> old, String* value,
                                           bool attributeCreated,
                                           bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_r == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_cx == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_cy == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_transform == name) {
        if (m_transform == nullptr ||
            value->equals(transform()->baseVal()->toString()) == false) {
            transform()->baseVal()->updateListByAttribute();
            transform()->animVal()->updateListByAttribute();
        }

        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    }
}

void SVGCircleElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_transform == name) {
        transform()->baseVal()->updateAttributeByList();
    } else if (ss->m_r == name) {
        setAttribute(ss->m_r, r()->baseVal()->valueAsString());
    } else if (ss->m_cx == name) {
        setAttribute(ss->m_cx, cx()->baseVal()->valueAsString());
    } else if (ss->m_cy == name) {
        setAttribute(ss->m_cy, cy()->baseVal()->valueAsString());
    }
}

void* SVGCircleElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGCircleElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGCircleElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);

        GC_set_bit(desc, GC_WORD_OFFSET(SVGCircleElement, m_transform));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGCircleElement, m_cx));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGCircleElement, m_cy));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGCircleElement, m_r));

        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGCircleElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGCircleElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Nullable<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(r, R, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cx, CX, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cy, CY, cssCustomValues);
}

SVGAnimatedTransformList* SVGCircleElement::transform()
{
    if (m_transform == nullptr) {
        SVGTransformList* baseVal = new SVGTransformList(
            this, starfish()->staticStrings()->m_transform);
        SVGTransformList* animVal = new SVGTransformList(
            this, starfish()->staticStrings()->m_transform, true);
        m_transform =
            new SVGAnimatedTransformList(document(), baseVal, animVal);
    }

    return m_transform;
}
} // namespace Starfish
