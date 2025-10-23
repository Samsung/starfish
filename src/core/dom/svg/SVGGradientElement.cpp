/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "core/dom/svg/SVGGradientElement.h"
#include "core/dom/svg/SVGSVGElement.h"

namespace Starfish {

SVGGradientElement::SVGGradientElement(Document* document,
                                       const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_gradientUnits(nullptr)
    , m_gradientTransform(nullptr)
    , m_spreadMethod(nullptr)
{
}

void SVGGradientElement::didNodeInserted(Node* parent, Node* newChild)
{
    SVGElement::didNodeInserted(parent, newChild);

    if (parent == this && newChild->isSVGStopElement()) {
        attributeOfPaintServerLikeUpdated(false);
    }
}

void SVGGradientElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    SVGElement::didNodeRemoved(parent, oldChild);

    if (parent == this && oldChild->isSVGStopElement()) {
        attributeOfPaintServerLikeUpdated(false);
    }
}

void SVGGradientElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGElement::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_gradientUnits == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        attributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_gradientTransform == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        attributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_spreadMethod == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        attributeOfPaintServerLikeUpdated(false);
    }
}

void SVGGradientElement::didAttributeChanged(QualifiedName name,
                                             Optional<String*> old,
                                             String* value,
                                             bool attributeCreated,
                                             bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_gradientUnits == name) {
        if (gradientUnits()->isUpdated() == false) {
            if (value->equals("userSpaceOnUse")) {
                m_gradientUnits->setBaseValWithoutUpdateAttribute(
                    SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
            } else if (value->equals("objectBoundingBox")) {
                m_gradientUnits->setBaseValWithoutUpdateAttribute(
                    SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
            }
        }
    } else if (ss->m_gradientTransform == name) {
        if (value->equals(gradientTransform()->baseVal()->toString()) ==
            false) {
            gradientTransform()->baseVal()->updateListByAttribute();
        }
    } else if (ss->m_spreadMethod == name) {
        if (spreadMethod()->isUpdated() == false) {
            if (value->equals("pad")) {
                m_spreadMethod->setBaseValWithoutUpdateAttribute(
                    SVG_SPREADMETHOD_PAD);
            } else if (value->equals("reflect")) {
                m_spreadMethod->setBaseValWithoutUpdateAttribute(
                    SVG_SPREADMETHOD_REFLECT);
            } else if (value->equals("repeat")) {
                m_spreadMethod->setBaseValWithoutUpdateAttribute(
                    SVG_SPREADMETHOD_REPEAT);
            }
        }
    }
}

void SVGGradientElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_gradientUnits == name) {
        m_gradientUnits->updateAttribute();
    } else if (ss->m_gradientTransform == name) {
        m_gradientTransform->baseVal()->updateAttributeByList();
    } else if (ss->m_spreadMethod == name) {
        m_spreadMethod->updateAttribute();
    }
}

void* SVGGradientElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGGradientElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGGradientElement)] = { 0 };
        SVGGradientElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGGradientElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGAnimatedEnumeration* SVGGradientElement::gradientUnits()
{
    if (!m_gradientUnits.hasValue()) {
        m_gradientUnits = new SVGAnimatedEnumeration(
            this, staticStrings()->m_gradientUnits,
            SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
            SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
            SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    }
    return m_gradientUnits.value();
}

SVGAnimatedTransformList* SVGGradientElement::gradientTransform()
{
    if (!m_gradientTransform.hasValue()) {
        SVGTransformList* baseVal =
            new SVGTransformList(this, staticStrings()->m_gradientTransform);
        SVGTransformList* animVal = new SVGTransformList(
            this, staticStrings()->m_gradientTransform, baseVal);

        m_gradientTransform =
            new SVGAnimatedTransformList(document(), baseVal, animVal);
    }
    return m_gradientTransform.value();
}

SVGAnimatedEnumeration* SVGGradientElement::spreadMethod()
{
    if (!m_spreadMethod.hasValue()) {
        m_spreadMethod = new SVGAnimatedEnumeration(
            this, staticStrings()->m_spreadMethod, SVG_SPREADMETHOD_PAD,
            SVG_SPREADMETHOD_PAD, SVG_SPREADMETHOD_REPEAT);
    }
    return m_spreadMethod.value();
}

} // namespace Starfish
