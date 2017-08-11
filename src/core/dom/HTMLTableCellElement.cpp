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
#include "core/dom/HTMLTableCellElement.h"
#include "core/dom/HTMLTableElement.h"
#include "core/style/CSSParser.h"

namespace StarFish {

void HTMLTableCellElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    HTMLTableElement* table = tableElement();
    if (table && table->hasCellPaddingAttribute()) {
        String* value = table->cellpadding();
        if (value && !value->equals(String::emptyString)) {
            // Use px as the default unit
            if (!value->contains("px") && !value->contains("%")) {
                value = value->concat(String::createASCIIString("px"));
            }
        }

        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseLengthOrPercent(value->utf8Data(), false,
                                                    &pair)) {
            pair.setKeyKind(CSSStyleValuePair::PaddingTop);
            cssValues.push_back(pair);
            pair.setKeyKind(CSSStyleValuePair::PaddingRight);
            cssValues.push_back(pair);
            pair.setKeyKind(CSSStyleValuePair::PaddingBottom);
            cssValues.push_back(pair);
            pair.setKeyKind(CSSStyleValuePair::PaddingLeft);
            cssValues.push_back(pair);
        }
    }
}

HTMLTableElement* HTMLTableCellElement::tableElement()
{
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (!(p->isHTMLTableElement() || p->isHTMLTableSectionElement() ||
              p->isHTMLTableRowElement())) {
            return nullptr;
        }
        if (p->isHTMLTableElement()) {
            return p->asHTMLTableElement();
        }
    }
    return nullptr;
}

uint32_t HTMLTableCellElement::colSpan()
{
    Nullable<String*> colSpan =
        getAttribute(starFish()->staticStrings()->m_colspan);
    if (colSpan.hasValue()) {
        int colSpanInt = String::parseInt(colSpan.getValue());
        return colSpanInt <= 0 ? 1 : (uint32_t)colSpanInt;
    }
    return 1;
}

void HTMLTableCellElement::setColSpan(uint32_t colSpan)
{
    setAttribute(starFish()->staticStrings()->m_colspan,
                 String::fromInt(colSpan));
}

uint32_t HTMLTableCellElement::rowSpan()
{
    Nullable<String*> rowSpan =
        getAttribute(starFish()->staticStrings()->m_rowspan);
    if (rowSpan.hasValue()) {
        int rowSpanInt = String::parseInt(rowSpan.getValue());
        return rowSpanInt <= 0 ? 1 : (uint32_t)rowSpanInt;
    }
    return 1;
}

void HTMLTableCellElement::setRowSpan(uint32_t rowSpan)
{
    setAttribute(starFish()->staticStrings()->m_rowspan,
                 String::fromInt(rowSpan));
}

String* HTMLTableCellElement::bgColor()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_bgColor);
}

void HTMLTableCellElement::setBgColor(String* bgColor)
{
    setAttribute(starFish()->staticStrings()->m_bgColor, bgColor);
}

String* HTMLTableCellElement::ch()
{
    Nullable<String*> ret = getAttribute(starFish()->staticStrings()->m_char);
    if (ret.hasValue()) {
        return ret.getValue();
    }
    // TODO : Set defualt Value that is the decimal point character for the
    // current language as set by the lang attribute
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return String::createASCIIString(".");
}

void HTMLTableCellElement::setCh(String* ch)
{
    setAttribute(starFish()->staticStrings()->m_char, ch);
}
}
