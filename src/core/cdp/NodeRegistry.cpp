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
#include "NodeRegistry.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"

#include <string>

namespace Starfish {

NodeRegistry::NodeRegistry()
    : m_next(1)
{
}

int NodeRegistry::getOrCreate(Node* node)
{
    auto it = m_nodeToId.find(node);
    if (it != m_nodeToId.end()) {
        return it->second;
    }
    int id = m_next++;
    m_nodeToId[node] = id;
    m_idToNode[id] = node;
    return id;
}

Node* NodeRegistry::lookup(int nodeId)
{
    auto it = m_idToNode.find(nodeId);
    if (it != m_idToNode.end()) {
        return it->second;
    }
    return nullptr;
}

bool NodeRegistry::has(Node* node) const
{
    return m_nodeToId.find(node) != m_nodeToId.end();
}

void NodeRegistry::reset()
{
    m_nodeToId.clear();
    m_idToNode.clear();
    m_next = 1;
}

static void addStringMember(rapidjson::Value& obj, const char* key,
                            const std::string& v,
                            rapidjson::Document::AllocatorType& alloc)
{
    obj.AddMember(rapidjson::Value(key, alloc),
                  rapidjson::Value(v.c_str(), v.size(), alloc), alloc);
}

void NodeRegistry::serializeNode(Node* node, int depth, rapidjson::Value& out,
                                 rapidjson::Document::AllocatorType& alloc)
{
    out.SetObject();

    int nodeId = getOrCreate(node);
    out.AddMember("nodeId", nodeId, alloc);
    out.AddMember("backendNodeId", nodeId, alloc);

    if (node->parentNode()) {
        out.AddMember("parentId", getOrCreate(node->parentNode()), alloc);
    }

    out.AddMember("nodeType", (int)node->nodeType(), alloc);

    String* nodeName = node->nodeName();
    addStringMember(out, "nodeName",
                    nodeName ? nodeName->toUTF8NonGCString() : std::string(),
                    alloc);

    std::string localName;
    if (node->isElement()) {
        String* ln = node->localName();
        if (ln) {
            localName = ln->toUTF8NonGCString();
        }
    }
    addStringMember(out, "localName", localName, alloc);

    std::string nodeValue;
    Optional<String*> nv = node->nodeValue();
    if (nv.hasValue() && nv.value()) {
        nodeValue = nv.value()->toUTF8NonGCString();
    }
    addStringMember(out, "nodeValue", nodeValue, alloc);

    // child count via firstChild/nextSibling traversal
    int childCount = 0;
    for (Node* c = node->firstChild(); c; c = c->nextSibling()) {
        childCount++;
    }
    out.AddMember("childNodeCount", childCount, alloc);

    // attributes (element only)
    if (node->isElement() && node->asElement()->hasAttributes()) {
        Element* el = node->asElement();
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
        out.AddMember("attributes", attrs, alloc);
    }

    // children if depth allows (depth<0 = full, else descend while depth>0)
    if ((depth < 0 || depth > 0) && childCount > 0) {
        rapidjson::Value children(rapidjson::kArrayType);
        int childDepth = (depth < 0) ? -1 : depth - 1;
        for (Node* c = node->firstChild(); c; c = c->nextSibling()) {
            rapidjson::Value child(rapidjson::kObjectType);
            serializeNode(c, childDepth, child, alloc);
            children.PushBack(child, alloc);
        }
        out.AddMember("children", children, alloc);
    }
}

} // namespace Starfish

#endif
