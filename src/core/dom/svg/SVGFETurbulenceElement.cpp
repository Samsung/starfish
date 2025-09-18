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
#include "core/dom/svg/SVGFETurbulenceElement.h"
#include "core/dom/svg/SVGAnimatedInteger.h"
#include "core/dom/DOMTokenList.h"

namespace Starfish {
SVGFETurbulenceElement::SVGFETurbulenceElement(Document* document,
                                               const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFETurbulenceElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFETurbulenceElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFETurbulenceElement)] = { 0 };
        SVGFilterPrimitiveStandardAttributes::fillGCDescriptor(desc);
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFETurbulenceElement, m_baseFrequencyX));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFETurbulenceElement, m_baseFrequencyY));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFETurbulenceElement, m_numOctaves));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFETurbulenceElement, m_seed));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFETurbulenceElement, m_stitchTiles));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFETurbulenceElement, m_type));

        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFETurbulenceElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFETurbulenceElement::didAttributeChanged(QualifiedName name,
                                                 Optional<String*> old,
                                                 String* value,
                                                 bool attributeCreated,
                                                 bool attributeRemoved)
{
    SVGFilterPrimitiveStandardAttributes::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_baseFrequency == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);

        if (value->containsWhitespace()) {
            auto s = StringUtils::split(
                value->stripAndCollapseASCIIwhitespace()->toUTF8NonGCString(),
                ' ');
            try {
                if (s.size() == 1) {
                    float f = std::stof(s[0]);
                    baseFrequencyX()->setBaseVal(f, true);
                    baseFrequencyY()->setBaseVal(f, true);
                } else if (s.size() > 1) {
                    baseFrequencyX()->setBaseVal(std::stof(s[0]), true);
                    baseFrequencyY()->setBaseVal(std::stof(s[1]), true);
                }
            } catch (...) {
                // stof throws when argument is invalid
                baseFrequencyX()->setBaseVal(0, true);
                baseFrequencyY()->setBaseVal(0, true);
            }
        } else {
            float f = String::parseFloat(value);
            baseFrequencyX()->setBaseVal(f, true);
            baseFrequencyY()->setBaseVal(f, true);
        }
    } else if (ss->m_baseFrequencyX == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        baseFrequencyX()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_baseFrequencyY == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        baseFrequencyY()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_seed == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        seed()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_numOctaves == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        numOctaves()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_type == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        if (type()->isUpdated() == false) {
            if (value->equals("fractalNoise")) {
                type()->setBaseValWithoutUpdateAttribute(
                    SVGFETurbulenceElement::TurbulenceType::
                        SVG_TURBULENCE_TYPE_FRACTALNOISE);
            } else if (value->equals("turbulence")) {
                type()->setBaseValWithoutUpdateAttribute(
                    SVGFETurbulenceElement::TurbulenceType::
                        SVG_TURBULENCE_TYPE_TURBULENCE);
            } else {
                type()->setBaseValWithoutUpdateAttribute(
                    SVGFETurbulenceElement::TurbulenceType::
                        SVG_TURBULENCE_TYPE_UNKNOWN);
            }
        }
    } else if (ss->m_stitchTiles == name) {
        notifyAttributeOfPaintServerLikeUpdated(true);
        if (stitchTiles()->isUpdated() == false) {
            if (value->equals("nostitch")) {
                stitchTiles()->setBaseValWithoutUpdateAttribute(
                    SVGFETurbulenceElement::StitchType::
                        SVG_STITCHTYPE_NOSTITCH);
            } else if (value->equals("stitch")) {
                stitchTiles()->setBaseValWithoutUpdateAttribute(
                    SVGFETurbulenceElement::StitchType::SVG_STITCHTYPE_STITCH);
            } else {
                stitchTiles()->setBaseValWithoutUpdateAttribute(
                    SVGFETurbulenceElement::StitchType::SVG_STITCHTYPE_UNKNOWN);
            }
        }
    }
}

void SVGFETurbulenceElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_baseFrequencyX == name) {
        setAttribute(ss->m_baseFrequencyX,
                     String::fromFloat(baseFrequencyX()->baseVal()));
    } else if (ss->m_baseFrequencyY == name) {
        setAttribute(ss->m_baseFrequencyY,
                     String::fromFloat(baseFrequencyY()->baseVal()));
    } else if (ss->m_seed == name) {
        setAttribute(ss->m_seed, String::fromFloat(seed()->baseVal()));
    } else if (ss->m_numOctaves == name) {
        setAttribute(ss->m_numOctaves,
                     String::fromFloat(numOctaves()->baseVal()));
    } else if (ss->m_type == name) {
        m_type->updateAttribute();
    } else if (ss->m_stitchTiles == name) {
        m_stitchTiles->updateAttribute();
    }
}

void SVGFETurbulenceElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGFilterPrimitiveStandardAttributes::styleForPresentationAttribute(
        cssValues, cssCustomValues);
}

SVGAnimatedNumber* SVGFETurbulenceElement::baseFrequencyX()
{
    if (!m_baseFrequencyX.hasValue()) {
        m_baseFrequencyX = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_baseFrequencyX, 0, 0);
    }
    return m_baseFrequencyX.getValue();
}

SVGAnimatedNumber* SVGFETurbulenceElement::baseFrequencyY()
{
    if (!m_baseFrequencyY.hasValue()) {
        m_baseFrequencyY = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_baseFrequencyY, 0, 0);
    }
    return m_baseFrequencyY.getValue();
}

SVGAnimatedInteger* SVGFETurbulenceElement::numOctaves()
{
    if (!m_numOctaves.hasValue()) {
        m_numOctaves = new SVGAnimatedInteger(
            this, starfish()->staticStrings()->m_numOctaves, 0, 0);
    }
    return m_numOctaves.getValue();
}

SVGAnimatedNumber* SVGFETurbulenceElement::seed()
{
    if (!m_seed.hasValue()) {
        m_seed = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_seed, 0, 0);
    }
    return m_seed.getValue();
}

SVGAnimatedEnumeration* SVGFETurbulenceElement::stitchTiles()
{
    if (!m_stitchTiles.hasValue()) {
        m_stitchTiles =
            new SVGAnimatedEnumeration(this, staticStrings()->m_stitchTiles,
                                       StitchType::SVG_STITCHTYPE_NOSTITCH,
                                       StitchType::SVG_STITCHTYPE_NOSTITCH,
                                       StitchType::SVG_STITCHTYPE_NOSTITCH);
    }
    return m_stitchTiles.getValue();
}

SVGAnimatedEnumeration* SVGFETurbulenceElement::type()
{
    if (!m_type.hasValue()) {
        m_type = new SVGAnimatedEnumeration(
            this, staticStrings()->m_type,
            TurbulenceType::SVG_TURBULENCE_TYPE_TURBULENCE,
            TurbulenceType::SVG_TURBULENCE_TYPE_TURBULENCE,
            TurbulenceType::SVG_TURBULENCE_TYPE_TURBULENCE);
    }
    return m_type.getValue();
}
} // namespace Starfish
