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

#ifdef STARFISH_ENABLE_TTS

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/HTMLTextEditable.h"
#include "core/dom/Node.h"
#include "core/dom/Text.h"
#include "core/modules/tts/TextAlternativeHelper.h"
#include "core/page/WebView.h"

namespace StarFish {

TextAlternativeHelper::TextAlternativeHelper(StarFish* starfish)
    : StarFishHoldable(starfish)
    , m_inAriaLabelledbyOrArialDescribedBy(false)
{
}

String* TextAlternativeHelper::getComputedTextAlternative(Node* node)
{
    node->document()->browsingContext()->webView()->layoutIfNeeds();
    appendTextAlternativeIfNeeds(node);
    return finalize();
}

void TextAlternativeHelper::appendTextAlternativeIfNeeds(Node* node)
{
    // 5.6.1.3. Text Alternative Computation

    if (node->isCharacterData() && node->asCharacterData()->isText()) {
        // Rule 3. Text nodes are often visited because they are children of an
        // element
        // that uses rule 2C to collect text from its children. However, because
        // it is possible to specify textual content using CSS rules, it is
        // necessary for user agents to combine such content, as appropriate,
        // with the text referenced by the text nodes to produce a complete text
        // alternative. An example is the use of CSS :before and :after
        // pseudo-elements, where the user agent combines the textual content
        // specified in the style sheet with that given in the DOM.
        //  When an image replaces text, then the UA should use the original
        //  text, since that text is presumably the equivalent.
        //  When text replaces an image, then the UA should provide that text.
        //  When new text replaces old, then the UA should include the new text,
        //  since that is what is rendered on screen.
        auto textContent = node->textContent();
        if (textContent.hasValue()) {
            // TODO : Consider pseudo-elements
            appendTextAlterNative(textContent.getValue());
        }
        return;
    }

    if (!node->isElement()) {
        return;
    }

    if (isVisitedNode(node)) {
        return;
    }

    // Track visited nodes
    m_vistiedNodes.insert(node);

    // Rule 1. Skip hidden elements unless the author specifies to use them via
    // an
    // aria-labelledby or aria-describedby being used in the current
    // computation.
    if (!m_inAriaLabelledbyOrArialDescribedBy && isAriaHidden(node)) {
        return;
    }

    // Rule 2. For any non-skipped elements:
    // A. Authors specify an element's text alternative in content attributes,
    // used in this order of preference:
    // - The aria-labelledby attribute takes precedence as the element's text
    //   alternative unless this computation is already occurring as the result
    //   of a recursive aria-labelledby declaration
    if (!m_inAriaLabelledbyOrArialDescribedBy) {
        if (appendFromAriaByTypeIfNeeds(node, AriaByType::AriaLabelledBy)) {
            return;
        }
    }
    // - If aria-labelledby is empty or undefined, the aria-label attribute,
    //   which defines an explicit text string, is used. However, if this
    //   computation is already occurring as the result of a recursive text
    //   alternative computation and the current element is an embedded control
    //   as defined in rule 2B, ignore the aria-label attribute and skip
    //   directly to rule 2B.
    const bool isEmbCtrl = isEmbeddedControl(node);

    if (!isEmbCtrl && appendFromAriaLabelIfNeeds(node)) {
        return;
    }
    // - If aria-labelledby and aria-label are both empty or undefined, and if
    //   the element is not marked as presentational (role="presentation", check
    //   for the presence of an equivalent host language attribute or element
    //   for associating a label, and use those mechanisms to determine a text
    //   alternative. For example, in HTML, the img element's alt attribute
    //   defines a label string and the label element references the form
    //   element it labels.
    if (!isEmbCtrl && appendFromAltAttributeIfNeeds(node)) {
        return;
    }

    // B. Authors sometimes embed a control within the label of another widget,
    // where the user can adjust the embedded control's value. For example,
    // consider a check box label that contains a text input field: "Flash the
    // screen [input] times". If the user has entered "5" for the embedded text
    // input, the complete label is "Flash the screen 5 times". For such cases,
    // include the value of the embedded control as part of the text alternative
    // in the following manner:
    //  If the embedded control is a text field, use its value.
    //  If the embedded control is a menu, use the text alternative of the
    //  chosen menu item.
    //  If the embedded control is a select or combobox, use the chosen option.
    //  If the embedded control is a range (e.g. a spinbutton or slider), use
    //  the value of the aria-valuetext attribute if available, or otherwise the
    //  value of the aria-valuenow attribute.
    if (isEmbCtrl) {
        if (appendFromEmbeddedControlIfNeeds(node)) {
            return;
        }
    }

    // C. Otherwise, if the attributes checked in rules A and B didn't provide
    // results, text is collected from descendant content if the current
    // element's role allows "Name From: contents." The text alternatives for
    // child nodes will be concatenated, using this same set of rules. This same
    // rule may apply to a child, which means the computation becomes recursive
    // and can result in text being collected in all the nodes in this subtree,
    // no matter how deep they are. However, any given descendant subtree may
    // instead collect their part of the text alternative from the preferred
    // markup described in A and B above. These author-specified attributes are
    // assumed to provide the correct text alternative for the entire subtree.
    // All in all, the node rules are applied consistently as text alternatives
    // are collected from descendants, and each containing element in those
    // descendants may or may not allow their contents to be used. Each node in
    // the subtree is consulted only once. If text has been collected from a
    // child node, and is referenced by another IDREF in some descendant node,
    // then that second, or subsequent, reference is not followed. This is done
    // to avoid infinite loops.

    // StarFish doesn't support the "role" attribute, so ignore it
    Node* child = node->firstChild();
    while (child) {
        appendTextAlternativeIfNeeds(child);
        child = child->nextSibling();
    }

    // 6.1.2. Description Computation
    if (!m_inAriaLabelledbyOrArialDescribedBy) {
        if (appendFromAriaByTypeIfNeeds(node, AriaByType::ArialDescribedBy)) {
            return;
        }
    }
}

bool TextAlternativeHelper::appendTextAlterNative(String* text)
{
    String* textAlt = text->trim();
    if (textAlt != String::emptyString) {
        m_textAlts.push_back(textAlt);
        return true;
    }
    return false;
}

bool TextAlternativeHelper::isVisitedNode(Node* node)
{
    return m_vistiedNodes.find(node) != m_vistiedNodes.end();
}

bool TextAlternativeHelper::isAriaHidden(Node* node)
{
    if (!node->isElement()) {
        return false;
    }

    String* value = node->asElement()->getAttributeOrEmpty(
        starFish()->staticStrings()->m_ariaHidden);
    if (value->equalsIgnoreCase("true")) {
        return true;
    }

    Element* parent = node->parentElement();
    if (parent) {
        return isAriaHidden(parent);
    }
    return false;
}

bool TextAlternativeHelper::appendFromAriaByTypeIfNeeds(Node* node,
                                                        AriaByType type)
{
    if (!node->isElement()) {
        return false;
    }

    String* value = nullptr;
    switch (type) {
    case AriaByType::AriaLabelledBy:
        value = node->asElement()->getAttributeOrEmpty(
            starFish()->staticStrings()->m_ariaLabelledby);
        break;
    case AriaByType::ArialDescribedBy:
        value = node->asElement()->getAttributeOrEmpty(
            starFish()->staticStrings()->m_ariaDescribedby);
        break;
    default:
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }

    GCVector<StringView> tokens;
    StringUtils::tokenize(value, " ", 1, tokens);
    size_t oldSize = m_textAlts.size();

    if (node->document()) {
        return false;
    }

    if (!tokens.empty()) {
        m_inAriaLabelledbyOrArialDescribedBy = true;
    }

    for (auto id : tokens) {
        Element* element = node->document()->getElementById(&id);
        if (element) {
            if (isVisitedNode(element)) {
                continue;
            }
            appendTextAlternativeIfNeeds(element);
        }
    }

    m_inAriaLabelledbyOrArialDescribedBy = false;
    return oldSize != m_textAlts.size();
}

bool TextAlternativeHelper::appendFromAriaLabelIfNeeds(Node* node)
{
    String* value = node->asElement()->getAttributeOrEmpty(
        starFish()->staticStrings()->m_ariaLabel);
    return appendTextAlterNative(value);
}

bool TextAlternativeHelper::appendFromAltAttributeIfNeeds(Node* node)
{
    // StarFish doesn't support the "role" attribute,
    // so just get from the alt attribute
    String* value = node->asElement()->getAttributeOrEmpty(
        starFish()->staticStrings()->m_alt);
    return appendTextAlterNative(value);
}

bool TextAlternativeHelper::appendFromEmbeddedControlIfNeeds(Node* node)
{
    // Note : Please see a implemation of isEmbeddedControl
    if (!node->isHTMLElement()) {
        return false;
    }

    String* value = nullptr;
    if (node->isHTMLInputElement()) {
        if (node->asHTMLInputElement()->isEditableType() ||
            node->asHTMLInputElement()->type()->equals("button")) {
            value = node->asElement()->getAttributeOrEmpty(
                starFish()->staticStrings()->m_value);
        }
    } else if (node->isHTMLTextEditable()) {
        // TODO input | textarea -> HTMLTextEditable
        value = node->asHTMLTextEditable()->textValue();
    } else if (node->isHTMLSelectElement()) {
        HTMLOptionElement* oe = node->asElement()
                                    ->asHTMLSelectElement()
                                    ->firstSelectedOptionElement();
        if (oe) {
            value =
                oe->getAttributeOrEmpty(starFish()->staticStrings()->m_value);
        }
    }

    if (value) {
        return appendTextAlterNative(value);
    }

    return false;
}

bool TextAlternativeHelper::isEmbeddedControl(Node* node)
{
    // If the embedded control is a text field, use its value.
    if (node->isHTMLInputElement()) {
        // Non-Normative, If the type is a button, the value is visible, in
        // which case it is necessary to read the value.
        if (node->asHTMLInputElement()->type()->equals("button")) {
            return true;
        } else {
            return node->asHTMLInputElement()->isEditableType();
        }
    }

    if (node->isHTMLTextEditable()) {
        // TODO input | textarea -> HTMLTextEditable
        return true;
    }

    // If the embedded control is a menu, use the text alternative of the
    //  chosen menu item.
    // -> StarFish doesn't support

    // If the embedded control is a select or combobox, use the chosen
    // option.
    if (node->isHTMLSelectElement()) {
        return true;
    }
    // If the embedded control is a range (e.g. a spinbutton or slider), use
    //  the value of the aria-valuetext attribute if available, or
    //  otherwise the value of the aria-valuenow attribute.
    //  -> StarFish doesn't support

    // else
    return false;
}

String* TextAlternativeHelper::finalize()
{
    if (!m_textAlts.empty()) {
        StringBuilder builder;
        for (auto text : m_textAlts) {
            builder.appendString(text);
            builder.appendChar(' ');
        }
        return builder.finalize();
    }
    return String::emptyString;
}
}
#endif
