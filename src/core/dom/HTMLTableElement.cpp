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
#include "core/dom/HTMLCollection.h"

namespace StarFish {

void* HTMLTableElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLTableElement)] = { 0 };
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
    } else if (name == starFish()->staticStrings()->m_width) {
        setNeedsStyleRecalc();
    }
}

void HTMLTableElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

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
    String* w = width();
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
    String* bgColor = this->bgColor();
    if (!bgColor->equals(String::emptyString)) {
        CSSStyleValuePair pair;
        CSSTokenValue token = w->toNullableUTF8String().m_buffer;
        if (pair.updateValueUnitColor(token)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundColor);
            cssValues.push_back(pair);
        }
    }
}

QualifiedName HTMLTableElement::name()
{
    return starFish()->staticStrings()->m_tableTagName;
}

String* HTMLTableElement::width()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_width);
}

void HTMLTableElement::setWidth(String* width)
{
    setAttribute(starFish()->staticStrings()->m_width, width);
}

String* HTMLTableElement::bgColor()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_bgColor);
}

void HTMLTableElement::setBgColor(String* bgColor)
{
    setAttribute(starFish()->staticStrings()->m_bgColor, bgColor);
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
        new HTMLCollection(this, NodeListImpl::TableRowsFilter, data, false);

    rareData->putActiveHtmlCollectionListWithQuery(activeLists,
                                                   this->localName(), m_rows);
    return m_rows;
}
}
