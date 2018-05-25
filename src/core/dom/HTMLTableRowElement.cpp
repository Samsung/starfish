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

#include "core/dom/DOMException.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLTDElement.h"
#include "core/dom/HTMLTableElement.h"
#include "core/dom/HTMLTableSectionElement.h"

namespace StarFish {
void* HTMLTableRowElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLTableRowElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLTableRowElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLTableRowElement, m_cells));
        HTMLTablePartElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLTableRowElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

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

inline HTMLCollection* findTableSectionRows(const HTMLTableRowElement& row)
{
    auto* parent = row.parentNode();
    if (parent->isHTMLTableSectionElement()) {
        return parent->asHTMLTableSectionElement()->rows();
    } else if (parent->isHTMLTableElement()) {
        return parent->asHTMLTableElement()->rows();
    }
    return nullptr;
}

int32_t HTMLTableRowElement::sectionRowIndex()
{
    HTMLCollection* rows = findTableSectionRows(*this);
    if (!rows) {
        return -1;
    }

    size_t length = rows->length();
    for (size_t i = 0; i < length; i++) {
        if (rows->item(i) == this) {
            return i;
        }
    }
    return -1;
}

HTMLCollection* HTMLTableRowElement::cells()
{
    if (m_cells) {
        return m_cells;
    }

    RareNodeMembers* rareData = ensureRareMembers();
    ActiveHTMLCollectionList* activeLists =
        rareData->ensureActiveHtmlCollectionListForTagName();
    m_cells =
        new HTMLCollection(this, NodeListImpl::TableCellsFilter, nullptr, true);
    rareData->putActiveHtmlCollectionListWithQuery(activeLists,
                                                   this->localName(), m_cells);
    return m_cells;
}

HTMLTableCellElement* HTMLTableRowElement::insertCell(int32_t index)
{
    HTMLCollection* cells = this->cells();
    if (index < -1 ||
        (index != -1 && static_cast<size_t>(index) > cells->length())) {
        throw new DOMException(document(), DOMException::INDEX_SIZE_ERR);
    }

    HTMLTDElement* cell = new HTMLTDElement(document());
    if (index == -1 || static_cast<size_t>(index) == cells->length()) {
        appendChild(cell);
    } else {
        insertBefore(cell, cells->item(index));
    }
    return cell;
}

void HTMLTableRowElement::deleteCell(long index)
{
    HTMLCollection* cells = this->cells();
    if (index == -1) {
        if (cells->length() == 0) {
            return;
        } else {
            index = cells->length() - 1;
        }
    } else if (index < -1 || static_cast<size_t>(index) >= cells->length()) {
        throw new DOMException(document(), DOMException::INDEX_SIZE_ERR);
    }

    removeChild(cells->item(index));
}
}
