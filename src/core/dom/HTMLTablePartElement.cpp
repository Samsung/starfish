/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/HTMLTablePartElement.h"
#include "core/style/Style.h"

namespace StarFish {
void HTMLTablePartElement::didAttributeChanged(QualifiedName name, String* old,
                                               String* value,
                                               bool attributeCreated,
                                               bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_align ||
        name == starFish()->staticStrings()->m_valign) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    }
}

void HTMLTablePartElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* valign = getAttributeOrEmpty(starFish()->staticStrings()->m_valign);
    if (isValidValign(valign)) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::VerticalAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::VerticalAlignValueKind);
        pair.setValue(valignValue(valign));
        cssValues.push_back(pair);
    }

    String* align = getAttributeOrEmpty(starFish()->staticStrings()->m_align);
    if (isValidAlign(align)) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::TextAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::TextAlignValueKind);
        pair.setValue(alignValue(align));
        cssValues.push_back(pair);
    }
}

bool HTMLTablePartElement::isValidAlign(String* align)
{
    if (align->isEmpty()) {
        return false;
    }

    if (align->equalsIgnoreCase("left") || align->equalsIgnoreCase("right") ||
        align->equalsIgnoreCase("middle") ||
        align->equalsIgnoreCase("center") ||
        align->equalsIgnoreCase("-starfish-center")) {
        return true;
    }
    return false;
}

TextAlignValue HTMLTablePartElement::alignValue(String* align)
{
    STARFISH_ASSERT(!align->isEmpty());
    if (align->equalsIgnoreCase("left")) {
        return TextAlignValue::LeftTextAlignValue;
    } else if (align->equalsIgnoreCase("right")) {
        return TextAlignValue::RightTextAlignValue;
    } else if (align->equalsIgnoreCase("middle") ||
               align->equalsIgnoreCase("center")) {
        return TextAlignValue::CenterTextAlignValue;
    } else if (align->equalsIgnoreCase("-starfish-center")) {
        return TextAlignValue::StarFishCenterTextAlignValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

bool HTMLTablePartElement::isValidValign(String* valign)
{
    if (valign->isEmpty()) {
        return false;
    }

    if (valign->equalsIgnoreCase("baseline") ||
        valign->equalsIgnoreCase("top") || valign->equalsIgnoreCase("middle") ||
        valign->equalsIgnoreCase("bottom")) {
        return true;
    }
    return false;
}

VerticalAlignValue HTMLTablePartElement::valignValue(String* valign)
{
    STARFISH_ASSERT(!valign->isEmpty());
    if (valign->equalsIgnoreCase("baseline")) {
        return VerticalAlignValue::BaselineVAlignValue;
    } else if (valign->equalsIgnoreCase("top")) {
        return VerticalAlignValue::TopVAlignValue;
    } else if (valign->equalsIgnoreCase("middle")) {
        return VerticalAlignValue::MiddleVAlignValue;
    } else if (valign->equalsIgnoreCase("bottom")) {
        return VerticalAlignValue::BottomVAlignValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}
}
