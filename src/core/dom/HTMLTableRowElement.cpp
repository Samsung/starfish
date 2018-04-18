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
#include "core/dom/HTMLTableRowElement.h"
#include "core/dom/HTMLTableElement.h"
#include "HTMLCollection.h"

namespace StarFish {

QualifiedName HTMLTableRowElement::name()
{
    return starFish()->staticStrings()->m_trTagName;
}

void HTMLTableRowElement::didAttributeChanged(QualifiedName name, String* old,
                                              String* value,
                                              bool attributeCreated,
                                              bool attributeRemoved)
{
    HTMLTablePartElement::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);
    if (name == starFish()->staticStrings()->m_bgcolor) {
        setNeedsStyleRecalc();
    }
}

void HTMLTableRowElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLTablePartElement::styleForPresentationAttribute(cssValues);

    String* bgColor =
        getAttributeOrEmpty(starFish()->staticStrings()->m_bgcolor);
    if (!bgColor->equals(String::emptyString)) {
        CSSStyleValuePair pair;
        CSSTokenValue token = bgColor->toNullableUTF8String().m_buffer;
        if (pair.updateValueUnitColor(token)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundColor);
            cssValues.push_back(pair);
        }
    }
}

String* HTMLTableRowElement::ch()
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

void HTMLTableRowElement::setCh(String* ch)
{
    setAttribute(starFish()->staticStrings()->m_char, ch);
}

inline HTMLTableElement* findTable(const HTMLTableRowElement& row)
{
    auto* parent = row.parentNode();
    if (parent->isHTMLTableElement()) {
        return parent->asHTMLTableElement();
    }
    if (parent->isHTMLTableSectionElement()) {
        auto* grandparent = parent->parentNode();
        if (grandparent->isHTMLTableElement()) {
            return grandparent->asHTMLTableElement();
        }
    }
    return nullptr;
}

int32_t HTMLTableRowElement::rowIndex()
{
    HTMLTableElement* table = findTable(*this);
    if (!table) {
        return -1;
    }

    HTMLCollection* rows = table->rows();
    size_t length = rows->length();
    for (size_t i = 0; i < length; i++) {
        if (rows->item(i) == this) {
            return i;
        }
    }
    return -1;
}
}
