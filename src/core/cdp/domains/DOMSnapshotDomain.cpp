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
#include "DOMSnapshotDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../NodeRegistry.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/DOMRect.h"
#include "core/style/CSSStyleDeclaration.h"

#include "rapidjson/document.h"
#include <cstring>
#include <unordered_map>
#include <vector>
#include <string>

namespace Starfish {

static Document* mainDocument(WebView* wv)
{
    BrowsingContext* bc = wv->mainBrowsingContext();
    return bc ? bc->document() : nullptr;
}

// Dedup string table. index(s) returns the table position for s, appending it
// on first use. The empty string is pre-seeded at index 0 (CDP convention used
// for "no value"). Index -1 means "absent" and is never registered here.
class StringTable {
public:
    StringTable()
    {
        index(std::string());
    }
    int index(const std::string& s)
    {
        auto it = m_map.find(s);
        if (it != m_map.end()) {
            return it->second;
        }
        int idx = (int)m_strings.size();
        m_map.emplace(s, idx);
        m_strings.push_back(s);
        return idx;
    }
    const std::vector<std::string>& strings() const
    {
        return m_strings;
    }

private:
    std::unordered_map<std::string, int> m_map;
    std::vector<std::string> m_strings;
};

static std::string utf8(String* s)
{
    return s ? s->toUTF8NonGCString() : std::string();
}

// Index a String*, returning -1 for null and the empty-string slot (0) for "".
static int strIdxOrEmpty(StringTable& tbl, String* s)
{
    return tbl.index(utf8(s));
}

// Read the requested computed-style property values for an element, returning
// one string-table index per requested property (in request order). Missing /
// empty values map to the empty-string slot.
static void collectStyles(Element* el, const std::vector<std::string>& props,
                          StringTable& tbl, std::vector<int>& outStyleIdx)
{
    CSSStyleDeclaration* style = el->getComputedStyle();
    outStyleIdx.clear();
    outStyleIdx.reserve(props.size());
    for (const std::string& p : props) {
        std::string v;
        if (style) {
            String* name = String::fromUTF8(p.c_str(), p.size());
            String* val = style->getPropertyValue(name);
            v = utf8(val);
        }
        outStyleIdx.push_back(tbl.index(v));
    }
}

void DOMSnapshotDomain::processMessage(CDPCommand& cmd,
                                       const std::string& method)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    if (method == "enable") {
        s->domSnapshotEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->domSnapshotEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method != "captureSnapshot" && method != "getSnapshot") {
        cmd.sendError(-32601, "'method' wasn't found");
        return;
    }

    Document* doc = mainDocument(wv);
    if (!doc) {
        cmd.sendError(-32000, "No document");
        return;
    }

    // Requested computed-style property names. captureSnapshot uses
    // "computedStyles"; legacy getSnapshot uses "computedStyleWhitelist".
    std::vector<std::string> props;
    const char* propKey =
        (method == "getSnapshot") ? "computedStyleWhitelist" : "computedStyles";
    if (cmd.params() && cmd.params()->HasMember(propKey) &&
        (*cmd.params())[propKey].IsArray()) {
        const rapidjson::Value& arr = (*cmd.params())[propKey];
        for (rapidjson::SizeType i = 0; i < arr.Size(); i++) {
            if (arr[i].IsString()) {
                props.push_back(arr[i].GetString());
            }
        }
    }

    bool includeDOMRects = cmd.params() &&
                           cmd.params()->HasMember("includeDOMRects") &&
                           (*cmd.params())["includeDOMRects"].IsBool() &&
                           (*cmd.params())["includeDOMRects"].GetBool();

    StringTable tbl;

    // Pre-order walk: assign each node a document-order index. parentIndex[i]
    // references the parent's index (-1 for the root). The traversal mirrors
    // serializeNode's firstChild/nextSibling order.
    std::vector<Node*> order;
    std::vector<int> parentIndex;
    {
        // Iterative pre-order with an explicit (node, parentIdx) stack reversed
        // so children come out in document order.
        struct Frame {
            Node* node;
            int parentIdx;
        };
        std::vector<Frame> stack;
        stack.push_back({ doc, -1 });
        while (!stack.empty()) {
            Frame f = stack.back();
            stack.pop_back();
            int myIdx = (int)order.size();
            order.push_back(f.node);
            parentIndex.push_back(f.parentIdx);
            std::vector<Node*> kids;
            for (Node* c = f.node->firstChild(); c; c = c->nextSibling()) {
                kids.push_back(c);
            }
            for (size_t i = kids.size(); i-- > 0;) {
                stack.push_back({ kids[i], myIdx });
            }
        }
    }

    rapidjson::Document outd;
    rapidjson::Document::AllocatorType& alloc = outd.GetAllocator();

    // nodes.* parallel arrays.
    rapidjson::Value nodeType(rapidjson::kArrayType);
    rapidjson::Value parentIdxArr(rapidjson::kArrayType);
    rapidjson::Value nodeName(rapidjson::kArrayType);
    rapidjson::Value nodeValue(rapidjson::kArrayType);
    rapidjson::Value backendNodeId(rapidjson::kArrayType);
    rapidjson::Value attributes(rapidjson::kArrayType);

    // layout.* parallel arrays (element nodes with a box only).
    rapidjson::Value layoutNodeIndex(rapidjson::kArrayType);
    rapidjson::Value layoutStyles(rapidjson::kArrayType);
    rapidjson::Value layoutBounds(rapidjson::kArrayType);
    rapidjson::Value layoutText(rapidjson::kArrayType);

    for (size_t i = 0; i < order.size(); i++) {
        Node* n = order[i];
        nodeType.PushBack((int)n->nodeType(), alloc);
        parentIdxArr.PushBack(parentIndex[i], alloc);
        nodeName.PushBack(strIdxOrEmpty(tbl, n->nodeName()), alloc);

        Optional<String*> nv = n->nodeValue();
        String* nvStr = (nv.hasValue() && nv.value()) ? nv.value() : nullptr;
        nodeValue.PushBack(strIdxOrEmpty(tbl, nvStr), alloc);

        backendNodeId.PushBack(reg->getOrCreate(n), alloc);

        // attributes: flat [nameIdx, valueIdx, ...] per node.
        rapidjson::Value attrIdx(rapidjson::kArrayType);
        if (n->isElement() && n->asElement()->hasAttributes()) {
            Element* el = n->asElement();
            size_t count = el->attributeCount();
            for (size_t a = 0; a < count; a++) {
                String* name = el->getAssuredAttributeName(a).localName();
                String* value = el->getAssuredAttribute(a);
                attrIdx.PushBack(strIdxOrEmpty(tbl, name), alloc);
                attrIdx.PushBack(strIdxOrEmpty(tbl, value), alloc);
            }
        }
        attributes.PushBack(attrIdx, alloc);

        // layout entry for element nodes (those that have a box / rect).
        if (n->isElement()) {
            Element* el = n->asElement();
            DOMRect* rect = el->getBoundingClientRect();
            std::vector<int> styleIdx;
            collectStyles(el, props, tbl, styleIdx);

            layoutNodeIndex.PushBack((int)i, alloc);

            rapidjson::Value styleRow(rapidjson::kArrayType);
            for (int si : styleIdx) {
                styleRow.PushBack(si, alloc);
            }
            layoutStyles.PushBack(styleRow, alloc);

            rapidjson::Value bounds(rapidjson::kArrayType);
            bounds.PushBack(rect->x(), alloc);
            bounds.PushBack(rect->y(), alloc);
            bounds.PushBack(rect->width(), alloc);
            bounds.PushBack(rect->height(), alloc);
            layoutBounds.PushBack(bounds, alloc);

            layoutText.PushBack(tbl.index(std::string()), alloc);
        }
    }

    // Assemble document object.
    rapidjson::Value nodesObj(rapidjson::kObjectType);
    nodesObj.AddMember("parentIndex", parentIdxArr, alloc);
    nodesObj.AddMember("nodeType", nodeType, alloc);
    nodesObj.AddMember("nodeName", nodeName, alloc);
    nodesObj.AddMember("nodeValue", nodeValue, alloc);
    nodesObj.AddMember("backendNodeId", backendNodeId, alloc);
    nodesObj.AddMember("attributes", attributes, alloc);

    rapidjson::Value layoutObj(rapidjson::kObjectType);
    layoutObj.AddMember("nodeIndex", layoutNodeIndex, alloc);
    layoutObj.AddMember("styles", layoutStyles, alloc);
    layoutObj.AddMember("bounds", layoutBounds, alloc);
    layoutObj.AddMember("text", layoutText, alloc);

    rapidjson::Value document(rapidjson::kObjectType);
    document.AddMember("documentURL", tbl.index(utf8(doc->urlString())), alloc);
    document.AddMember("title", tbl.index(utf8(doc->title())), alloc);
    document.AddMember("baseURL", tbl.index(utf8(doc->urlString())), alloc);
    document.AddMember("contentLanguage", tbl.index(std::string()), alloc);
    document.AddMember("encodingName", tbl.index(utf8(doc->characterSet())),
                       alloc);
    document.AddMember("nodes", nodesObj, alloc);
    document.AddMember("layout", layoutObj, alloc);

    rapidjson::Value documents(rapidjson::kArrayType);
    documents.PushBack(document, alloc);

    // strings table (built last so every index registered above is included).
    rapidjson::Value strings(rapidjson::kArrayType);
    for (const std::string& str : tbl.strings()) {
        strings.PushBack(rapidjson::Value(str.c_str(), str.size(), alloc),
                         alloc);
    }

    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("documents", documents, alloc);
    result.AddMember("strings", strings, alloc);

    (void)
        includeDOMRects; // bounds are always populated; flag accepted as no-op
    cmd.sendResult(result, outd);
}

} // namespace Starfish

#endif
