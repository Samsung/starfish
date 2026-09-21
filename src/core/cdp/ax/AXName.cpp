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
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"

#include <cctype>
#include <cstring>
#include <set>

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

    std::string subtreeText(Node* node)
    {
        Optional<String*> text = node->textContent();
        return axCollapseWhitespace(text.hasValue() ? toUTF8(text.value())
                                                    : std::string());
    }

    // accname 2B walks the elements an idref list names. A node already on the
    // path contributes nothing: re-entering it would loop.
    std::string textAlternativeOf(Element* element, std::set<Node*>& visited);

    std::string textFromIdRefs(Element* element, const std::string& idRefs,
                               std::set<Node*>& visited)
    {
        Document* document = element->document();
        if (!document || idRefs.empty()) {
            return std::string();
        }

        std::string result;
        size_t start = 0;
        while (start < idRefs.size()) {
            size_t end = idRefs.find(' ', start);
            std::string id = idRefs.substr(start, end - start);
            if (!id.empty()) {
                Element* referenced = document->getElementById(
                    String::fromUTF8(id.data(), id.size()));
                if (referenced) {
                    std::string text = textAlternativeOf(referenced, visited);
                    if (!text.empty()) {
                        if (!result.empty()) {
                            result.push_back(' ');
                        }
                        result += text;
                    }
                }
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        return axCollapseWhitespace(result);
    }

    std::string textAlternativeOf(Element* element, std::set<Node*>& visited)
    {
        // accname step 2A: a node already being visited ends this branch. It
        // must not fall back to the element's content, which is how a cycle
        // would otherwise reappear in the name.
        if (visited.find(element) != visited.end()) {
            return std::string();
        }
        visited.insert(element);

        std::string referenced =
            textFromIdRefs(element, axLabelledByAttribute(element), visited);
        if (!referenced.empty()) {
            return referenced;
        }

        std::string ariaLabel = axAttribute(element, "aria-label");
        return ariaLabel.empty() ? subtreeText(element) : ariaLabel;
    }

    // The text of every <label> that names this control: an ancestor <label>,
    // and any <label for="..."> that points at its id.
    std::string nativeLabelText(Element* element)
    {
        std::string result;
        for (Element* parent = element->parentElement(); parent;
             parent = parent->parentElement()) {
            if (lowered(toUTF8(parent->localName())) == "label") {
                result = subtreeText(parent);
                break;
            }
        }

        std::string id = axAttribute(element, "id");
        Document* document = element->document();
        if (id.empty() || !document) {
            return result;
        }

        Traverse::traverse(document, [&](Node* node) {
            if (!node->isElement()) {
                return;
            }
            Element* candidate = node->asElement();
            if (lowered(toUTF8(candidate->localName())) != "label" ||
                axAttribute(candidate, "for") != id) {
                return;
            }
            std::string text = subtreeText(candidate);
            if (!text.empty()) {
                if (!result.empty()) {
                    result.push_back(' ');
                }
                result += text;
            }
        });
        return axCollapseWhitespace(result);
    }

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
    std::set<Node*> visited;
    visited.insert(element);
    return textFromIdRefs(element, idRefs, visited);
}

std::string axNameForElement(Element* element, const std::string& tag,
                             const std::string& role)
{
    std::string referenced =
        axTextFromIdRefs(element, axLabelledByAttribute(element));
    if (!referenced.empty()) {
        return referenced;
    }

    std::string ariaLabel = axAttribute(element, "aria-label");
    if (!ariaLabel.empty()) {
        return ariaLabel;
    }

    if (tag == "button" || tag == "input" || tag == "meter" ||
        tag == "output" || tag == "progress" || tag == "select" ||
        tag == "textarea") {
        std::string nativeLabel = nativeLabelText(element);
        if (!nativeLabel.empty()) {
            return nativeLabel;
        }
    }
    if (tag == "img") {
        return axAttribute(element, "alt");
    }
    if (tag == "input") {
        std::string type = lowered(axAttribute(element, "type"));
        if (type == "button" || type == "submit" || type == "reset") {
            std::string value = axAttribute(element, "value");
            return value.empty() ? axAttribute(element, "placeholder") : value;
        }
        std::string placeholder = axAttribute(element, "placeholder");
        if (!placeholder.empty()) {
            return placeholder;
        }
        return axAttribute(element, "title");
    }
    if (axRoleSupportsNameFromContents(role)) {
        return subtreeText(element);
    }

    std::string title = axAttribute(element, "title");
    if (!title.empty() && !axRoleIsNameProhibited(role)) {
        return title;
    }
    return std::string();
}

} // namespace Starfish

#endif
