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
#include "core/dom/svg/SVGPathElement.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

QualifiedName SVGPathElement::name()
{
    return starFish()->staticStrings()->m_svgpathTagName;
}

void SVGPathElement::didAttributeChanged(QualifiedName name, String* old,
                                         String* value, bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();

    if (ss->m_d == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    }
}

void SVGPathElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);

    String* d = getAttributeOrEmpty(starFish()->staticStrings()->m_d);
    CSSStyleDeclaration decl;
    auto buf = d->toUTF8NonGCString();
    decl.setD(buf.data(), buf.length(), false);
    if (decl.hasCSSValuePair(CSSStyleValuePair::KeyKind::D)) {
        cssValues.push_back(decl.cssValues()[0]);
    } else if (d->length()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::D);
        pair.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        pair.setStringValue(d);
        cssValues.push_back(pair);
    }
}
}
