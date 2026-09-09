/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishHTMLSlotElement__
#define __StarfishHTMLSlotElement__

#include "core/dom/HTMLElement.h"

namespace Starfish {

class ShadowRoot;

struct AssignedNodesOptions {
    AssignedNodesOptions()
        : m_flatten(false)
    {
    }

    // Define getter/setters
    DEFINE_GETTER_SETTER(bool, flatten, Flatten);

    bool m_flatten;
};

class HTMLSlotElement : public HTMLElement {
public:
    friend class ShadowRoot;

    HTMLSlotElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLSlotElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didNodeRemoved(Node* parent, Node* oldChild) override;

    virtual void didNodeRemovedFromDocumentTree() override
    {
        // A formerly-assigned node that is not itself being removed from the
        // document (e.g. only this <slot> left the shadow tree, not its
        // host) gets no didNodeRemovedFromDocumentTree() call of its own, and
        // nothing else will ever revisit it: Element::firstRenderingChild()
        // only descends into a shadow-root host's shadow tree, so the
        // top-down style-recalc walk skips an unassigned light-DOM child for
        // good. Clear its now-stale style here instead of leaving it behind.
        for (auto* assignee : m_assignedNodes) {
            assignee->setIsSlotted(false);
            assignee->clearCachedStyleRecursively();
        }
        m_assignedNodes.clear();
    }

    String* slotName();

    // Flat-tree children of a slot are its assigned nodes, or its own
    // children (fallback content) when nothing is assigned (css-scoping-1
    // #flat-tree). RenderingSiblingIterator continues along the assigned list
    // from the node returned here.
    virtual Node* firstRenderingChild() override
    {
        if (m_assignedNodes.size()) {
            return m_assignedNodes[0];
        }
        return firstChild();
    }

    const GCVector<Node*>& immutableAssignedNodes() const
    {
        return m_assignedNodes;
    }

    // Drops all assigned slottables and marks them unslotted. Shared by the
    // shadow-tree slot-assignment paths that rebuild or detach assignments.
    void clearAssignedNodes();

    GCVector<Node*> assignedNodes(
        Optional<AssignedNodesOptions> options = nullptr);
    GCVector<Element*> assignedElements(
        Optional<AssignedNodesOptions> options = nullptr);

private:
    // Signal slotchange after this slot's fallback content (its own children)
    // is mutated while it has no assigned nodes. See .cpp for spec rationale.
    void signalSlotChangeForFallbackMutation();

    // WHATWG DOM "find flattened slottables": the flat-tree distribution used
    // by assignedNodes({flatten:true}). Recursively resolves slotted slots and
    // falls back to this slot's own slottable children when nothing is
    // assigned.
    void findFlattenedSlottables(GCVector<Node*>& result);

    static inline void fillGCDescriptor(GC_word* desc)
    {
        HTMLElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLSlotElement, m_assignedNodes));
    }

    GCVector<Node*> m_assignedNodes;
};
} // namespace Starfish

#endif
