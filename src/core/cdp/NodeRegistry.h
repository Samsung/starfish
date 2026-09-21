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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPNodeRegistry__)
#define __StarfishCDPNodeRegistry__

#include "rapidjson/document.h"

namespace Starfish {

class Node;

// Node* <-> nodeId mapping. GC: inherited so GC tracks the held Node* maps.
// Main-thread only.
class NodeRegistry : public gc {
public:
    NodeRegistry();

    int getOrCreate(Node* node); // register if new, return nodeId (>=1)
    Node* lookup(int nodeId);    // nullptr if absent
    bool has(Node* node) const;
    void reset(); // navigate: drop all mappings, m_next=1

    // An id alone does not mean the client knows the node: the registry also
    // hands out ids to commands that never serialize one, such as the
    // Accessibility queries. These two say what the client has actually been
    // sent, so DOM.setChildNodes only has to carry the levels it is missing.
    bool wasSent(Node* node) const;
    bool childrenWereSent(Node* node) const;
    void markChildrenSent(Node* node);

    // Serialize a CDP Node. depth: 0=no children, N=N levels, <0=full.
    // `send` records the node, and every descendant written with it, as
    // known to the client. Pass false for a command that returns a node
    // without binding it, such as DOM.describeNode.
    void serializeNode(Node* node, int depth, rapidjson::Value& out,
                       rapidjson::Document::AllocatorType& alloc,
                       bool send = true);

private:
    struct Entry {
        int id;
        // Both are about what the client has seen, not about the node.
        bool sent;
        bool childrenSent;
    };

    Entry* entryFor(Node* node);

    GCUnorderedMap<Node*, Entry> m_nodeToId;
    GCUnorderedMap<int, Node*> m_idToNode;
    int m_next;
};

} // namespace Starfish

#endif
