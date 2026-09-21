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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAXSerializer__)
#define __StarfishCDPAXSerializer__

#include "rapidjson/document.h"
#include <string>

namespace Starfish {

class AXNode;
class Element;
class Node;
class NodeRegistry;
class PageDomain;

// Turns AX nodes into the Accessibility domain's AXNode objects.
class AXSerializer {
public:
    AXSerializer(NodeRegistry* registry, PageDomain* page,
                 rapidjson::Document::AllocatorType* allocator)
        : m_registry(registry)
        , m_page(page)
        , m_allocator(allocator)
    {
    }

    // `forceNameAndRole` makes an ignored node report its computed name and
    // role instead of "none". queryAXTree is the only caller that asks for
    // it: its results are matched on name and role, so reporting "none"
    // would hide why a node matched.
    rapidjson::Value serialize(AXNode* node, bool forceNameAndRole = false);

    // The node the protocol reports for a DOM node that has no AX node at
    // all. It carries no place in the tree, so it has neither childIds nor
    // a parentId.
    rapidjson::Value serializeMissing(Node* domNode);

private:
    void addRole(rapidjson::Value& out, const std::string& role);
    void addNameWithSources(rapidjson::Value& out, AXNode* node);
    void addDocumentName(rapidjson::Value& out, AXNode* node);
    void addProperties(rapidjson::Value& out, AXNode* node);
    void addDescriptionAndValue(rapidjson::Value& out, AXNode* node);
    void addIgnoredReasons(rapidjson::Value& out, AXNode* node);
    rapidjson::Value relatedNodes(Element* element, const std::string& idRefs,
                                  bool includeText);
    rapidjson::Value stringValue(const std::string& text);

    NodeRegistry* m_registry;
    PageDomain* m_page;
    rapidjson::Document::AllocatorType* m_allocator;
};

} // namespace Starfish

#endif
