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
#include "core/dom/Traverse.h"
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
    , m_styleResolver(new StyleResolver(m_document))
    , m_adoptedStyleSheetsProxy(nullptr)
{
    // add ua sheet
    m_styleResolver->addSheet(document->styleResolver().sheets()[0]);
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
                for (auto n : slot->m_assignedNodes) {
                    n->setIsSlotted(false);
                }
                slot->m_assignedNodes.clear();
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

    // Clear existing assignments
    for (auto iter : m_namedSlotElements) {
        for (auto n : iter.second->m_assignedNodes) {
            n->setIsSlotted(false);
        }
        iter.second->m_assignedNodes.clear();
    }

    // Traverse host children and assign to appropriate slots
    Node* node = host()->firstChild();
    while (node) {
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

    Traverse::traverse(oldChild,
                       [](Node* nd) { nd->setIsInShadowRoot(false); });

    updateSlotElements();

    if (isInDocumentScope()) {
        document()->updateDOMVersion();
    }
}

} // namespace Starfish
