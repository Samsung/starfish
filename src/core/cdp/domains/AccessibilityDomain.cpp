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
#include "PageDomain.h"
#include "../CDPCommand.h"
#include "../CDPDispatcher.h"
#include "../CDPSession.h"
#include "../NodeRegistry.h"
#include "../RemoteObject.h"
#include "../TargetContext.h"
#include "../AXChangeNotifier.h"
#include "../ax/AXSerializer.h"
#include "../ax/AXTree.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/dom/ShadowRoot.h"
#include "core/modules/message_loop/Timer.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "EscargotPublic.h"

#include "rapidjson/document.h"
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace Starfish {

namespace {

    Node* nodeFromObjectId(CDPDispatcher* dispatcher, const char* objectId)
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

    // Add the children of an AX node, and keep going through the layers the
    // client cannot address: an ignored node and a node with no DOM node of its
    // own are both in the tree only to hold their children, so their children
    // come along (AddChildren in inspector_accessibility_agent.cc).
    void collectChildren(AXNode* node, AXSerializer& serializer,
                         rapidjson::Value& nodes,
                         rapidjson::Document::AllocatorType& allocator)
    {
        std::vector<AXNode*> reachable(node->children().rbegin(),
                                       node->children().rend());
        while (!reachable.empty()) {
            AXNode* descendant = reachable.back();
            reachable.pop_back();
            if (descendant->ignored()) {
                reachable.insert(reachable.end(),
                                 descendant->children().rbegin(),
                                 descendant->children().rend());
            }
            nodes.PushBack(serializer.serialize(descendant), allocator);
        }
    }

    // The unignored nodes directly under this one. An ignored node is only a
    // layer the client cannot address, so its own children stand in for it.
    // Depth in getFullAXTree counts these, not the layers in between.
    void collectUnignoredChildren(AXNode* node, std::vector<AXNode*>& out)
    {
        for (AXNode* child : node->children()) {
            if (child->ignored()) {
                collectUnignoredChildren(child, out);
            } else {
                out.push_back(child);
            }
        }
    }

} // namespace

// Resolve the nodeId, backendNodeId or objectId every node-taking command
// accepts. Returns null and answers the command on failure.
static Node* resolveTargetNode(CDPDispatcher* dispatcher, CDPCommand& cmd)
{
    NodeRegistry* registry = dispatcher->nodeRegistry();
    if (!cmd.params()) {
        cmd.sendError(-32000,
                      "Either nodeId, backendNodeId or objectId "
                      "must be specified");
        return nullptr;
    }
    if (cmd.params()->HasMember("nodeId") &&
        (*cmd.params())["nodeId"].IsInt()) {
        Optional<Node*> node =
            registry->lookup((*cmd.params())["nodeId"].GetInt());
        if (!node) {
            cmd.sendError(-32000, "Could not find node with given id");
            return nullptr;
        }
        return node.value();
    }
    if (cmd.params()->HasMember("backendNodeId") &&
        (*cmd.params())["backendNodeId"].IsInt()) {
        Optional<Node*> node =
            registry->lookup((*cmd.params())["backendNodeId"].GetInt());
        if (!node) {
            cmd.sendError(-32000, "No node found for given backend id");
            return nullptr;
        }
        return node.value();
    }
    if (cmd.params()->HasMember("objectId") &&
        (*cmd.params())["objectId"].IsString()) {
        Node* node = nodeFromObjectId(dispatcher,
                                      (*cmd.params())["objectId"].GetString());
        if (!node) {
            cmd.sendError(-32000, "Invalid remote object id");
            return nullptr;
        }
        return node;
    }
    cmd.sendError(-32000,
                  "Either nodeId, backendNodeId or objectId "
                  "must be specified");
    return nullptr;
}

// The document a command works on: the frame it names, or the main frame.
Optional<Document*> AccessibilityDomain::documentFor(CDPCommand& cmd)
{
    Optional<BrowsingContext*> context =
        m_dispatcher->webView()->mainBrowsingContext();
    if (cmd.params() && cmd.params()->HasMember("frameId") &&
        (*cmd.params())["frameId"].IsString()) {
        context = m_dispatcher->page()->browsingContextForFrameId(
            (*cmd.params())["frameId"].GetString());
    }
    if (!context) {
        return Optional<Document*>();
    }
    return context->document();
}

std::unique_ptr<AXTree> AccessibilityDomain::buildTree(Document* document)
{
    // The tree is read off the layout boxes, so layout has to be current
    // before it is walked. Chromium updates style and layout at the top of
    // every accessibility command for the same reason.
    m_dispatcher->webView()->layoutIfNeeded();
    return AXTree::build(document, m_dispatcher->nodeRegistry());
}

void AccessibilityDomain::processMessage(CDPCommand& cmd,
                                         const std::string& method)
{
    CDPSession* session = m_dispatcher->session();
    NodeRegistry* registry = m_dispatcher->nodeRegistry();

    if (method == "enable") {
        session->accessibilityEnabled = true;
        // Let the DOM mutation sites start reporting changes.
        g_axNodeChangesObserved = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        session->accessibilityEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getFullAXTree" || method == "getRootAXNode") {
        if (method == "getRootAXNode" && !session->accessibilityEnabled) {
            cmd.sendError(-32000, "Accessibility has not been enabled.");
            return;
        }
        Optional<Document*> document = documentFor(cmd);
        if (!document) {
            cmd.sendError(-32000, "No document");
            return;
        }

        int depth = -1; // unlimited
        if (cmd.params() && cmd.params()->HasMember("depth") &&
            (*cmd.params())["depth"].IsInt()) {
            depth = (*cmd.params())["depth"].GetInt();
        }

        std::unique_ptr<AXTree> tree = buildTree(document.value());
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& allocator = out.GetAllocator();
        AXSerializer serializer(registry, m_dispatcher->page(), &allocator);
        rapidjson::Value result(rapidjson::kObjectType);

        if (method == "getRootAXNode") {
            session->axNodesRequested.insert(tree->root()->id());
            result.AddMember("node", serializer.serialize(tree->root()),
                             allocator);
            cmd.sendResult(result, out);
            return;
        }

        // Depth counts unignored levels. Every level also brings the
        // ignored layers under it, because the client cannot ask for the
        // children of a node it was never told about.
        rapidjson::Value nodes(rapidjson::kArrayType);
        nodes.PushBack(serializer.serialize(tree->root()), allocator);
        std::vector<std::pair<AXNode*, int>> pending;
        pending.push_back(std::make_pair(tree->root(), 1));
        for (size_t i = 0; i < pending.size(); i++) {
            AXNode* node = pending[i].first;
            int level = pending[i].second;
            collectChildren(node, serializer, nodes, allocator);
            if (depth != -1 && level >= depth) {
                continue;
            }
            std::vector<AXNode*> unignored;
            collectUnignoredChildren(node, unignored);
            for (AXNode* child : unignored) {
                pending.push_back(std::make_pair(child, level + 1));
            }
        }
        result.AddMember("nodes", nodes, allocator);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getAXNodeAndAncestors" || method == "getPartialAXTree") {
        if (method == "getAXNodeAndAncestors" &&
            !session->accessibilityEnabled) {
            cmd.sendError(-32000, "Accessibility has not been enabled.");
            return;
        }

        Node* targetNode = resolveTargetNode(m_dispatcher, cmd);
        if (!targetNode) {
            return;
        }
        Document* document = targetNode->document();
        if (!document) {
            cmd.sendError(-32000, "No document");
            return;
        }

        std::unique_ptr<AXTree> tree = buildTree(document);
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& allocator = out.GetAllocator();
        AXSerializer serializer(registry, m_dispatcher->page(), &allocator);
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value nodes(rapidjson::kArrayType);

        AXNode* target = tree->forDOMNode(targetNode);
        if (!target) {
            nodes.PushBack(serializer.serializeMissing(targetNode), allocator);
        } else {
            nodes.PushBack(serializer.serialize(target), allocator);
        }

        bool fetchRelatives = true;
        if (method == "getPartialAXTree" && cmd.params() &&
            cmd.params()->HasMember("fetchRelatives") &&
            (*cmd.params())["fetchRelatives"].IsBool()) {
            fetchRelatives = (*cmd.params())["fetchRelatives"].GetBool();
        }

        if (method == "getPartialAXTree" && !fetchRelatives) {
            result.AddMember("nodes", nodes, allocator);
            cmd.sendResult(result, out);
            return;
        }
        if (!target && method == "getAXNodeAndAncestors") {
            // A node with no AX node has no ancestors to report either.
            result.AddMember("nodes", nodes, allocator);
            cmd.sendResult(result, out);
            return;
        }

        if (method == "getPartialAXTree" && target && !target->ignored()) {
            collectChildren(target, serializer, nodes, allocator);
        }

        // The ancestors. When the inspected node is ignored it is missing
        // from the first ancestor's childIds, so it is put back at the
        // front to keep the chain connected.
        AXNode* ancestor = target ? target->parent()
                                  : nearestAXAncestor(tree.get(), targetNode);
        bool connectTarget = !target || target->ignored();
        while (ancestor) {
            rapidjson::Value value = serializer.serialize(ancestor);
            if (connectTarget) {
                rapidjson::Value childIds(rapidjson::kArrayType);
                const char* id = target ? target->id().c_str() : "ax-no-node";
                childIds.PushBack(rapidjson::Value(id, strlen(id), allocator),
                                  allocator);
                for (rapidjson::SizeType i = 0; i < value["childIds"].Size();
                     i++) {
                    childIds.PushBack(
                        rapidjson::Value(value["childIds"][i], allocator),
                        allocator);
                }
                value["childIds"].Swap(childIds);
                connectTarget = false;
            }
            nodes.PushBack(value, allocator);
            ancestor = ancestor->parent();
        }
        if (method == "getAXNodeAndAncestors") {
            for (rapidjson::SizeType i = 0; i < nodes.Size(); i++) {
                session->axNodesRequested.insert(
                    nodes[i]["nodeId"].GetString());
            }
        }

        result.AddMember("nodes", nodes, allocator);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "queryAXTree") {
        Node* rootNode = resolveTargetNode(m_dispatcher, cmd);
        if (!rootNode) {
            return;
        }
        // The accessibility tree has no node for a shadow root, so the
        // search starts at the host instead.
        if (rootNode->isShadowRoot()) {
            rootNode = rootNode->asShadowRoot()->host();
        }
        if (!rootNode) {
            cmd.sendError(-32602, "Root DOM node could not be found");
            return;
        }
        Document* document = rootNode->document();
        if (!document) {
            cmd.sendError(-32000, "No document");
            return;
        }

        std::string wantedName;
        std::string wantedRole;
        bool hasName = false;
        bool hasRole = false;
        if (cmd.params()->HasMember("accessibleName") &&
            (*cmd.params())["accessibleName"].IsString()) {
            wantedName = (*cmd.params())["accessibleName"].GetString();
            hasName = true;
        }
        if (cmd.params()->HasMember("role") &&
            (*cmd.params())["role"].IsString()) {
            wantedRole = (*cmd.params())["role"].GetString();
            hasRole = true;
        }

        std::unique_ptr<AXTree> tree = buildTree(document);
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& allocator = out.GetAllocator();
        AXSerializer serializer(registry, m_dispatcher->page(), &allocator);
        rapidjson::Value nodes(rapidjson::kArrayType);

        AXNode* start = tree->forDOMNode(rootNode);
        std::vector<AXNode*> reachable;
        if (start) {
            reachable.push_back(start);
        }
        while (!reachable.empty()) {
            AXNode* node = reachable.back();
            reachable.pop_back();
            reachable.insert(reachable.end(), node->children().rbegin(),
                             node->children().rend());
            // Chromium's result carries the StaticText node but not the box
            // under it: see the two nodes the pinned
            // accessibility-query-axtree-expected.txt dumps for the name
            // "title".
            if (node->kind() == AXNodeKind::InlineText) {
                continue;
            }
            if (hasName && node->name() != wantedName) {
                continue;
            }
            // An ignored node reports the role "none" over the protocol,
            // but it is matched on the role it actually computed.
            if (hasRole && node->role() != wantedRole) {
                continue;
            }
            nodes.PushBack(serializer.serialize(node, true), allocator);
        }

        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("nodes", nodes, allocator);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getChildAXNodes") {
        if (!session->accessibilityEnabled) {
            cmd.sendError(-32000, "Accessibility has not been enabled.");
            return;
        }
        if (!cmd.params() || !cmd.params()->HasMember("id") ||
            !(*cmd.params())["id"].IsString()) {
            cmd.sendError(-32602, "id is required");
            return;
        }
        Optional<Document*> document = documentFor(cmd);
        if (!document) {
            cmd.sendError(-32000, "No document");
            return;
        }

        std::unique_ptr<AXTree> tree = buildTree(document.value());
        AXNode* parent = tree->forId((*cmd.params())["id"].GetString());
        if (!parent) {
            cmd.sendError(-32602, "Invalid ID");
            return;
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& allocator = out.GetAllocator();
        AXSerializer serializer(registry, m_dispatcher->page(), &allocator);
        rapidjson::Value nodes(rapidjson::kArrayType);
        collectChildren(parent, serializer, nodes, allocator);
        for (rapidjson::SizeType i = 0; i < nodes.Size(); i++) {
            session->axNodesRequested.insert(nodes[i]["nodeId"].GetString());
        }

        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("nodes", nodes, allocator);
        cmd.sendResult(result, out);
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

namespace {

    // The coalescing window Chromium uses between a change and the event
    // (kNodeSyncThrottlePeriod in inspector_accessibility_agent.cc).
    const unsigned kDirtyFlushMilliseconds = 250;

    // Re-resolves the session when the timer fires: the session can be gone by
    // then, so the id is kept rather than the pointer.
    struct DirtyFlushData {
        CDPDispatcher* dispatcher;
        std::string sessionId;
    };

} // namespace

void AccessibilityDomain::emitLoadComplete(const std::string& sessionId)
{
    TargetContext* context = m_dispatcher->contextForSession(sessionId);
    if (!context || !context->session ||
        !context->session->accessibilityEnabled) {
        return;
    }
    Optional<BrowsingContext*> browsingContext =
        context->webView->mainBrowsingContext();
    if (!browsingContext || !browsingContext->document()) {
        return;
    }

    // The serializer reads the frame id through the page domain, which
    // takes its target from the dispatcher. Nothing selected one here.
    CDPDispatcher::ScopedTarget target(m_dispatcher, context);

    context->webView->layoutIfNeeded();
    std::unique_ptr<AXTree> tree =
        AXTree::build(browsingContext->document(), context->nodeRegistry);

    // The tree the client held belongs to the previous document. Only the
    // new root is known to it now.
    context->session->axDirtyNodes.clear();
    context->session->axNodesRequested.clear();
    context->session->axNodesRequested.insert(tree->root()->id());

    rapidjson::Document out;
    rapidjson::Document::AllocatorType& allocator = out.GetAllocator();
    AXSerializer serializer(context->nodeRegistry, m_dispatcher->page(),
                            &allocator);
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("root", serializer.serialize(tree->root()), allocator);
    CDPCommand event(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    event.sendEvent("Accessibility.loadComplete", params, out);
}

void AccessibilityDomain::markNodeDirty(Node* domNode)
{
    if (!domNode) {
        return;
    }
    for (TargetContext* context : m_dispatcher->contexts()) {
        CDPSession* session = context->session;
        if (!session || !session->accessibilityEnabled ||
            session->axNodesRequested.empty()) {
            continue;
        }
        // The id is derived from the backend node id, so it can be formed
        // without building a tree. Building one here would cost a full
        // walk on every DOM mutation.
        if (!context->nodeRegistry->has(domNode)) {
            continue;
        }
        std::string id =
            "ax-" + std::to_string(context->nodeRegistry->getOrCreate(domNode));
        if (!session->axNodesRequested.count(id)) {
            continue;
        }
        session->axDirtyNodes.insert(id);

        GlobalScope* scope =
            context->webView->mainBrowsingContext()
                ? context->webView->mainBrowsingContext()->window()
                : nullptr;
        if (session->axFlushTimerId != SIZE_MAX || !context->webView->timer() ||
            !scope) {
            continue;
        }
        DirtyFlushData* data = new DirtyFlushData();
        data->dispatcher = m_dispatcher;
        data->sessionId = session->sessionId;
        session->axFlushTimerId = context->webView->timer()->addTimer(
            kDirtyFlushMilliseconds, scope,
            &AccessibilityDomain::onDirtyFlushTimer, data, false);
    }
}

void AccessibilityDomain::onDirtyFlushTimer(void* data)
{
    DirtyFlushData* flush = static_cast<DirtyFlushData*>(data);
    CDPDispatcher* dispatcher = flush->dispatcher;
    std::string sessionId = flush->sessionId;
    delete flush;

    TargetContext* context = dispatcher->contextForSession(sessionId);
    if (!context || !context->session) {
        return;
    }
    CDPSession* session = context->session;
    session->axFlushTimerId = SIZE_MAX;
    if (session->axDirtyNodes.empty()) {
        return;
    }
    std::set<std::string> dirty;
    dirty.swap(session->axDirtyNodes);

    Optional<BrowsingContext*> browsingContext =
        context->webView->mainBrowsingContext();
    if (!browsingContext || !browsingContext->document()) {
        return;
    }
    CDPDispatcher::ScopedTarget target(dispatcher, context);
    context->webView->layoutIfNeeded();
    std::unique_ptr<AXTree> tree =
        AXTree::build(browsingContext->document(), context->nodeRegistry);

    rapidjson::Document out;
    rapidjson::Document::AllocatorType& allocator = out.GetAllocator();
    AXSerializer serializer(context->nodeRegistry, dispatcher->page(),
                            &allocator);
    rapidjson::Value nodes(rapidjson::kArrayType);
    for (const std::string& id : dirty) {
        // A node the change removed from the tree has nothing to report.
        if (AXNode* node = tree->forId(id)) {
            nodes.PushBack(serializer.serialize(node), allocator);
        }
    }
    if (nodes.Empty()) {
        return;
    }

    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("nodes", nodes, allocator);
    CDPCommand event(dispatcher, Optional<int64_t>(), sessionId, nullptr);
    event.sendEvent("Accessibility.nodesUpdated", params, out);
}

// The AX node of the nearest ancestor that has one. Used when the inspected
// node itself has none, so the reported chain still starts somewhere.
AXNode* AccessibilityDomain::nearestAXAncestor(AXTree* tree, Node* domNode)
{
    for (Node* parent = domNode->isShadowRoot()
                            ? domNode->asShadowRoot()->host()
                            : domNode->parentNode();
         parent;
         parent = parent->isShadowRoot() ? parent->asShadowRoot()->host()
                                         : parent->parentNode()) {
        if (AXNode* found = tree->forDOMNode(parent)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace Starfish

#endif
