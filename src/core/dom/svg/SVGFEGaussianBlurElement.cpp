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
#include "core/dom/svg/SVGFEGaussianBlurElement.h"
#include "core/dom/DOMTokenList.h"

namespace Starfish {
SVGFEGaussianBlurElement::SVGFEGaussianBlurElement(Document* document,
                                                   const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEGaussianBlurElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEGaussianBlurElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEGaussianBlurElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_in));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_stdDeviationX));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_stdDeviationY));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_edgeMode));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFEGaussianBlurElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEGaussianBlurElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGFilterPrimitiveStandardAttributes::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        notifyAttributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_stdDeviation == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_stdDeviationX == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_stdDeviationY == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    } else if (ss->m_edgeMode == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
    }
}

void SVGFEGaussianBlurElement::didAttributeChanged(QualifiedName name,
                                                   Optional<String*> old,
                                                   String* value,
                                                   bool attributeCreated,
                                                   bool attributeRemoved)
{
    SVGFilterPrimitiveStandardAttributes::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_in == name) {
        in()->setBaseVal(value, true);
    } else if (ss->m_stdDeviation == name) {
        GCVector<StringView> tokens;
        DOMTokenList::tokenize(value, tokens);
        if (tokens.size() == 1) {
            stdDeviationX()->setBaseVal(String::parseFloat(&tokens[0]), true);
            stdDeviationY()->setBaseVal(String::parseFloat(&tokens[0]), true);
        } else if (tokens.size() > 1) {
            stdDeviationX()->setBaseVal(String::parseFloat(&tokens[0]), true);
            stdDeviationY()->setBaseVal(String::parseFloat(&tokens[1]), true);
        }
    } else if (ss->m_stdDeviationX == name) {
        stdDeviationX()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_stdDeviationY == name) {
        stdDeviationY()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_edgeMode == name) {
        if (edgeMode()->isUpdated() == false) {
            if (value->equals("duplicate")) {
                m_edgeMode->setBaseValWithoutUpdateAttribute(
                    EdgeMode::SVG_EDGEMODE_DUPLICATE);
            } else if (value->equals("wrap")) {
                m_edgeMode->setBaseValWithoutUpdateAttribute(
                    EdgeMode::SVG_EDGEMODE_WRAP);
            } else if (value->equals("mirror")) {
                m_edgeMode->setBaseValWithoutUpdateAttribute(
                    EdgeMode::SVG_EDGEMODE_MIRROR);
            } else if (value->equals("none")) {
                m_edgeMode->setBaseValWithoutUpdateAttribute(
                    EdgeMode::SVG_EDGEMODE_NONE);
            } else {
                m_edgeMode->setBaseValWithoutUpdateAttribute(
                    EdgeMode::SVG_EDGEMODE_UNKNOWN);
            }
        }
    }
}

void SVGFEGaussianBlurElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in == name) {
        setAttribute(ss->m_in, in()->baseVal());
    } else if (ss->m_stdDeviation == name) {
        setAttribute(ss->m_stdDeviationX,
                     String::fromFloat(stdDeviationX()->baseVal()));
        setAttribute(ss->m_stdDeviationY,
                     String::fromFloat(stdDeviationY()->baseVal()));
    } else if (ss->m_stdDeviationX == name) {
        setAttribute(ss->m_stdDeviationX,
                     String::fromFloat(stdDeviationX()->baseVal()));
    } else if (ss->m_stdDeviationY == name) {
        setAttribute(ss->m_stdDeviationY,
                     String::fromFloat(stdDeviationY()->baseVal()));
    } else if (ss->m_edgeMode == name) {
        m_edgeMode->updateAttribute();
    }
}

void SVGFEGaussianBlurElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGFilterPrimitiveStandardAttributes::styleForPresentationAttribute(
        cssValues, matchedRules, cssCustomValues);
}

SVGAnimatedString* SVGFEGaussianBlurElement::in()
{
    if (!m_in.hasValue()) {
        m_in = new SVGAnimatedString(this, starfish()->staticStrings()->m_in,
                                     String::emptyString, String::emptyString);
    }
    return m_in.getValue();
}

SVGAnimatedNumber* SVGFEGaussianBlurElement::stdDeviationX()
{
    if (!m_stdDeviationX.hasValue()) {
        m_stdDeviationX = new SVGAnimatedNumberWithFallbackAttribute(
            this, starfish()->staticStrings()->m_stdDeviationX,
            starfish()->staticStrings()->m_stdDeviation, 0);
    }
    return m_stdDeviationX.getValue();
}

SVGAnimatedNumber* SVGFEGaussianBlurElement::stdDeviationY()
{
    if (!m_stdDeviationY.hasValue()) {
        m_stdDeviationY = new SVGAnimatedNumberWithFallbackAttribute(
            this, starfish()->staticStrings()->m_stdDeviationY,
            starfish()->staticStrings()->m_stdDeviation, 0);
    }
    return m_stdDeviationY.getValue();
}

SVGAnimatedEnumeration* SVGFEGaussianBlurElement::edgeMode()
{
    if (!m_edgeMode.hasValue()) {
        m_edgeMode = new SVGAnimatedEnumeration(
            this, staticStrings()->m_edgeMode, EdgeMode::SVG_EDGEMODE_NONE,
            EdgeMode::SVG_EDGEMODE_NONE, EdgeMode::SVG_EDGEMODE_MIRROR);
    }
    return m_edgeMode.getValue();
}

void SVGFEGaussianBlurElement::setStdDeviation(float stdDeviationX,
                                               float stdDeviationY)
{
    this->stdDeviationX()->setBaseVal(stdDeviationX);
    this->stdDeviationY()->setBaseVal(stdDeviationY);
}
} // namespace Starfish
