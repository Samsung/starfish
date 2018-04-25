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
#include "core/dom/HTMLTableCellElement.h"

#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLTableElement.h"
#include "core/dom/HTMLTableRowElement.h"
#include "core/style/CSSParser.h"

namespace StarFish {

void HTMLTableCellElement::didAttributeChanged(QualifiedName name, String* old,
                                               String* value,
                                               bool attributeCreated,
                                               bool attributeRemoved)
{
    HTMLTablePartElement::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);
    if (name == starFish()->staticStrings()->m_nowrap ||
        name == starFish()->staticStrings()->m_height ||
        name == starFish()->staticStrings()->m_width ||
        name == starFish()->staticStrings()->m_bgcolor) {
        setNeedsStyleRecalc();
    }
}

void HTMLTableCellElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLTablePartElement::styleForPresentationAttribute(cssValues);

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
        auto s = value->toUTF8NonGCString();
        if (CSSPropertyParser::parseLength(
                s.data(), CSSPropertyParser::AllowPercent, &pair)) {
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

    String* nowrap = getAttributeOrEmpty(starFish()->staticStrings()->m_nowrap);
    if (!nowrap->isEmpty()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::WhiteSpace);
        pair.setValueKind(CSSStyleValuePair::ValueKind::WhiteSpaceValueKind);
        pair.setValue(WhiteSpaceValue::NoWrapWhiteSpaceValue);
    }

    String* h = getAttributeOrEmpty(starFish()->staticStrings()->m_height);
    if (!h->isEmpty()) {
        // Use px as the default unit
        if (!h->contains("px") && !h->contains("%")) {
            h = h->concat(String::createASCIIString("px"));
        }

        CSSStyleValuePair pair;
        CSSTokenVector tokens;
        CSSTokenValue token = h->toNullableUTF8String().m_buffer;
        tokens.push_back(token);
        if (pair.updateValueHeight(document(), tokens)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Height);
            cssValues.push_back(pair);
        }
    }

    String* w = getAttributeOrEmpty(starFish()->staticStrings()->m_width);
    if (!w->equals(String::emptyString)) {
        // Use px as the default unit
        if (!w->contains("px") && !w->contains("%")) {
            w = w->concat(String::createASCIIString("px"));
        }

        CSSStyleValuePair pair;
        CSSTokenVector tokens;
        CSSTokenValue token = w->toNullableUTF8String().m_buffer;
        tokens.push_back(token);
        if (pair.updateValueWidth(document(), tokens)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Width);
            cssValues.push_back(pair);
        }
    }

    String* bgColor =
        getAttributeOrEmpty(starFish()->staticStrings()->m_bgcolor);
    if (!bgColor->isEmpty()) {
        CSSStyleValuePair pair;
        CSSTokenValue token = bgColor->toNullableUTF8String().m_buffer;
        if (pair.updateValueUnitColor(token)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundColor);
            cssValues.push_back(pair);
        }
    }

    if (HTMLTableElement* tableElement = findParentTable()) {
        HTMLTableElement::CellBorders borders = tableElement->cellBorders();
        switch (borders) {
        case HTMLTableElement::NoBorders:
            break;
        case HTMLTableElement::InsetBorders:
            additionalBorderRulesTop(cssValues, "1px", "inset");
            additionalBorderRulesRight(cssValues, "1px", "inset");
            additionalBorderRulesBottom(cssValues, "1px", "inset");
            additionalBorderRulesLeft(cssValues, "1px", "inset");
            break;
        case HTMLTableElement::SolidBordersRowsOnly:
            additionalBorderRulesTop(cssValues, "thin", "solid");
            additionalBorderRulesBottom(cssValues, "thin", "solid");
            break;
        case HTMLTableElement::SolidBordersColsOnly:
            additionalBorderRulesRight(cssValues, "thin", "solid");
            additionalBorderRulesLeft(cssValues, "thin", "solid");
            break;
        case HTMLTableElement::SolidBorders:
            additionalBorderRulesTop(cssValues, "1px", "solid");
            additionalBorderRulesRight(cssValues, "1px", "solid");
            additionalBorderRulesBottom(cssValues, "1px", "solid");
            additionalBorderRulesLeft(cssValues, "1px", "solid");
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        String* padding = tableElement->cellpadding();
        if (!padding->isEmpty()) {
            additionalPadding(cssValues, padding);
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
        int colSpanVal = String::parseInt(colSpan.getValue());
        if (colSpanVal < 1) {
            return 1;
        }
        if (colSpanVal > MAX_COLSPAN) {
            return MAX_COLSPAN;
        }
        return colSpanVal;
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
        int rowSpanVal = String::parseInt(rowSpan.getValue());
        if (rowSpanVal < 1) {
            return 1;
        }
        if (rowSpanVal > MAX_ROWSPAN) {
            return MAX_ROWSPAN;
        }
        return rowSpanVal;
    }

    return 1;
}

void HTMLTableCellElement::setRowSpan(uint32_t rowSpan)
{
    setAttribute(starFish()->staticStrings()->m_rowspan,
                 String::fromInt(rowSpan));
}

int32_t HTMLTableCellElement::cellIndex()
{
    Node* parent = parentNode();
    if (parent->isHTMLTableRowElement()) {
        HTMLTableRowElement* row = parent->asHTMLTableRowElement();
        HTMLCollection* cells = row->cells();
        size_t length = cells->length();
        for (size_t i = 0; i < length; i++) {
            if (cells->item(i) == this) {
                return i;
            }
        }
    }
    return -1;
}

String* HTMLTableCellElement::scope()
{
    String* scope = getAttributeOrEmpty(starFish()->staticStrings()->m_scope);
    if (scope->isEmpty() || scope->equalsIgnoreCase("col") ||
        scope->equalsIgnoreCase("colgroup") || scope->equalsIgnoreCase("row") ||
        scope->equalsIgnoreCase("rowgroup")) {
        return scope;
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void HTMLTableCellElement::setScope(String* scope)
{
    setAttribute(starFish()->staticStrings()->m_scope, scope);
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
