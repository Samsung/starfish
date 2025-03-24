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
#include "core/dom/svg/SVGFECompositeElement.h"

namespace Starfish {
void* SVGFECompositeElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFECompositeElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFECompositeElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFECompositeElement, m_in1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFECompositeElement, m_in2));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFECompositeElement, m_operator));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFECompositeElement, m_k1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFECompositeElement, m_k2));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFECompositeElement, m_k3));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFECompositeElement, m_k4));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFECompositeElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFECompositeElement::didAttributeChanged(QualifiedName name,
                                                Optional<String*> old,
                                                String* value,
                                                bool attributeCreated,
                                                bool attributeRemoved)
{
    SVGFilterPrimitiveStandardAttributes::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in1 == name || ss->m_in == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        in1()->setBaseVal(value, true);
    } else if (ss->m_in2 == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        in2()->setBaseVal(value, true);
    } else if (ss->m_k1 == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        k1()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_k2 == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        k2()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_k3 == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        k3()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_k4 == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        k4()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_operator == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
        if (domOperator()->isUpdated() == false) {
            if (value->equals("over")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_OVER);
            } else if (value->equals("in")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_IN);
            } else if (value->equals("out")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_OUT);
            } else if (value->equals("atop")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_ATOP);
            } else if (value->equals("xor")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_XOR);
            } else if (value->equals("lighter")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_LIGHTER);
            } else if (value->equals("arithmetic")) {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_ARITHMETIC);
            } else {
                m_operator->setBaseValWithoutUpdateAttribute(
                    CompositeOperator::SVG_FECOMPOSITE_OPERATOR_UNKNOWN);
            }
        }
    }
}

void SVGFECompositeElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in1 == name) {
        setAttribute(ss->m_in1, in1()->baseVal());
    } else if (ss->m_in2 == name) {
        setAttribute(ss->m_in2, in2()->baseVal());
    } else if (ss->m_operator == name) {
        m_operator->updateAttribute();
    }
}

SVGAnimatedString* SVGFECompositeElement::in1()
{
    if (!m_in1.hasValue()) {
        m_in1 = new SVGAnimatedString(this, starfish()->staticStrings()->m_in1,
                                      String::emptyString, String::emptyString);
    }
    return m_in1.getValue();
}

SVGAnimatedString* SVGFECompositeElement::in2()
{
    if (!m_in2.hasValue()) {
        m_in2 = new SVGAnimatedString(this, starfish()->staticStrings()->m_in2,
                                      String::emptyString, String::emptyString);
    }
    return m_in2.getValue();
}

SVGAnimatedEnumeration* SVGFECompositeElement::domOperator()
{
    if (!m_operator.hasValue()) {
        m_operator = new SVGAnimatedEnumeration(
            this, staticStrings()->m_edgeMode,
            CompositeOperator::SVG_FECOMPOSITE_OPERATOR_OVER,
            CompositeOperator::SVG_FECOMPOSITE_OPERATOR_OVER,
            CompositeOperator::SVG_FECOMPOSITE_OPERATOR_LIGHTER);
    }
    return m_operator.getValue();
}

SVGAnimatedNumber* SVGFECompositeElement::k1()
{
    if (!m_k1.hasValue()) {
        m_k1 = new SVGAnimatedNumber(this, starfish()->staticStrings()->m_k1, 0,
                                     0);
    }
    return m_k1.getValue();
}

SVGAnimatedNumber* SVGFECompositeElement::k2()
{
    if (!m_k2.hasValue()) {
        m_k2 = new SVGAnimatedNumber(this, starfish()->staticStrings()->m_k2, 0,
                                     0);
    }
    return m_k2.getValue();
}

SVGAnimatedNumber* SVGFECompositeElement::k3()
{
    if (!m_k3.hasValue()) {
        m_k3 = new SVGAnimatedNumber(this, starfish()->staticStrings()->m_k3, 0,
                                     0);
    }
    return m_k3.getValue();
}

SVGAnimatedNumber* SVGFECompositeElement::k4()
{
    if (!m_k4.hasValue()) {
        m_k4 = new SVGAnimatedNumber(this, starfish()->staticStrings()->m_k4, 0,
                                     0);
    }
    return m_k4.getValue();
}

} // namespace Starfish
