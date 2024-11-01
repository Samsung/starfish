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

#ifndef __StarfishTraverse__
#define __StarfishTraverse__

#include "core/dom/Element.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/HTMLSlotElement.h"

namespace Starfish {
class Traverse {
    Traverse()
    {
    }

public:
    template <typename Func>
    static void traverse(Node* node, Func func)
    {
        func(node);
        Node* child = node->firstChild();
        while (child) {
            traverse(child, func);
            child = child->nextSibling();
        }
    }

    template <typename Func>
    static void traverseIncludingShadowDOM(Node* node, Func func)
    {
        func(node);
        Node* child = node->firstChild();
        while (child) {
            traverseIncludingShadowDOM(child, func);
            child = child->nextSibling();
        }
        if (node->isElement()) {
            auto sr = node->asElement()->internalShadowRoot();
            if (sr) {
                func(sr.value());
                sr->firstChild();
                Node* child = sr->firstChild();
                while (child) {
                    traverseIncludingShadowDOM(child, func);
                    child = child->nextSibling();
                }
            }
        }
    }

    template <typename Func>
    static Node* findDescendant(Node* parent, Func matchingRule)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (matchingRule(child)) {
                return child;
            } else {
                Node* matchedDescendant = findDescendant(child, matchingRule);
                if (matchedDescendant) {
                    return matchedDescendant;
                }
            }
            child = child->nextSibling();
        }
        return nullptr;
    }

    template <typename Coll, typename Func>
    static bool collectDescendants(Coll& collection, Node* root, Func filter,
                                   bool shouldOnlyMatchFirstElement)
    {
        Node* child = root->firstChild();
        while (child) {
            if (child->isElement()) {
                Element* elm = child->asElement();
                if (filter(elm)) {
                    collection.push_back(child->asElement());
                    if (shouldOnlyMatchFirstElement) {
                        return true;
                    }
                }
            }

            if (collectDescendants(collection, child, filter,
                                   shouldOnlyMatchFirstElement)) {
                return true;
            }
            child = child->nextSibling();
        }

        return false;
    }

    static bool findMatchedDescendants(Node* root,
                                       bool (*filter)(Element*, void*),
                                       void* filterData,
                                       void (*callback)(Element*, void*),
                                       void* callbackData,
                                       bool shouldOnlyMatchFirstElement)
    {
        Node* child = root->firstChild();
        while (child) {
            if (child->isElement()) {
                Element* elm = child->asElement();
                if (filter(elm, filterData)) {
                    callback(elm, callbackData);
                    if (shouldOnlyMatchFirstElement) {
                        return true;
                    }
                }
            }

            if (findMatchedDescendants(child, filter, filterData, callback,
                                       callbackData,
                                       shouldOnlyMatchFirstElement)) {
                return true;
            }
            child = child->nextSibling();
        }
        return false;
    }

    template <typename Func>
    static Node* firstChild(Node* parent, Func matchingRule)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (matchingRule(child)) {
                return child;
            } else {
                child = child->nextSibling();
            }
        }
        return nullptr;
    }

    static Node* firstChild(Node* parent)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (child->isElement()) {
                return child;
            } else {
                child = child->nextSibling();
            }
        }
        return nullptr;
    }

    template <typename Func>
    static Node* lastChild(Node* parent, Func matchingRule)
    {
        Node* child = parent->lastChild();
        while (child) {
            if (matchingRule(child)) {
                return child;
            } else {
                child = child->previousSibling();
            }
        }
        return nullptr;
    }

    template <typename Func>
    static Node* firstAncestor(Node* current, Func matchingRule)
    {
        Node* ancestor = current->parentNode();
        while (ancestor && !matchingRule(ancestor)) {
            ancestor = ancestor->parentNode();
        }
        return ancestor;
    }

    static Node* nextAncestorSibling(const Node* current,
                                     const Node* stayWithin)
    {
        STARFISH_ASSERT(!current->nextSibling());
        STARFISH_ASSERT(current != stayWithin);
        for (Node* parent = current->parentNode(); parent;
             parent = parent->parentNode()) {
            if (parent == stayWithin) {
                return 0;
            }
            if (parent->nextSibling()) {
                return parent->nextSibling();
            }
        }
        return 0;
    }

    static Node* next(const Node* current, const Node* stayWithin)
    {
        if (current->hasChildNodes()) {
            return current->firstChild();
        }
        if (current == stayWithin) {
            return 0;
        }
        if (current->nextSibling()) {
            return current->nextSibling();
        }
        return nextAncestorSibling(current, stayWithin);
    }

    static Node* previous(const Node* current, const Node* stayWithin)
    {
        if (current == stayWithin) {
            return nullptr;
        }

        if (current->previousSibling()) {
            Node* previous = current->previousSibling();
            while (Node* child = previous->lastChild()) {
                previous = child;
            }
            return previous;
        }

        return current->parentNode();
    }

    static Element* nextElement(Node* current, const Node* stayWithin)
    {
        Node* node = next(current, stayWithin);
        while (node && !node->isElement())
            node = next(node, stayWithin);
        if (node) {
            return node->asElement();
        }
        return nullptr;
    }

    static Node* nextSkippingChildren(Node* current, const Node* stayWithin)
    {
        if (current == stayWithin) {
            return 0;
        }
        if (current->nextSibling()) {
            return current->nextSibling();
        }
        return nextAncestorSibling(current, stayWithin);
    }

    static Element* nextSkippingChildrenElement(Node* current,
                                                const Node* stayWithin)
    {
        Node* node = nextSkippingChildren(current, stayWithin);
        while (node && !node->isElement()) {
            node = nextSkippingChildren(node, stayWithin);
        }
        if (node) {
            return node->asElement();
        }
        return nullptr;
    }

    static Node* nextPostOrder(Node* current, const Node* stayWithin)
    {
        if (current == stayWithin) {
            return nullptr;
        }
        if (!current->nextSibling()) {
            return current->parentNode();
        }

        Node* next = current->nextSibling();
        while (Node* firstChild = next->firstChild()) {
            next = firstChild;
        }
        return next;
    }

    static Node* childAt(Node* parent, unsigned index)
    {
        Node* child = parent->firstChild();
        while (child && index) {
            child = child->nextSibling();
            index--;
        }
        return child;
    }

    static Node* childAtOrNull(Node* parent, unsigned index)
    {
        Node* child = parent->firstChild();
        while (index) {
            if (!child) {
                return nullptr;
            }
            child = child->nextSibling();
            index--;
        }
        return child;
    }

    static Node* commonAncestor(Node* nodeA, Node* nodeB)
    {
        STARFISH_ASSERT(nodeA && nodeB);

        if (nodeA == nodeB) {
            return nodeA;
        }

        if (root(nodeA) != root(nodeB)) {
            return nullptr;
        }

        int depthA = 0;
        int depthB = 0;
        for (Node* node = nodeA; node; node = node->parentNode()) {
            depthA++;
        }
        for (Node* node = nodeB; node; node = node->parentNode()) {
            depthB++;
        }

        Node* ancestorA = nodeA;
        Node* ancestorB = nodeB;
        if (depthA > depthB) {
            while (depthA-- > depthB) {
                ancestorA = ancestorA->parentNode();
            }
        } else {
            while (depthA < depthB--) {
                ancestorB = ancestorB->parentNode();
            }
        }

        while (ancestorA != ancestorB) {
            ancestorA = ancestorA->parentNode();
            ancestorB = ancestorB->parentNode();
        }

        STARFISH_ASSERT(ancestorA && ancestorB);
        return ancestorA;
    }

    static Node* root(Node* current)
    {
        Node* node = current;
        while (node->parentNode()) {
            node = node->parentNode();
        }
        return node;
    }
};

class NextNodeTraversalStrategy {
public:
    static Node* startNode(const Node& parent)
    {
        return parent.firstChild();
    }
    static Node* nextNode(const Node& current)
    {
        return current.nextSibling();
    }
};

class PreviousNodeTraversalStrategy {
public:
    static Node* startNode(const Node& parent)
    {
        return parent.lastChild();
    }
    static Node* nextNode(const Node& current)
    {
        return current.previousSibling();
    }
};

class RenderingSiblingIterator {
public:
    RenderingSiblingIterator(Node* node)
        : m_currentNode(node)
        , m_slotAssignedNodesIndex(SIZE_MAX)
    {
        updateNode(node);
    }

    RenderingSiblingIterator(const RenderingSiblingIterator& other)
        : m_currentNode(other.m_currentNode)
        , m_slotAssignedNodesIndex(other.m_slotAssignedNodesIndex)
        , m_slotAssignedNodes(other.m_slotAssignedNodes)
    {
    }

    const RenderingSiblingIterator& operator=(
        const RenderingSiblingIterator& other)
    {
        m_currentNode = other.m_currentNode;
        m_slotAssignedNodesIndex = other.m_slotAssignedNodesIndex;
        m_slotAssignedNodes = other.m_slotAssignedNodes;
        return *this;
    }

    Optional<Node*> next()
    {
        if (m_slotAssignedNodesIndex != SIZE_MAX) {
            if (m_slotAssignedNodesIndex < m_slotAssignedNodes.size()) {
                Node* c = m_slotAssignedNodes[m_slotAssignedNodesIndex];
                m_slotAssignedNodesIndex++;
                STARFISH_ASSERT(!c->isHTMLSlotElement());
                return c;
            }
            m_currentNode = m_currentNode->nextSibling();
        }
        Optional<Node*> c = m_currentNode;
        if (m_currentNode) {
            updateNode(m_currentNode->nextSibling());
        }
        STARFISH_ASSERT(!c || !c->isHTMLSlotElement());
        return c;
    }

private:
    void updateNode(Node* node)
    {
        while (node) {
            if (LIKELY(!node->isHTMLSlotElement())) {
                m_slotAssignedNodes.clear();
                m_slotAssignedNodesIndex = SIZE_MAX;
                m_currentNode = node;
                return;
            }
            m_slotAssignedNodesIndex = 0;
            m_currentNode = node;
            AssignedNodesOptions opt;
            opt.setFlatten(true);
            m_slotAssignedNodes = node->asHTMLSlotElement()->assignedNodes(
                Optional<AssignedNodesOptions>(opt));
            if (!m_slotAssignedNodes.size()) {
                RenderingSiblingIterator iter = node->firstRenderingChild();
                while (true) {
                    Optional<Node*> child = iter.next();
                    if (!child) {
                        break;
                    }
                    m_slotAssignedNodes.push_back(child.value());
                }
            }

            if (m_slotAssignedNodes.size()) {
                return;
            } else {
                node = node->nextSibling();
            }
        }

        m_slotAssignedNodes.clear();
        m_slotAssignedNodesIndex = SIZE_MAX;
        m_currentNode = nullptr;
    }

    Optional<Node*> m_currentNode;
    size_t m_slotAssignedNodesIndex;
    GCVector<Node*> m_slotAssignedNodes;
};
} // namespace Starfish

#endif
