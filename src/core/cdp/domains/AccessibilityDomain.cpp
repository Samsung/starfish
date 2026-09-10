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
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/style/ComputedStyle.h"

#include "rapidjson/document.h"
#include <cstring>
#include <string>
#include <cctype>

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

static bool isHiddenFromAccessibility(Element* element)
{
    if (hasAttr(element, "hidden") ||
        lower(collapse(attr(element, "aria-hidden"))) == "true") {
        return true;
    }

    ComputedStyle* style = element->style();
    return style &&
           (style->display() == DisplayValue::NoneDisplayValue ||
            style->visibility() != VisibilityValue::VisibleVisibilityValue);
}

// Map an element to an ARIA role. Explicit role attribute wins. Returns "" for
// elements that carry no semantic role (folded as "generic"/ignored).
static std::string roleForElement(Element* el, const std::string& tag)
{
    std::string explicitRole = lower(collapse(attr(el, "role")));
    if (!explicitRole.empty()) {
        // role may be a token list; take first token.
        size_t sp = explicitRole.find(' ');
        return sp == std::string::npos ? explicitRole
                                       : explicitRole.substr(0, sp);
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
    if (tag == "header") {
        return "banner";
    }
    if (tag == "footer") {
        return "contentinfo";
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

// Compute the accessible name: aria-label > (control value/alt) > text content.
static std::string nameForElement(Element* el, const std::string& tag,
                                  const std::string& role)
{
    std::string aria = collapse(attr(el, "aria-label"));
    if (!aria.empty()) {
        return aria;
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
        // textbox-likes: placeholder is a reasonable fallback name source.
        std::string ph = collapse(attr(el, "placeholder"));
        if (!ph.empty()) {
            return ph;
        }
        std::string title = collapse(attr(el, "title"));
        return title;
    }
    // Name-from-content roles.
    if (role == "button" || role == "link" || role == "heading") {
        return subtreeText(el);
    }
    if (tag == "button") {
        return subtreeText(el);
    }
    // Otherwise leave to be derived from children by the client.
    return std::string();
}

namespace {

    struct AXBuilder {
        NodeRegistry* reg;
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

        void addRoleAndName(rapidjson::Value& node, const std::string& role,
                            const std::string& name)
        {
            rapidjson::Value roleValue(rapidjson::kObjectType);
            roleValue.AddMember("type", "role", *alloc);
            roleValue.AddMember(
                "value", rapidjson::Value(role.c_str(), role.size(), *alloc),
                *alloc);
            node.AddMember("role", roleValue, *alloc);

            rapidjson::Value nameValue(rapidjson::kObjectType);
            nameValue.AddMember("type", "computedString", *alloc);
            nameValue.AddMember(
                "value", rapidjson::Value(name.c_str(), name.size(), *alloc),
                *alloc);
            node.AddMember("name", nameValue, *alloc);
        }

        void addTextNode(Node* domNode, int parentBackendId)
        {
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
            addRoleAndName(node, "StaticText", name);
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

            rapidjson::Value inlineNode(rapidjson::kObjectType);
            inlineNode.AddMember(
                "nodeId",
                rapidjson::Value(inlineId.c_str(), inlineId.size(), *alloc),
                *alloc);
            inlineNode.AddMember("ignored", false, *alloc);
            addRoleAndName(inlineNode, "InlineTextBox", name);
            inlineNode.AddMember(
                "properties", rapidjson::Value(rapidjson::kArrayType), *alloc);
            inlineNode.AddMember("backendDOMNodeId", backendId, *alloc);
            inlineNode.AddMember(
                "parentId",
                rapidjson::Value(nodeId.c_str(), nodeId.size(), *alloc),
                *alloc);
            inlineNode.AddMember(
                "childIds", rapidjson::Value(rapidjson::kArrayType), *alloc);
            nodes->PushBack(inlineNode, *alloc);
        }

        // Add each node before its descendants.
        void addNode(Node* domNode, int parentBackendId,
                     bool includeDescendants = true)
        {
            if (domNode->nodeType() == Node::TEXT_NODE) {
                addTextNode(domNode, parentBackendId);
                return;
            }

            Element* el = domNode->asElement();
            int backendId = reg->getOrCreate(el);
            std::string nodeId = "ax-" + std::to_string(backendId);

            std::string tag = lower(toUTF8(el->localName()));
            std::string role = roleForElement(el, tag);
            bool ignored =
                role.empty() || tag == "html" || tag == "head" || tag == "body";
            std::string name = nameForElement(el, tag, role);

            rapidjson::Value n(rapidjson::kObjectType);
            n.AddMember("nodeId",
                        rapidjson::Value(nodeId.c_str(), nodeId.size(), *alloc),
                        *alloc);
            n.AddMember("ignored", ignored, *alloc);

            if (!ignored) {
                addRoleAndName(n, role, name);
            }

            n.AddMember("properties", rapidjson::Value(rapidjson::kArrayType),
                        *alloc);
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
            n.AddMember("childIds", childIds, *alloc);

            nodes->PushBack(n, *alloc);

            if (includeDescendants) {
                for (Node* c = el->firstChild(); c; c = c->nextSibling()) {
                    if (shouldAddNode(c)) {
                        addNode(c, backendId);
                    }
                }
            }
        }

        void addNodesMatchingRole(Node* domNode, const std::string& role)
        {
            if (domNode->nodeType() == Node::ELEMENT_NODE) {
                Element* element = domNode->asElement();
                if (isHiddenFromAccessibility(element)) {
                    return;
                }
                std::string tag = lower(toUTF8(element->localName()));
                if (roleForElement(element, tag) == role) {
                    Node* parent = domNode->parentNode();
                    int parentBackendId =
                        parent ? reg->getOrCreate(parent) : -1;
                    addNode(domNode, parentBackendId, false);
                }
            }

            for (Node* child = domNode->firstChild(); child;
                 child = child->nextSibling()) {
                addNodesMatchingRole(child, role);
            }
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

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value nodes(rapidjson::kArrayType);

        // Synthetic RootWebArea as array[0]. puppeteer's createTree() roots the
        // tree at the first array entry; a single interesting root makes
        // snapshot() expose the document's content as one tree instead of a
        // flat list (it only returns serializeTree(...)[0]). Mark it focusable
        // so puppeteer's isInteresting() keeps it.
        int rootBackend = reg->getOrCreate(root.value());
        std::string rootChildAx = "ax-" + std::to_string(rootBackend);
        {
            rapidjson::Value rn(rapidjson::kObjectType);
            rn.AddMember("nodeId", "ax-root", alloc);
            rn.AddMember("ignored", false, alloc);
            rapidjson::Value roleVal(rapidjson::kObjectType);
            roleVal.AddMember("type", "role", alloc);
            roleVal.AddMember("value", "RootWebArea", alloc);
            rn.AddMember("role", roleVal, alloc);
            rapidjson::Value nameVal(rapidjson::kObjectType);
            nameVal.AddMember("type", "computedString", alloc);
            std::string documentName = toUTF8(doc->title());
            nameVal.AddMember("value",
                              rapidjson::Value(documentName.c_str(),
                                               documentName.size(), alloc),
                              alloc);
            rn.AddMember("name", nameVal, alloc);
            rapidjson::Value props(rapidjson::kArrayType);
            rapidjson::Value focusable(rapidjson::kObjectType);
            focusable.AddMember("name", "focusable", alloc);
            rapidjson::Value fv(rapidjson::kObjectType);
            fv.AddMember("type", "booleanOrUndefined", alloc);
            fv.AddMember("value", true, alloc);
            focusable.AddMember("value", fv, alloc);
            props.PushBack(focusable, alloc);
            rn.AddMember("properties", props, alloc);
            rn.AddMember("backendDOMNodeId", rootBackend, alloc);
            rapidjson::Value childIds(rapidjson::kArrayType);
            childIds.PushBack(rapidjson::Value(rootChildAx.c_str(),
                                               rootChildAx.size(), alloc),
                              alloc);
            rn.AddMember("childIds", childIds, alloc);
            nodes.PushBack(rn, alloc);
        }

        AXBuilder b{ reg, &nodes, &alloc };
        b.addNode(root.value(), -1);

        if (method == "getRootAXNode") {
            // array[0] is the synthetic RootWebArea.
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

    if (method == "queryAXTree") {
        if (!cmd.params() || !cmd.params()->HasMember("backendNodeId") ||
            !(*cmd.params())["backendNodeId"].IsInt() ||
            !cmd.params()->HasMember("role") ||
            !(*cmd.params())["role"].IsString()) {
            cmd.sendError(-32602, "backendNodeId and role are required");
            return;
        }

        Optional<Node*> root =
            reg->lookup((*cmd.params())["backendNodeId"].GetInt());
        if (!root) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value nodes(rapidjson::kArrayType);
        const char* role = (*cmd.params())["role"].GetString();
        AXBuilder builder{ reg, &nodes, &alloc };
        builder.addNodesMatchingRole(root.value(), role);
        result.AddMember("nodes", nodes, alloc);
        cmd.sendResult(result, out);
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
