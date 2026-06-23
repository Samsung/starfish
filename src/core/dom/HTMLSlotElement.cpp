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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLSlotElement.h"
#include "core/dom/ShadowRoot.h"

namespace Starfish {

HTMLSlotElement::HTMLSlotElement(Document* document, const QualifiedName& qname)
    : HTMLElement(document, qname)
{
}

void* HTMLSlotElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLSlotElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLSlotElement)] = { 0 };
        HTMLSlotElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLSlotElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* HTMLSlotElement::slotName()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_name);
}

void HTMLSlotElement::clearAssignedNodes()
{
    for (auto n : m_assignedNodes) {
        n->setIsSlotted(false);
    }
    m_assignedNodes.clear();
}

void HTMLSlotElement::didAttributeChanged(QualifiedName name,
                                          Optional<String*> old, String* value,
                                          bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starfish()->staticStrings()->m_name) {
        Node* nd = parentNode();
        while (nd) {
            if (nd->isShadowRoot()) {
                nd->asShadowRoot()->updateSlotElements();
                break;
            }
            nd = nd->parentNode();
        }
    }
}

void HTMLSlotElement::signalSlotChangeForFallbackMutation()
{
    // WHATWG DOM "insert"/"remove": when a node is inserted into or removed
    // from a slot whose assigned nodes is empty, signal a slot change for the
    // slot (it is rendering its fallback content). Ancestor slots showing
    // fallback are reached by slotchange bubbling, so only the directly mutated
    // slot is signaled here.
    if (!isInShadowRoot() || immutableAssignedNodes().size()) {
        return;
    }
    document()->signalSlotChange(this);
}

void HTMLSlotElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    // The insertion dispatch walks this up the ancestor chain with parent fixed
    // to the insertion point; act only on a direct child insertion into this.
    if (parent == this) {
        signalSlotChangeForFallbackMutation();
    }
}

void HTMLSlotElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    HTMLElement::didNodeRemoved(parent, oldChild);
    if (parent == this) {
        signalSlotChangeForFallbackMutation();
    }
}

GCVector<Node*> HTMLSlotElement::assignedNodes(
    Optional<AssignedNodesOptions> options)
{
    if (options && options.value().flatten()) {
        GCVector<Node*> result = m_assignedNodes;
        for (size_t i = 0; i < result.size(); i++) {
            Node* nd = result[i];
            if (nd->isHTMLSlotElement()) {
                auto subNodes = nd->asHTMLSlotElement()->assignedNodes(options);
                result.erase(i);
                for (size_t j = 0; j < subNodes.size(); j++) {
                    result.insert(i + j, subNodes[j]);
                }
                i--;
            }
        }
        return result;
    } else {
        return m_assignedNodes;
    }
}

GCVector<Element*> HTMLSlotElement::assignedElements(
    Optional<AssignedNodesOptions> options)
{
    GCVector<Node*> tempResult = assignedNodes(options);
    GCVector<Element*> result;
    for (Node* nd : tempResult) {
        if (nd->isElement()) {
            result.push_back(nd->asElement());
        }
    }
    return result;
}

} // namespace Starfish
