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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAXTree__)
#define __StarfishCDPAXTree__

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Starfish {

class Document;
class Element;
class Node;
class NodeRegistry;

// Why a node carries no accessible semantics. The names are the ones the
// Accessibility domain reports inside AXNode.ignoredReasons.
enum class AXIgnoredReason {
    ActiveModalDialog,
    AriaHiddenElement,
    AriaHiddenSubtree,
    EmptyAlt,
    EmptyText,
    NotRendered,
    NotVisible,
    PresentationalRole,
    ProbablyPresentational,
    Uninteresting,
};

const char* axIgnoredReasonName(AXIgnoredReason reason);

// What produced an AX node. One DOM text node produces both a StaticText
// node and the InlineTextBox under it, so the DOM node alone does not
// identify an AX node.
enum class AXNodeKind {
    Root,       // the document
    Element,    // an element
    StaticText, // a text node
    InlineText, // the box under a StaticText node
};

// One node of the accessibility tree.
//
// Lifetime: an AXNode lives exactly as long as the AXTree that owns it, and
// a tree is built for one command and dropped when it returns. Every Node*
// reached from here is attached to the document that was walked, so the DOM
// keeps it alive without this class holding a GC reference.
class AXNode {
public:
    AXNode(Node* domNode, AXNodeKind kind, const std::string& id)
        : m_domNode(domNode)
        , m_kind(kind)
        , m_id(id)
    {
    }

    Node* domNode() const
    {
        return m_domNode;
    }
    AXNodeKind kind() const
    {
        return m_kind;
    }
    const std::string& id() const
    {
        return m_id;
    }

    AXNode* parent() const
    {
        return m_parent;
    }
    // Children including the ignored ones. A node that is not included in
    // the tree never appears here: its own children took its place.
    const std::vector<AXNode*>& children() const
    {
        return m_children;
    }

    // The ARIA role name, or an empty string for a node with no role. An
    // ignored node reports "none" over the protocol whatever this says.
    const std::string& role() const
    {
        return m_role;
    }
    const std::string& name() const
    {
        return m_name;
    }

    bool ignored() const
    {
        return m_ignored;
    }
    bool includedInTree() const
    {
        return m_includedInTree;
    }
    const std::vector<AXIgnoredReason>& ignoredReasons() const
    {
        return m_ignoredReasons;
    }
    // The element an ignoredReason points at, for the reasons that name one
    // (the aria-hidden ancestor, the modal dialog). Null otherwise.
    Element* ignoredReasonTarget() const
    {
        return m_ignoredReasonTarget;
    }

    // The element this node came from, or null for the document and for
    // text.
    Element* element() const;

private:
    friend class AXTreeBuilder;

    Node* m_domNode;
    AXNode* m_parent = nullptr;
    std::vector<AXNode*> m_children;
    AXNodeKind m_kind;
    std::string m_id;
    std::string m_role;
    std::string m_name;
    bool m_ignored = false;
    bool m_includedInTree = true;
    std::vector<AXIgnoredReason> m_ignoredReasons;
    Element* m_ignoredReasonTarget = nullptr;
};

// The accessibility tree of one document.
//
// An id is derived from the node registry's backend id, so it names the same
// node across rebuilds for as long as the DOM node lives. That is what lets
// a later command, or the nodesUpdated event, refer to a node an earlier
// command handed out.
class AXTree {
public:
    static std::unique_ptr<AXTree> build(Document* document,
                                         NodeRegistry* registry);
    ~AXTree();

    AXNode* root() const
    {
        return m_root;
    }
    // The AX node built from this DOM node, or null when the DOM node has
    // none. A text node maps to its StaticText node, not the box under it.
    AXNode* forDOMNode(Node* node) const;
    AXNode* forId(const std::string& id) const;

    Document* document() const
    {
        return m_document;
    }

private:
    friend class AXTreeBuilder;

    Document* m_document = nullptr;
    AXNode* m_root = nullptr;
    std::vector<std::unique_ptr<AXNode>> m_nodes;
    std::map<Node*, AXNode*> m_byDOMNode;
    std::map<std::string, AXNode*> m_byId;
};

} // namespace Starfish

#endif
