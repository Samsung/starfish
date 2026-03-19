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
#include "core/dom/svg/SVGFEDisplacementMapElement.h"
#include "core/dom/svg/SVGAnimatedInteger.h"
#include "core/dom/DOMTokenList.h"

namespace Starfish {

SVGFEDisplacementMapElement::SVGFEDisplacementMapElement(
    Document* document, const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEDisplacementMapElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEDisplacementMapElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEDisplacementMapElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEDisplacementMapElement, m_in1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEDisplacementMapElement, m_in2));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEDisplacementMapElement, m_scale));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEDisplacementMapElement,
                                        m_xChannelSelector));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEDisplacementMapElement,
                                        m_yChannelSelector));

        descr =
            GC_make_descriptor(desc, GC_WORD_LEN(SVGFEDisplacementMapElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEDisplacementMapElement::didAttributeChanged(QualifiedName name,
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
    } else if (ss->m_scale == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        scale()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_xChannelSelector == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        if (xChannelSelector()->isUpdated() == false) {
            if (value->equals("R")) {
                xChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_R);
            } else if (value->equals("G")) {
                xChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_G);
            } else if (value->equals("B")) {
                xChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_B);
            } else if (value->equals("A")) {
                xChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_A);
            } else {
                xChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_UNKNOWN);
            }
        }
    } else if (ss->m_yChannelSelector == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        if (yChannelSelector()->isUpdated() == false) {
            if (value->equals("R")) {
                yChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_R);
            } else if (value->equals("G")) {
                yChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_G);
            } else if (value->equals("B")) {
                yChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_B);
            } else if (value->equals("A")) {
                yChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_A);
            } else {
                yChannelSelector()->setBaseValWithoutUpdateAttribute(
                    SVGFEDisplacementMapElement::ChannelSelector::
                        SVG_CHANNEL_UNKNOWN);
            }
        }
    }
}

void SVGFEDisplacementMapElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in2 == name) {
        setAttribute(ss->m_in2, in2()->baseVal());
    } else if (ss->m_in == name || ss->m_in1 == name) {
        setAttribute(ss->m_in1, in1()->baseVal());
    } else if (ss->m_scale == name) {
        setAttribute(ss->m_scale, String::fromFloat(scale()->baseVal()));
    } else if (ss->m_xChannelSelector == name) {
        m_xChannelSelector->updateAttribute();
    } else if (ss->m_yChannelSelector == name) {
        m_yChannelSelector->updateAttribute();
    }
}

void SVGFEDisplacementMapElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGFilterPrimitiveStandardAttributes::styleForPresentationAttribute(
        cssValues, matchedRules, cssCustomValues);
}

SVGAnimatedString* SVGFEDisplacementMapElement::in1()
{
    if (!m_in1.hasValue()) {
        m_in1 = new SVGAnimatedString(this, starfish()->staticStrings()->m_in1,
                                      String::emptyString, String::emptyString);
    }
    return m_in1.getValue();
}

SVGAnimatedString* SVGFEDisplacementMapElement::in2()
{
    if (!m_in2.hasValue()) {
        m_in2 = new SVGAnimatedString(this, starfish()->staticStrings()->m_in2,
                                      String::emptyString, String::emptyString);
    }
    return m_in2.getValue();
}

SVGAnimatedNumber* SVGFEDisplacementMapElement::scale()
{
    if (!m_scale.hasValue()) {
        m_scale = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_scale, 0);
    }
    return m_scale.getValue();
}

SVGAnimatedEnumeration* SVGFEDisplacementMapElement::xChannelSelector()
{
    if (!m_xChannelSelector.hasValue()) {
        m_xChannelSelector = new SVGAnimatedEnumeration(
            this, staticStrings()->m_xChannelSelector,
            ChannelSelector::SVG_CHANNEL_A, ChannelSelector::SVG_CHANNEL_A,
            ChannelSelector::SVG_CHANNEL_A);
    }
    return m_xChannelSelector.getValue();
}

SVGAnimatedEnumeration* SVGFEDisplacementMapElement::yChannelSelector()
{
    if (!m_yChannelSelector.hasValue()) {
        m_yChannelSelector = new SVGAnimatedEnumeration(
            this, staticStrings()->m_yChannelSelector,
            ChannelSelector::SVG_CHANNEL_A, ChannelSelector::SVG_CHANNEL_A,
            ChannelSelector::SVG_CHANNEL_A);
    }
    return m_yChannelSelector.getValue();
}

} // namespace Starfish
