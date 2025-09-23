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
#include "core/dom/svg/SVGFEMorphologyElement.h"
#include "core/dom/DOMTokenList.h"

namespace Starfish {
SVGFEMorphologyElement::SVGFEMorphologyElement(Document* document,
                                               const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEMorphologyElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEMorphologyElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEMorphologyElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEMorphologyElement, m_in));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEMorphologyElement, m_in));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEMorphologyElement, m_radiusX));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEMorphologyElement, m_radiusY));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEMorphologyElement, m_operator));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFEMorphologyElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEMorphologyElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGFilterPrimitiveStandardAttributes::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name || ss->m_in1 == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_radius == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_radiusX == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_radiusY == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_operator == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    }
}

void SVGFEMorphologyElement::didAttributeChanged(QualifiedName name,
                                                 Optional<String*> old,
                                                 String* value,
                                                 bool attributeCreated,
                                                 bool attributeRemoved)
{
    SVGFilterPrimitiveStandardAttributes::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_in == name || ss->m_in1 == name) {
        in()->setBaseVal(value, true);
    } else if (ss->m_radius == name) {
        radiusX()->setBaseVal(String::parseFloat(value), true);
        radiusY()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_radiusX == name) {
        radiusX()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_radiusY == name) {
        radiusY()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_operator == name) {
        if (domOperator()->isUpdated() == false) {
            if (value->equals("erode")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    MorphologyOperator::SVG_MORPHOLOGY_OPERATOR_ERODE);
            } else if (value->equals("dilate")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    MorphologyOperator::SVG_MORPHOLOGY_OPERATOR_DILATE);
            } else {
                m_operator->setBaseValWithoutUpdateAttribute(
                    MorphologyOperator::SVG_MORPHOLOGY_OPERATOR_UNKNOWN);
            }
        }
    }
}

void SVGFEMorphologyElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name || ss->m_in1 == name) {
        setAttribute(ss->m_in, in()->baseVal());
    } else if (ss->m_radiusX == name) {
        setAttribute(ss->m_radiusX, String::fromFloat(radiusX()->baseVal()));
    } else if (ss->m_radiusY == name) {
        setAttribute(ss->m_radiusY, String::fromFloat(radiusY()->baseVal()));
    } else if (ss->m_operator == name) {
        m_operator->updateAttribute();
    }
}

void SVGFEMorphologyElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGFilterPrimitiveStandardAttributes::styleForPresentationAttribute(
        cssValues, cssCustomValues);
}

SVGAnimatedString* SVGFEMorphologyElement::in()
{
    if (!m_in.hasValue()) {
        m_in = new SVGAnimatedString(this, starfish()->staticStrings()->m_in,
                                     String::emptyString, String::emptyString);
    }
    return m_in.getValue();
}

SVGAnimatedNumber* SVGFEMorphologyElement::radiusX()
{
    if (!m_radiusX.hasValue()) {
        m_radiusX = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_radiusX, 0, 0);
    }
    return m_radiusX.getValue();
}

SVGAnimatedNumber* SVGFEMorphologyElement::radiusY()
{
    if (!m_radiusY.hasValue()) {
        m_radiusY = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_radiusY, 0, 0);
    }
    return m_radiusY.getValue();
}

SVGAnimatedEnumeration* SVGFEMorphologyElement::domOperator()
{
    if (!m_operator.hasValue()) {
        m_operator = new SVGAnimatedEnumeration(
            this, staticStrings()->m_operator,
            MorphologyOperator::SVG_MORPHOLOGY_OPERATOR_ERODE,
            MorphologyOperator::SVG_MORPHOLOGY_OPERATOR_ERODE,
            MorphologyOperator::SVG_MORPHOLOGY_OPERATOR_DILATE);
    }
    return m_operator.getValue();
}
} // namespace Starfish
