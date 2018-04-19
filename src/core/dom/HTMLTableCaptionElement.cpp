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
#include "core/dom/HTMLTableCaptionElement.h"
#include "core/style/Style.h"

namespace StarFish {
void HTMLTableCaptionElement::didAttributeChanged(QualifiedName name,
                                                  String* old, String* value,
                                                  bool attributeCreated,
                                                  bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_align) {
        setNeedsStyleRecalc();
    }
}

void HTMLTableCaptionElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* align = getAttributeOrEmpty(starFish()->staticStrings()->m_align);
    if (!align->equals(String::emptyString)) {
        if (align->equalsIgnoreCase("top")) {
            CSSStyleValuePair pair;
            pair.setKeyKind(CSSStyleValuePair::KeyKind::CaptionSide);
            pair.setValueKind(
                CSSStyleValuePair::ValueKind::CaptionSideValueKind);
            pair.setValue(CaptionSideValue::TopCaptionSideValue);
            cssValues.push_back(pair);
        } else if (align->equalsIgnoreCase("bottom")) {
            CSSStyleValuePair pair;
            pair.setKeyKind(CSSStyleValuePair::KeyKind::CaptionSide);
            pair.setValueKind(
                CSSStyleValuePair::ValueKind::CaptionSideValueKind);
            pair.setValue(CaptionSideValue::BottomCaptionSideValue);
            cssValues.push_back(pair);
        }
    }
}

QualifiedName HTMLTableCaptionElement::name()
{
    return starFish()->staticStrings()->m_captionTagName;
}
}
