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

namespace StarFish {

QualifiedName SVGSVGElement::name()
{
    return starFish()->staticStrings()->m_svgsvgTagName;
}

void SVGSVGElement::didAttributeChanged(QualifiedName name, String* old,
                                        String* value, bool attributeCreated,
                                        bool attributeRemoved)
{
    if (name == starFish()->staticStrings()->m_width) {
        setNeedsStyleRecalc();
    } else if (name == starFish()->staticStrings()->m_height) {
        setNeedsStyleRecalc();
    }
}

void SVGSVGElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    Element::styleForPresentationAttribute(cssValues);
    CSSStyleValuePair pair;

    String* width = getAttributeOrEmpty(starFish()->staticStrings()->m_width);
    if (width->length()) {
        pair.setKeyKind(CSSStyleValuePair::Width);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Length);
        auto s = width->toUTF8NonGCString();
        CSSPropertyParser::parseLength(s.data(), false, true, &pair);
        cssValues.push_back(pair);
    }

    String* height = getAttributeOrEmpty(starFish()->staticStrings()->m_height);
    if (height->length()) {
        pair.setKeyKind(CSSStyleValuePair::Height);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Length);
        auto s = height->toUTF8NonGCString();
        CSSPropertyParser::parseLength(s.data(), false, true, &pair);
        cssValues.push_back(pair);
    }
}
}
