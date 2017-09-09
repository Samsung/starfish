/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
            if (CSSPropertyParser::parseNumber(tokens[0].data(), false, &x) &&
                CSSPropertyParser::parseNumber(tokens[1].data(), false, &y) &&
                CSSPropertyParser::parseNumber(tokens[2].data(), false, &w) &&
                CSSPropertyParser::parseNumber(tokens[3].data(), false, &h)) {
                m_viewBox = Unit::Rect(x, y, w, h);
                m_hasViewBox = true;
            }
        }
    }
}

void SVGSVGElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);

    if (hasAttribute(starFish()->staticStrings()->m_width) == SIZE_MAX &&
        hasAttribute(starFish()->staticStrings()->m_height) == SIZE_MAX) {
        if (m_hasViewBox) {
            CSSStyleValuePair pair;
            pair.setKeyKind(CSSStyleValuePair::Width);
            pair.setValueKind(CSSStyleValuePair::Percentage);
            pair.setPercentageValue(m_viewBox.width() / m_viewBox.height());
            cssValues.push_back(pair);
            pair.setKeyKind(CSSStyleValuePair::Height);
            pair.setPercentageValue(m_viewBox.height() / m_viewBox.width());
            cssValues.push_back(pair);
        }
    }
}
}
