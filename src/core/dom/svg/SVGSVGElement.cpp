/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

void* SVGSVGElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGSVGElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGSVGElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGSVGElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName SVGSVGElement::name()
{
    return starFish()->staticStrings()->m_svgsvgTagName;
}

void SVGSVGElement::didAttributeChanged(QualifiedName name, String* old,
                                        String* value, bool attributeCreated,
                                        bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    if (name == starFish()->staticStrings()->m_viewBox) {
        m_hasViewBox = false;
        auto utf8Str = value->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, utf8Str.data(),
                                              utf8Str.length(), ",", 1);
        if (tokens.size() == 4) {
            float x, y, w, h;
            if (CSSPropertyParser::parseNumber(tokens[0].data(), 0, &x) &&
                CSSPropertyParser::parseNumber(tokens[1].data(), 0, &y) &&
                CSSPropertyParser::parseNumber(tokens[2].data(), 0, &w) &&
                CSSPropertyParser::parseNumber(tokens[3].data(), 0, &h)) {
                m_viewBox = Unit::Rect(x, y, w, h);
                m_hasViewBox = true;
            }
        }

        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsLayout();
    } else if (name == starFish()->staticStrings()->m_width) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    } else if (name == starFish()->staticStrings()->m_height) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    }
}

void SVGSVGElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);

    if (hasAttribute(starFish()->staticStrings()->m_width) == SIZE_MAX &&
        hasAttribute(starFish()->staticStrings()->m_height) == SIZE_MAX) {
        if (m_hasViewBox) {
            float w, h;
            if (m_viewBox.width() / m_viewBox.height() > 1) {
                w = 1;
                h = m_viewBox.height() / m_viewBox.width();
            } else {
                w = m_viewBox.width() / m_viewBox.height();
                h = 1;
            }

            CSSStyleValuePair pair;
            pair.setKeyKind(CSSStyleValuePair::Width);
            pair.setValueKind(CSSStyleValuePair::Percentage);
            pair.setPercentageValue(w);
            cssValues.push_back(pair);
            pair.setKeyKind(CSSStyleValuePair::Height);
            pair.setPercentageValue(h);
            cssValues.push_back(pair);
        }
    }
}
}
