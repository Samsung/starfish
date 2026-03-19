/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/HTMLTablePartElement.h"
#include "core/style/Style.h"
#include "core/style/StyleRule.h"

namespace Starfish {
void HTMLTablePartElement::didAttributeChanged(QualifiedName name,
                                               Optional<String*> old,
                                               String* value,
                                               bool attributeCreated,
                                               bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starfish()->staticStrings()->m_align ||
        name == starfish()->staticStrings()->m_valign) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    }
}

void HTMLTablePartElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues, matchedRules,
                                               cssCustomValues);

    String* valign = getAttributeOrEmpty(starfish()->staticStrings()->m_valign);
    if (isValidValign(valign)) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::VerticalAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::VerticalAlignValueKind);
        pair.setValue(valignValue(valign));
        cssValues.push_back(pair);
    }

    String* align = getAttributeOrEmpty(starfish()->staticStrings()->m_align);
    if (isValidAlign(align)) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::TextAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::TextAlignValueKind);
        pair.setValue(alignValue(align));
        cssValues.push_back(pair);
    }

    auto table = findParentTable();
    if (table && table->style()->borderCollapse() ==
                     BorderCollapseValue::CollapseBorderCollapseValue) {
        if (isHTMLTHElement() || isHTMLTableCellElement()) {
            if (parentElement()->isHTMLTableRowElement()) {
                CSSStyleValuePair pair;
                pair.setValueKind(CSSStyleValuePair::ValueKind::Inherit);

                // find tag name rules..
                auto begin = &matchedRules[0];
                auto iter = begin;
                auto end = matchedRules.data() + matchedRules.size();
                Optional<size_t> lastTagNameIndex;
                while (iter != end) {
                    if (!iter->first->isUARule()) {
                        if (iter->first->isSimpleTagSelector()) {
                            if (iter->first->selectorList()[0]
                                    .m_selector->selectorText() == name()) {
                                lastTagNameIndex = std::distance(begin, iter);
                            }
                        }
                    }
                    iter++;
                }
                if (lastTagNameIndex) {
                    CSSSelectorList sl(matchedRules[lastTagNameIndex.value()]
                                           .first->selectorList());
                    CSSStyleDeclaration* decl =
                        new CSSStyleDeclaration(document());

#define APPEND_BORDER_RULES(POS)                                     \
    pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Width); \
    decl->addValuePair(pair);                                        \
    pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Style); \
    decl->addValuePair(pair);                                        \
    pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Color); \
    decl->addValuePair(pair);

                    APPEND_BORDER_RULES(Top)
                    APPEND_BORDER_RULES(Bottom)

                    if (!previousElementSibling()) {
                        APPEND_BORDER_RULES(Left)
                    }

                    if (!nextElementSibling()) {
                        APPEND_BORDER_RULES(Right)
                    }
#undef APPEND_BORDER_RULES

                    StyleRule* rule = new StyleRule(std::move(sl), decl);
                    matchedRules.insert(
                        lastTagNameIndex.value() + 1,
                        std::make_pair(rule, document()->documentURI()));
                } else {
#define APPEND_BORDER_RULES(POS)                                     \
    pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Width); \
    cssValues.push_back(pair);                                       \
    pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Style); \
    cssValues.push_back(pair);                                       \
    pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Color); \
    cssValues.push_back(pair);

                    APPEND_BORDER_RULES(Top)
                    APPEND_BORDER_RULES(Bottom)

                    if (!previousElementSibling()) {
                        APPEND_BORDER_RULES(Left)
                    }

                    if (!nextElementSibling()) {
                        APPEND_BORDER_RULES(Right)
                    }
#undef APPEND_BORDER_RULES
                }
            }
        }
    }
}

void HTMLTablePartElement::didComputedStyleChanged(
    ComputedStyle* oldStyle, ComputedStyle* newStyle,
    Optional<StyleResolveContext*> ctx)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle, ctx);

    // NOTE: We can arrange text align on ComputedStyle::arrangeStyleValues
    // but it needs additional check "isHTMLTablePartElement"
    // so I implement this part on here
    if (newStyle && newStyle->orignalTextAlign() ==
                        TextAlignValue::InternalCenterTextAlignValue) {
        auto parent = parentElement();
        if (parent->style()->isSpecifiedTextAlign()) {
            newStyle->setTextAlign(parent->style()->textAlign(), false);
        }
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
        align->equalsIgnoreCase("-webkit-center")) {
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
    } else if (align->equalsIgnoreCase("-webkit-center")) {
        return TextAlignValue::WebKitCenterTextAlignValue;
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

HTMLTableElement* HTMLTablePartElement::findParentTable()
{
    Node* parent = parentNode();
    while (parent && !parent->isHTMLTableElement()) {
        parent = parent->parentNode();
    }
    if (parent == nullptr) {
        return nullptr;
    }
    return parent->asHTMLTableElement();
}

#define ADDITIONAL_BORDER_RULES(POS, ...)                                    \
    void HTMLTablePartElement::additionalBorderRules##POS(                   \
        CSSStyleValuePairVectorHolder& cssValues, const char* width,         \
        const char* style)                                                   \
    {                                                                        \
        {                                                                    \
            CSSStyleValuePair pair;                                          \
            CSSTokenVector tokens;                                           \
            CSSTokenValue token(width);                                      \
            tokens.push_back(token);                                         \
            pair.updateValueBorder##POS##Width(document(), tokens);          \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Width); \
            cssValues.push_back(pair);                                       \
        }                                                                    \
        {                                                                    \
            CSSStyleValuePair pair;                                          \
            CSSTokenVector tokens;                                           \
            CSSTokenValue token(style);                                      \
            tokens.push_back(token);                                         \
            pair.updateValueBorder##POS##Style(document(), tokens);          \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Style); \
            cssValues.push_back(pair);                                       \
        }                                                                    \
        {                                                                    \
            CSSStyleValuePair pair;                                          \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Border##POS##Color); \
            pair.setValueKind(CSSStyleValuePair::ValueKind::Inherit);        \
            cssValues.push_back(pair);                                       \
        }                                                                    \
    }
GEN_FOURSIDE(ADDITIONAL_BORDER_RULES)
#undef ADDITIONAL_BORDER_RULES

void HTMLTablePartElement::additionalPadding(
    CSSStyleValuePairVectorHolder& cssValues, String* padding)
{
#define ADDITIONAL_PADDING(POS, ...)                                   \
    {                                                                  \
        CSSStyleValuePair pair;                                        \
        CSSTokenVector tokens;                                         \
        CSSTokenValue token(padding->toUTF8NonGCString());             \
        tokens.push_back(token);                                       \
        if (pair.updateValueLength(tokens, 0)) {                       \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Padding##POS); \
            cssValues.push_back(pair);                                 \
        }                                                              \
    }

    GEN_FOURSIDE(ADDITIONAL_PADDING);
#undef ADDITIONAL_PADDING
}
} // namespace Starfish
