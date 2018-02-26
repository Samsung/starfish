/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/svg/SVGTextElement.h"

namespace StarFish {

QualifiedName SVGTextElement::name()
{
    return starFish()->staticStrings()->m_svgtextTagName;
}

void SVGTextElement::didAttributeChanged(QualifiedName name, String* old,
                                         String* value, bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();

    if (ss->m_fontDashSize == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    } else if (ss->m_fontDashFamily == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    }
}

void SVGTextElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);

    StaticStrings* ss = starFish()->staticStrings();
    {
        auto attr = getAttribute(ss->m_fontDashFamily);
        if (attr.hasValue()) {
            CSSStyleDeclaration decl;
            auto str = attr.getValue()->toUTF8NonGCString();
            decl.setFontFamily(str.data(), str.length(), false);
            if (decl.cssValues().size()) {
                cssValues.push_back(decl.cssValues()[0]);
            }
        }
    }
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(fontDashSize, FontSize);
}
}
