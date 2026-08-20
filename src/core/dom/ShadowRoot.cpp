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
#include "core/dom/ShadowRoot.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLSlotElement.h"
#include "core/dom/MutationObservationScope.h"
#include "core/dom/Traverse.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/xml/XMLSerializer.h"
#include "core/style/AdoptedStyleSheets.h"

namespace Starfish {

DEFINE_EVENT_LISTENER(ShadowRoot, slotchange);

ShadowRoot::ShadowRoot(Document* document, ShadowRootMode mode, Element* host)
    : DocumentFragment(document)
    , m_mode(mode)
    , m_delegatesFocus(false)
    , m_slotAssignmentEnum(SlotAssignmentMode::Named)
    , m_clonable(false)
    , m_serializable(false)
    , m_availableToElementInternals(false)
    , m_declarative(false)
    , m_host(host)
    , m_styleResolver(new StyleResolver(m_document, this))
    , m_adoptedStyleSheetsProxy(nullptr)
{
    // add ua sheet
    m_styleResolver->addSheet(document->styleResolver().sheets()[0]);
}

String* ShadowRoot::innerHTML()
{
    return XMLSerializer::serializeToXML(this, false);
}

void ShadowRoot::setInnerHTML(String* html)
{
    ChildListMutationObservationScope scope;
    scope.startChildListMutationScope(this);
    while (firstChild()) {
        removeChild(firstChild());
    }

    // The fragment parsing algorithm takes an *element* as its context, and the
    // spec picks the shadow host for a ShadowRoot -- markup like <td> must be
    // parsed as if it were written inside the host, not inside a fragment with
    // no element context at all.
    DocumentFragment* df = fragmentParsingAlgorithm(document(), html, host());
    appendChild(df);
}

ScriptProxyObject ShadowRoot::adoptedStyleSheetsObservableArray(
    Escargot::ExecutionStateRef* state)
{
    return AdoptedStyleSheets::observableArray(state, this);
}

void ShadowRoot::setAdoptedStyleSheetsFromObservableArray(
    Escargot::ExecutionStateRef* state, Escargot::ValueRef* value)
{
    AdoptedStyleSheets::setFromObservableArray(state, this, value);
}

void* ShadowRoot::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ShadowRoot));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(ShadowRoot)] = { 0 };
        ShadowRoot::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ShadowRoot));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* ShadowRoot::mode() const
{
    if (isOpened()) {
        return starfish()->staticStrings()->m_open.localName();
    }
    return starfish()->staticStrings()->m_close.localName();
}

void ShadowRoot::updateSlotElements(bool shouldConnectSlotWithSlottables)
{
    m_namedSlotElements.clear();
    if (slotAssignmentEnum() == SlotAssignmentMode::Named) {
        Traverse::traverse(this, [&](Node* node) {
            if (node->isHTMLSlotElement()) {
                auto slot = node->asHTMLSlotElement();
                auto slotName = slot->slotName();
                auto iter = m_namedSlotElements.find(slotName);
                if (iter == m_namedSlotElements.end()) {
                    m_namedSlotElements.insert(std::make_pair(slotName, slot));
                }
                // Assignments are cleared by connectSlotWithSlottables()
                // below, which also snapshots them first to detect
                // slotchange. Clearing here would lose that snapshot on the
                // name=/slot-set-change path.
            }
        });
    } else {
        STARFISH_UNIMPLEMENTED("SlotAssignmentMode::Manual");
    }

    if (shouldConnectSlotWithSlottables) {
        connectSlotWithSlottables();
    }
}

void ShadowRoot::assignSlot()
{
    updateSlotElements(false);
    connectSlotWithSlottables();
}

void ShadowRoot::connectSlotWithSlottables()
{
    // Assigns host's children to slots based on slot attribute or default slot.
    // Elements with slot="name" go to matching named slot; others go to default
    // slot. Text nodes always go to default slot (empty string key).

    // Gather every <slot> in this shadow tree (tree order). When several slots
    // share a name only the first in tree order is assigned slottables (the
    // winner, recorded in m_namedSlotElements); the rest are left empty. We
    // must still snapshot, clear, and check those losers for slotchange,
    // because inserting/removing a slot can shift which one wins and a slot
    // that loses its slottables fires slotchange too.
    //
    // These locals retain GC pointers (slots, and the snapshot below) across
    // reassignment, which allocates and may trigger GC. Use GC-tracked vectors
    // so their contents are scanned and stay alive — a std::vector's heap
    // buffer is not scanned by the collector, so it cannot keep its elements
    // reachable.
    GCVector<HTMLSlotElement*> slots;
    Traverse::traverse(this, [&](Node* node) {
        if (node->isHTMLSlotElement()) {
            slots.push_back(node->asHTMLSlotElement());
        }
    });

    // Snapshot each slot's assigned nodes before reassigning so we can detect
    // changes and "signal a slot change" (WHATWG DOM): slotchange fires only
    // for slots whose assigned-node list actually changes.
    GCVector<GCVector<Node*>> oldAssignedNodes;
    oldAssignedNodes.reserve(slots.size());
    for (auto* slot : slots) {
        oldAssignedNodes.push_back(slot->m_assignedNodes);
    }

    // Clear existing assignments
    for (auto* slot : slots) {
        slot->clearAssignedNodes();
    }

    // Traverse host children and assign to appropriate slots
    Node* node = host()->firstChild();
    while (node != nullptr) {
        if (node->isElement()) {
            auto slotName = node->asElement()->slot();
            // Named slot: element has slot attribute
            // Default slot: element has no slot attribute
            auto iter = m_namedSlotElements.find(
                slotName->length() ? slotName : String::emptyString);
            if (iter != m_namedSlotElements.end()) {
                iter->second->m_assignedNodes.push_back(node);
                node->setIsSlotted(true);
            }
        } else if (node->isText()) {
            // Text nodes always assigned to default slot
            auto iter = m_namedSlotElements.find(String::emptyString);
            if (iter != m_namedSlotElements.end()) {
                iter->second->m_assignedNodes.push_back(node);
                node->setIsSlotted(true);
            }
        }
        node = node->nextSibling();
    }

    auto contains = [](const GCVector<Node*>& nodes, Node* node) -> bool {
        for (size_t i = 0; i < nodes.size(); i++) {
            if (nodes[i] == node) {
                return true;
            }
        }
        return false;
    };

    // Signal a slot change for each slot whose assignment differs from before.
    for (size_t i = 0; i < slots.size(); i++) {
        const GCVector<Node*>& oldNodes = oldAssignedNodes[i];
        const GCVector<Node*>& newNodes = slots[i]->m_assignedNodes;
        bool changed = oldNodes.size() != newNodes.size();
        for (size_t j = 0; !changed && j < oldNodes.size(); j++) {
            if (oldNodes[j] != newNodes[j]) {
                changed = true;
            }
        }
        if (changed) {
            document()->signalSlotChange(slots[i]);
        }

        // A node newly assigned here (whether it had no slot before, or a
        // different one) may now be reached by this tree's `::slotted()`
        // rules, yet the mutation that caused the reassignment -- a change
        // elsewhere in the shadow tree, not on the node itself -- never marks
        // it dirty on its own.
        //
        // A node that loses its assignment here needs the same call for a
        // different reason: once unassigned, Node::renderingParentNode()
        // resolves straight to the host (assignedSlotInternal() no longer
        // finds a slot for it), so setNeedsStyleRecalc() propagates the
        // "child needs recalc" bit up through the host and its ancestors --
        // the only thing that makes the top-down style-recalc walk redescend
        // into this shadow tree. Skipping that call left a slot that just
        // lost its last assigned node (and so started rendering its fallback
        // content) unvisited: the fallback content kept a null ComputedStyle,
        // and FrameTreeBuilder crashed dereferencing it.
        for (size_t j = 0; j < oldNodes.size(); j++) {
            if (!contains(newNodes, oldNodes[j])) {
                oldNodes[j]->setNeedsStyleRecalc();
            }
        }
        for (size_t j = 0; j < newNodes.size(); j++) {
            if (!contains(oldNodes, newNodes[j])) {
                newNodes[j]->setNeedsStyleRecalc();
            }
        }
    }
}

Optional<HTMLSlotElement*> ShadowRoot::assignedSlot(String* name)
{
    auto iter = m_namedSlotElements.find(name);
    if (iter == m_namedSlotElements.end()) {
        return nullptr;
    }
    return iter->second;
}

void ShadowRoot::didNodeInserted(Node* parent, Node* newChild)
{
    DocumentFragment::didNodeInserted(parent, newChild);

    Traverse::traverse(newChild, [](Node* nd) { nd->setIsInShadowRoot(true); });

    updateSlotElements();

    if (isInDocumentScope()) {
        document()->updateDOMVersion();
    }
}

void ShadowRoot::didNodeRemoved(Node* parent, Node* oldChild)
{
    DocumentFragment::didNodeRemoved(parent, oldChild);

    Traverse::traverse(oldChild, [](Node* nd) {
        nd->setIsInShadowRoot(false);
        // A <slot> detached from this tree holds no slottables. The document
        // -tree cleanup that normally clears this (didNodeRemovedFromDocument
        // Tree) is gated on isInDocumentScope() and is skipped for detached
        // shadow trees, so clear here so assignedNodes() reflects removal.
        if (nd->isHTMLSlotElement()) {
            nd->asHTMLSlotElement()->clearAssignedNodes();
        }
    });

    updateSlotElements();

    if (isInDocumentScope()) {
        document()->updateDOMVersion();
    }
}

} // namespace Starfish
