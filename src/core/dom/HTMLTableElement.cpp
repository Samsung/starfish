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
#include "core/dom/HTMLTableElement.h"

#include "core/dom/DOMException.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLTBodyElement.h"
#include "core/dom/HTMLTFootElement.h"
#include "core/dom/HTMLTHeadElement.h"
#include "core/dom/HTMLTableColGroupElement.h"
#include "core/dom/HTMLTableRowElement.h"
#include "core/dom/HTMLTableSectionElement.h"
#include "core/style/CSSParser.h"

namespace StarFish {
void* HTMLTableElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLTableElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLTableElement, m_rows));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLTableElement, m_tBodies));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLTableElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLTableElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_cellpadding) {
        if (attributeCreated) {
            m_hasCellPaddingAttribute = true;
        }
        if (attributeRemoved) {
            m_hasCellPaddingAttribute = false;
        }
        setNeedsStyleRecalc();
    } else if (name == starFish()->staticStrings()->m_cellspacing) {
        if (attributeCreated) {
            m_hasCellSpacingAttribute = true;
        }
        if (attributeRemoved) {
            m_hasCellSpacingAttribute = false;
        }
        setNeedsStyleRecalc();
    } else if (name == starFish()->staticStrings()->m_width ||
               name == starFish()->staticStrings()->m_bgcolor ||
               name == starFish()->staticStrings()->m_align) {
        setNeedsStyleRecalc();
    }
}

void HTMLTableElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* w = getAttributeOrEmpty(starFish()->staticStrings()->m_width);
    if (!w->isEmpty()) {
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

    String* align = getAttributeOrEmpty(starFish()->staticStrings()->m_align);
    if (isValidAlign(align)) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::TextAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::TextAlignValueKind);
        pair.setValue(alignValue(align));
        cssValues.push_back(pair);
    }

    if (m_hasCellSpacingAttribute) {
        String* value = cellspacing();
        if (value && !value->equals(String::emptyString)) {
            // Use px as the default unit
            if (!value->contains("px") && !value->contains("%")) {
                value = value->concat(String::createASCIIString("px"));
            }
        }
        CSSStyleValuePair pair;
        CSSTokenVector tokens;
        CSSTokenValue token = value->toNullableUTF8String().m_buffer;
        tokens.push_back(token);
        if (pair.updateValueBorderSpacing(document(), tokens)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BorderSpacing);
            cssValues.push_back(pair);
        }
    }
}

QualifiedName HTMLTableElement::name()
{
    return starFish()->staticStrings()->m_tableTagName;
}

HTMLTableCaptionElement* HTMLTableElement::caption()
{
    Node* child = firstChild();
    while (child) {
        if (child->isHTMLTableCaptionElement()) {
            return child->asHTMLTableCaptionElement();
        }
        child = child->nextSibling();
    }
    return nullptr;
}

void HTMLTableElement::setCaption(HTMLTableCaptionElement* caption)
{
    if (caption && !caption->isHTMLTableCaptionElement()) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
                               "Failed to set the 'caption' property on "
                               "'HTMLTableElement': The provided value is not "
                               "of type 'HTMLTableCaptionElement'.");
    }

    deleteCaption();
    if (!caption) {
        insertBefore(caption, firstChild());
    }
}

HTMLTableCaptionElement* HTMLTableElement::createCaption()
{
    HTMLTableCaptionElement* caption = this->caption();
    if (!caption) {
        caption = new HTMLTableCaptionElement(document());
        insertBefore(caption, firstChild());
    }
    return caption;
}

void HTMLTableElement::deleteCaption()
{
    Node* child = firstChild();
    while (child) {
        if (child->isHTMLTableCaptionElement()) {
            removeChild(child);
            return;
        }
        child = child->nextSibling();
    }
}

HTMLTableSectionElement* HTMLTableElement::tHead()
{
    Node* child = firstChild();
    while (child) {
        if (child->isHTMLTHeadElement()) {
            return child->asHTMLTHeadElement();
        }
        child = child->nextSibling();
    }
    return nullptr;
}

void HTMLTableElement::setTHead(HTMLTableSectionElement* tHead)
{
    if (tHead && !tHead->isHTMLTHeadElement()) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
                               "Failed to set the 'tHead' property on "
                               "'HTMLTableElement': The provided value is not "
                               "of type 'HTMLTHeadElement'.");
    }

    deleteTHead();
    if (!tHead) {
        Node* child = firstChild();
        while (child) {
            if (!child->isHTMLTableCaptionElement() &&
                !child->isHTMLTableColGroupElement()) {
                break;
            }
            child = child->nextSibling();
        }
        insertBefore(tHead, child);
    }
}

HTMLTableSectionElement* HTMLTableElement::createTHead()
{
    HTMLTableSectionElement* tHead = this->tHead();
    if (!tHead) {
        tHead = new HTMLTHeadElement(document());
        Node* child = firstChild();
        while (child) {
            if (!child->isHTMLTableCaptionElement() &&
                !child->isHTMLTableColGroupElement()) {
                break;
            }
            child = child->nextSibling();
        }
        insertBefore(tHead, child);
    }
    return tHead;
}

void HTMLTableElement::deleteTHead()
{
    Node* child = firstChild();
    while (child) {
        if (child->isHTMLTHeadElement()) {
            removeChild(child);
            return;
        }
        child = child->nextSibling();
    }
}

HTMLTableSectionElement* HTMLTableElement::tFoot()
{
    Node* child = firstChild();
    while (child) {
        if (child->isHTMLTFootElement()) {
            return child->asHTMLTFootElement();
        }
        child = child->nextSibling();
    }
    return nullptr;
}

void HTMLTableElement::setTFoot(HTMLTableSectionElement* tFoot)
{
    if (tFoot && !tFoot->isHTMLTFootElement()) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
                               "Failed to set the 'tFoot' property on "
                               "'HTMLTableElement': The provided value is not "
                               "of type 'HTMLTFootElement'.");
    }

    deleteTHead();
    if (!tFoot) {
        Node* child = firstChild();
        while (child) {
            if (!child->isHTMLTableCaptionElement() &&
                !child->isHTMLTableColGroupElement() &&
                !child->isHTMLTHeadElement()) {
                break;
            }
            child = child->nextSibling();
        }
        insertBefore(tFoot, child);
    }
}

HTMLTableSectionElement* HTMLTableElement::createTFoot()
{
    HTMLTableSectionElement* tFoot = this->tFoot();
    if (!tFoot) {
        tFoot = new HTMLTFootElement(document());
        Node* child = firstChild();
        while (child) {
            if (!child->isHTMLTableCaptionElement() &&
                !child->isHTMLTableColGroupElement() &&
                !child->isHTMLTHeadElement()) {
                break;
            }
            child = child->nextSibling();
        }
        insertBefore(tFoot, child);
    }
    return tFoot;
}

void HTMLTableElement::deleteTFoot()
{
    Node* child = firstChild();
    while (child) {
        if (child->isHTMLTFootElement()) {
            removeChild(child);
            return;
        }
        child = child->nextSibling();
    }
}

HTMLCollection* HTMLTableElement::tBodies()
{
    if (m_tBodies) {
        return m_tBodies;
    }

    RareNodeMembers* rareData = ensureRareMembers();
    ActiveHTMLCollectionList* activeLists =
        rareData->ensureActiveHtmlCollectionListForTagName();

    m_tBodies =
        new HTMLCollection(this, NodeListImpl::TBodiesFilter, nullptr, true);

    rareData->putActiveHtmlCollectionListWithQuery(
        activeLists, starFish()->staticStrings()->m_tbodies.localName(),
        m_tBodies);
    return m_tBodies;
}

HTMLTableSectionElement* HTMLTableElement::createTBody()
{
    HTMLTableSectionElement* tBody = new HTMLTBodyElement(document());

    Node* child = lastChild();
    Node* lastTBody = nullptr;
    while (child) {
        if (child->isHTMLTBodyElement()) {
            lastTBody = child;
            break;
        }
        child = child->previousSibling();
    }
    if (lastTBody) {
        insertBefore(tBody, lastTBody->nextSibling());
    } else {
        appendChild(tBody);
    }
    return tBody;
}

struct TableRowsCollectionData : public gc {
    Node* root;
    Node* lastNode;
    GCVector<GCVector<Node*>> tag;
};

HTMLCollection* HTMLTableElement::rows()
{
    if (m_rows) {
        return m_rows;
    }
    RareNodeMembers* rareData = ensureRareMembers();
    ActiveHTMLCollectionList* activeLists =
        rareData->ensureActiveHtmlCollectionListForTagName();

    TableRowsCollectionData* data = new TableRowsCollectionData;
    data->root = this;
    data->lastNode = nullptr;
    data->tag.resize(4);

    m_rows =
        new HTMLCollection(this, NodeListImpl::TableRowsFilter, data, true);

    rareData->putActiveHtmlCollectionListWithQuery(
        activeLists, starFish()->staticStrings()->m_rows.localName(), m_rows);
    return m_rows;
}

HTMLTableRowElement* HTMLTableElement::insertRow(int32_t index)
{
    HTMLCollection* rows = this->rows();
    size_t rowsLength = rows->length();
    if (index < -1 ||
        (index != -1 && static_cast<size_t>(index) > rowsLength)) {
        throw new DOMException(document(), DOMException::INDEX_SIZE_ERR);
    }

    HTMLTableRowElement* row = new HTMLTableRowElement(document());
    HTMLCollection* tBodies = this->tBodies();
    size_t tBodiesLength = tBodies->length();
    if (rowsLength == 0 && tBodiesLength == 0) {
        HTMLTBodyElement* tBody = new HTMLTBodyElement(document());
        tBody->appendChild(row);
        appendChild(tBody);
    } else if (rowsLength == 0) {
        Element* lastTBody = tBodies->item(tBodiesLength - 1);
        lastTBody->appendChild(row);
    } else if (index == -1 || static_cast<size_t>(index) == rowsLength) {
        Element* lastRow = rows->item(rowsLength - 1);
        STARFISH_ASSERT(lastRow && lastRow->parentNode());
        lastRow->parentNode()->appendChild(row);
    } else {
        Element* indexedRow = rows->item(index);
        STARFISH_ASSERT(indexedRow && indexedRow->parentNode());
        indexedRow->parentNode()->insertBefore(row, indexedRow);
    }
    return row;
}

void HTMLTableElement::deleteRow(int32_t index)
{
    HTMLCollection* rows = this->rows();
    size_t rowsLength = rows->length();
    if (index == -1) {
        if (rows->length() == 0) {
            return;
        } else {
            index = rowsLength - 1;
        }
    } else if (index < -1 || static_cast<size_t>(index) >= rowsLength) {
        throw new DOMException(document(), DOMException::INDEX_SIZE_ERR);
    }

    Element* row = rows->item(index);
    STARFISH_ASSERT(row && row->parentNode());
    row->parentNode()->removeChild(row);
}

String* HTMLTableElement::cellspacing()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_cellspacing);
}

void HTMLTableElement::setCellspacing(String* cellspacing)
{
    setAttribute(starFish()->staticStrings()->m_cellspacing, cellspacing);
}

String* HTMLTableElement::cellpadding()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_cellpadding);
}

void HTMLTableElement::setCellpadding(String* cellpadding)
{
    setAttribute(starFish()->staticStrings()->m_cellpadding, cellpadding);
}

bool HTMLTableElement::isValidAlign(String* align)
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

TextAlignValue HTMLTableElement::alignValue(String* align)
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
}
