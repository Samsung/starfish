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
#include "core/dom/svg/SVGFilterElement.h"
#include "core/modules/canvas/filter/Filter.h"

namespace Starfish {

void* SVGFilterElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFilterElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFilterElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterElement, m_filter));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterElement, m_filterUnits));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterElement, m_primitiveUnits));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterElement, m_x));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterElement, m_y));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterElement, m_width));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterElement, m_height));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFilterElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFilterElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGElement::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_filterUnits == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        attributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_primitiveUnits == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        attributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_x == name) {
        attributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_y == name) {
        attributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_width == name) {
        attributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_height == name) {
        attributeOfPaintServerLikeUpdated(true);
    }
}

void SVGFilterElement::didAttributeChanged(QualifiedName name,
                                           Optional<String*> old, String* value,
                                           bool attributeCreated,
                                           bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_filterUnits == name) {
        if (filterUnits()->isUpdated() == false) {
            if (value->equals("userSpaceOnUse")) {
                m_filterUnits->setBaseValWithoutUpdateAttribute(
                    SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
            } else if (value->equals("objectBoundingBox")) {
                m_filterUnits->setBaseValWithoutUpdateAttribute(
                    SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
            }
        }
    } else if (ss->m_primitiveUnits == name) {
        if (primitiveUnits()->isUpdated() == false) {
            if (value->equals("userSpaceOnUse")) {
                m_primitiveUnits->setBaseValWithoutUpdateAttribute(
                    SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
            } else if (value->equals("objectBoundingBox")) {
                m_primitiveUnits->setBaseValWithoutUpdateAttribute(
                    SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
            }
        }
    } else if (ss->m_x == name) {
        x()->baseVal()->setValueAsString(value, true);
    } else if (ss->m_y == name) {
        y()->baseVal()->setValueAsString(value, true);
    } else if (ss->m_width == name) {
        width()->baseVal()->setValueAsString(value, true);
    } else if (ss->m_height == name) {
        height()->baseVal()->setValueAsString(value, true);
    }
}

void SVGFilterElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_filterUnits == name) {
        m_filterUnits->updateAttribute();
    } else if (ss->m_primitiveUnits == name) {
        m_primitiveUnits->updateAttribute();
    } else if (ss->m_x == name) {
        setAttribute(ss->m_x, x()->baseVal()->valueAsString());
    } else if (ss->m_y == name) {
        setAttribute(ss->m_y, y()->baseVal()->valueAsString());
    } else if (ss->m_width == name) {
        setAttribute(ss->m_width, width()->baseVal()->valueAsString());
    } else if (ss->m_height == name) {
        setAttribute(ss->m_height, height()->baseVal()->valueAsString());
    }
}

void SVGFilterElement::didNodeInserted(Node* parent, Node* newChild)
{
    SVGElement::didNodeInserted(parent, newChild);
    attributeOfPaintServerLikeUpdated(true);
}

void SVGFilterElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    SVGElement::didNodeRemoved(parent, oldChild);
    attributeOfPaintServerLikeUpdated(true);
}

void SVGFilterElement::attributeOfPaintServerLikeUpdated(bool alsoNeedsLayout)
{
    SVGElement::attributeOfPaintServerLikeUpdated(alsoNeedsLayout);

    filter()->setNeedsUpdate();
}

Filter* SVGFilterElement::filter()
{
    if (!m_filter.hasValue()) {
        m_filter = new Filter(this);
    }
    return m_filter.value();
}

void SVGFilterElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
}

SVGAnimatedEnumeration* SVGFilterElement::filterUnits()
{
    if (!m_filterUnits.hasValue()) {
        m_filterUnits = new SVGAnimatedEnumeration(
            this, staticStrings()->m_filterUnits,
            SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
            SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
            SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    }
    return m_filterUnits.getValue();
}
SVGAnimatedEnumeration* SVGFilterElement::primitiveUnits()
{
    if (!m_primitiveUnits.hasValue()) {
        m_primitiveUnits = new SVGAnimatedEnumeration(
            this, staticStrings()->m_primitiveUnits,
            SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE,
            SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE,
            SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    }
    return m_primitiveUnits.getValue();
}

SVGAnimatedLength* SVGFilterElement::x()
{
    if (!m_x.hasValue()) {
        SVGLength* baseVal =
            new SVGLength(this, staticStrings()->m_x,
                          SVGLength::SVG_LENGTHTYPE_PERCENTAGE, -10);
        m_x = new SVGAnimatedLength(document(), baseVal, nullptr);
    }
    return m_x.value();
}

SVGAnimatedLength* SVGFilterElement::y()
{
    if (!m_y.hasValue()) {
        SVGLength* baseVal =
            new SVGLength(this, staticStrings()->m_y,
                          SVGLength::SVG_LENGTHTYPE_PERCENTAGE, -10);
        m_y = new SVGAnimatedLength(document(), baseVal, nullptr);
    }
    return m_y.value();
}

SVGAnimatedLength* SVGFilterElement::width()
{
    if (!m_width.hasValue()) {
        SVGLength* baseVal =
            new SVGLength(this, staticStrings()->m_width,
                          SVGLength::SVG_LENGTHTYPE_PERCENTAGE, 120);
        m_width = new SVGAnimatedLength(document(), baseVal, nullptr);
    }
    return m_width.value();
}

SVGAnimatedLength* SVGFilterElement::height()
{
    if (!m_height.hasValue()) {
        SVGLength* baseVal =
            new SVGLength(this, staticStrings()->m_height,
                          SVGLength::SVG_LENGTHTYPE_PERCENTAGE, 120);
        m_height = new SVGAnimatedLength(document(), baseVal, nullptr);
    }
    return m_height.value();
}

} // namespace Starfish
