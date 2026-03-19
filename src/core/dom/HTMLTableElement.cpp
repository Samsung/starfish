/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/HTMLTableElement.h"

#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLTBodyElement.h"
#include "core/dom/HTMLTFootElement.h"
#include "core/dom/HTMLTHeadElement.h"
#include "core/dom/HTMLTableCaptionElement.h"
#include "core/dom/HTMLTableColGroupElement.h"
#include "core/dom/HTMLTableRowElement.h"
#include "core/dom/HTMLTableSectionElement.h"
#include "core/style/CSSParser.h"

namespace Starfish {
#define PUSH_PAIR_BORDER_WIDTH(POS, ...)                                     \
    {                                                                        \
        if (!border->contains("px")) {                                       \
            border = border->concat(String::createASCIIString("px"));        \
        }                                                                    \
        CSSStyleValuePair pair;                                              \
        CSSTokenVector tokens;                                               \
        CSSTokenValue token(border->toUTF8NonGCString());                    \
        tokens.push_back(token);                                             \
        if (pair.updateValueBorder##POS##Width(document(), tokens)) {        \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Width); \
            cssValues.push_back(pair);                                       \
        }                                                                    \
    }

#define PUSH_PAIR_BORDER_COLOR(POS, ...)                                     \
    {                                                                        \
        CSSStyleValuePair pair;                                              \
        CSSTokenVector tokens;                                               \
        CSSTokenValue token(bordercolor->toUTF8NonGCString());               \
        tokens.push_back(token);                                             \
        if (pair.updateValueBorder##POS##Color(document(), tokens)) {        \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Color); \
            cssValues.push_back(pair);                                       \
        }                                                                    \
    }

#define PUSH_PAIR_BORDER_FRAME(POS, pos)                                     \
    {                                                                        \
        CSSStyleValuePair pair;                                              \
        CSSTokenVector tokens;                                               \
        if (border_##pos) {                                                  \
            CSSTokenValue token("solid");                                    \
            tokens.push_back(token);                                         \
        } else {                                                             \
            CSSTokenValue token("hidden");                                   \
            tokens.push_back(token);                                         \
        }                                                                    \
        if (pair.updateValueBorder##POS##Style(document(), tokens)) {        \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Style); \
            cssValues.push_back(pair);                                       \
        }                                                                    \
    }                                                                        \
    {                                                                        \
        CSSStyleValuePair pair;                                              \
        CSSTokenVector tokens;                                               \
        CSSTokenValue token("thin");                                         \
        tokens.push_back(token);                                             \
        if (pair.updateValueBorder##POS##Width(document(), tokens)) {        \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Width); \
            cssValues.push_back(pair);                                       \
        }                                                                    \
    }

#define PUSH_PAIR_BORDER_STYLE(POS, ...)                                     \
    {                                                                        \
        CSSStyleValuePair pair;                                              \
        CSSTokenVector tokens;                                               \
        CSSTokenValue token(borderStyle);                                    \
        tokens.push_back(token);                                             \
        if (pair.updateValueBorder##POS##Style(document(), tokens)) {        \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Style); \
            cssValues.push_back(pair);                                       \
        }                                                                    \
    }

void* HTMLTableElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLTableElement));
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

HTMLTableElement::Rules getRulesFromRulesAttributeValue(String* rules)
{
    if (!rules || rules->isEmpty()) {
        return HTMLTableElement::UnsetRules;
    }
    if (rules->equalsIgnoreCase("none")) {
        return HTMLTableElement::NoneRules;
    } else if (rules->equalsIgnoreCase("groups")) {
        return HTMLTableElement::GroupsRules;
    } else if (rules->equalsIgnoreCase("rows")) {
        return HTMLTableElement::RowsRules;
    } else if (rules->equalsIgnoreCase("cols")) {
        return HTMLTableElement::ColsRules;
    } else if (rules->equalsIgnoreCase("all")) {
        return HTMLTableElement::AllRules;
    }
    return HTMLTableElement::UnsetRules;
}

HTMLTableElement::CellBorders getCellBordersFromRule(
    HTMLTableElement::Rules rules, bool hasBorder, bool hasBorderColor)
{
    switch (rules) {
    case HTMLTableElement::UnsetRules:
        if (!hasBorder) {
            return HTMLTableElement::NoBorders;
        }
        if (hasBorderColor) {
            return HTMLTableElement::SolidBorders;
        }
        return HTMLTableElement::InsetBorders;
    case HTMLTableElement::NoneRules:
    case HTMLTableElement::GroupsRules:
        return HTMLTableElement::NoBorders;
    case HTMLTableElement::RowsRules:
        return HTMLTableElement::SolidBordersRowsOnly;
    case HTMLTableElement::ColsRules:
        return HTMLTableElement::SolidBordersColsOnly;
    case HTMLTableElement::AllRules:
        return HTMLTableElement::SolidBorders;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return HTMLTableElement::NoBorders;
}

void HTMLTableElement::didAttributeChanged(QualifiedName name,
                                           Optional<String*> old, String* value,
                                           bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starfish()->staticStrings()->m_align ||
        name == starfish()->staticStrings()->m_frame ||
        name == starfish()->staticStrings()->m_width ||
        name == starfish()->staticStrings()->m_bgcolor) {
        setNeedsStyleRecalc();
    } else if (name == starfish()->staticStrings()->m_border) {
        m_hasBorder = value && !value->isEmpty();
        setNeedsStyleRecalc();
    } else if (name == starfish()->staticStrings()->m_bordercolor) {
        m_hasBorderColor = value && !value->isEmpty();
        setNeedsStyleRecalc();
    } else if (name == starfish()->staticStrings()->m_rules) {
        m_rules = getRulesFromRulesAttributeValue(value);
        setNeedsStyleRecalc();
    } else if (name == starfish()->staticStrings()->m_cellpadding) {
        if (attributeCreated) {
            m_hasCellPaddingAttribute = true;
        }
        if (attributeRemoved) {
            m_hasCellPaddingAttribute = false;
        }
        setNeedsStyleRecalc();
    } else if (name == starfish()->staticStrings()->m_cellspacing) {
        if (attributeCreated) {
            m_hasCellSpacingAttribute = true;
        }
        if (attributeRemoved) {
            m_hasCellSpacingAttribute = false;
        }
        setNeedsStyleRecalc();
    }
}

bool getBordersFromFrameAttributeValue(String* frame, bool& border_top,
                                       bool& border_right, bool& border_bottom,
                                       bool& border_left)
{
    border_top = border_right = border_bottom = border_left = false;
    if (frame->equalsIgnoreCase("void")) {
    } else if (frame->equalsIgnoreCase("above")) {
        border_top = true;
    } else if (frame->equalsIgnoreCase("below")) {
        border_bottom = true;
    } else if (frame->equalsIgnoreCase("hsides")) {
        border_top = border_bottom = true;
    } else if (frame->equalsIgnoreCase("vsides")) {
        border_right = border_left = true;
    } else if (frame->equalsIgnoreCase("lhs")) {
        border_left = true;
    } else if (frame->equalsIgnoreCase("rhs")) {
        border_right = true;
    } else if (frame->equalsIgnoreCase("box") ||
               frame->equalsIgnoreCase("border")) {
        border_top = border_right = border_bottom = border_left = true;
    } else {
        return false;
    }
    return true;
}

void HTMLTableElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues, matchedRules,
                                               cssCustomValues);

    String* align = getAttributeOrEmpty(starfish()->staticStrings()->m_align);
    if (isValidAlign(align)) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::TextAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::TextAlignValueKind);
        pair.setValue(alignValue(align));
        cssValues.push_back(pair);
    }

    if (m_hasBorder) {
        String* border =
            getAttributeOrEmpty(starfish()->staticStrings()->m_border);
        GEN_FOURSIDE(PUSH_PAIR_BORDER_WIDTH);
    }

    if (m_hasBorderColor) {
        String* bordercolor =
            getAttributeOrEmpty(starfish()->staticStrings()->m_bordercolor);
        GEN_FOURSIDE(PUSH_PAIR_BORDER_COLOR);
    }

    String* frame = getAttributeOrEmpty(starfish()->staticStrings()->m_frame);
    bool hasFrame = !frame->isEmpty();
    if (hasFrame) {
        bool border_top, border_right, border_bottom, border_left;
        hasFrame = getBordersFromFrameAttributeValue(
            frame, border_top, border_right, border_bottom, border_left);
        if (hasFrame) {
            GEN_FOURSIDE(PUSH_PAIR_BORDER_FRAME);
        }
    }

    if (m_rules != UnsetRules) {
        CSSStyleValuePair pair;
        CSSTokenVector tokens;
        CSSTokenValue token("collapse");
        tokens.push_back(token);
        if (pair.updateValueBorderCollapse(document(), tokens)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BorderCollapse);
            cssValues.push_back(pair);
        }
    }

    if (!hasFrame) {
        const char* borderStyle = nullptr;
        if (m_hasBorderColor) {
            borderStyle = "solid";
        } else if (m_hasBorder) {
            borderStyle = "outset";
        } else if (m_rules != UnsetRules) {
            borderStyle = "hidden";
        }
        if (borderStyle) {
            GEN_FOURSIDE(PUSH_PAIR_BORDER_STYLE);
        }
    }

    String* w = getAttributeOrEmpty(starfish()->staticStrings()->m_width);
    if (!w->isEmpty()) {
        // Use px as the default unit
        if (!w->contains("px") && !w->contains("%")) {
            w = w->concat(String::createASCIIString("px"));
        }

        CSSStyleValuePair pair;
        CSSTokenVector tokens;
        CSSTokenValue token(w->toUTF8NonGCString());
        ;
        tokens.push_back(token);
        if (pair.updateValueWidth(document(), tokens)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Width);
            cssValues.push_back(pair);
        }
    }

    String* bgColor =
        getAttributeOrEmpty(starfish()->staticStrings()->m_bgcolor);
    if (!bgColor->isEmpty()) {
        CSSStyleValuePair pair;
        CSSTokenValue token(bgColor->toUTF8NonGCString());
        if (pair.updateValueUnitColor(token)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundColor);
            cssValues.push_back(pair);
        }
    }

    if (m_hasCellSpacingAttribute) {
        String* value =
            getAttributeOrEmpty(starfish()->staticStrings()->m_cellspacing);
        if (value && !value->equals(String::emptyString)) {
            // Use px only
            if (!value->contains("px")) {
                value = value->concat(String::createASCIIString("px"));
            }
        }
        CSSStyleValuePair pair;
        CSSTokenVector tokens;
        CSSTokenValue token(value->toUTF8NonGCString());
        tokens.push_back(token);
        if (pair.updateValueBorderSpacing(document(), tokens)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BorderSpacing);
            cssValues.push_back(pair);
        }
    }
}

Optional<HTMLTableCaptionElement*> HTMLTableElement::caption()
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

void HTMLTableElement::setCaption(Optional<HTMLTableCaptionElement*> caption)
{
    if (caption && !caption->isHTMLTableCaptionElement()) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Failed to set the 'caption' property on "
                               "'HTMLTableElement': The provided value is not "
                               "of type 'HTMLTableCaptionElement'.");
    }

    deleteCaption();
    if (!caption) {
        insertBefore(caption.value(), firstChild());
    }
}

HTMLTableCaptionElement* HTMLTableElement::createCaption()
{
    Optional<HTMLTableCaptionElement*> caption = this->caption();
    if (!caption) {
        caption = new HTMLTableCaptionElement(
            document(), starfish()->staticStrings()->m_captionTagName);
        insertBefore(caption.value(), firstChild());
    }
    return caption.value();
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

Optional<HTMLTableSectionElement*> HTMLTableElement::tHead()
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

void HTMLTableElement::setTHead(Optional<HTMLTableSectionElement*> tHead)
{
    if (tHead && !tHead->isHTMLTHeadElement()) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
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
        insertBefore(tHead.value(), child);
    }
}

HTMLTableSectionElement* HTMLTableElement::createTHead()
{
    Optional<HTMLTableSectionElement*> tHead = this->tHead();
    if (!tHead) {
        tHead = new HTMLTHeadElement(
            document(), starfish()->staticStrings()->m_theadTagName);
        Node* child = firstChild();
        while (child) {
            if (!child->isHTMLTableCaptionElement() &&
                !child->isHTMLTableColGroupElement()) {
                break;
            }
            child = child->nextSibling();
        }
        insertBefore(tHead.value(), child);
    }
    return tHead.value();
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

Optional<HTMLTableSectionElement*> HTMLTableElement::tFoot()
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

void HTMLTableElement::setTFoot(Optional<HTMLTableSectionElement*> tFoot)
{
    if (tFoot && !tFoot->isHTMLTFootElement()) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
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
        insertBefore(tFoot.value(), child);
    }
}

HTMLTableSectionElement* HTMLTableElement::createTFoot()
{
    Optional<HTMLTableSectionElement*> tFoot = this->tFoot();
    if (!tFoot) {
        tFoot = new HTMLTFootElement(
            document(), starfish()->staticStrings()->m_tfootTagName);
        Node* child = firstChild();
        while (child) {
            if (!child->isHTMLTableCaptionElement() &&
                !child->isHTMLTableColGroupElement() &&
                !child->isHTMLTHeadElement()) {
                break;
            }
            child = child->nextSibling();
        }
        insertBefore(tFoot.value(), child);
    }
    return tFoot.value();
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
        activeLists, starfish()->staticStrings()->m_tbodies.localName(),
        m_tBodies);
    return m_tBodies;
}

HTMLTableSectionElement* HTMLTableElement::createTBody()
{
    HTMLTableSectionElement* tBody = new HTMLTBodyElement(
        document(), starfish()->staticStrings()->m_tbodyTagName);

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
        activeLists, starfish()->staticStrings()->m_rows.localName(), m_rows);
    return m_rows;
}

HTMLTableRowElement* HTMLTableElement::insertRow(int32_t index)
{
    HTMLCollection* rows = this->rows();
    size_t rowsLength = rows->length();
    if (index < -1 ||
        (index != -1 && static_cast<size_t>(index) > rowsLength)) {
        throw new DOMException(executionContext(),
                               DOMException::INDEX_SIZE_ERR);
    }

    HTMLTableRowElement* row = new HTMLTableRowElement(
        document(), starfish()->staticStrings()->m_trTagName);
    HTMLCollection* tBodies = this->tBodies();
    size_t tBodiesLength = tBodies->length();
    if (rowsLength == 0 && tBodiesLength == 0) {
        HTMLTBodyElement* tBody = new HTMLTBodyElement(
            document(), starfish()->staticStrings()->m_tbodyTagName);
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
        throw new DOMException(executionContext(),
                               DOMException::INDEX_SIZE_ERR);
    }

    Element* row = rows->item(index);
    STARFISH_ASSERT(row && row->parentNode());
    row->parentNode()->removeChild(row);
}

bool HTMLTableElement::isValidAlign(String* align)
{
    if (align->isEmpty()) {
        return false;
    }

    if (align->equalsIgnoreCase("left") || align->equalsIgnoreCase("right") ||
        align->equalsIgnoreCase("middle") ||
        align->equalsIgnoreCase("center") ||
        align->equalsIgnoreCase("-webkit-center")) {
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
    } else if (align->equalsIgnoreCase("-webkit-center")) {
        return TextAlignValue::WebKitCenterTextAlignValue;
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

bool HTMLTableElement::groupRules()
{
    return m_rules == GroupsRules;
}

HTMLTableElement::CellBorders HTMLTableElement::cellBorders()
{
    return getCellBordersFromRule(m_rules, m_hasBorder, m_hasBorderColor);
}

String* HTMLTableElement::cellpadding()
{
    String* value =
        getAttributeOrEmpty(starfish()->staticStrings()->m_cellpadding);
    if (value && !value->isEmpty()) {
        // Use px only
        if (!value->contains("px")) {
            value = value->concat(String::createASCIIString("px"));
        }
    }
    return value;
}
} // namespace Starfish
