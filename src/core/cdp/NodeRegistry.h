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

    // Serialize a CDP Node. depth: 0=no children, N=N levels, <0=full.
    void serializeNode(Node* node, int depth, rapidjson::Value& out,
                       rapidjson::Document::AllocatorType& alloc);

private:
    GCUnorderedMap<Node*, int> m_nodeToId;
    GCUnorderedMap<int, Node*> m_idToNode;
    int m_next;
};

} // namespace Starfish

#endif
