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
#include "core/dom/HTMLFontElement.h"

namespace StarFish {

QualifiedName HTMLFontElement::name()
{
    return starFish()->staticStrings()->m_fontTagName;
}

void HTMLFontElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_color) {
        if (attributeCreated) {
            m_hasColorAttribute = true;
        }
        if (attributeRemoved) {
            m_hasColorAttribute = false;
        }
        if (!old->equals(value)) {
            setAttribute(starFish()->staticStrings()->m_color, value);
            setNeedsStyleRecalc();
        }
    }
}

void HTMLFontElement::styleForPresentationAttribute(
    GCVector<CSSStyleValuePair>& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    if (m_hasColorAttribute) {
        CSSStyleValuePair pair;
        String* c = getAttributeOrEmpty(starFish()->staticStrings()->m_color);
        auto utf8Str = c->toNullableUTF8String();
        // TODO: Some obsolete legacy attributes parse colors in a more
        // complicated manner, using the rules for parsing a legacy color value.
        // https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#rules-for-parsing-a-legacy-colour-value
        if (pair.updateValueUnitColor(utf8Str.m_buffer)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Color);
            cssValues.push_back(pair);
        }
    }
}

String* HTMLFontElement::color()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_color);
}

void HTMLFontElement::setColor(String* color)
{
    setAttribute(starFish()->staticStrings()->m_color, color);
}
}
