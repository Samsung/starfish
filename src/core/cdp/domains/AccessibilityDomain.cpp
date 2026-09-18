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
#include "AccessibilityDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../NodeRegistry.h"
#include "PageDomain.h"
#include "core/page/WebView.h"
#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/Traverse.h"
#include "core/page/BrowsingContext.h"

#include <functional>
#include <string>
#include <rapidjson/document.h>
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/style/ComputedStyle.h"
#include "../RemoteObject.h"
#include "binding/ScriptWrappable.h"
#include "EscargotPublic.h"

#include "rapidjson/document.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <cctype>
#include <unordered_map>

namespace Starfish {

static std::string toUTF8(String* s)
{
    return s ? s->toUTF8NonGCString() : std::string();
}

// Read attribute by literal name; returns empty string if absent.
static std::string attr(Element* el, const char* name)
{
    Optional<String*> v =
        el->getAttribute(String::fromUTF8(name, strlen(name)));
    return v.hasValue() ? toUTF8(v.value()) : std::string();
}

static bool hasAttr(Element* el, const char* name)
{
    return el->getAttribute(String::fromUTF8(name, strlen(name))).hasValue();
}

static std::string lower(std::string s)
{
    for (char& c : s) {
        c = (char)std::tolower((unsigned char)c);
    }
    return s;
}

// Collapse ASCII whitespace runs to single spaces and trim ends.
static std::string collapse(const std::string& in)
{
    std::string out;
    bool prevSpace = true; // trims leading
    for (char c : in) {
        bool sp =
            (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
        if (sp) {
            if (!prevSpace) {
                out.push_back(' ');
            }
            prevSpace = true;
        } else {
            out.push_back(c);
            prevSpace = false;
        }
    }
    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

static Node* nodeFromObjectId(CDPDispatcher* dispatcher, const char* objectId)
{
    if (!objectId || strncmp(objectId, "OBJ-", 4) != 0) {
        return nullptr;
    }

    char* end = nullptr;
    long id = strtol(objectId + 4, &end, 10);
    if (end == objectId + 4 || *end != '\0' || id <= 0) {
        return nullptr;
    }

    Escargot::ObjectRef* object =
        dispatcher->remoteObjectStore()->lookup(static_cast<int>(id));
    if (!object || !object->extraData()) {
        return nullptr;
    }

    ScriptWrappable* wrappable =
        static_cast<ScriptWrappable*>(object->extraData());
    return wrappable->isNode() ? wrappable->asNode() : nullptr;
}

static Node* parentForAXTraversal(Node* node)
{
    if (node->isShadowRoot()) {
        return node->asShadowRoot()->host();
    }
    Node* parent = node->parentNode();
    return parent && parent->isShadowRoot() ? parent->asShadowRoot()->host()
                                            : parent;
}

// Map an ARIA role string to Chromium's internal chromeRole number.
// Values match Chromium's ax_role.pdl enum (ui/accessibility/ax_enums.h).
static int chromeRoleForRole(const std::string& role)
{
    static const std::unordered_map<std::string, int> mapping = {
        { "RootWebArea", 144 },   { "article", 5 },
        { "main", 118 },          { "paragraph", 133 },
        { "heading", 96 },        { "button", 9 },
        { "link", 110 },          { "image", 99 },
        { "textbox", 170 },       { "StaticText", 158 },
        { "checkbox", 14 },       { "combobox", 209 },
        { "banner", 7 },          { "contentinfo", 26 },
        { "complementary", 22 },  { "dialog", 35 },
        { "alertDialog", 3 },     { "figure", 84 },
        { "caption", 11 },        { "generic", 88 },
        { "list", 111 },          { "listitem", 115 },
        { "table", 167 },         { "row", 145 },
        { "cell", 13 },           { "columnheader", 19 },
        { "form", 87 },           { "region", 143 },
        { "Iframe", 97 },         { "radio", 141 },
        { "radiogroup", 142 },    { "slider", 155 },
        { "menu", 122 },          { "menuitem", 124 },
        { "option", 113 },        { "alert", 2 },
        { "progressbar", 140 },   { "searchbox", 153 },
        { "spinbutton", 156 },    { "status", 159 },
        { "switch", 163 },        { "disclosureTriangle", 37 },
        { "treeitem", 180 },      { "navigation", 130 },
        { "InlineTextBox", 101 }, { "group", 93 },
        { "tab", 164 },           { "tablist", 165 },
        { "tabPanel", 166 },
    };
    auto it = mapping.find(role);
    return it != mapping.end() ? it->second : 0;
}

static bool isNotRendered(Element* element)
{
    if (hasAttr(element, "hidden")) {
        return true;
    }

    ComputedStyle* style = element->style();
    if (style && style->display() == DisplayValue::NoneDisplayValue) {
        return true;
    }

    Element* parent = element->parentElement();
    return parent && isNotRendered(parent);
}

static bool isHiddenFromAccessibility(Element* element)
{
    if (isNotRendered(element) ||
        lower(collapse(attr(element, "aria-hidden"))) == "true") {
        return true;
    }

    ComputedStyle* style = element->style();
    if (style &&
        style->visibility() != VisibilityValue::VisibleVisibilityValue) {
        return true;
    }

    Element* parent = element->parentElement();
    return parent && isHiddenFromAccessibility(parent);
}

static bool hasAriaHiddenAncestor(Element* element)
{
    for (Element* parent = element->parentElement(); parent;
         parent = parent->parentElement()) {
        if (lower(collapse(attr(parent, "aria-hidden"))) == "true") {
            return true;
        }
    }
    return false;
}

// Map an element to an ARIA role. Explicit role attribute wins. Returns "" for
// elements that carry no semantic role (folded as "generic"/ignored).
static std::string roleForElement(Element* el, const std::string& tag)
{
    std::string explicitRole = lower(collapse(attr(el, "role")));
    if (!explicitRole.empty()) {
        // role may be a token list; take first token.
        size_t sp = explicitRole.find(' ');
        explicitRole =
            sp == std::string::npos ? explicitRole : explicitRole.substr(0, sp);
        if (explicitRole == "none" || explicitRole == "presentation") {
            return std::string();
        }
        return explicitRole;
    }

    if (tag == "button") {
        return "button";
    }
    if (tag == "iframe") {
        return "Iframe";
    }
    if (tag == "a") {
        return hasAttr(el, "href") ? "link" : "generic";
    }
    if (tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" ||
        tag == "h5" || tag == "h6") {
        return "heading";
    }
    if (tag == "img") {
        return "image";
    }
    if (tag == "input") {
        std::string type = lower(attr(el, "type"));
        if (type == "checkbox") {
            return "checkbox";
        }
        if (type == "radio") {
            return "radio";
        }
        if (type == "button" || type == "submit" || type == "reset") {
            return "button";
        }
        if (type == "range") {
            return "slider";
        }
        if (type == "hidden") {
            return "";
        }
        // text, search, email, url, tel, password, number, "" ...
        return "textbox";
    }
    if (tag == "textarea") {
        return "textbox";
    }
    if (tag == "select") {
        return "combobox";
    }
    if (tag == "nav") {
        return "navigation";
    }
    if (tag == "main") {
        return "main";
    }
    if (tag == "article") {
        return "article";
    }
    if (tag == "section") {
        return "region";
    }
    if (tag == "header") {
        return "banner";
    }
    if (tag == "footer") {
        return "contentinfo";
    }
    if (tag == "aside") {
        return "complementary";
    }
    if (tag == "ul" || tag == "ol") {
        return "list";
    }
    if (tag == "li") {
        return "listitem";
    }
    if (tag == "p") {
        return "paragraph";
    }
    if (tag == "figure") {
        return "figure";
    }
    if (tag == "figcaption") {
        return "caption";
    }
    if (tag == "table") {
        return "table";
    }
    if (tag == "tr") {
        return "row";
    }
    if (tag == "td") {
        return "cell";
    }
    if (tag == "th") {
        return "columnheader";
    }
    if (tag == "form") {
        return "form";
    }
    return "generic";
}

// Direct text-content of element children (own text nodes only), collapsed.
static std::string directText(Element* el)
{
    std::string out;
    for (Node* c = el->firstChild(); c; c = c->nextSibling()) {
        if (c->nodeType() == Node::TEXT_NODE) {
            Optional<String*> t = c->textContent();
            if (t.hasValue()) {
                out += toUTF8(t.value());
            }
        }
    }
    return collapse(out);
}

// All descendant text, collapsed. Used for name computation of containers like
// button/link/heading.
static std::string subtreeText(Node* node)
{
    Optional<String*> t = node->textContent();
    return collapse(t.hasValue() ? toUTF8(t.value()) : std::string());
}

static std::string labelledByAttribute(Element* element)
{
    std::string value = attr(element, "aria-labelledby");
    if (value.empty()) {
        value = attr(element, "aria-labeledby");
    }
    return collapse(value);
}

static std::string referencedTextAlternative(Element* element,
                                             GCUnorderedSet<Node*>& visited);

static std::string textFromIdRefs(Element* element, const std::string& idRefs,
                                  GCUnorderedSet<Node*>& visited)
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
                std::string text =
                    referencedTextAlternative(referenced, visited);
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
    return collapse(result);
}

static std::string referencedTextAlternative(Element* element,
                                             GCUnorderedSet<Node*>& visited)
{
    if (visited.find(element) != visited.end()) {
        return subtreeText(element);
    }
    visited.insert(element);

    std::string labelledBy = labelledByAttribute(element);
    std::string referenced = textFromIdRefs(element, labelledBy, visited);
    if (!referenced.empty()) {
        return referenced;
    }

    std::string ariaLabel = collapse(attr(element, "aria-label"));
    return ariaLabel.empty() ? subtreeText(element) : ariaLabel;
}

static std::string nativeLabelText(Element* element)
{
    std::string result;
    for (Element* parent = element->parentElement(); parent;
         parent = parent->parentElement()) {
        if (lower(toUTF8(parent->localName())) == "label") {
            result = subtreeText(parent);
            break;
        }
    }

    std::string id = attr(element, "id");
    Document* document = element->document();
    if (id.empty() || !document) {
        return result;
    }

    Traverse::traverse(document, [&](Node* node) {
        if (!node->isElement()) {
            return;
        }
        Element* candidate = node->asElement();
        if (lower(toUTF8(candidate->localName())) != "label" ||
            attr(candidate, "for") != id) {
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
    return collapse(result);
}

static std::string textForNode(Node* node)
{
    Optional<String*> value = node->textContent();
    std::string text = value.hasValue() ? toUTF8(value.value()) : std::string();
    std::string normalized = collapse(text);
    if (normalized.empty()) {
        return normalized;
    }
    if (!text.empty() && std::isspace((unsigned char)text.front())) {
        normalized.insert(normalized.begin(), ' ');
    }
    if (!text.empty() && std::isspace((unsigned char)text.back())) {
        normalized.push_back(' ');
    }
    return normalized;
}

// These role groups come from the ARIA role definitions consumed by
// Chromium's RoleSupportsNameFromContents() and RoleIsNameProhibited().
static bool roleSupportsNameFromContents(const std::string& role)
{
    return role == "button" || role == "cell" || role == "checkbox" ||
           role == "columnheader" || role == "gridcell" || role == "heading" ||
           role == "link" || role == "menuitem" || role == "menuitemcheckbox" ||
           role == "menuitemradio" || role == "option" || role == "radio" ||
           role == "row" || role == "rowheader" || role == "sectionhead" ||
           role == "switch" || role == "tab" || role == "tooltip" ||
           role == "treeitem";
}

static bool roleIsNameProhibited(const std::string& role)
{
    return role == "caption" || role == "code" || role == "definition" ||
           role == "deletion" || role == "emphasis" || role == "generic" ||
           role == "insertion" || role == "mark" || role == "none" ||
           role == "paragraph" || role == "presentation" || role == "strong" ||
           role == "subscript" || role == "suggestion" ||
           role == "superscript" || role == "term" || role == "time";
}

// Compute the accessible name using the sources exercised by CDP AX queries.
static std::string nameForElement(Element* el, const std::string& tag,
                                  const std::string& role)
{
    GCUnorderedSet<Node*> visited;
    visited.insert(el);
    std::string labelledBy = labelledByAttribute(el);
    std::string referenced = textFromIdRefs(el, labelledBy, visited);
    if (!referenced.empty()) {
        return referenced;
    }

    std::string ariaLabel = collapse(attr(el, "aria-label"));
    if (!ariaLabel.empty()) {
        return ariaLabel;
    }

    if (tag == "button" || tag == "input" || tag == "meter" ||
        tag == "output" || tag == "progress" || tag == "select" ||
        tag == "textarea") {
        std::string nativeLabel = nativeLabelText(el);
        if (!nativeLabel.empty()) {
            return nativeLabel;
        }
    }
    if (tag == "img") {
        return collapse(attr(el, "alt"));
    }
    if (tag == "input") {
        std::string type = lower(attr(el, "type"));
        if (type == "button" || type == "submit" || type == "reset") {
            std::string v = collapse(attr(el, "value"));
            return v.empty() ? collapse(attr(el, "placeholder")) : v;
        }
        std::string ph = collapse(attr(el, "placeholder"));
        if (!ph.empty()) {
            return ph;
        }
        std::string title = collapse(attr(el, "title"));
        return title;
    }
    if (roleSupportsNameFromContents(role)) {
        return subtreeText(el);
    }

    std::string title = collapse(attr(el, "title"));
    if (!title.empty() && !roleIsNameProhibited(role)) {
        return title;
    }
    return std::string();
}

namespace {

    struct AXBuilder {
        NodeRegistry* reg;
        PageDomain* page;
        rapidjson::Value* nodes;
        rapidjson::Document::AllocatorType* alloc;

        std::string axId(Node* node)
        {
            return "ax-" + std::to_string(reg->getOrCreate(node));
        }

        bool shouldAddNode(Node* node)
        {
            if (node->nodeType() == Node::TEXT_NODE) {
                return !textForNode(node).empty();
            }
            if (node->nodeType() != Node::ELEMENT_NODE) {
                return false;
            }

            Element* element = node->asElement();
            return lower(toUTF8(element->localName())) != "head" &&
                   !isHiddenFromAccessibility(element);
        }

        bool isIgnored(Node* node)
        {
            if (node->nodeType() != Node::ELEMENT_NODE) {
                return false;
            }

            Element* element = node->asElement();
            std::string tag = lower(toUTF8(element->localName()));
            return isHiddenFromAccessibility(element) ||
                   roleForElement(element, tag).empty() || tag == "html" ||
                   tag == "head" || tag == "body";
        }

        bool hasAXNode(Node* node)
        {
            if (!node->isElement()) {
                return node->nodeType() == Node::TEXT_NODE;
            }
            Element* element = node->asElement();
            std::string tag = lower(toUTF8(element->localName()));
            return tag != "head" && !isNotRendered(element);
        }

        rapidjson::Value relatedNodes(Element* element,
                                      const std::string& idRefs,
                                      bool includeText)
        {
            rapidjson::Value related(rapidjson::kArrayType);
            Document* document = element->document();
            if (!document) {
                return related;
            }

            size_t start = 0;
            while (start < idRefs.size()) {
                size_t end = idRefs.find(' ', start);
                std::string id = idRefs.substr(start, end - start);
                if (!id.empty()) {
                    Element* target = document->getElementById(
                        String::fromUTF8(id.data(), id.size()));
                    if (target) {
                        rapidjson::Value item(rapidjson::kObjectType);
                        item.AddMember("backendDOMNodeId",
                                       reg->getOrCreate(target), *alloc);
                        item.AddMember(
                            "idref",
                            rapidjson::Value(id.c_str(), id.size(), *alloc),
                            *alloc);
                        if (includeText) {
                            GCUnorderedSet<Node*> visited;
                            std::string text =
                                referencedTextAlternative(target, visited);
                            item.AddMember("text",
                                           rapidjson::Value(text.c_str(),
                                                            text.size(),
                                                            *alloc),
                                           *alloc);
                        }
                        related.PushBack(item, *alloc);
                    }
                }
                if (end == std::string::npos) {
                    break;
                }
                start = end + 1;
            }
            return related;
        }

        void addRoleAndName(rapidjson::Value& node, const std::string& role,
                            const std::string& name, Element* el = nullptr)
        {
            rapidjson::Value roleValue(rapidjson::kObjectType);
            const char* roleType =
                role == "StaticText" || role == "InlineTextBox" ? "internalRole"
                                                                : "role";
            roleValue.AddMember(
                "type", rapidjson::Value(roleType, strlen(roleType), *alloc),
                *alloc);
            roleValue.AddMember(
                "value", rapidjson::Value(role.c_str(), role.size(), *alloc),
                *alloc);
            node.AddMember("role", roleValue, *alloc);

            // chromeRole (internal role number)
            int cr = chromeRoleForRole(role);
            rapidjson::Value chromeRoleValue(rapidjson::kObjectType);
            chromeRoleValue.AddMember("type", "internalRole", *alloc);
            chromeRoleValue.AddMember("value", cr, *alloc);
            node.AddMember("chromeRole", chromeRoleValue, *alloc);

            // name with sources
            rapidjson::Value nameValue(rapidjson::kObjectType);
            nameValue.AddMember("type", "computedString", *alloc);
            nameValue.AddMember(
                "value", rapidjson::Value(name.c_str(), name.size(), *alloc),
                *alloc);

            // Build name sources: aria-labelledby, aria-label, contents, title.
            // The sources list is always present; `superseded` marks sources
            // that were not used (a higher-priority source provided the name).
            rapidjson::Value sources(rapidjson::kArrayType);
            std::string labelBy = el ? labelledByAttribute(el) : std::string();
            bool hasLabelBy = !labelBy.empty();
            bool hasLabel = el && hasAttr(el, "aria-label");
            bool hasTitle = el && hasAttr(el, "title");
            bool supportsNameFromContent = roleSupportsNameFromContents(role);
            bool nameFromContent = supportsNameFromContent && !name.empty() &&
                                   !hasLabel && !hasLabelBy;

            bool usedLabelBy = false;
            if (el && hasLabelBy) {
                GCUnorderedSet<Node*> visited;
                visited.insert(el);
                usedLabelBy = !textFromIdRefs(el, labelBy, visited).empty();
            }
            bool usedLabel = !usedLabelBy && hasLabel &&
                             !collapse(attr(el, "aria-label")).empty();
            bool usedContent = !usedLabelBy && !usedLabel && nameFromContent;
            bool usedTitle =
                !usedLabelBy && !usedLabel && !usedContent && hasTitle;

            if (el) {
                // aria-labelledby source
                {
                    rapidjson::Value src(rapidjson::kObjectType);
                    src.AddMember("attribute", "aria-labelledby", *alloc);
                    src.AddMember("type", "relatedElement", *alloc);
                    if (hasLabelBy) {
                        rapidjson::Value attributeValue(rapidjson::kObjectType);
                        attributeValue.AddMember("type", "idrefList", *alloc);
                        attributeValue.AddMember(
                            "value",
                            rapidjson::Value(labelBy.c_str(), labelBy.size(),
                                             *alloc),
                            *alloc);
                        rapidjson::Value related =
                            relatedNodes(el, labelBy, true);
                        if (!related.Empty()) {
                            attributeValue.AddMember("relatedNodes", related,
                                                     *alloc);
                        }
                        src.AddMember("attributeValue", attributeValue, *alloc);
                    }
                    if (usedLabelBy) {
                        rapidjson::Value value(rapidjson::kObjectType);
                        value.AddMember("type", "computedString", *alloc);
                        value.AddMember(
                            "value",
                            rapidjson::Value(name.c_str(), name.size(), *alloc),
                            *alloc);
                        src.AddMember("value", value, *alloc);
                    } else if (hasLabelBy) {
                        src.AddMember("superseded", true, *alloc);
                    }
                    sources.PushBack(src, *alloc);
                }
                // aria-label source
                {
                    rapidjson::Value src(rapidjson::kObjectType);
                    src.AddMember("attribute", "aria-label", *alloc);
                    src.AddMember("type", "attribute", *alloc);
                    if (hasLabel) {
                        std::string label = attr(el, "aria-label");
                        rapidjson::Value attributeValue(rapidjson::kObjectType);
                        attributeValue.AddMember("type", "string", *alloc);
                        attributeValue.AddMember("value",
                                                 rapidjson::Value(label.c_str(),
                                                                  label.size(),
                                                                  *alloc),
                                                 *alloc);
                        src.AddMember("attributeValue", attributeValue, *alloc);
                    }
                    if (usedLabel) {
                        rapidjson::Value value(rapidjson::kObjectType);
                        value.AddMember("type", "computedString", *alloc);
                        value.AddMember(
                            "value",
                            rapidjson::Value(name.c_str(), name.size(), *alloc),
                            *alloc);
                        src.AddMember("value", value, *alloc);
                    } else if (usedLabelBy) {
                        src.AddMember("superseded", true, *alloc);
                    }
                    sources.PushBack(src, *alloc);
                }
                // contents source (only for name-from-content roles)
                if (supportsNameFromContent) {
                    rapidjson::Value src(rapidjson::kObjectType);
                    src.AddMember("type", "contents", *alloc);
                    if (usedContent) {
                        rapidjson::Value val(rapidjson::kObjectType);
                        val.AddMember("type", "computedString", *alloc);
                        val.AddMember(
                            "value",
                            rapidjson::Value(name.c_str(), name.size(), *alloc),
                            *alloc);
                        src.AddMember("value", val, *alloc);
                    }
                    sources.PushBack(src, *alloc);
                }
                // title source
                if (!roleIsNameProhibited(role)) {
                    rapidjson::Value src(rapidjson::kObjectType);
                    src.AddMember("attribute", "title", *alloc);
                    src.AddMember("type", "attribute", *alloc);
                    if (!usedTitle &&
                        (usedLabelBy || usedLabel || usedContent)) {
                        src.AddMember("superseded", true, *alloc);
                    }
                    sources.PushBack(src, *alloc);
                }
            }
            nameValue.AddMember("sources", sources, *alloc);
            node.AddMember("name", nameValue, *alloc);
        }

        void addIgnoredData(rapidjson::Value& node, Element* element,
                            const std::string& tag)
        {
            rapidjson::Value roleValue(rapidjson::kObjectType);
            roleValue.AddMember("type", "role", *alloc);
            roleValue.AddMember("value", "none", *alloc);
            node.AddMember("role", roleValue, *alloc);

            rapidjson::Value chromeRoleValue(rapidjson::kObjectType);
            chromeRoleValue.AddMember("type", "internalRole", *alloc);
            chromeRoleValue.AddMember("value", 0, *alloc);
            node.AddMember("chromeRole", chromeRoleValue, *alloc);

            const char* reason = "uninteresting";
            ComputedStyle* style = element->style();
            if (isNotRendered(element) || tag == "head") {
                reason = "notRendered";
            } else if (lower(collapse(attr(element, "aria-hidden"))) ==
                       "true") {
                reason = "ariaHiddenElement";
            } else if (hasAriaHiddenAncestor(element)) {
                reason = "ariaHiddenSubtree";
            } else if (style && style->visibility() !=
                                    VisibilityValue::VisibleVisibilityValue) {
                reason = "notVisible";
            } else if (hasAttr(element, "role") &&
                       (lower(collapse(attr(element, "role"))) == "none" ||
                        lower(collapse(attr(element, "role"))) ==
                            "presentation")) {
                reason = "presentationalRole";
            }

            rapidjson::Value reasons(rapidjson::kArrayType);
            rapidjson::Value property(rapidjson::kObjectType);
            property.AddMember("name", rapidjson::Value(reason, *alloc),
                               *alloc);
            rapidjson::Value value(rapidjson::kObjectType);
            value.AddMember("type", "boolean", *alloc);
            value.AddMember("value", true, *alloc);
            property.AddMember("value", value, *alloc);
            reasons.PushBack(property, *alloc);
            node.AddMember("ignoredReasons", reasons, *alloc);
        }

        void addRelatedProperty(rapidjson::Value& properties, Element* element,
                                const char* attribute, const char* propertyName,
                                const char* type, bool includeValue,
                                bool includeText)
        {
            std::string idRefs = collapse(attr(element, attribute));
            if (idRefs.empty()) {
                return;
            }
            rapidjson::Value related =
                relatedNodes(element, idRefs, includeText);
            if (related.Empty()) {
                return;
            }

            rapidjson::Value property(rapidjson::kObjectType);
            property.AddMember("name", rapidjson::Value(propertyName, *alloc),
                               *alloc);
            rapidjson::Value value(rapidjson::kObjectType);
            value.AddMember("type", rapidjson::Value(type, *alloc), *alloc);
            if (includeValue) {
                value.AddMember(
                    "value",
                    rapidjson::Value(idRefs.c_str(), idRefs.size(), *alloc),
                    *alloc);
            }
            value.AddMember("relatedNodes", related, *alloc);
            property.AddMember("value", value, *alloc);
            properties.PushBack(property, *alloc);
        }

        void addProperties(rapidjson::Value& node, Element* element,
                           const std::string& tag, const std::string& role)
        {
            rapidjson::Value properties(rapidjson::kArrayType);

            if (hasAttr(element, "tabindex") || tag == "input" ||
                tag == "button" || tag == "select" || tag == "textarea" ||
                (tag == "a" && hasAttr(element, "href"))) {
                rapidjson::Value property(rapidjson::kObjectType);
                property.AddMember("name", "focusable", *alloc);
                rapidjson::Value value(rapidjson::kObjectType);
                value.AddMember("type", "booleanOrUndefined", *alloc);
                value.AddMember("value", true, *alloc);
                property.AddMember("value", value, *alloc);
                properties.PushBack(property, *alloc);
            }

            if (role == "heading" && tag.size() == 2 && tag[0] == 'h' &&
                tag[1] >= '1' && tag[1] <= '6') {
                rapidjson::Value property(rapidjson::kObjectType);
                property.AddMember("name", "level", *alloc);
                rapidjson::Value value(rapidjson::kObjectType);
                value.AddMember("type", "integer", *alloc);
                value.AddMember("value", tag[1] - '0', *alloc);
                property.AddMember("value", value, *alloc);
                properties.PushBack(property, *alloc);
            }

            addRelatedProperty(properties, element, "aria-activedescendant",
                               "activedescendant", "idref", false, false);
            addRelatedProperty(properties, element, "aria-controls", "controls",
                               "idrefList", true, false);
            addRelatedProperty(properties, element, "aria-describedby",
                               "describedby", "idrefList", true, true);
            addRelatedProperty(properties, element, "aria-details", "details",
                               "idrefList", true, false);
            addRelatedProperty(properties, element, "aria-errormessage",
                               "errormessage", "idrefList", true, false);
            addRelatedProperty(properties, element, "aria-flowto", "flowto",
                               "idrefList", true, false);
            addRelatedProperty(properties, element, "aria-labelledby",
                               "labelledby", "nodeList", false, true);
            addRelatedProperty(properties, element, "aria-owns", "owns",
                               "idrefList", true, false);

            node.AddMember("properties", properties, *alloc);
        }

        void addDescriptionAndValue(rapidjson::Value& node, Element* element,
                                    const std::string& tag)
        {
            std::string describedBy =
                collapse(attr(element, "aria-describedby"));
            if (!describedBy.empty()) {
                GCUnorderedSet<Node*> visited;
                visited.insert(element);
                std::string description =
                    textFromIdRefs(element, describedBy, visited);
                if (!description.empty()) {
                    rapidjson::Value value(rapidjson::kObjectType);
                    value.AddMember("type", "computedString", *alloc);
                    value.AddMember("value",
                                    rapidjson::Value(description.c_str(),
                                                     description.size(),
                                                     *alloc),
                                    *alloc);
                    node.AddMember("description", value, *alloc);
                }
            }

            std::string controlValue;
            if (tag == "input" || tag == "textarea" || tag == "select") {
                controlValue = attr(element, "value");
            } else if (hasAttr(element, "aria-valuetext")) {
                controlValue = attr(element, "aria-valuetext");
            } else if (hasAttr(element, "aria-valuenow")) {
                controlValue = attr(element, "aria-valuenow");
            }
            if (!controlValue.empty()) {
                rapidjson::Value value(rapidjson::kObjectType);
                value.AddMember("type", "string", *alloc);
                value.AddMember("value",
                                rapidjson::Value(controlValue.c_str(),
                                                 controlValue.size(), *alloc),
                                *alloc);
                node.AddMember("value", value, *alloc);
            }
        }

        void addDOMNodeWithoutAXNode(Node* domNode, int parentBackendId)
        {
            int backendId = reg->getOrCreate(domNode);
            std::string nodeId = "ax-" + std::to_string(backendId);
            rapidjson::Value node(rapidjson::kObjectType);
            node.AddMember(
                "nodeId",
                rapidjson::Value(nodeId.c_str(), nodeId.size(), *alloc),
                *alloc);
            node.AddMember("ignored", true, *alloc);

            rapidjson::Value role(rapidjson::kObjectType);
            role.AddMember("type", "role", *alloc);
            role.AddMember("value", "none", *alloc);
            node.AddMember("role", role, *alloc);

            rapidjson::Value chromeRole(rapidjson::kObjectType);
            chromeRole.AddMember("type", "internalRole", *alloc);
            chromeRole.AddMember("value", 0, *alloc);
            node.AddMember("chromeRole", chromeRole, *alloc);

            rapidjson::Value reasons(rapidjson::kArrayType);
            rapidjson::Value reason(rapidjson::kObjectType);
            reason.AddMember("name", "notRendered", *alloc);
            rapidjson::Value reasonValue(rapidjson::kObjectType);
            reasonValue.AddMember("type", "boolean", *alloc);
            reasonValue.AddMember("value", true, *alloc);
            reason.AddMember("value", reasonValue, *alloc);
            reasons.PushBack(reason, *alloc);
            node.AddMember("ignoredReasons", reasons, *alloc);
            node.AddMember("backendDOMNodeId", backendId, *alloc);

            if (parentBackendId >= 0) {
                std::string parentId = "ax-" + std::to_string(parentBackendId);
                node.AddMember(
                    "parentId",
                    rapidjson::Value(parentId.c_str(), parentId.size(), *alloc),
                    *alloc);
            }
            nodes->PushBack(node, *alloc);
        }

        void addTextNode(Node* domNode, int parentBackendId,
                         bool includeDescendants, int depth = -1)
        {
            // Check depth before adding text nodes
            if (depth >= 0 && depth <= 0) {
                return;
            }

            std::string name = textForNode(domNode);
            if (name.empty()) {
                return;
            }

            int backendId = reg->getOrCreate(domNode);
            std::string nodeId = "ax-" + std::to_string(backendId);
            std::string inlineId = nodeId + "-inline";

            rapidjson::Value node(rapidjson::kObjectType);
            node.AddMember(
                "nodeId",
                rapidjson::Value(nodeId.c_str(), nodeId.size(), *alloc),
                *alloc);
            node.AddMember("ignored", false, *alloc);
            addRoleAndName(node, "StaticText", name, nullptr);
            node.AddMember("properties",
                           rapidjson::Value(rapidjson::kArrayType), *alloc);
            node.AddMember("backendDOMNodeId", backendId, *alloc);
            std::string parentId = "ax-" + std::to_string(parentBackendId);
            node.AddMember(
                "parentId",
                rapidjson::Value(parentId.c_str(), parentId.size(), *alloc),
                *alloc);
            rapidjson::Value childIds(rapidjson::kArrayType);
            childIds.PushBack(
                rapidjson::Value(inlineId.c_str(), inlineId.size(), *alloc),
                *alloc);
            node.AddMember("childIds", childIds, *alloc);
            nodes->PushBack(node, *alloc);

            // Only add InlineTextBox if depth allows
            if (includeDescendants && (depth < 0 || depth > 1)) {
                rapidjson::Value inlineNode(rapidjson::kObjectType);
                inlineNode.AddMember(
                    "nodeId",
                    rapidjson::Value(inlineId.c_str(), inlineId.size(), *alloc),
                    *alloc);
                inlineNode.AddMember("ignored", false, *alloc);
                addRoleAndName(inlineNode, "InlineTextBox", name, nullptr);
                inlineNode.AddMember("properties",
                                     rapidjson::Value(rapidjson::kArrayType),
                                     *alloc);
                inlineNode.AddMember("backendDOMNodeId", backendId, *alloc);
                inlineNode.AddMember(
                    "parentId",
                    rapidjson::Value(nodeId.c_str(), nodeId.size(), *alloc),
                    *alloc);
                inlineNode.AddMember("childIds",
                                     rapidjson::Value(rapidjson::kArrayType),
                                     *alloc);
                nodes->PushBack(inlineNode, *alloc);
            }
        }

        void addDocumentNode(Document* document, bool includeDescendants,
                             int depth)
        {
            int backendId = reg->getOrCreate(document);
            std::string nodeId = "ax-" + std::to_string(backendId);

            rapidjson::Value node(rapidjson::kObjectType);
            node.AddMember(
                "nodeId",
                rapidjson::Value(nodeId.c_str(), nodeId.size(), *alloc),
                *alloc);
            node.AddMember("ignored", false, *alloc);

            rapidjson::Value role(rapidjson::kObjectType);
            role.AddMember("type", "internalRole", *alloc);
            role.AddMember("value", "RootWebArea", *alloc);
            node.AddMember("role", role, *alloc);

            rapidjson::Value chromeRole(rapidjson::kObjectType);
            chromeRole.AddMember("type", "internalRole", *alloc);
            chromeRole.AddMember("value", chromeRoleForRole("RootWebArea"),
                                 *alloc);
            node.AddMember("chromeRole", chromeRole, *alloc);

            rapidjson::Value name(rapidjson::kObjectType);
            name.AddMember("type", "computedString", *alloc);
            std::string title = toUTF8(document->title());
            name.AddMember(
                "value", rapidjson::Value(title.c_str(), title.size(), *alloc),
                *alloc);
            rapidjson::Value sources(rapidjson::kArrayType);
            rapidjson::Value labelledBy(rapidjson::kObjectType);
            labelledBy.AddMember("attribute", "aria-labelledby", *alloc);
            labelledBy.AddMember("type", "relatedElement", *alloc);
            sources.PushBack(labelledBy, *alloc);
            rapidjson::Value ariaLabel(rapidjson::kObjectType);
            ariaLabel.AddMember("attribute", "aria-label", *alloc);
            ariaLabel.AddMember("type", "attribute", *alloc);
            sources.PushBack(ariaLabel, *alloc);
            rapidjson::Value supersededAriaLabel(rapidjson::kObjectType);
            supersededAriaLabel.AddMember("attribute", "aria-label", *alloc);
            supersededAriaLabel.AddMember("superseded", true, *alloc);
            supersededAriaLabel.AddMember("type", "attribute", *alloc);
            sources.PushBack(supersededAriaLabel, *alloc);
            rapidjson::Value nativeTitle(rapidjson::kObjectType);
            nativeTitle.AddMember("nativeSource", "title", *alloc);
            nativeTitle.AddMember("type", "relatedElement", *alloc);
            if (!title.empty()) {
                rapidjson::Value titleValue(rapidjson::kObjectType);
                titleValue.AddMember("type", "computedString", *alloc);
                titleValue.AddMember(
                    "value",
                    rapidjson::Value(title.c_str(), title.size(), *alloc),
                    *alloc);
                nativeTitle.AddMember("value", titleValue, *alloc);
            }
            sources.PushBack(nativeTitle, *alloc);
            name.AddMember("sources", sources, *alloc);
            node.AddMember("name", name, *alloc);

            rapidjson::Value properties(rapidjson::kArrayType);
            rapidjson::Value focusable(rapidjson::kObjectType);
            focusable.AddMember("name", "focusable", *alloc);
            rapidjson::Value focusableValue(rapidjson::kObjectType);
            focusableValue.AddMember("type", "booleanOrUndefined", *alloc);
            focusableValue.AddMember("value", true, *alloc);
            focusable.AddMember("value", focusableValue, *alloc);
            properties.PushBack(focusable, *alloc);
            rapidjson::Value url(rapidjson::kObjectType);
            url.AddMember("name", "url", *alloc);
            rapidjson::Value urlValue(rapidjson::kObjectType);
            urlValue.AddMember("type", "string", *alloc);
            std::string documentURL = toUTF8(document->urlString());
            urlValue.AddMember("value",
                               rapidjson::Value(documentURL.c_str(),
                                                documentURL.size(), *alloc),
                               *alloc);
            url.AddMember("value", urlValue, *alloc);
            properties.PushBack(url, *alloc);
            node.AddMember("properties", properties, *alloc);
            node.AddMember("backendDOMNodeId", backendId, *alloc);

            std::string frameId =
                page->frameIdForBrowsingContext(document->browsingContext());
            if (!frameId.empty()) {
                node.AddMember(
                    "frameId",
                    rapidjson::Value(frameId.c_str(), frameId.size(), *alloc),
                    *alloc);
            }

            rapidjson::Value childIds(rapidjson::kArrayType);
            Optional<Element*> root = document->documentElement();
            if (root) {
                std::string childId = axId(root.value());
                childIds.PushBack(
                    rapidjson::Value(childId.c_str(), childId.size(), *alloc),
                    *alloc);
            }
            node.AddMember("childIds", childIds, *alloc);
            nodes->PushBack(node, *alloc);

            if (includeDescendants && root && depth != 0) {
                int childDepth = depth < 0 ? -1 : depth - 1;
                addNode(root.value(), backendId, true, childDepth);
            }
        }

        // Add each node before its descendants.
        void addNode(Node* domNode, int parentBackendId,
                     bool includeDescendants = true, int depth = -1,
                     bool forceNameAndRole = false)
        {
            // Check depth before adding: depth=0 means don't add this node
            if (depth == 0) {
                return;
            }

            if (domNode->nodeType() == Node::TEXT_NODE) {
                addTextNode(domNode, parentBackendId, includeDescendants,
                            depth);
                return;
            }
            if (domNode->nodeType() == Node::DOCUMENT_NODE) {
                addDocumentNode(domNode->asDocument(), includeDescendants,
                                depth);
                return;
            }
            if (domNode->nodeType() != Node::ELEMENT_NODE) {
                addDOMNodeWithoutAXNode(domNode, parentBackendId);
                return;
            }

            Element* el = domNode->asElement();
            int backendId = reg->getOrCreate(el);
            std::string nodeId = "ax-" + std::to_string(backendId);

            std::string tag = lower(toUTF8(el->localName()));
            std::string role = roleForElement(el, tag);
            bool ignored = isIgnored(domNode);
            std::string name = nameForElement(el, tag, role);

            rapidjson::Value n(rapidjson::kObjectType);
            n.AddMember("nodeId",
                        rapidjson::Value(nodeId.c_str(), nodeId.size(), *alloc),
                        *alloc);
            n.AddMember("ignored", ignored, *alloc);

            if (!ignored || forceNameAndRole) {
                addRoleAndName(n, role, name, el);
                addDescriptionAndValue(n, el, tag);
            } else {
                addIgnoredData(n, el, tag);
            }

            if (!ignored || forceNameAndRole) {
                addProperties(n, el, tag, role);
            }
            n.AddMember("backendDOMNodeId", backendId, *alloc);
            if (parentBackendId >= 0) {
                std::string pid = "ax-" + std::to_string(parentBackendId);
                n.AddMember("parentId",
                            rapidjson::Value(pid.c_str(), pid.size(), *alloc),
                            *alloc);
            }

            rapidjson::Value childIds(rapidjson::kArrayType);
            for (Node* c = el->firstChild(); c; c = c->nextSibling()) {
                if (shouldAddNode(c)) {
                    std::string childId = axId(c);
                    childIds.PushBack(rapidjson::Value(childId.c_str(),
                                                       childId.size(), *alloc),
                                      *alloc);
                }
            }
            Optional<ShadowRoot*> shadowRoot = el->internalShadowRoot();
            if (shadowRoot) {
                for (Node* c = shadowRoot->firstChild(); c;
                     c = c->nextSibling()) {
                    if (shouldAddNode(c)) {
                        std::string childId = axId(c);
                        childIds.PushBack(rapidjson::Value(childId.c_str(),
                                                           childId.size(),
                                                           *alloc),
                                          *alloc);
                    }
                }
            }
            n.AddMember("childIds", childIds, *alloc);

            nodes->PushBack(n, *alloc);

            if (includeDescendants) {
                // depth: -1 means unlimited, 1 means no children of non-ignored
                // nodes Decrement depth only for non-ignored nodes
                bool shouldRecurse = includeDescendants;
                if (depth >= 0) {
                    shouldRecurse = (depth > 1);
                }
                if (shouldRecurse) {
                    for (Node* c = el->firstChild(); c; c = c->nextSibling()) {
                        if (shouldAddNode(c)) {
                            int childDepth = depth;
                            // Only decrement depth for non-ignored nodes
                            if (childDepth >= 0 && !ignored) {
                                childDepth--;
                            }
                            addNode(c, backendId, true, childDepth);
                        }
                    }
                    if (shadowRoot) {
                        for (Node* c = shadowRoot->firstChild(); c;
                             c = c->nextSibling()) {
                            if (shouldAddNode(c)) {
                                int childDepth = depth;
                                if (childDepth >= 0 && !ignored) {
                                    childDepth--;
                                }
                                addNode(c, backendId, true, childDepth);
                            }
                        }
                    }
                }
            }
        }

        void addNodesMatching(Node* domNode, const std::string* name,
                              const std::string* role)
        {
            bool matches = false;
            if (domNode->nodeType() == Node::DOCUMENT_NODE) {
                Document* document = domNode->asDocument();
                std::string documentName = toUTF8(document->title());
                matches = (!name || documentName == *name) &&
                          (!role || *role == "RootWebArea");
                if (matches) {
                    addDocumentNode(document, false, -1);
                }
            } else if (domNode->nodeType() == Node::ELEMENT_NODE) {
                Element* element = domNode->asElement();
                std::string tag = lower(toUTF8(element->localName()));
                std::string r = roleForElement(element, tag);
                std::string n = nameForElement(element, tag, r);
                matches = (!name || n == *name) && (!role || r == *role);
            } else if (domNode->nodeType() == Node::TEXT_NODE) {
                std::string n = textForNode(domNode);
                matches =
                    (!name || n == *name) && (!role || *role == "StaticText");
            }

            if (matches && domNode->nodeType() != Node::DOCUMENT_NODE) {
                Node* parent = parentForAXTraversal(domNode);
                int parentBackendId = parent ? reg->getOrCreate(parent) : -1;
                addNode(domNode, parentBackendId, false, -1, true);
            }

            for (Node* child = domNode->firstChild(); child;
                 child = child->nextSibling()) {
                addNodesMatching(child, name, role);
            }

            if (domNode->isElement()) {
                Optional<ShadowRoot*> shadowRoot =
                    domNode->asElement()->internalShadowRoot();
                if (shadowRoot) {
                    for (Node* child = shadowRoot->firstChild(); child;
                         child = child->nextSibling()) {
                        addNodesMatching(child, name, role);
                    }
                }
            }
        }

        void addChildren(Node* domNode, int parentBackendId)
        {
            for (Node* child = domNode->firstChild(); child;
                 child = child->nextSibling()) {
                if (!shouldAddNode(child)) {
                    continue;
                }
                addNode(child, parentBackendId,
                        child->nodeType() == Node::TEXT_NODE);
                if (child->isElement() && isIgnored(child)) {
                    addChildren(child, reg->getOrCreate(child));
                }
            }

            if (domNode->isElement()) {
                Optional<ShadowRoot*> shadowRoot =
                    domNode->asElement()->internalShadowRoot();
                if (shadowRoot) {
                    for (Node* child = shadowRoot->firstChild(); child;
                         child = child->nextSibling()) {
                        if (!shouldAddNode(child)) {
                            continue;
                        }
                        addNode(child, parentBackendId,
                                child->nodeType() == Node::TEXT_NODE);
                        if (child->isElement() && isIgnored(child)) {
                            addChildren(child, reg->getOrCreate(child));
                        }
                    }
                }
            }
        }

        void prependChildIdToLastNode(Node* child)
        {
            if (nodes->Empty()) {
                return;
            }
            rapidjson::Value& parent = (*nodes)[nodes->Size() - 1];
            if (!parent.HasMember("childIds") ||
                !parent["childIds"].IsArray()) {
                return;
            }

            rapidjson::Value childIds(rapidjson::kArrayType);
            std::string childId = axId(child);
            childIds.PushBack(
                rapidjson::Value(childId.c_str(), childId.size(), *alloc),
                *alloc);
            rapidjson::Value& existingChildIds = parent["childIds"];
            for (rapidjson::SizeType i = 0; i < existingChildIds.Size(); i++) {
                childIds.PushBack(rapidjson::Value(existingChildIds[i], *alloc),
                                  *alloc);
            }
            parent["childIds"].Swap(childIds);
        }
    };

} // namespace

void AccessibilityDomain::processMessage(CDPCommand& cmd,
                                         const std::string& method)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    if (method == "enable") {
        s->accessibilityEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->accessibilityEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getFullAXTree" || method == "getRootAXNode") {
        if (method == "getRootAXNode" && !s->accessibilityEnabled) {
            cmd.sendError(-32000, "Accessibility has not been enabled.");
            return;
        }

        Optional<BrowsingContext*> context = wv->mainBrowsingContext();
        if (cmd.params() && cmd.params()->HasMember("frameId") &&
            (*cmd.params())["frameId"].IsString()) {
            context = m_dispatcher->page()->browsingContextForFrameId(
                (*cmd.params())["frameId"].GetString());
        }
        Optional<Document*> document;
        if (context) {
            document = context->document();
        }
        if (!document) {
            cmd.sendError(-32000, "No document");
            return;
        }
        Document* doc = document.value();
        Optional<Element*> root = doc->documentElement();
        if (!root) {
            cmd.sendError(-32000, "No document element");
            return;
        }

        // Parse optional depth parameter
        int depth = -1; // -1 means unlimited
        if (cmd.params() && cmd.params()->HasMember("depth") &&
            (*cmd.params())["depth"].IsInt()) {
            depth = (*cmd.params())["depth"].GetInt();
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value nodes(rapidjson::kArrayType);

        AXBuilder b{ reg, m_dispatcher->page(), &nodes, &alloc };
        b.addDocumentNode(doc, false, -1);
        if (depth == 0) {
            b.addChildren(doc, reg->getOrCreate(doc));
        } else {
            b.addNode(root.value(), reg->getOrCreate(doc), true, depth);
        }

        if (method == "getRootAXNode") {
            if (nodes.Size() > 0) {
                rapidjson::Value copy(nodes[0], alloc);
                result.AddMember("node", copy, alloc);
            }
            cmd.sendResult(result, out);
            return;
        }

        result.AddMember("nodes", nodes, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getAXNodeAndAncestors" || method == "getPartialAXTree") {
        if (method == "getAXNodeAndAncestors" && !s->accessibilityEnabled) {
            cmd.sendError(-32000, "Accessibility has not been enabled.");
            return;
        }

        Node* targetNode = nullptr;
        int targetBackendId = -1;
        if (!cmd.params() ||
            (cmd.params()->IsObject() && !cmd.params()->HasMember("nodeId") &&
             !cmd.params()->HasMember("backendNodeId") &&
             !cmd.params()->HasMember("objectId"))) {
            cmd.sendError(-32000,
                          "Either nodeId, backendNodeId or objectId "
                          "must be specified");
            return;
        }
        if (cmd.params()->HasMember("nodeId") &&
            (*cmd.params())["nodeId"].IsInt()) {
            int nid = (*cmd.params())["nodeId"].GetInt();
            Optional<Node*> n = reg->lookup(nid);
            if (!n) {
                cmd.sendError(-32000, "Could not find node with given id");
                return;
            }
            targetNode = n.value();
            targetBackendId = nid;
        } else if (cmd.params()->HasMember("backendNodeId") &&
                   (*cmd.params())["backendNodeId"].IsInt()) {
            int bid = (*cmd.params())["backendNodeId"].GetInt();
            Optional<Node*> n = reg->lookup(bid);
            if (!n) {
                cmd.sendError(-32000, "No node found for given backend id");
                return;
            }
            targetNode = n.value();
            targetBackendId = bid;
        } else if (cmd.params()->HasMember("objectId") &&
                   (*cmd.params())["objectId"].IsString()) {
            targetNode = nodeFromObjectId(
                m_dispatcher, (*cmd.params())["objectId"].GetString());
            if (!targetNode) {
                cmd.sendError(-32000, "Invalid remote object id");
                return;
            }
            targetBackendId = reg->getOrCreate(targetNode);
        } else {
            cmd.sendError(-32000,
                          "Either nodeId, backendNodeId or objectId "
                          "must be specified");
            return;
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value nodes(rapidjson::kArrayType);
        AXBuilder builder{ reg, m_dispatcher->page(), &nodes, &alloc };
        Node* targetParent = parentForAXTraversal(targetNode);
        int targetParentBackendId =
            targetParent ? reg->getOrCreate(targetParent) : -1;

        if (method == "getAXNodeAndAncestors") {
            builder.addNode(targetNode, targetParentBackendId, false);
            if (!builder.hasAXNode(targetNode)) {
                result.AddMember("nodes", nodes, alloc);
                cmd.sendResult(result, out);
                return;
            }

            Node* p = parentForAXTraversal(targetNode);
            while (p) {
                Node* gp = parentForAXTraversal(p);
                int gpb = gp ? reg->getOrCreate(gp) : -1;
                builder.addNode(p, gpb, false);
                p = gp;
            }
        } else {
            bool fetchRelatives = true;
            if (cmd.params()->HasMember("fetchRelatives") &&
                (*cmd.params())["fetchRelatives"].IsBool()) {
                fetchRelatives = (*cmd.params())["fetchRelatives"].GetBool();
            }
            if (fetchRelatives) {
                builder.addNode(targetNode, targetParentBackendId, false);

                if (!builder.isIgnored(targetNode)) {
                    builder.addChildren(targetNode, targetBackendId);
                }

                Node* parent = parentForAXTraversal(targetNode);
                bool connectTargetToParent = !builder.hasAXNode(targetNode);
                while (parent) {
                    Node* grandparent = parentForAXTraversal(parent);
                    int grandparentBackendId =
                        grandparent ? reg->getOrCreate(grandparent) : -1;
                    builder.addNode(parent, grandparentBackendId, false);
                    if (connectTargetToParent) {
                        builder.prependChildIdToLastNode(targetNode);
                        connectTargetToParent = false;
                    }
                    parent = grandparent;
                }
            } else {
                builder.addNode(targetNode, targetParentBackendId, false);
            }
        }

        result.AddMember("nodes", nodes, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "queryAXTree") {
        Node* rootNode = nullptr;
        if (!cmd.params()) {
            cmd.sendError(-32000,
                          "Either nodeId, backendNodeId or objectId "
                          "must be specified");
            return;
        }
        if (cmd.params()->HasMember("nodeId") &&
            (*cmd.params())["nodeId"].IsInt()) {
            int nid = (*cmd.params())["nodeId"].GetInt();
            Optional<Node*> n = reg->lookup(nid);
            if (!n) {
                cmd.sendError(-32000, "Could not find node with given id");
                return;
            }
            rootNode = n.value();
        } else if (cmd.params()->HasMember("backendNodeId") &&
                   (*cmd.params())["backendNodeId"].IsInt()) {
            int bid = (*cmd.params())["backendNodeId"].GetInt();
            Optional<Node*> n = reg->lookup(bid);
            if (!n) {
                cmd.sendError(-32000, "No node found for given backend id");
                return;
            }
            rootNode = n.value();
        } else if (cmd.params()->HasMember("objectId") &&
                   (*cmd.params())["objectId"].IsString()) {
            rootNode = nodeFromObjectId(
                m_dispatcher, (*cmd.params())["objectId"].GetString());
            if (!rootNode) {
                cmd.sendError(-32000, "Invalid remote object id");
                return;
            }
        } else {
            cmd.sendError(-32000,
                          "Either nodeId, backendNodeId or objectId "
                          "must be specified");
            return;
        }

        if (rootNode->isShadowRoot()) {
            rootNode = rootNode->asShadowRoot()->host();
        }

        std::string accessibleName;
        std::string roleFilter;
        bool hasName = false, hasRole = false;
        if (cmd.params()->HasMember("accessibleName") &&
            (*cmd.params())["accessibleName"].IsString()) {
            accessibleName = (*cmd.params())["accessibleName"].GetString();
            hasName = true;
        }
        if (cmd.params()->HasMember("role") &&
            (*cmd.params())["role"].IsString()) {
            roleFilter = (*cmd.params())["role"].GetString();
            hasRole = true;
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value nodes(rapidjson::kArrayType);
        AXBuilder builder{ reg, m_dispatcher->page(), &nodes, &alloc };

        builder.addNodesMatching(rootNode, hasName ? &accessibleName : nullptr,
                                 hasRole ? &roleFilter : nullptr);

        result.AddMember("nodes", nodes, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getChildAXNodes") {
        if (!s->accessibilityEnabled) {
            cmd.sendError(-32000, "Accessibility has not been enabled.");
            return;
        }

        if (!cmd.params() || !cmd.params()->HasMember("id") ||
            !(*cmd.params())["id"].IsString()) {
            cmd.sendError(-32602, "id is required");
            return;
        }

        std::string axNodeId = (*cmd.params())["id"].GetString();

        // Get the document, optionally using frameId
        Optional<BrowsingContext*> context = wv->mainBrowsingContext();
        if (cmd.params() && cmd.params()->HasMember("frameId") &&
            (*cmd.params())["frameId"].IsString()) {
            Optional<BrowsingContext*> frameContext =
                m_dispatcher->page()->browsingContextForFrameId(
                    (*cmd.params())["frameId"].GetString());
            if (frameContext) {
                context = frameContext;
            }
        }
        Optional<Document*> document;
        if (context) {
            document = context->document();
        }
        if (!document) {
            cmd.sendError(-32000, "No document");
            return;
        }

        Node* parentNode = nullptr;
        int parentBackendId = -1;
        if (axNodeId == "ax-root") {
            parentNode = document.value();
            parentBackendId = reg->getOrCreate(parentNode);
        } else if (axNodeId.compare(0, 3, "ax-") == 0) {
            const char* value = axNodeId.c_str() + 3;
            char* end = nullptr;
            long parsedId = strtol(value, &end, 10);
            if (end == value || *end != '\0' || parsedId <= 0) {
                cmd.sendError(-32602, "Invalid ID");
                return;
            }
            parentBackendId = static_cast<int>(parsedId);
            Optional<Node*> node = reg->lookup(parentBackendId);
            if (!node) {
                cmd.sendError(-32000, "Could not find node with given id");
                return;
            }
            parentNode = node.value();
        } else {
            cmd.sendError(-32602, "Invalid AXNodeId format");
            return;
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value nodes(rapidjson::kArrayType);

        AXBuilder builder{ reg, m_dispatcher->page(), &nodes, &alloc };
        builder.addChildren(parentNode, parentBackendId);

        result.AddMember("nodes", nodes, alloc);
        cmd.sendResult(result, out);
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
