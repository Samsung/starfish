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

#include "core/dom/HTMLTableSectionElement.h"

#include "core/dom/DOMException.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLTableRowElement.h"

namespace StarFish {
HTMLTableSectionElement::HTMLTableSectionElement(Document* document)
    : HTMLTablePartElement(document)
    , m_rows(nullptr)
{
}

void* HTMLTableSectionElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLTableSectionElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLTableSectionElement, m_rows));
        HTMLTablePartElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLTableSectionElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

HTMLCollection* HTMLTableSectionElement::rows()
{
    if (m_rows) {
        return m_rows;
    }

    RareNodeMembers* rareData = ensureRareMembers();
    ActiveHTMLCollectionList* activeLists =
        rareData->ensureActiveHtmlCollectionListForTagName();
    m_rows =
        new HTMLCollection(this, NodeListImpl::TableRowsFilter, nullptr, true);
    rareData->putActiveHtmlCollectionListWithQuery(activeLists,
                                                   this->localName(), m_rows);
    return m_rows;
}

HTMLElement* HTMLTableSectionElement::insertRow(long index)
{
    HTMLCollection* rows = this->rows();
    if (index < -1 ||
        (index != -1 && static_cast<size_t>(index) > rows->length())) {
        throw new DOMException(document(), DOMException::INDEX_SIZE_ERR);
    }

    HTMLTableRowElement* row = new HTMLTableRowElement(document());
    if (index == -1 || static_cast<size_t>(index) == rows->length()) {
        appendChild(row);
    } else {
        insertBefore(row, rows->item(index));
    }
    return row;
}

void HTMLTableSectionElement::deleteRow(long index)
{
    HTMLCollection* rows = this->rows();
    if (index == -1) {
        if (rows->length() == 0) {
            return;
        } else {
            index = rows->length() - 1;
        }
    } else if (index < -1 || static_cast<size_t>(index) >= rows->length()) {
        throw new DOMException(document(), DOMException::INDEX_SIZE_ERR);
    }

    removeChild(rows->item(index));
}

String* HTMLTableSectionElement::ch()
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

void HTMLTableSectionElement::setCh(String* ch)
{
    setAttribute(starFish()->staticStrings()->m_char, ch);
}
}
