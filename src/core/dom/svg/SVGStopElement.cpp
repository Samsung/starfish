/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#include "core/dom/svg/SVGStopElement.h"
#include "core/dom/svg/SVGGradientElement.h"
#include "core/style/GradientData.h"
#include "core/style/CSSParser.h"

namespace Starfish {

void SVGStopElement::didAttributeChanged(QualifiedName name,
                                         Optional<String*> old, String* value,
                                         bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();
    if (name == ss->m_stopColor || name == ss->m_stopOpacity ||
        name == ss->m_offset) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        auto p = parentElement();
        while (p && !p->isSVGGradientElement()) {
            p = p->parentElement();
        }
        if (p) {
            p->asSVGGradientElement()->paintingAttributesUpdated();
        }
    }
}

ColorStop* SVGStopElement::colorStop()
{
    ColorStop* colorStop = new ColorStop();

    // https://svgwg.org/svg2-draft/pservers.html#GradientStopAttributes
    String* offsetStr =
        getAttributeOrEmpty(starfish()->staticStrings()->m_offset);

    Length offset(Length::Type::Percent, 0);
    if (offsetStr->length() > 0) {
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseNumberOrPercentage(
                offsetStr->toUTF8NonGCString().data(), 0, &pair)) {
            float offsetVal = 0;
            Length::Type offsetType = Length::Type::Auto;
            if (pair.valueKind() == CSSStyleValuePair::ValueKind::Number) {
                offsetType = Length::Type::Percent;
                offsetVal = pair.numberValue();
            } else if (pair.valueKind() ==
                       CSSStyleValuePair::ValueKind::Percentage) {
                offsetType = Length::Type::Percent;
                offsetVal = pair.percentageValue();
            }
            offsetVal = offsetVal < 0 ? 0 : (offsetVal > 1 ? 1 : offsetVal);
            offset = Length(offsetType, offsetVal);
        }
    }
    colorStop->setOffset(offset);

    Unit::Color color = style()->stopColor()->color();
    color.m_a = style()->stopOpacity() * 0xff;
    colorStop->setColor(color);

    return colorStop;
}

void SVGStopElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);

    String* stopColor = getAttributeOrVarReferencedValue(
        starfish()->staticStrings()->m_stopColor, cssCustomValues);
    if (stopColor->length()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::StopColor);
        CSSTokenVector tokens;
        auto str = stopColor->toUTF8NonGCString();
        CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(), str.length());
        if (pair.updateValueStopColor(document(), tokens)) {
            cssValues.push_back(pair);
        }
    }

    String* stopOpacity = getAttributeOrVarReferencedValue(
        starfish()->staticStrings()->m_stopOpacity, cssCustomValues);
    if (stopOpacity->length()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::StopOpacity);
        CSSTokenVector tokens;
        auto str = stopOpacity->toUTF8NonGCString();
        CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(), str.length());
        if (pair.updateValueStopOpacity(document(), tokens)) {
            cssValues.push_back(pair);
        }
    }
}
} // namespace Starfish
