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

#include "core/dom/MutationObservationScope.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/MutationObserver.h"
#include "core/dom/MutationRecord.h"
#include "core/dom/NodeList.h"

namespace Starfish {

std::unordered_set<Node*> MutationObservationScope::m_onScopeSet;

MutationObservationScope::MutationObservationScope()
{
    m_optionTypes = MutationObserverOptionType::kNone;
}

MutationObservationScope::~MutationObservationScope()
{
    endMutationScope();
}

void MutationObservationScope::startAttributeMutationScope(
    Node* target, const Optional<QualifiedName>& name,
    Nullable<String*> oldValue)
{
    if (m_onScopeSet.find(target) != m_onScopeSet.end()) {
        return;
    }

    m_isStarted = true;
    m_target = target;
    m_name = name;
    m_optionTypes = MutationObserverOptionType::kAttributes |
                    MutationObserverOptionType::kAttributeOldValue;
    m_type =
        AtomicString::createAtomicString(m_target->starfish(), "attributes");
    m_oldValue = oldValue;
    m_onScopeSet.insert(target);
}

void MutationObservationScope::startCharacterDataMutationScope(
    Node* target, Nullable<String*> oldValue)
{
    if (m_onScopeSet.find(target) != m_onScopeSet.end()) {
        return;
    }

    m_isStarted = true;
    m_target = target;
    m_optionTypes = MutationObserverOptionType::kCharacterData |
                    MutationObserverOptionType::kCharacterDataOldValue;
    m_type =
        AtomicString::createAtomicString(m_target->starfish(), "characterData");
    m_oldValue = oldValue;
    m_onScopeSet.insert(target);
}

void MutationObservationScope::endMutationScope()
{
    if (m_isStarted) {
        enqueueMutationRecordIfNeeds();
    }

    m_onScopeSet.erase(m_target);
    m_isStarted = false;
}

void MutationObservationScope::enqueueMutationRecordIfNeeds()
{
    MutationObserverOptionType observerTypes = mutationTypes(m_optionTypes);
    if (!m_target->document()->hasMutationObserversOfType(observerTypes)) {
        return;
    }

    GCVector<MutationObserverRegistration*> interestedObserversRegistry =
        m_target->interestedObservers(observerTypes, m_name);

    String* attrName = nullptr;
    String* attrNamesapce = nullptr;
    if (m_name.hasValue()) {
        QualifiedName qname = m_name.getValue();
        attrName = qname.localName();
        if (qname.hasNamespaceURI()) {
            attrNamesapce = qname.namespaceURI().getValue().string();
        }
    }
    for (auto* registration : interestedObserversRegistry) {
        String* attributeOldValue = nullptr;
        if (!!(deliveryOptions(registration->options()) &
               deliveryOptions(m_optionTypes)) &&
            m_oldValue) {
            attributeOldValue = m_oldValue.getValue();
        }
        MutationRecord* record = new MutationRecord(
            m_target->executionContext(), m_type.string(), m_target, attrName,
            attrNamesapce, attributeOldValue);
        registration->observer()->enqueueMutationRecord(record);
    }
}

std::unordered_map<Node*, RefPtr<MutatedNodes>>*
    ChildListMutationObservationScope::s_onScopeMap = nullptr;

ChildListMutationObservationScope::ChildListMutationObservationScope()
{
    if (s_onScopeMap == nullptr) {
        s_onScopeMap = new std::unordered_map<Node*, RefPtr<MutatedNodes>>();
#ifndef NDEBUG
        m_isRootScope = true;
#endif
    }
}

ChildListMutationObservationScope::~ChildListMutationObservationScope()
{
    if (s_onScopeMap->at(m_target)->refCount() == 2) {
        enqueueChildListMutationRecordIfNeeds();
        MutatedNodes* m = m_mutatedChildren.get();
        s_onScopeMap->erase(m_target);
        m_mutatedChildren.release();
    }

    if (s_onScopeMap->empty()) {
        delete s_onScopeMap;
        s_onScopeMap = nullptr;
    }

#ifndef NDEBUG
    if (m_isRootScope) {
        STARFISH_ASSERT(s_onScopeMap == nullptr);
    }
#endif

    m_isStarted = false;
}

void ChildListMutationObservationScope::startChildListMutationScope(
    Node* target)
{
    m_target = target;
    m_isStarted = true;

    if (s_onScopeMap->find(m_target) == s_onScopeMap->end()) {
        s_onScopeMap->insert(
            std::make_pair(m_target, adoptRef(new (NoGC) MutatedNodes())));
    }
    m_mutatedChildren = s_onScopeMap->at(m_target);
}

void ChildListMutationObservationScope::childAdded(Node* child)
{
    updateSiblingIfNeeds(child, false);
    m_mutatedChildren->addedChilds.push_back(child);
}

void ChildListMutationObservationScope::childRemoved(Node* child,
                                                     bool forceUpdateSibling)
{
    updateSiblingIfNeeds(child, forceUpdateSibling);
    m_mutatedChildren->removedChilds.push_back(child);
}

bool ChildListMutationObservationScope::isEmptyChildList()
{
    return m_mutatedChildren->addedChilds.empty() &&
           m_mutatedChildren->removedChilds.empty();
}

void ChildListMutationObservationScope::enqueueChildListMutationRecordIfNeeds()
{
    if (s_onScopeMap->at(m_target)->refCount() != 2 || !m_isStarted) {
        return;
    }

    if (!m_target->document()->hasMutationObserversOfType(
            MutationObserverOptionType::kChildList)) {
        return;
    }

    if (isEmptyChildList()) {
        return;
    }

    GCVector<MutationObserverRegistration*> interestedObserversRegistry =
        m_target->interestedObservers(MutationObserverOptionType::kChildList,
                                      nullptr);

    for (auto* registration : interestedObserversRegistry) {
        MutationRecord* record = new MutationRecord(
            m_target->executionContext(),
            AtomicString::createAtomicString(m_target->starfish(), "childList"),
            m_target, m_mutatedChildren->addedChilds,
            m_mutatedChildren->removedChilds,
            m_mutatedChildren->previousSibling, m_mutatedChildren->nextSibling);
        registration->observer()->enqueueMutationRecord(record);
    }
    GCVector<Node*>().swap(m_mutatedChildren->addedChilds);
    GCVector<Node*>().swap(m_mutatedChildren->removedChilds);
}

void ChildListMutationObservationScope::updateSiblingIfNeeds(
    Node* child, bool forceUpdateSibling)
{
    if (isEmptyChildList() || forceUpdateSibling) {
        m_mutatedChildren->previousSibling = child->previousSibling();
        m_mutatedChildren->nextSibling = child->nextSibling();
    }
}
} // namespace Starfish
