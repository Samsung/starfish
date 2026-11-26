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
#include <algorithm>
#include "Starfish.h"
#include "DOMDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../NodeRegistry.h"
#include "../RemoteObject.h"
#include "PageDomain.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/DOMRect.h"
#include "core/dom/CharacterData.h"
#include "core/dom/NodeList.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/Event.h"
#include "core/dom/EventTarget.h"
#include "core/fileapi/File.h"
#include "core/fileapi/FilePropertyBag.h"
#include "StaticStrings.h"
#include "binding/generated/BufferSourceOrBlobOrDOMStringUnion.h"
#include "binding/ScriptWrappable.h"
#include "binding/ScriptBindingInstance.h"
#include "EscargotPublic.h"

#include "rapidjson/document.h"
#include <cstring>
#include <set>

namespace Starfish {

static Document* mainDocument(WebView* wv)
{
    BrowsingContext* bc = wv->mainBrowsingContext();
    return bc ? bc->document() : nullptr;
}

// Resolve a Runtime objectId ("OBJ-<n>") to the DOM Node it handles, or
// nullptr if the handle is missing or is not a node.
static Node* nodeFromObjectId(CDPDispatcher* disp, const char* oid)
{
    ScriptWrappable* w = wrappableForObjectId(disp->remoteObjectStore(), oid);
    return w && w->isNode() ? w->asNode() : nullptr;
}

// Pushes the children of node so that popping visits them in document order.
static void pushChildrenInReverse(Node* node, std::vector<Node*>& stack)
{
    size_t first = stack.size();
    for (Node* c = node->firstChild(); c; c = c->nextSibling()) {
        stack.push_back(c);
    }
    std::reverse(stack.begin() + first, stack.end());
}

// The node a command names: by objectId (an ElementHandle) first, then by
// backendNodeId when the command accepts one, then by nodeId.
static Node* nodeFromParams(CDPDispatcher* disp, CDPCommand& cmd,
                            bool acceptBackendNodeId)
{
    if (hasStringParam(cmd, "objectId")) {
        return nodeFromObjectId(disp, (*cmd.params())["objectId"].GetString());
    }
    NodeRegistry* reg = disp->nodeRegistry();
    if (acceptBackendNodeId && cmd.params() &&
        cmd.params()->HasMember("backendNodeId") &&
        (*cmd.params())["backendNodeId"].IsInt()) {
        return reg->lookup((*cmd.params())["backendNodeId"].GetInt());
    }
    return reg->lookup(paramNodeId(cmd));
}

// nodeFromParams for a command that needs an element. Replies that there is
// no such node and returns nullptr otherwise.
static Element* elementFromParams(CDPDispatcher* disp, CDPCommand& cmd,
                                  bool acceptBackendNodeId = false)
{
    Node* node = nodeFromParams(disp, cmd, acceptBackendNodeId);
    if (!node || !node->isElement()) {
        cmd.sendError(-32000, "Could not find node with given id");
        return nullptr;
    }
    return node->asElement();
}

// The parent the DOM domain reports. A shadow root reports its host, and a
// document reports the frame owner that embeds it, so one chain reaches the
// main document from anywhere in the page.
static Node* innerParentNode(Node* node)
{
    if (node->isShadowRoot()) {
        return node->asShadowRoot()->host();
    }
    return node->parentNode();
}

// Tell the client about one level of children and record that it was sent.
static void sendChildNodes(CDPDispatcher* dispatcher, CDPCommand& cmd,
                           NodeRegistry* reg, Node* parent, int depth)
{
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("parentId", reg->getOrCreate(parent), alloc);
    rapidjson::Value nodes(rapidjson::kArrayType);
    for (Node* c = parent->firstChild(); c; c = c->nextSibling()) {
        rapidjson::Value child(rapidjson::kObjectType);
        reg->serializeNode(c, depth - 1, child, alloc);
        nodes.PushBack(child, alloc);
    }
    params.AddMember("nodes", nodes, alloc);
    reg->markChildrenSent(parent);

    CDPCommand evt(dispatcher, Optional<int64_t>(), cmd.sessionId(), nullptr);
    evt.sendEvent("DOM.setChildNodes", params, out);
}

// A command answers with a node id, but the client only knows the nodes it
// has been sent. Walk up to the nearest node it does know and send one
// DOM.setChildNodes per level below that, so the id it gets back resolves.
// This mirrors InspectorDOMAgent::PushNodePathToFrontend.
static int pushNodePathToFrontend(CDPDispatcher* dispatcher, CDPCommand& cmd,
                                  NodeRegistry* reg, Node* node)
{
    if (!node) {
        return 0;
    }
    if (reg->wasSent(node)) {
        return reg->getOrCreate(node);
    }

    // Collect the ancestors the client is missing, nearest first.
    GCVector<Node*> path;
    for (Node* parent = innerParentNode(node); parent;
         parent = innerParentNode(parent)) {
        path.push_back(parent);
        if (reg->wasSent(parent)) {
            break;
        }
    }
    // Nothing on the chain is known, so there is no place to attach the
    // node. Hand back the id anyway: it is still valid as a backend id.
    if (path.empty() || !reg->wasSent(path.back())) {
        return reg->getOrCreate(node);
    }

    for (size_t i = path.size(); i > 0; i--) {
        Node* parent = path[i - 1];
        if (!reg->childrenWereSent(parent)) {
            sendChildNodes(dispatcher, cmd, reg, parent, 1);
        }
    }
    return reg->getOrCreate(node);
}

void DOMDomain::handleEnable(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    s->domEnabled = true;
    cmd.sendResultEmpty();
}

void DOMDomain::handleDisable(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    s->domEnabled = false;
    cmd.sendResultEmpty();
}

void DOMDomain::handleGetDocument(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    Document* doc = mainDocument(wv);
    if (!doc) {
        cmd.sendError(-32000, "No document");
        return;
    }
    int depth = 3;
    depth = paramInt(cmd, "depth", depth);
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value root(rapidjson::kObjectType);
    reg->serializeNode(doc, depth, root, alloc);
    result.AddMember("root", root, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleRequestChildNodes(CDPCommand& cmd, const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    int nodeId = paramNodeId(cmd);
    Node* node = reg->lookup(nodeId);
    if (!node) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    int depth = 1;
    depth = paramInt(cmd, "depth", depth);
    cmd.sendResultEmpty();
    sendChildNodes(m_dispatcher, cmd, reg, node, depth);
}

void DOMDomain::handleQuerySelector(CDPCommand& cmd, const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    int nodeId = paramNodeId(cmd);
    Node* node = reg->lookup(nodeId);
    if (!node || !hasStringParam(cmd, "selector")) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    const char* sel = (*cmd.params())["selector"].GetString();
    Element* found = node->querySelector(String::fromUTF8(sel, strlen(sel)));
    int foundId =
        found ? pushNodePathToFrontend(m_dispatcher, cmd, reg, found) : 0;
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("nodeId", foundId, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleQuerySelectorAll(CDPCommand& cmd, const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    int nodeId = paramNodeId(cmd);
    Node* node = reg->lookup(nodeId);
    if (!node || !hasStringParam(cmd, "selector")) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    const char* sel = (*cmd.params())["selector"].GetString();
    NodeList* list = node->querySelectorAll(String::fromUTF8(sel, strlen(sel)));
    // Every match is pushed before the result goes out, so the client
    // has seen each node by the time it reads the ids.
    GCVector<int> matched;
    if (list) {
        for (size_t i = 0; i < list->length(); i++) {
            Node* n = list->item(i);
            if (n) {
                matched.push_back(
                    pushNodePathToFrontend(m_dispatcher, cmd, reg, n));
            }
        }
    }
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value ids(rapidjson::kArrayType);
    for (int id : matched) {
        ids.PushBack(id, alloc);
    }
    result.AddMember("nodeIds", ids, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleDescribeNode(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    Node* node = nodeFromParams(m_dispatcher, cmd, true);
    if (!node) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    int depth = 0;
    depth = paramInt(cmd, "depth", depth);
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value n(rapidjson::kObjectType);
    // describeNode hands back a node without binding it to the client's
    // node id space, so nothing here counts as sent.
    reg->serializeNode(node, depth, n, alloc, false);
    if (node->isHTMLIFrameElement()) {
        Optional<BrowsingContext*> context =
            node->asHTMLIFrameElement()->browsingContext();
        if (context) {
            std::string frameId =
                m_dispatcher->page()->frameIdForBrowsingContext(
                    context.value());
            if (!frameId.empty()) {
                addStringMember(n, "frameId", frameId, alloc);
            }
        }
    }
    result.AddMember("node", n, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleGetOuterHTML(CDPCommand& cmd, const std::string&)
{
    Element* element = elementFromParams(m_dispatcher, cmd);
    if (!element) {
        return;
    }
    String* html = element->outerHTML();
    std::string s = html ? html->toUTF8NonGCString() : std::string();
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    addStringMember(result, "outerHTML", s, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleSetOuterHTML(CDPCommand& cmd, const std::string&)
{
    Element* element = elementForNodeIdParam(m_dispatcher, cmd);
    if (!element) {
        return;
    }
    if (!hasStringParam(cmd, "outerHTML")) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    const char* outer = (*cmd.params())["outerHTML"].GetString();
    element->setOuterHTML(String::fromUTF8(outer, strlen(outer)));
    cmd.sendResultEmpty();
}

void DOMDomain::handleFocus(CDPCommand& cmd, const std::string&)
{
    Element* element = elementFromParams(m_dispatcher, cmd);
    if (!element) {
        return;
    }
    element->focus();
    cmd.sendResultEmpty();
}

void DOMDomain::handleScrollIntoViewIfNeeded(CDPCommand& cmd,
                                             const std::string&)
{
    Element* element = elementFromParams(m_dispatcher, cmd);
    if (!element) {
        return;
    }
    element->scrollIntoViewIfNeeded();
    cmd.sendResultEmpty();
}

void DOMDomain::handleGetAttributes(CDPCommand& cmd, const std::string&)
{
    Element* el = elementForNodeIdParam(m_dispatcher, cmd);
    if (!el) {
        return;
    }
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value attrs(rapidjson::kArrayType);
    size_t count = el->attributeCount();
    for (size_t i = 0; i < count; i++) {
        String* name = el->getAssuredAttributeName(i).localName();
        String* value = el->getAssuredAttribute(i);
        std::string n = name ? name->toUTF8NonGCString() : std::string();
        std::string v = value ? value->toUTF8NonGCString() : std::string();
        attrs.PushBack(rapidjson::Value(n.c_str(), n.size(), alloc), alloc);
        attrs.PushBack(rapidjson::Value(v.c_str(), v.size(), alloc), alloc);
    }
    result.AddMember("attributes", attrs, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleSetAttributeValue(CDPCommand& cmd, const std::string&)
{
    int nodeId = paramNodeId(cmd);
    Element* element = elementForNodeIdParam(m_dispatcher, cmd);
    if (!element) {
        return;
    }
    if (!hasStringParam(cmd, "name") || !hasStringParam(cmd, "value")) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    const char* name = (*cmd.params())["name"].GetString();
    const char* value = (*cmd.params())["value"].GetString();
    element->setAttribute(String::fromUTF8(name, strlen(name)),
                          String::fromUTF8(value, strlen(value)));
    cmd.sendResultEmpty();

    // Emit DOM.attributeModified.
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("nodeId", nodeId, alloc);
    params.AddMember("name", rapidjson::Value(name, strlen(name), alloc),
                     alloc);
    params.AddMember("value", rapidjson::Value(value, strlen(value), alloc),
                     alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(), nullptr);
    evt.sendEvent("DOM.attributeModified", params, out);
}

void DOMDomain::handleRemoveAttribute(CDPCommand& cmd, const std::string&)
{
    int nodeId = paramNodeId(cmd);
    Element* element = elementForNodeIdParam(m_dispatcher, cmd);
    if (!element) {
        return;
    }
    if (!hasStringParam(cmd, "name")) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    const char* name = (*cmd.params())["name"].GetString();
    element->removeAttribute(String::fromUTF8(name, strlen(name)));
    cmd.sendResultEmpty();

    // Emit DOM.attributeRemoved.
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("nodeId", nodeId, alloc);
    params.AddMember("name", rapidjson::Value(name, strlen(name), alloc),
                     alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(), nullptr);
    evt.sendEvent("DOM.attributeRemoved", params, out);
}

void DOMDomain::handleSetNodeValue(CDPCommand& cmd, const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    int nodeId = paramNodeId(cmd);
    Node* node = reg->lookup(nodeId);
    if (!node || !node->isCharacterData() || !hasStringParam(cmd, "value")) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    const char* value = (*cmd.params())["value"].GetString();
    node->asCharacterData()->setData(String::fromUTF8(value, strlen(value)));
    cmd.sendResultEmpty();

    // Emit DOM.characterDataModified.
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("nodeId", nodeId, alloc);
    params.AddMember("characterData",
                     rapidjson::Value(value, strlen(value), alloc), alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(), nullptr);
    evt.sendEvent("DOM.characterDataModified", params, out);
}

void DOMDomain::handleRemoveNode(CDPCommand& cmd, const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    int nodeId = paramNodeId(cmd);
    Node* node = reg->lookup(nodeId);
    if (!node || !node->parentNode()) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    int parentId = reg->getOrCreate(node->parentNode());
    node->remove();
    cmd.sendResultEmpty();

    // Emit DOM.childNodeRemoved.
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("parentNodeId", parentId, alloc);
    params.AddMember("nodeId", nodeId, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(), nullptr);
    evt.sendEvent("DOM.childNodeRemoved", params, out);
}

void DOMDomain::handleResolveNode(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    // DOM nodeId/backendNodeId -> Runtime RemoteObject handle.
    Node* node = nullptr;
    if (cmd.params() && cmd.params()->HasMember("backendNodeId") &&
        (*cmd.params())["backendNodeId"].IsInt()) {
        node = reg->lookup((*cmd.params())["backendNodeId"].GetInt());
    }
    if (!node) {
        node = reg->lookup(paramNodeId(cmd));
    }
    if (!node) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    BrowsingContext* bc = wv->mainBrowsingContext();
    if (!bc || !bc->scriptBindingInstance()) {
        cmd.sendError(-32000, "No scripting context");
        return;
    }
    ScriptBindingInstance* sbi = bc->scriptBindingInstance();
    Escargot::ObjectRef* objRef = node->scriptObject();
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value remote(rapidjson::kObjectType);
    serializeRemoteObject(sbi, m_dispatcher->remoteObjectStore(),
                          (Escargot::ValueRef*)objRef, false, remote, alloc);
    result.AddMember("object", remote, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleSetFileInputFiles(CDPCommand& cmd, const std::string&)
{
    Node* node = nodeFromParams(m_dispatcher, cmd, true);
    if (!node || !node->isElement() ||
        !node->asElement()->isHTMLInputElement()) {
        cmd.sendError(-32000, "Node is not a file input element");
        return;
    }
    HTMLInputElement* input = node->asElement()->asHTMLInputElement();
    if (!input->type() || !input->type()->equals("file")) {
        cmd.sendError(-32000, "Node is not a file input element");
        return;
    }
    if (!cmd.params() || !cmd.params()->HasMember("files") ||
        !(*cmd.params())["files"].IsArray()) {
        cmd.sendError(-32602, "files array is required");
        return;
    }

    ExecutionContext* ec = input->executionContext();
    GCVector<File*>* files = new GCVector<File*>();
    const rapidjson::Value& arr = (*cmd.params())["files"];
    for (rapidjson::SizeType i = 0; i < arr.Size(); i++) {
        if (!arr[i].IsString()) {
            continue;
        }
        std::string path = arr[i].GetString();

        // basename
        size_t slash = path.find_last_of('/');
        std::string base =
            (slash == std::string::npos) ? path : path.substr(slash + 1);

        // Read file content (best effort). Size/type derive from content.
        std::string content;
        FILE* fp = fopen(path.c_str(), "rb");
        if (fp) {
            char buf[8192];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
                content.append(buf, n);
            }
            fclose(fp);
        }

        GCVector<BufferSourceOrBlobOrDOMString> fileBits;
        fileBits.push_back(BufferSourceOrBlobOrDOMString::createDOMString(
            String::fromUTF8(content.data(), content.size())));
        File* f =
            new File(ec, fileBits, String::fromUTF8(base.data(), base.size()),
                     FilePropertyBag());
        files->push_back(f);
    }

    input->setSelectedFiles(files);

    // Fire input then change, matching the user-driven file selection flow.
    // HTMLFormControl::fireEvent is protected, so dispatch directly the
    // same way it does.
    StaticStrings* ss = ec->starfish()->staticStrings();
    Event* inputEvent =
        new Event(ec, ss->m_input.localName(), EventInit(true, false));
    input->dispatchEventByUA(input, inputEvent);
    Event* changeEvent =
        new Event(ec, ss->m_change.localName(), EventInit(true, false));
    input->dispatchEventByUA(input, changeEvent);

    cmd.sendResultEmpty();
}

void DOMDomain::handleGetBoxModel(CDPCommand& cmd, const std::string& method)
{
    Element* element =
        elementFromParams(m_dispatcher, cmd, /*acceptBackendNodeId=*/true);
    if (!element) {
        return;
    }
    // getBoundingClientRect() runs layout if needed and returns CSS-px
    // coordinates (layout px, no devicePixelRatio applied) relative to the
    // viewport -- exactly the coordinate space CDP clients expect.
    DOMRect* rect = element->getBoundingClientRect();
    double x = rect->x();
    double y = rect->y();
    double w = rect->width();
    double h = rect->height();

    rapidjson::Document outd;
    rapidjson::Document::AllocatorType& alloc = outd.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);

    if (method == "getContentQuads") {
        rapidjson::Value quads(rapidjson::kArrayType);
        if (w > 0 || h > 0) {
            quads.PushBack(cdpQuad(x, y, w, h, alloc), alloc);
        }
        result.AddMember("quads", quads, alloc);
    } else {
        // getBoxModel. MVP: content/padding/border/margin all use the
        // border-box rect from getBoundingClientRect(). Width/height are
        // the border-box dimensions, which is what CDP reports.
        rapidjson::Value model(rapidjson::kObjectType);
        model.AddMember("content", cdpQuad(x, y, w, h, alloc), alloc);
        model.AddMember("padding", cdpQuad(x, y, w, h, alloc), alloc);
        model.AddMember("border", cdpQuad(x, y, w, h, alloc), alloc);
        model.AddMember("margin", cdpQuad(x, y, w, h, alloc), alloc);
        model.AddMember("width", (int)(w + 0.5), alloc);
        model.AddMember("height", (int)(h + 0.5), alloc);
        result.AddMember("model", model, alloc);
    }
    cmd.sendResult(result, outd);
}

// The text test of DOM.performSearch's plain-text fallback.
static bool nodeMatchesSearchText(Node* n, String* needle, bool xpathTag)
{
    bool hit = false;
    if (n->isElement()) {
        Element* el = n->asElement();
        String* tag = el->localName();
        if (xpathTag) {
            if (tag && tag->equalsIgnoreCase(needle)) {
                hit = true;
            }
        } else {
            if (tag && tag->contains(needle, false)) {
                hit = true;
            }
            size_t ac = el->attributeCount();
            for (size_t i = 0; !hit && i < ac; i++) {
                String* v = el->getAssuredAttribute(i);
                if (v && v->contains(needle, false)) {
                    hit = true;
                }
            }
        }
    } else if (!xpathTag && n->isCharacterData()) {
        Optional<String*> tc = n->textContent();
        if (tc.hasValue() && tc.value() &&
            tc.value()->contains(needle, false)) {
            hit = true;
        }
    }
    return hit;
}

void DOMDomain::handlePerformSearch(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    Document* doc = mainDocument(wv);
    if (!doc || !hasStringParam(cmd, "query")) {
        cmd.sendError(-32602, "query is required");
        return;
    }
    const char* q = (*cmd.params())["query"].GetString();
    String* query = String::fromUTF8(q, strlen(q));

    std::vector<Node*> matches;

    // 1) Try the query as a CSS selector. querySelectorAll throws a
    //    DOMException* on an invalid/unsupported selector (e.g. an XPath
    //    "//p"); fall back to a plain-text walk in that case. A
    //    syntactically valid selector that matches nothing also falls
    //    through to text.
    bool selectorOk = false;
    try {
        NodeList* list = doc->querySelectorAll(query);
        if (list) {
            for (size_t i = 0; i < list->length(); i++) {
                Node* n = list->item(i);
                if (n) {
                    matches.push_back(n);
                }
            }
            selectorOk = true;
        }
    } catch (...) {
        // Not a usable CSS selector; fall through to text search.
    }

    // 2) Plain-text fallback. Used when the query was not a CSS selector or
    //    matched nothing. Matches (case-insensitively) element tag names,
    //    any attribute value, and text/comment node data by substring. A
    //    leading "//" (XPath-style "//tag") is stripped first so "//p"
    //    finds <p> elements. No XPath engine exists, so only this shape is
    //    honored.
    if (!selectorOk || matches.empty()) {
        String* needle = query;
        // For a bare "//tag" XPath, match element tag names by exact
        // (case-insensitive) equality rather than substring, approximating
        // the XPath node test; other queries use substring matching.
        bool xpathTag = false;
        if (query->length() >= 3 && query->charAt(0) == '/' &&
            query->charAt(1) == '/') {
            String* rest = query->substring(2, query->length() - 2);
            // Only a simple tag name (no path steps / predicates).
            if (rest->indexOf('/') == SIZE_MAX &&
                rest->indexOf('[') == SIZE_MAX &&
                rest->indexOf('@') == SIZE_MAX) {
                needle = rest;
                xpathTag = true;
            }
        }
        if (needle->length() > 0) {
            // Iterative pre-order walk of the whole document.
            std::vector<Node*> stack;
            stack.push_back(doc);
            while (!stack.empty()) {
                Node* n = stack.back();
                stack.pop_back();
                if (nodeMatchesSearchText(n, needle, xpathTag)) {
                    matches.push_back(n);
                }
                pushChildrenInReverse(n, stack);
            }
        }
    }

    // Register each match and store its nodeId under a fresh searchId.
    std::vector<int> ids;
    ids.reserve(matches.size());
    for (Node* n : matches) {
        ids.push_back(reg->getOrCreate(n));
    }
    std::string searchId = "search-" + std::to_string(++s->searchCounter);
    s->searchResults[searchId] = ids;

    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    addStringMember(result, "searchId", searchId, alloc);
    result.AddMember("resultCount", (int)ids.size(), alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleGetSearchResults(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    if (!hasStringParam(cmd, "searchId")) {
        cmd.sendError(-32602, "searchId is required");
        return;
    }
    std::string searchId = (*cmd.params())["searchId"].GetString();
    auto it = s->searchResults.find(searchId);
    if (it == s->searchResults.end()) {
        cmd.sendError(-32000, "No search results with given identifier found");
        return;
    }
    const std::vector<int>& all = it->second;
    int from = 0;
    int to = (int)all.size();
    if (cmd.params()->HasMember("fromIndex") &&
        (*cmd.params())["fromIndex"].IsInt()) {
        from = (*cmd.params())["fromIndex"].GetInt();
    }
    if (cmd.params()->HasMember("toIndex") &&
        (*cmd.params())["toIndex"].IsInt()) {
        to = (*cmd.params())["toIndex"].GetInt();
    }
    if (from < 0 || to > (int)all.size() || from > to) {
        cmd.sendError(-32000, "Invalid search result range");
        return;
    }
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value ids(rapidjson::kArrayType);
    for (int i = from; i < to; i++) {
        ids.PushBack(all[i], alloc);
    }
    result.AddMember("nodeIds", ids, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleDiscardSearchResults(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    if (!hasStringParam(cmd, "searchId")) {
        cmd.sendError(-32602, "searchId is required");
        return;
    }
    std::string searchId = (*cmd.params())["searchId"].GetString();
    s->searchResults.erase(searchId);
    cmd.sendResultEmpty();
}

void DOMDomain::handleCollectClassNamesFromSubtree(CDPCommand& cmd,
                                                   const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    Node* node = reg->lookup(paramNodeId(cmd));
    if (!node) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    // Pre-order walk of the subtree; collect (deduped, order-preserving)
    // every whitespace-separated token of each element's class attribute.
    std::vector<std::string> ordered;
    std::set<std::string> seen;
    std::vector<Node*> stack;
    stack.push_back(node);
    while (!stack.empty()) {
        Node* n = stack.back();
        stack.pop_back();
        if (n->isElement()) {
            Optional<String*> cls =
                n->asElement()->getAttribute(String::fromUTF8("class", 5));
            if (cls.hasValue() && cls.value()) {
                std::string s8 = cls.value()->toUTF8NonGCString();
                size_t i = 0;
                while (i < s8.size()) {
                    while (i < s8.size() && (unsigned char)s8[i] <= ' ') {
                        i++;
                    }
                    size_t start = i;
                    while (i < s8.size() && (unsigned char)s8[i] > ' ') {
                        i++;
                    }
                    if (i > start) {
                        std::string tok = s8.substr(start, i - start);
                        if (seen.insert(tok).second) {
                            ordered.push_back(tok);
                        }
                    }
                }
            }
        }
        pushChildrenInReverse(n, stack);
    }
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value names(rapidjson::kArrayType);
    for (const std::string& t : ordered) {
        names.PushBack(rapidjson::Value(t.c_str(), t.size(), alloc), alloc);
    }
    result.AddMember("classNames", names, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleGetNodeForLocation(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    Document* doc = mainDocument(wv);
    BrowsingContext* bc = wv->mainBrowsingContext();
    if (!doc || !bc || !cmd.params() || !cmd.params()->HasMember("x") ||
        !cmd.params()->HasMember("y") || !(*cmd.params())["x"].IsNumber() ||
        !(*cmd.params())["y"].IsNumber()) {
        cmd.sendError(-32602, "x and y are required");
        return;
    }
    float x = (float)(*cmd.params())["x"].GetDouble();
    float y = (float)(*cmd.params())["y"].GetDouble();
    // hitTest() runs layout and returns the deepest hit node (may be a text
    // node). Fall back to the document element if nothing was hit.
    Node* node = bc->hitTest(x, y);
    if (!node) {
        node = doc->documentElement();
    }
    if (!node) {
        cmd.sendError(-32000, "No node at given location");
        return;
    }
    int backendNodeId = pushNodePathToFrontend(m_dispatcher, cmd, reg, node);
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("backendNodeId", backendNodeId, alloc);
    // nodeId and backendNodeId share one id space in this registry.
    result.AddMember("nodeId", backendNodeId, alloc);
    addStringMember(result, "frameId", s->frameId, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleCopyTo(CDPCommand& cmd, const std::string& method)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    int nodeId = paramNodeId(cmd);
    Node* node = reg->lookup(nodeId);
    int targetId = 0;
    targetId = paramInt(cmd, "targetNodeId", targetId);
    Node* target = reg->lookup(targetId);
    if (!node || !target) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    // Optional anchor: insert the node before this existing child of
    // target.
    Node* anchor = nullptr;
    if (cmd.params() && cmd.params()->HasMember("insertBeforeNodeId") &&
        (*cmd.params())["insertBeforeNodeId"].IsInt()) {
        anchor = reg->lookup((*cmd.params())["insertBeforeNodeId"].GetInt());
    }

    Node* placed = nullptr;
    try {
        if (method == "copyTo") {
            placed = node->cloneNode(true);
        } else {
            placed = node;
            if (placed->parentNode()) {
                placed->parentNode()->removeChild(placed);
            }
        }
        if (anchor && anchor->parentNode() == target) {
            target->insertBefore(placed, Optional<Node*>(anchor));
        } else {
            target->appendChild(placed);
        }
    } catch (...) {
        cmd.sendError(-32000, "Failed to relocate node");
        return;
    }

    int placedId = reg->getOrCreate(placed);
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("nodeId", placedId, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleGetFlattenedDocument(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    Document* doc = mainDocument(wv);
    if (!doc) {
        cmd.sendError(-32000, "No document");
        return;
    }
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value nodes(rapidjson::kArrayType);
    // Pre-order walk; serialize every node at depth 0 (no nested children),
    // so the flat array carries each node once with childNodeCount but no
    // inline "children" subtrees.
    std::vector<Node*> stack;
    stack.push_back(doc);
    while (!stack.empty()) {
        Node* n = stack.back();
        stack.pop_back();
        rapidjson::Value sn(rapidjson::kObjectType);
        reg->serializeNode(n, 0, sn, alloc);
        nodes.PushBack(sn, alloc);
        pushChildrenInReverse(n, stack);
    }
    result.AddMember("nodes", nodes, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleSetNodeName(CDPCommand& cmd, const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    int nodeId = paramNodeId(cmd);
    Node* node = reg->lookup(nodeId);
    if (!node || !node->isElement() || !hasStringParam(cmd, "name")) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    Element* oldEl = node->asElement();
    Node* parent = oldEl->parentNode();
    if (!parent) {
        cmd.sendError(-32000, "Cannot rename node without a parent");
        return;
    }
    const char* name = (*cmd.params())["name"].GetString();
    Document* doc = oldEl->ownerDocument();
    if (!doc) {
        cmd.sendError(-32000, "No document");
        return;
    }

    // HTML tag names are immutable, so the standard approach is to create a
    // fresh element with the new name, carry over the attributes and
    // children, and swap it into the parent via replaceChild.
    Element* newEl = nullptr;
    try {
        newEl = doc->createElement(String::fromUTF8(name, strlen(name)));
        // Copy attributes.
        size_t ac = oldEl->attributeCount();
        for (size_t i = 0; i < ac; i++) {
            String* an = oldEl->getAssuredAttributeName(i).localName();
            String* av = oldEl->getAssuredAttribute(i);
            if (an) {
                newEl->setAttribute(an, av ? av : String::emptyString);
            }
        }
        // Move children (front to back keeps document order).
        while (Node* c = oldEl->firstChild()) {
            newEl->appendChild(c);
        }
        parent->replaceChild(newEl, oldEl);
    } catch (...) {
        cmd.sendError(-32000, "Failed to rename node");
        return;
    }

    int newNodeId = reg->getOrCreate(newEl);
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("nodeId", newNodeId, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleRequestNode(CDPCommand& cmd, const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    Node* node = nullptr;
    if (hasStringParam(cmd, "objectId")) {
        node = nodeFromObjectId(m_dispatcher,
                                (*cmd.params())["objectId"].GetString());
    }
    if (!node) {
        cmd.sendError(-32000, "Could not find node with given id");
        return;
    }
    int nodeId = pushNodePathToFrontend(m_dispatcher, cmd, reg, node);
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("nodeId", nodeId, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::handleMarkUndoableState(CDPCommand& cmd, const std::string&)
{
    // Undo/redo history is not tracked; acknowledge so clients proceed.
    cmd.sendResultEmpty();
}

void DOMDomain::handleGetNodeStackTraces(CDPCommand& cmd, const std::string&)
{
    // Node creation stack traces are not collected; report none.
    rapidjson::Document out;
    rapidjson::Value result(rapidjson::kObjectType);
    cmd.sendResult(result, out);
}

void DOMDomain::handlePushNodesByBackendIdsToFrontend(CDPCommand& cmd,
                                                      const std::string&)
{
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    // backendNodeId and nodeId share one id space here, so a known
    // backendId maps straight to the same frontend nodeId. The client
    // still has to be told about the node first.
    GCVector<int> pushed;
    if (cmd.params() && cmd.params()->HasMember("backendNodeIds") &&
        (*cmd.params())["backendNodeIds"].IsArray()) {
        const rapidjson::Value& in = (*cmd.params())["backendNodeIds"];
        for (rapidjson::SizeType i = 0; i < in.Size(); i++) {
            int backendId = in[i].IsInt() ? in[i].GetInt() : 0;
            Node* n = reg->lookup(backendId);
            pushed.push_back(pushNodePathToFrontend(m_dispatcher, cmd, reg, n));
        }
    }
    rapidjson::Document out;
    rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value ids(rapidjson::kArrayType);
    for (int id : pushed) {
        ids.PushBack(id, alloc);
    }
    result.AddMember("nodeIds", ids, alloc);
    cmd.sendResult(result, out);
}

void DOMDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    static const CDPMethodHandler<DOMDomain> handlers[] = {
        { "enable", &DOMDomain::handleEnable },
        { "disable", &DOMDomain::handleDisable },
        { "getDocument", &DOMDomain::handleGetDocument },
        { "requestChildNodes", &DOMDomain::handleRequestChildNodes },
        { "querySelector", &DOMDomain::handleQuerySelector },
        { "querySelectorAll", &DOMDomain::handleQuerySelectorAll },
        { "describeNode", &DOMDomain::handleDescribeNode },
        { "getOuterHTML", &DOMDomain::handleGetOuterHTML },
        { "setOuterHTML", &DOMDomain::handleSetOuterHTML },
        { "focus", &DOMDomain::handleFocus },
        { "scrollIntoViewIfNeeded", &DOMDomain::handleScrollIntoViewIfNeeded },
        { "getAttributes", &DOMDomain::handleGetAttributes },
        { "setAttributeValue", &DOMDomain::handleSetAttributeValue },
        { "removeAttribute", &DOMDomain::handleRemoveAttribute },
        { "setNodeValue", &DOMDomain::handleSetNodeValue },
        { "removeNode", &DOMDomain::handleRemoveNode },
        { "resolveNode", &DOMDomain::handleResolveNode },
        { "setFileInputFiles", &DOMDomain::handleSetFileInputFiles },
        { "getBoxModel", &DOMDomain::handleGetBoxModel },
        { "getContentQuads", &DOMDomain::handleGetBoxModel },
        { "performSearch", &DOMDomain::handlePerformSearch },
        { "getSearchResults", &DOMDomain::handleGetSearchResults },
        { "discardSearchResults", &DOMDomain::handleDiscardSearchResults },
        { "collectClassNamesFromSubtree",
          &DOMDomain::handleCollectClassNamesFromSubtree },
        { "getNodeForLocation", &DOMDomain::handleGetNodeForLocation },
        { "copyTo", &DOMDomain::handleCopyTo },
        { "moveTo", &DOMDomain::handleCopyTo },
        { "getFlattenedDocument", &DOMDomain::handleGetFlattenedDocument },
        { "setNodeName", &DOMDomain::handleSetNodeName },
        { "requestNode", &DOMDomain::handleRequestNode },
        { "markUndoableState", &DOMDomain::handleMarkUndoableState },
        { "undo", &DOMDomain::handleMarkUndoableState },
        { "redo", &DOMDomain::handleMarkUndoableState },
        { "getNodeStackTraces", &DOMDomain::handleGetNodeStackTraces },
        { "pushNodesByBackendIdsToFrontend",
          &DOMDomain::handlePushNodesByBackendIdsToFrontend },
    };
    if (CDPHandlerFn<DOMDomain> handler =
            findCDPMethodHandler(handlers, method)) {
        (this->*handler)(cmd, method);
        return;
    }
    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
