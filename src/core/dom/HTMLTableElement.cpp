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
#include "core/dom/DOMException.h"

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
    if (!bgColor->equals(String::emptyString)) {
        CSSStyleValuePair pair;
        CSSTokenValue token = w->toNullableUTF8String().m_buffer;
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
