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
    if (!oid || strncmp(oid, "OBJ-", 4) != 0) {
        return nullptr;
    }
    int id = atoi(oid + 4);
    Escargot::ObjectRef* obj = disp->remoteObjectStore()->lookup(id);
    if (!obj || !obj->extraData()) {
        return nullptr;
    }
    ScriptWrappable* w = (ScriptWrappable*)obj->extraData();
    return w->isNode() ? w->asNode() : nullptr;
}

static int paramNodeId(CDPCommand& cmd)
{
    if (cmd.params() && cmd.params()->HasMember("nodeId") &&
        (*cmd.params())["nodeId"].IsInt()) {
        return (*cmd.params())["nodeId"].GetInt();
    }
    return 0;
}

void DOMDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    if (method == "enable") {
        s->domEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->domEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getDocument") {
        Document* doc = mainDocument(wv);
        if (!doc) {
            cmd.sendError(-32000, "No document");
            return;
        }
        int depth = 3;
        if (cmd.params() && cmd.params()->HasMember("depth") &&
            (*cmd.params())["depth"].IsInt()) {
            depth = (*cmd.params())["depth"].GetInt();
        }
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value root(rapidjson::kObjectType);
        reg->serializeNode(doc, depth, root, alloc);
        result.AddMember("root", root, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "requestChildNodes") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        int depth = 1;
        if (cmd.params() && cmd.params()->HasMember("depth") &&
            (*cmd.params())["depth"].IsInt()) {
            depth = (*cmd.params())["depth"].GetInt();
        }
        cmd.sendResultEmpty();

        // Emit DOM.setChildNodes.
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("parentId", nodeId, alloc);
        rapidjson::Value nodes(rapidjson::kArrayType);
        for (Node* c = node->firstChild(); c; c = c->nextSibling()) {
            rapidjson::Value child(rapidjson::kObjectType);
            reg->serializeNode(c, depth - 1, child, alloc);
            nodes.PushBack(child, alloc);
        }
        params.AddMember("nodes", nodes, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("DOM.setChildNodes", params, out);
        return;
    }

    if (method == "querySelector") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node || !cmd.params() || !cmd.params()->HasMember("selector") ||
            !(*cmd.params())["selector"].IsString()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        const char* sel = (*cmd.params())["selector"].GetString();
        Element* found =
            node->querySelector(String::fromUTF8(sel, strlen(sel)));
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("nodeId", found ? reg->getOrCreate(found) : 0, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "querySelectorAll") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node || !cmd.params() || !cmd.params()->HasMember("selector") ||
            !(*cmd.params())["selector"].IsString()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        const char* sel = (*cmd.params())["selector"].GetString();
        NodeList* list =
            node->querySelectorAll(String::fromUTF8(sel, strlen(sel)));
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value ids(rapidjson::kArrayType);
        if (list) {
            for (size_t i = 0; i < list->length(); i++) {
                Node* n = list->item(i);
                if (n) {
                    ids.PushBack(reg->getOrCreate(n), alloc);
                }
            }
        }
        result.AddMember("nodeIds", ids, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "describeNode") {
        // Accept objectId, backendNodeId, or nodeId.
        Node* node = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            node = nodeFromObjectId(m_dispatcher,
                                    (*cmd.params())["objectId"].GetString());
        } else if (cmd.params() && cmd.params()->HasMember("backendNodeId") &&
                   (*cmd.params())["backendNodeId"].IsInt()) {
            node = reg->lookup((*cmd.params())["backendNodeId"].GetInt());
        } else {
            node = reg->lookup(paramNodeId(cmd));
        }
        if (!node) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        int depth = 0;
        if (cmd.params() && cmd.params()->HasMember("depth") &&
            (*cmd.params())["depth"].IsInt()) {
            depth = (*cmd.params())["depth"].GetInt();
        }
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value n(rapidjson::kObjectType);
        reg->serializeNode(node, depth, n, alloc);
        if (node->isHTMLIFrameElement()) {
            Optional<BrowsingContext*> context =
                node->asHTMLIFrameElement()->browsingContext();
            if (context) {
                std::string frameId =
                    m_dispatcher->page()->frameIdForBrowsingContext(
                        context.value());
                if (!frameId.empty()) {
                    n.AddMember("frameId",
                                rapidjson::Value(frameId.c_str(),
                                                 frameId.size(), alloc),
                                alloc);
                }
            }
        }
        result.AddMember("node", n, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getOuterHTML") {
        // Accept either objectId (ElementHandle) or nodeId.
        Node* node = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            node = nodeFromObjectId(m_dispatcher,
                                    (*cmd.params())["objectId"].GetString());
        } else {
            node = reg->lookup(paramNodeId(cmd));
        }
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        String* html = node->asElement()->outerHTML();
        std::string s = html ? html->toUTF8NonGCString() : std::string();
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("outerHTML",
                         rapidjson::Value(s.c_str(), s.size(), alloc), alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "setOuterHTML") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node || !node->isElement() || !cmd.params() ||
            !cmd.params()->HasMember("outerHTML") ||
            !(*cmd.params())["outerHTML"].IsString()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        const char* outer = (*cmd.params())["outerHTML"].GetString();
        node->asElement()->setOuterHTML(String::fromUTF8(outer, strlen(outer)));
        cmd.sendResultEmpty();
        return;
    }

    if (method == "focus") {
        // Accept either objectId (ElementHandle) or nodeId.
        Node* node = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            node = nodeFromObjectId(m_dispatcher,
                                    (*cmd.params())["objectId"].GetString());
        } else {
            node = reg->lookup(paramNodeId(cmd));
        }
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        node->asElement()->focus();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "scrollIntoViewIfNeeded") {
        // Accept either objectId (ElementHandle) or nodeId.
        Node* node = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            node = nodeFromObjectId(m_dispatcher,
                                    (*cmd.params())["objectId"].GetString());
        } else {
            node = reg->lookup(paramNodeId(cmd));
        }
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        node->asElement()->scrollIntoViewIfNeeded();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getAttributes") {
        Node* node = reg->lookup(paramNodeId(cmd));
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        Element* el = node->asElement();
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
        return;
    }

    if (method == "setAttributeValue") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node || !node->isElement() || !cmd.params() ||
            !cmd.params()->HasMember("name") ||
            !(*cmd.params())["name"].IsString() ||
            !cmd.params()->HasMember("value") ||
            !(*cmd.params())["value"].IsString()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        const char* name = (*cmd.params())["name"].GetString();
        const char* value = (*cmd.params())["value"].GetString();
        node->asElement()->setAttribute(String::fromUTF8(name, strlen(name)),
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
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("DOM.attributeModified", params, out);
        return;
    }

    if (method == "removeAttribute") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node || !node->isElement() || !cmd.params() ||
            !cmd.params()->HasMember("name") ||
            !(*cmd.params())["name"].IsString()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        const char* name = (*cmd.params())["name"].GetString();
        node->asElement()->removeAttribute(
            String::fromUTF8(name, strlen(name)));
        cmd.sendResultEmpty();

        // Emit DOM.attributeRemoved.
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("nodeId", nodeId, alloc);
        params.AddMember("name", rapidjson::Value(name, strlen(name), alloc),
                         alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("DOM.attributeRemoved", params, out);
        return;
    }

    if (method == "setNodeValue") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node || !node->isCharacterData() || !cmd.params() ||
            !cmd.params()->HasMember("value") ||
            !(*cmd.params())["value"].IsString()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        const char* value = (*cmd.params())["value"].GetString();
        node->asCharacterData()->setData(
            String::fromUTF8(value, strlen(value)));
        cmd.sendResultEmpty();

        // Emit DOM.characterDataModified.
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("nodeId", nodeId, alloc);
        params.AddMember("characterData",
                         rapidjson::Value(value, strlen(value), alloc), alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("DOM.characterDataModified", params, out);
        return;
    }

    if (method == "removeNode") {
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
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("DOM.childNodeRemoved", params, out);
        return;
    }

    if (method == "resolveNode") {
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
                              (Escargot::ValueRef*)objRef, false, remote,
                              alloc);
        result.AddMember("object", remote, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "setFileInputFiles") {
        // Accept objectId (ElementHandle), backendNodeId or nodeId.
        Node* node = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            node = nodeFromObjectId(m_dispatcher,
                                    (*cmd.params())["objectId"].GetString());
        } else if (cmd.params() && cmd.params()->HasMember("backendNodeId") &&
                   (*cmd.params())["backendNodeId"].IsInt()) {
            node = reg->lookup((*cmd.params())["backendNodeId"].GetInt());
        } else {
            node = reg->lookup(paramNodeId(cmd));
        }
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
            File* f = new File(ec, fileBits,
                               String::fromUTF8(base.data(), base.size()),
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
        return;
    }

    if (method == "getBoxModel" || method == "getContentQuads") {
        // Accept either objectId (ElementHandle) or nodeId.
        Node* node = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            node = nodeFromObjectId(m_dispatcher,
                                    (*cmd.params())["objectId"].GetString());
        } else {
            node = reg->lookup(paramNodeId(cmd));
        }
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        // getBoundingClientRect() runs layout if needed and returns CSS-px
        // coordinates (layout px, no devicePixelRatio applied) relative to the
        // viewport -- exactly the coordinate space CDP clients expect.
        DOMRect* rect = node->asElement()->getBoundingClientRect();
        double x = rect->x();
        double y = rect->y();
        double w = rect->width();
        double h = rect->height();

        rapidjson::Document outd;
        rapidjson::Document::AllocatorType& alloc = outd.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);

        // Build a clockwise quad [x1,y1, x2,y2, x3,y3, x4,y4] from a box rect.
        auto makeQuad = [&alloc](double qx, double qy, double qw,
                                 double qh) -> rapidjson::Value {
            rapidjson::Value q(rapidjson::kArrayType);
            q.PushBack(qx, alloc);
            q.PushBack(qy, alloc);
            q.PushBack(qx + qw, alloc);
            q.PushBack(qy, alloc);
            q.PushBack(qx + qw, alloc);
            q.PushBack(qy + qh, alloc);
            q.PushBack(qx, alloc);
            q.PushBack(qy + qh, alloc);
            return q;
        };

        if (method == "getContentQuads") {
            rapidjson::Value quads(rapidjson::kArrayType);
            if (w > 0 || h > 0) {
                quads.PushBack(makeQuad(x, y, w, h), alloc);
            }
            result.AddMember("quads", quads, alloc);
        } else {
            // getBoxModel. MVP: content/padding/border/margin all use the
            // border-box rect from getBoundingClientRect(). Width/height are
            // the border-box dimensions, which is what CDP reports.
            rapidjson::Value model(rapidjson::kObjectType);
            model.AddMember("content", makeQuad(x, y, w, h), alloc);
            model.AddMember("padding", makeQuad(x, y, w, h), alloc);
            model.AddMember("border", makeQuad(x, y, w, h), alloc);
            model.AddMember("margin", makeQuad(x, y, w, h), alloc);
            model.AddMember("width", (int)(w + 0.5), alloc);
            model.AddMember("height", (int)(h + 0.5), alloc);
            result.AddMember("model", model, alloc);
        }
        cmd.sendResult(result, outd);
        return;
    }

    if (method == "performSearch") {
        Document* doc = mainDocument(wv);
        if (!doc || !cmd.params() || !cmd.params()->HasMember("query") ||
            !(*cmd.params())["query"].IsString()) {
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
                    if (hit) {
                        matches.push_back(n);
                    }
                    // Push children so they are visited in document order.
                    std::vector<Node*> kids;
                    for (Node* c = n->firstChild(); c; c = c->nextSibling()) {
                        kids.push_back(c);
                    }
                    for (size_t i = kids.size(); i-- > 0;) {
                        stack.push_back(kids[i]);
                    }
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
        result.AddMember(
            "searchId",
            rapidjson::Value(searchId.c_str(), searchId.size(), alloc), alloc);
        result.AddMember("resultCount", (int)ids.size(), alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getSearchResults") {
        if (!cmd.params() || !cmd.params()->HasMember("searchId") ||
            !(*cmd.params())["searchId"].IsString()) {
            cmd.sendError(-32602, "searchId is required");
            return;
        }
        std::string searchId = (*cmd.params())["searchId"].GetString();
        auto it = s->searchResults.find(searchId);
        if (it == s->searchResults.end()) {
            cmd.sendError(-32000,
                          "No search results with given identifier found");
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
        return;
    }

    if (method == "discardSearchResults") {
        if (!cmd.params() || !cmd.params()->HasMember("searchId") ||
            !(*cmd.params())["searchId"].IsString()) {
            cmd.sendError(-32602, "searchId is required");
            return;
        }
        std::string searchId = (*cmd.params())["searchId"].GetString();
        s->searchResults.erase(searchId);
        cmd.sendResultEmpty();
        return;
    }

    if (method == "collectClassNamesFromSubtree") {
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
            std::vector<Node*> kids;
            for (Node* c = n->firstChild(); c; c = c->nextSibling()) {
                kids.push_back(c);
            }
            for (size_t i = kids.size(); i-- > 0;) {
                stack.push_back(kids[i]);
            }
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
        return;
    }

    if (method == "getNodeForLocation") {
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
        int backendNodeId = reg->getOrCreate(node);
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("backendNodeId", backendNodeId, alloc);
        // nodeId and backendNodeId share one id space in this registry.
        result.AddMember("nodeId", backendNodeId, alloc);
        result.AddMember(
            "frameId",
            rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc),
            alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "copyTo" || method == "moveTo") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        int targetId = 0;
        if (cmd.params() && cmd.params()->HasMember("targetNodeId") &&
            (*cmd.params())["targetNodeId"].IsInt()) {
            targetId = (*cmd.params())["targetNodeId"].GetInt();
        }
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
            anchor =
                reg->lookup((*cmd.params())["insertBeforeNodeId"].GetInt());
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
        return;
    }

    if (method == "getFlattenedDocument") {
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
            std::vector<Node*> kids;
            for (Node* c = n->firstChild(); c; c = c->nextSibling()) {
                kids.push_back(c);
            }
            for (size_t i = kids.size(); i-- > 0;) {
                stack.push_back(kids[i]);
            }
        }
        result.AddMember("nodes", nodes, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "setNodeName") {
        int nodeId = paramNodeId(cmd);
        Node* node = reg->lookup(nodeId);
        if (!node || !node->isElement() || !cmd.params() ||
            !cmd.params()->HasMember("name") ||
            !(*cmd.params())["name"].IsString()) {
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
        return;
    }

    if (method == "requestNode") {
        Node* node = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            node = nodeFromObjectId(m_dispatcher,
                                    (*cmd.params())["objectId"].GetString());
        }
        if (!node) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        int nodeId = reg->getOrCreate(node);
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("nodeId", nodeId, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "markUndoableState" || method == "undo" || method == "redo") {
        // Undo/redo history is not tracked; acknowledge so clients proceed.
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getNodeStackTraces") {
        // Node creation stack traces are not collected; report none.
        rapidjson::Document out;
        rapidjson::Value result(rapidjson::kObjectType);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "pushNodesByBackendIdsToFrontend") {
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value ids(rapidjson::kArrayType);
        if (cmd.params() && cmd.params()->HasMember("backendNodeIds") &&
            (*cmd.params())["backendNodeIds"].IsArray()) {
            const rapidjson::Value& in = (*cmd.params())["backendNodeIds"];
            for (rapidjson::SizeType i = 0; i < in.Size(); i++) {
                int backendId = in[i].IsInt() ? in[i].GetInt() : 0;
                // backendNodeId and nodeId share one id space here, so a known
                // backendId maps straight to the same frontend nodeId.
                Node* n = reg->lookup(backendId);
                ids.PushBack(n ? reg->getOrCreate(n) : 0, alloc);
            }
        }
        result.AddMember("nodeIds", ids, alloc);
        cmd.sendResult(result, out);
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
