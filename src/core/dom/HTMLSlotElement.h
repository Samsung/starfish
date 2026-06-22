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

    virtual void didNodeRemovedFromDocumentTree() override
    {
        m_assignedNodes.clear();
    }

    String* slotName();

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
    static inline void fillGCDescriptor(GC_word* desc)
    {
        HTMLElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLSlotElement, m_assignedNodes));
    }

    GCVector<Node*> m_assignedNodes;
};
} // namespace Starfish

#endif
