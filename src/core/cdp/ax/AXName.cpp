/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "AXName.h"
#include "AXRole.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/style/ComputedStyle.h"

#include <cctype>
#include <cstring>
#include <set>
#include <vector>

namespace Starfish {

namespace {

    std::string toUTF8(String* s)
    {
        return s ? s->toUTF8NonGCString() : std::string();
    }

    std::string lowered(std::string text)
    {
        for (char& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    }

    std::string tagOf(Element* element)
    {
        return element->localName() ? lowered(toUTF8(element->localName()))
                                    : std::string();
    }

    bool onlyWhitespace(const std::string& text)
    {
        for (char c : text) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                return false;
            }
        }
        return true;
    }

    bool isHtmlSpace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
    }

    std::string trimmed(const std::string& text)
    {
        size_t begin = 0;
        size_t end = text.size();
        while (begin < end && isHtmlSpace(text[begin])) {
            ++begin;
        }
        while (end > begin && isHtmlSpace(text[end - 1])) {
            --end;
        }
        return text.substr(begin, end - begin);
    }

    // The labels of a submit or reset button that has no value.
    const char* const kSubmitLabel = "Submit";
    const char* const kResetLabel = "Reset";

    // Where a piece of a name came from. It decides whether a space is put
    // between two neighbors in a name from contents.
    // The source types of the protocol.
    const char* const kTypeAttribute = "attribute";
    const char* const kTypeContents = "contents";
    const char* const kTypePlaceholder = "placeholder";
    const char* const kTypeRelatedElement = "relatedElement";

    enum class NameFrom {
        None,
        Attribute,
        AttributeExplicitlyEmpty,
        Contents,
        Placeholder,
        RelatedElement,
        Title,
        Value,
    };

    struct TextResult {
        std::string text;
        NameFrom from = NameFrom::None;
    };

    // The input types whose value a user types in.
    bool isTextEntryInputType(const std::string& type)
    {
        return type.empty() || type == "text" || type == "search" ||
               type == "email" || type == "url" || type == "tel" ||
               type == "password" || type == "number";
    }

    bool isTextControl(Element* element, const std::string& tag)
    {
        if (tag == "textarea") {
            return true;
        }
        return tag == "input" &&
               isTextEntryInputType(lowered(axAttribute(element, "type")));
    }

    bool isTextField(const std::string& role)
    {
        return role == "textbox" || role == "searchbox";
    }

    // The elements a <label> can name.
    bool isLabelable(Element* element, const std::string& tag)
    {
        if (tag == "input") {
            return lowered(axAttribute(element, "type")) != "hidden";
        }
        return tag == "button" || tag == "meter" || tag == "output" ||
               tag == "progress" || tag == "select" || tag == "textarea";
    }

    bool isControlRole(const std::string& role)
    {
        return role == "button" || role == "checkbox" || role == "combobox" ||
               role == "listbox" || role == "menuitem" ||
               role == "menuitemcheckbox" || role == "menuitemradio" ||
               role == "radio" || role == "searchbox" || role == "slider" ||
               role == "spinbutton" || role == "switch" || role == "tab" ||
               role == "textbox";
    }

    // Whether an element's contents join an ancestor's name. This is the
    // recursive case of SupportsNameFromContents in Chromium: most roles
    // that lack a name of their own still pass their text upward.
    bool contributesContentsToAncestor(Element* element, const std::string& tag,
                                       const std::string& role)
    {
        if (axRoleSupportsNameFromContents(role)) {
            return true;
        }
        if (tag == "iframe" || tag == "svg" || tag == "video" ||
            tag == "audio" || tag == "object" || tag == "embed" ||
            tag == "select" || tag == "img" || tag == "input") {
            return false;
        }
        if (role == "generic") {
            return tag != "body" && tag != "html";
        }
        return role == "term" || role == "math" || role == "caption" ||
               role == "code" || role == "definition" || role == "deletion" ||
               role == "emphasis" || role == "insertion" || role == "list" ||
               role == "listitem" || role == "mark" || role == "none" ||
               role == "paragraph" || role == "region" || role == "strong" ||
               role == "subscript" || role == "superscript" || role == "time" ||
               role == "DisclosureTriangle" || role == "disclosureTriangle" ||
               role == "LabelText" || role == "canvas";
    }

    bool isAriaHiddenElement(Element* element, const std::string& tag)
    {
        if (tag == "html" || tag == "body" || tag == "option") {
            return false;
        }
        return lowered(axAttribute(element, "aria-hidden")) == "true";
    }

    bool isHiddenViaStyle(Element* element)
    {
        if (!element->frame()) {
            return true;
        }
        ComputedStyle* style = element->style();
        return style &&
               style->visibility() != VisibilityValue::VisibleVisibilityValue;
    }

    // An element is visible when neither it nor an ancestor is hidden by
    // aria-hidden or by style.
    bool isVisible(Element* element)
    {
        for (Node* node = element; node && node->isElement();
             node = node->parentNode()) {
            Element* current = node->asElement();
            if (isAriaHiddenElement(current, tagOf(current))) {
                return false;
            }
        }
        return !isHiddenViaStyle(element);
    }

    // The nearest ancestor that lays out its content as a block. Two nodes
    // that share one are on the same line of text.
    Node* nearestBlockFlow(Node* node)
    {
        for (Node* parent = node->parentNode(); parent;
             parent = parent->parentNode()) {
            if (parent->isElement() && parent->frame() &&
                parent->frame()->isFrameBlockBox()) {
                return parent;
            }
        }
        return nullptr;
    }

    bool isInlineLevel(Node* node)
    {
        Frame* frame = node->frame();
        if (!frame) {
            return false;
        }
        return frame->isFrameText() || frame->isInlineLevel();
    }

    bool isAtomicInline(Node* node)
    {
        Frame* frame = node->frame();
        return frame && !frame->isFrameText() && frame->isAtomicInlineLevel();
    }

    bool isInSameBlockFlow(Node* first, Node* second)
    {
        if (!isInlineLevel(first) || !isInlineLevel(second)) {
            return false;
        }
        Node* firstFlow = nearestBlockFlow(first);
        return firstFlow && firstFlow == nearestBlockFlow(second);
    }

    bool isSpaceNeededForNameFrom(NameFrom from)
    {
        switch (from) {
        case NameFrom::None:
        case NameFrom::AttributeExplicitlyEmpty:
        case NameFrom::Contents:
            return false;
        case NameFrom::Attribute:
        case NameFrom::Placeholder:
        case NameFrom::RelatedElement:
        case NameFrom::Title:
        case NameFrom::Value:
            return true;
        }
        return false;
    }

    // The accname recursion of Chromium (AXNodeObject::TextAlternative),
    // computed on DOM nodes. One instance computes one name.
    class NameComputer {
    public:
        std::string compute(Element* element, const std::string& role)
        {
            m_topRole = role;
            TextResult result = textAlternative(element, false, nullptr);
            return axCollapseWhitespace(result.text);
        }

        // The text the elements named by an id list contribute to the
        // element that refers to them.
        std::string fromIdRefs(Element* element, const std::string& idRefs)
        {
            m_visited.insert(element);
            return axCollapseWhitespace(
                textFromElements(true, elementsFromIdRefs(element, idRefs)));
        }

        // Every source Chromium looks at to name the element, in order.
        std::vector<AXNameSource> sources(Element* element,
                                          const std::string& role)
        {
            m_topRole = role;
            std::string tag = tagOf(element);
            std::vector<AXNameSource> list;
            m_visited.insert(element);
            if (isHiddenForTextAlternative(element, tag, nullptr)) {
                return list;
            }

            bool found = false;
            collectAriaSources(element, list, found);
            bool explicitlyEmpty =
                collectNativeSources(element, tag, role, list, found);
            collectContentsSource(element, tag, role, list, found);

            bool prohibited = axRoleIsNameProhibited(role);
            if (!prohibited &&
                !(explicitlyEmpty && (tag == "img" || tag == "area"))) {
                collectTitleSource(element, list, found);
            }
            return list;
        }

        static std::vector<Element*> elementsFromIdRefs(
            Element* element, const std::string& idRefs)
        {
            std::vector<Element*> elements;
            Document* document = element->document();
            if (!document) {
                return elements;
            }
            size_t start = 0;
            while (start < idRefs.size()) {
                size_t end = idRefs.find(' ', start);
                std::string id = idRefs.substr(start, end - start);
                if (!id.empty()) {
                    Element* referenced = document->getElementById(
                        String::fromUTF8(id.data(), id.size()));
                    if (referenced) {
                        elements.push_back(referenced);
                    }
                }
                if (end == std::string::npos) {
                    break;
                }
                start = end + 1;
            }
            return elements;
        }

        // The labels that name a control, in tree order. A <label for>
        // names the element with that id. A <label> with no for attribute
        // names its first labelable descendant.
        static std::vector<Element*> labelsOf(Element* control)
        {
            std::vector<Element*> labels;
            Document* document = control->document();
            if (!document || !isLabelable(control, tagOf(control))) {
                return labels;
            }

            Element* wrapping = nullptr;
            for (Element* parent = control->parentElement(); parent;
                 parent = parent->parentElement()) {
                if (tagOf(parent) == "label" &&
                    !axHasAttribute(parent, "for") &&
                    firstLabelableDescendant(parent) == control) {
                    wrapping = parent;
                    break;
                }
            }

            std::string id = axAttribute(control, "id");
            if (id.empty()) {
                if (wrapping) {
                    labels.push_back(wrapping);
                }
                return labels;
            }

            Traverse::traverse(document, [&](Node* node) {
                if (!node->isElement()) {
                    return;
                }
                Element* candidate = node->asElement();
                if (candidate == wrapping) {
                    labels.push_back(candidate);
                    return;
                }
                if (tagOf(candidate) == "label" &&
                    axAttribute(candidate, "for") == id &&
                    document->getElementById(
                        String::fromUTF8(id.data(), id.size())) == control) {
                    labels.push_back(candidate);
                }
            });
            return labels;
        }

    private:
        static Element* firstLabelableDescendant(Element* root)
        {
            Element* found = nullptr;
            Traverse::traverse(root, [&](Node* node) {
                if (found || !node->isElement() || node == root) {
                    return;
                }
                Element* candidate = node->asElement();
                if (isLabelable(candidate, tagOf(candidate))) {
                    found = candidate;
                }
            });
            return found;
        }

        static std::vector<Node*> childrenOf(Node* parent)
        {
            std::vector<Node*> children;
            RenderingSiblingIterator iterator(parent->firstRenderingChild());
            while (Optional<Node*> next = iterator.next()) {
                if (!next.value()) {
                    break;
                }
                children.push_back(next.value());
            }
            return children;
        }

        // accname 2A: hidden content is left out of a name, unless the
        // element is reached through a label that is hidden itself. Then
        // there is no way to tell what was hidden on purpose.
        bool isHiddenForTextAlternative(Element* element,
                                        const std::string& tag,
                                        Element* labelRoot)
        {
            if (tag == "noscript") {
                return true;
            }
            if (tag == "title" || tag == "desc") {
                return false;
            }
            if (tag == "option" && !isAriaHiddenElement(element, tag)) {
                return false;
            }
            if (labelRoot && !isVisible(labelRoot)) {
                return false;
            }
            if (isAriaHiddenElement(element, tag)) {
                return true;
            }
            return isHiddenViaStyle(element);
        }

        TextResult textNodeAlternative(Node* node)
        {
            m_visited.insert(node);
            TextResult result;
            result.from = NameFrom::Contents;
            result.text = axTextForNode(node);
            if (result.text.empty() && node->frame()) {
                result.text = " "; // rendered whitespace between inlines
            }
            return result;
        }

        TextResult recursiveTextAlternative(Node* node, Element* labelRoot)
        {
            if (m_visited.count(node) && !labelRoot) {
                return TextResult();
            }
            return textAlternative(node, true, labelRoot);
        }

        // Several elements, as the target of aria-labelledby or the labels
        // of a control. Their texts are joined by spaces.
        std::string textFromElements(
            bool inLabelledByTraversal, const std::vector<Element*>& elements,
            std::vector<AXRelatedText>* related = nullptr,
            bool* anyElement = nullptr)
        {
            std::string accumulated;
            for (Element* element : elements) {
                TextResult result = recursiveTextAlternative(
                    element, inLabelledByTraversal ? element : nullptr);
                result.text = trimmed(result.text);
                m_visited.insert(element);
                if (related) {
                    related->push_back({ element, result.text });
                }
                if (!result.text.empty()) {
                    if (!accumulated.empty()) {
                        accumulated.push_back(' ');
                    }
                    accumulated += result.text;
                }
            }
            if (anyElement) {
                *anyElement = !elements.empty();
            }
            return accumulated;
        }

        void collectAriaSources(Element* element,
                                std::vector<AXNameSource>& list, bool& found)
        {
            // Step 2B. The misspelled attribute counts only when the correct
            // one is absent.
            const char* labelledByName =
                axHasAttribute(element, "aria-labeledby") &&
                        !axHasAttribute(element, "aria-labelledby")
                    ? "aria-labeledby"
                    : "aria-labelledby";
            AXNameSource labelledBy(found, labelledByName, kTypeRelatedElement);
            if (axHasAttribute(element, labelledByName)) {
                std::string idRefs = axAttribute(element, labelledByName);
                labelledBy.hasAttributeValue = true;
                labelledBy.attributeValue = idRefs;
                std::vector<AXRelatedText> related;
                bool anyElement = false;
                std::string text =
                    textFromElements(true, elementsFromIdRefs(element, idRefs),
                                     &related, &anyElement);
                if (anyElement && !onlyWhitespace(text)) {
                    labelledBy.related = related;
                    labelledBy.hasText = true;
                    labelledBy.text = text;
                    found = true;
                } else {
                    labelledBy.invalid = true;
                }
            }
            list.push_back(labelledBy);

            // Step 2C.
            AXNameSource label(found, "aria-label", kTypeAttribute);
            std::string ariaLabel = axAttribute(element, "aria-label");
            if (!onlyWhitespace(ariaLabel)) {
                label.hasText = true;
                label.text = ariaLabel;
                label.hasAttributeValue = true;
                label.attributeValue = ariaLabel;
                found = true;
            }
            list.push_back(label);
        }

        // A source that reads one attribute and takes its value as the text.
        void collectAttributeSource(Element* element, const char* attribute,
                                    const char* type,
                                    std::vector<AXNameSource>& list,
                                    bool& found, bool takeEmpty)
        {
            AXNameSource source(found, attribute, type);
            if (axHasAttribute(element, attribute)) {
                std::string value = axAttribute(element, attribute);
                if (takeEmpty || !value.empty()) {
                    source.hasText = true;
                    source.text = value;
                    source.hasAttributeValue = true;
                    source.attributeValue = value;
                    found = true;
                }
            }
            list.push_back(source);
        }

        // The title attribute, which is a source unless it repeats the
        // text of the element.
        void collectTitleSource(Element* element,
                                std::vector<AXNameSource>& list, bool& found)
        {
            AXNameSource source(found, "title", kTypeAttribute);
            if (axHasAttribute(element, "title")) {
                std::string title = axAttribute(element, "title");
                if (!innerTextEquals(element, title)) {
                    source.hasText = true;
                    source.text = title;
                    source.hasAttributeValue = true;
                    source.attributeValue = title;
                    found = true;
                }
            }
            list.push_back(source);
        }

        // The label text of an input button that has no value attribute.
        static std::string defaultButtonLabel(const std::string& type)
        {
            if (type == "submit") {
                return kSubmitLabel;
            }
            return type == "reset" ? kResetLabel : std::string();
        }

        // Step 2D. Returns whether the native text is an explicitly empty
        // alt attribute, which stops the title from naming the element.
        bool collectNativeSources(Element* element, const std::string& tag,
                                  const std::string& role,
                                  std::vector<AXNameSource>& list, bool& found)
        {
            std::string type = tag == "input"
                                   ? lowered(axAttribute(element, "type"))
                                   : std::string();

            if (isOptionWithOneTextChild(element, tag)) {
                std::string label = axAttribute(element, "label");
                if (label.empty()) {
                    label = axCollapseWhitespace(
                        toUTF8(element->textContent().value()));
                }
                if (!label.empty()) {
                    AXNameSource source(found, "", kTypeContents);
                    source.hasText = true;
                    source.text = label;
                    list.push_back(source);
                    found = true;
                }
            }

            if (isLabelable(element, tag)) {
                AXNameSource source(found, "", kTypeRelatedElement);
                source.nativeSource = "label";
                std::vector<Element*> labels = labelsOf(element);
                if (!labels.empty()) {
                    std::string id = axAttribute(element, "id");
                    for (Element* label : labels) {
                        bool byFor = !axAttribute(label, "for").empty() &&
                                     axAttribute(label, "for") == id;
                        source.nativeSource =
                            byFor ? "labelfor" : "labelwrapped";
                    }
                    std::vector<AXRelatedText> related;
                    std::string text = axCollapseWhitespace(
                        textFromElements(false, labels, &related));
                    source.related = related;
                    source.hasText = true;
                    source.text = text;
                    found = true;
                }
                list.push_back(source);
            }

            if (tag == "input" &&
                (type == "button" || type == "submit" || type == "reset")) {
                collectAttributeSource(element, "value", kTypeAttribute, list,
                                       found, true);
                // Only the text is wanted: Chromium sets no attribute value
                // for a button value.
                list.back().hasAttributeValue = false;
                return false;
            }

            if (tag == "input" && type == "image") {
                collectAttributeSource(element, "alt", kTypeAttribute, list,
                                       found, false);
                collectAttributeSource(element, "value", kTypeAttribute, list,
                                       found, true);
                list.back().hasAttributeValue = false;
                collectTitleSource(element, list, found);
                AXNameSource source(found, "type", kTypeAttribute);
                source.hasAttributeValue = true;
                source.attributeValue = type;
                source.hasText = true;
                source.text = kSubmitLabel;
                list.push_back(source);
                found = true;
                return false;
            }

            if (isTextControl(element, tag)) {
                collectTitleSource(element, list, found);
                collectAttributeSource(element, "placeholder", kTypePlaceholder,
                                       list, found, false);
            }
            if (isTextField(role)) {
                collectAttributeSource(element, "aria-placeholder",
                                       kTypePlaceholder, list, found, false);
                return false;
            }

            if (tag == "img" || tag == "area") {
                collectAttributeSource(element, "alt", kTypeAttribute, list,
                                       found, true);
                return axHasAttribute(element, "alt") &&
                       axAttribute(element, "alt").empty();
            }

            if (element->isSVGElement()) {
                AXNameSource source(found, "", kTypeRelatedElement);
                source.nativeSource = "title";
                std::string text = svgTitle(element);
                if (!text.empty()) {
                    source.hasText = true;
                    source.text = text;
                    found = true;
                }
                list.push_back(source);
            }
            return false;
        }

        // Steps 2F and 2G.
        void collectContentsSource(Element* element, const std::string& tag,
                                   const std::string& role,
                                   std::vector<AXNameSource>& list, bool& found)
        {
            if (isOptionWithOneTextChild(element, tag) ||
                !shouldIncludeContents(element, tag, role, false, nullptr)) {
                return;
            }
            AXNameSource source(found, "", kTypeContents);
            std::string text;
            std::string type = tag == "input"
                                   ? lowered(axAttribute(element, "type"))
                                   : std::string();
            if (tag == "input" &&
                (type == "button" || type == "submit" || type == "reset")) {
                // The button is drawn with its value, or with a default.
                text = axHasAttribute(element, "value")
                           ? axAttribute(element, "value")
                           : defaultButtonLabel(type);
            } else if (tag == "input" && type == "image") {
                // The image is drawn with the text that named it.
                for (const AXNameSource& earlier : list) {
                    if (earlier.hasText && !earlier.superseded) {
                        text = earlier.text;
                        break;
                    }
                }
            } else {
                text = trimmed(textFromDescendants(element, nullptr));
            }
            if (!text.empty()) {
                source.hasText = true;
                source.text = text;
                found = true;
            }
            list.push_back(source);
        }

        static bool isOptionWithOneTextChild(Element* element,
                                             const std::string& tag)
        {
            Node* child = element->firstChild();
            return tag == "option" && child && child == element->lastChild() &&
                   child->isText();
        }

        static std::string svgTitle(Element* element)
        {
            for (Node* child = element->firstChild(); child;
                 child = child->nextSibling()) {
                if (child->isElement() &&
                    tagOf(child->asElement()) == "title") {
                    Optional<String*> content = child->textContent();
                    return axCollapseWhitespace(content.hasValue()
                                                    ? toUTF8(content.value())
                                                    : std::string());
                }
            }
            return std::string();
        }

        bool innerTextEquals(Element* element, const std::string& text)
        {
            Optional<String*> content = element->textContent();
            return axCollapseWhitespace(content.hasValue()
                                            ? toUTF8(content.value())
                                            : std::string()) == text;
        }

        // The title attribute, unless it only repeats the element's text.
        std::string tooltip(Element* element)
        {
            if (!axHasAttribute(element, "title")) {
                return std::string();
            }
            std::string title = axAttribute(element, "title");
            return innerTextEquals(element, title) ? std::string() : title;
        }

        // The value an embedded control adds to the name of an element that
        // contains it (accname 2E).
        bool valueContribution(Element* element, const std::string& tag,
                               const std::string& role, std::string& out)
        {
            if (!element->isHTMLFormControl()) {
                return false;
            }
            if (tag == "select") {
                HTMLOptionElement* option = element->asHTMLSelectElement()
                                                ->firstSelectedOptionElement();
                String* text = option ? option->text() : nullptr;
                out = text ? axCollapseWhitespace(toUTF8(text)) : std::string();
                return true;
            }
            if (isTextField(role) && isTextControl(element, tag)) {
                String* value = element->asHTMLFormControl()->value();
                out = toUTF8(value);
                return true;
            }
            if (tag == "input" && (role == "slider" || role == "spinbutton")) {
                std::string valueText = axAttribute(element, "aria-valuetext");
                if (valueText.empty()) {
                    valueText = toUTF8(element->asHTMLFormControl()->value());
                }
                out = valueText;
                return true;
            }
            return false;
        }

        // accname 2D: the markup of the element itself.
        TextResult nativeTextAlternative(Element* element,
                                         const std::string& tag,
                                         const std::string& role)
        {
            TextResult result;
            std::string type = tag == "input"
                                   ? lowered(axAttribute(element, "type"))
                                   : std::string();

            if (tag == "option") {
                std::string label = axAttribute(element, "label");
                if (!label.empty()) {
                    result.from = NameFrom::Contents;
                    result.text = label;
                    return result;
                }
            }

            if (isLabelable(element, tag)) {
                std::vector<Element*> labels = labelsOf(element);
                if (!labels.empty()) {
                    std::string text =
                        axCollapseWhitespace(textFromElements(false, labels));
                    result.from = NameFrom::RelatedElement;
                    if (!text.empty()) {
                        result.text = text;
                        return result;
                    }
                }
            }

            if (tag == "input" &&
                (type == "button" || type == "submit" || type == "reset")) {
                result.from = NameFrom::Value;
                if (axHasAttribute(element, "value")) {
                    result.text = axAttribute(element, "value");
                    return result;
                }
                // A button with no value shows a default label. It is the
                // text the button is drawn with.
                if (type == "submit") {
                    result.from = NameFrom::Contents;
                    result.text = kSubmitLabel;
                } else if (type == "reset") {
                    result.from = NameFrom::Contents;
                    result.text = kResetLabel;
                }
                return result;
            }

            if (tag == "input" && type == "image") {
                bool altPresent = axHasAttribute(element, "alt");
                std::string alt = axAttribute(element, "alt");
                result.from = altPresent && alt.empty()
                                  ? NameFrom::AttributeExplicitlyEmpty
                                  : NameFrom::Attribute;
                if (!alt.empty()) {
                    result.text = alt;
                    return result;
                }
                result.from = NameFrom::Attribute;
                if (axHasAttribute(element, "value")) {
                    std::string value = axAttribute(element, "value");
                    if (!value.empty()) {
                        result.text = value;
                        return result;
                    }
                }
                std::string title = tooltip(element);
                if (!title.empty()) {
                    result.from = NameFrom::Title;
                    result.text = title;
                    return result;
                }
                result.from = NameFrom::Value;
                result.text = kSubmitLabel;
                return result;
            }

            if (isTextControl(element, tag)) {
                std::string title = tooltip(element);
                if (!title.empty()) {
                    result.from = NameFrom::Title;
                    result.text = title;
                    return result;
                }
                std::string placeholder = axAttribute(element, "placeholder");
                result.from = NameFrom::Placeholder;
                if (!placeholder.empty()) {
                    result.text = placeholder;
                    return result;
                }
            }
            if (isTextField(role)) {
                std::string placeholder =
                    axAttribute(element, "aria-placeholder");
                result.from = NameFrom::Placeholder;
                if (!placeholder.empty()) {
                    result.text = placeholder;
                }
                return result;
            }

            if (tag == "img" || tag == "area") {
                bool altPresent = axHasAttribute(element, "alt");
                std::string alt = axAttribute(element, "alt");
                result.from = altPresent && alt.empty()
                                  ? NameFrom::AttributeExplicitlyEmpty
                                  : NameFrom::Attribute;
                result.text = alt;
                return result;
            }

            if (element->isSVGElement()) {
                result.from = NameFrom::RelatedElement;
                result.text = svgTitle(element);
            }
            return result;
        }

        // The accumulated text of the children, with a space between two
        // children when the text would otherwise run together.
        std::string textFromDescendants(Node* parent, Element* labelRoot)
        {
            std::string accumulated;
            Node* previous = nullptr;
            NameFrom lastUsedFrom = NameFrom::None;
            const size_t kMaxDescendants = 100;
            for (Node* child : childrenOf(parent)) {
                if (m_visited.size() > kMaxDescendants) {
                    break;
                }

                Element* childElement =
                    child->isElement() ? child->asElement() : nullptr;
                std::string childTag =
                    childElement ? tagOf(childElement) : std::string();
                if (childElement && isHiddenForTextAlternative(
                                        childElement, childTag, labelRoot)) {
                    continue;
                }

                TextResult result;
                if (childElement && axHasPresentationalRole(childElement)) {
                    result.from = NameFrom::Contents;
                    result.text = textFromDescendants(child, labelRoot);
                } else {
                    result = recursiveTextAlternative(child, labelRoot);
                }

                // A space after a space is collapsed, as it is when the text
                // is drawn. An image between the two keeps them apart.
                if (!accumulated.empty() && isHtmlSpace(accumulated.back()) &&
                    !(previous && isAtomicInline(previous))) {
                    size_t skip = 0;
                    while (skip < result.text.size() &&
                           isHtmlSpace(result.text[skip])) {
                        ++skip;
                    }
                    result.text.erase(0, skip);
                }

                if (!result.text.empty() && previous && !accumulated.empty() &&
                    !isHtmlSpace(accumulated.back()) &&
                    !isHtmlSpace(result.text.front()) &&
                    shouldInsertSpace(previous, child, lastUsedFrom,
                                      result.from)) {
                    accumulated.push_back(' ');
                }
                accumulated += result.text;
                previous = child;
                if (!result.text.empty()) {
                    lastUsedFrom = result.from;
                }
            }
            return accumulated;
        }

        bool isControlNode(Node* node)
        {
            if (!node->isElement()) {
                return false;
            }
            Element* element = node->asElement();
            return isControlRole(axRoleForElement(element, tagOf(element)));
        }

        bool shouldInsertSpace(Node* previous, Node* next, NameFrom lastFrom,
                               NameFrom from)
        {
            if (!isInSameBlockFlow(next, previous)) {
                return true;
            }
            if (isAtomicInline(next) || isAtomicInline(previous)) {
                return true;
            }
            if (isSpaceNeededForNameFrom(lastFrom) ||
                isSpaceNeededForNameFrom(from)) {
                return true;
            }
            return isControlNode(previous) || isControlNode(next);
        }

        TextResult textAlternative(Node* node, bool recursive,
                                   Element* labelRoot)
        {
            if (node->isText()) {
                return textNodeAlternative(node);
            }
            Element* element = node->asElement();
            std::string tag = tagOf(element);
            std::string role =
                recursive ? axRoleForElement(element, tag) : m_topRole;
            TextResult result;

            if (tag == "br" && element->frame()) {
                m_visited.insert(element);
                result.from = NameFrom::Contents;
                result.text = "\n";
                return result;
            }

            // accname 2E applies to a control inside another element's
            // name, but not to the control being named.
            if (recursive && !m_visited.count(element)) {
                std::string value;
                if (valueContribution(element, tag, role, value)) {
                    result.from = NameFrom::Value;
                    result.text = value;
                    return result;
                }
            }

            bool alreadyVisited = m_visited.count(element) != 0;
            m_visited.insert(element);

            if (isHiddenForTextAlternative(element, tag, labelRoot)) {
                return result;
            }

            // accname 2B. The elements an aria-labelledby names count by
            // their own content and aria-label, not by what they point at.
            if (!labelRoot && !alreadyVisited) {
                std::string idRefs = axLabelledByAttribute(element);
                if (!idRefs.empty()) {
                    std::string text = textFromElements(
                        true, elementsFromIdRefs(element, idRefs));
                    if (!onlyWhitespace(text)) {
                        result.from = NameFrom::RelatedElement;
                        result.text = text;
                        return result;
                    }
                }
            }

            // accname 2C.
            std::string ariaLabel = axAttribute(element, "aria-label");
            if (!ariaLabel.empty()) {
                result.from = NameFrom::Attribute;
                result.text = ariaLabel;
                return result;
            }

            // accname 2D.
            TextResult native = nativeTextAlternative(element, tag, role);
            if (!native.text.empty()) {
                return native;
            }
            bool explicitlyEmpty =
                native.from == NameFrom::AttributeExplicitlyEmpty;

            // accname 2F and 2G.
            if (shouldIncludeContents(element, tag, role, recursive,
                                      labelRoot)) {
                result.from = NameFrom::Contents;
                result.text = textFromDescendants(element, labelRoot);
                if (!result.text.empty()) {
                    return result;
                }
            }

            // accname 2I. An image with an empty alt text is decorative, so
            // its title is not a name.
            bool prohibited = !recursive && axRoleIsNameProhibited(role);
            if ((labelRoot || !prohibited) &&
                !(explicitlyEmpty && (tag == "img" || tag == "area"))) {
                std::string title = tooltip(element);
                if (!title.empty()) {
                    result.from = NameFrom::Title;
                    result.text = title;
                    return result;
                }
            }

            result.from = explicitlyEmpty ? NameFrom::AttributeExplicitlyEmpty
                                          : NameFrom::None;
            result.text.clear();
            return result;
        }

        bool shouldIncludeContents(Element* element, const std::string& tag,
                                   const std::string& role, bool recursive,
                                   Element* labelRoot)
        {
            if (!labelRoot) {
                bool supported =
                    recursive
                        ? contributesContentsToAncestor(element, tag, role)
                        : (axRoleSupportsNameFromContents(role) ||
                           role == "DisclosureTriangle" ||
                           role == "disclosureTriangle");
                if (!supported) {
                    return false;
                }
            }
            if (tag == "select") {
                return false;
            }
            // A text field's value is not part of its own name, unless
            // aria-labelledby points at the field itself.
            if (isTextField(role) && labelRoot != element) {
                return false;
            }
            return true;
        }

        std::set<Node*> m_visited;
        std::string m_topRole;
    };

} // namespace

std::string axCollapseWhitespace(const std::string& text)
{
    std::string out;
    bool previousWasSpace = true; // trims the leading run
    for (char c : text) {
        bool space =
            (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
        if (space) {
            if (!previousWasSpace) {
                out.push_back(' ');
            }
            previousWasSpace = true;
        } else {
            out.push_back(c);
            previousWasSpace = false;
        }
    }
    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

std::string axAttribute(Element* element, const char* name)
{
    Optional<String*> value =
        element->getAttribute(String::fromUTF8(name, strlen(name)));
    return value.hasValue() ? axCollapseWhitespace(toUTF8(value.value()))
                            : std::string();
}

bool axHasAttribute(Element* element, const char* name)
{
    return element->getAttribute(String::fromUTF8(name, strlen(name)))
        .hasValue();
}

bool axHasAnyAriaAttribute(Element* element)
{
    size_t count = element->attributeCount();
    for (size_t i = 0; i < count; i++) {
        String* name = element->getAssuredAttributeName(i).localName();
        if (!name) {
            continue;
        }
        std::string local = lowered(name->toUTF8NonGCString());
        if (local.compare(0, 5, "aria-") == 0) {
            return true;
        }
    }
    return false;
}

std::string axTextForNode(Node* node)
{
    Optional<String*> value = node->textContent();
    std::string raw = value.hasValue() ? toUTF8(value.value()) : std::string();
    std::string normalized = axCollapseWhitespace(raw);
    if (normalized.empty()) {
        return normalized;
    }
    // The space that separated this text from its neighbor is part of the
    // rendered text, so one is kept at each end that had one.
    if (std::isspace(static_cast<unsigned char>(raw.front()))) {
        normalized.insert(normalized.begin(), ' ');
    }
    if (std::isspace(static_cast<unsigned char>(raw.back()))) {
        normalized.push_back(' ');
    }
    return normalized;
}

std::string axDocumentName(Document* document)
{
    return toUTF8(document->title());
}

std::string axLabelledByAttribute(Element* element)
{
    std::string value = axAttribute(element, "aria-labelledby");
    if (value.empty()) {
        value = axAttribute(element, "aria-labeledby");
    }
    return value;
}

std::string axTextFromIdRefs(Element* element, const std::string& idRefs)
{
    NameComputer computer;
    return computer.fromIdRefs(element, idRefs);
}

std::vector<AXNameSource> axNameSources(Element* element,
                                        const std::string& role)
{
    NameComputer computer;
    return computer.sources(element, role);
}

std::string axNameForElement(Element* element, const std::string& tag,
                             const std::string& role)
{
    NameComputer computer;
    return computer.compute(element, role);
}

} // namespace Starfish

#endif
