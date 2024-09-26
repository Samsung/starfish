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

void MutationObservationScope::startChildListMutationScope(Node* target)
{
    if (m_onScopeSet.find(target) != m_onScopeSet.end()) {
        return;
    }

    m_isStarted = true;
    m_target = target;
    m_optionTypes = MutationObserverOptionType::kChildList;
    m_type =
        AtomicString::createAtomicString(m_target->starfish(), "childList");
    m_onScopeSet.insert(target);
}

void MutationObservationScope::endMutationScope()
{
    if (!!(m_optionTypes & MutationObserverOptionType::kAttributes) ||
        !!(m_optionTypes & MutationObserverOptionType::kCharacterData)) {
        enqueueMutationRecordIfNeeds();
    } else if (!!(m_optionTypes & MutationObserverOptionType::kChildList)) {
        enqueueChildListMutationRecordIfNeeds();
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

void MutationObservationScope::enqueueChildListMutationRecordIfNeeds()
{
    if (!m_isStarted) {
        return;
    }

    MutationObserverOptionType observerTypes = mutationTypes(m_optionTypes);
    if (!m_target->document()->hasMutationObserversOfType(observerTypes)) {
        return;
    }

    if (isEmptyChildList()) {
        return;
    }

    GCVector<MutationObserverRegistration*> interestedObserversRegistry =
        m_target->interestedObservers(observerTypes, m_name);
    for (auto* registration : interestedObserversRegistry) {
        MutationRecord* record = new MutationRecord(
            m_target->executionContext(), m_type.string(), m_target,
            m_addedChilds, m_removedChilds, m_previousSibling, m_nextSibling);
        registration->observer()->enqueueMutationRecord(record);
    }

    GCVector<Node*>().swap(m_addedChilds);
    GCVector<Node*>().swap(m_removedChilds);
}

void MutationObservationScope::childAdded(Node* child)
{
    if (!m_isStarted) {
        return;
    }
    updateSiblingIfNeeds(child, false);
    m_addedChilds.push_back(child);
}

void MutationObservationScope::childRemoved(Node* child,
                                            bool forceUpdateSibling)
{
    if (!m_isStarted) {
        return;
    }
    updateSiblingIfNeeds(child, forceUpdateSibling);
    m_removedChilds.push_back(child);
}

bool MutationObservationScope::isEmptyChildList()
{
    return m_addedChilds.empty() && m_removedChilds.empty();
}

void MutationObservationScope::updateSiblingIfNeeds(Node* child,
                                                    bool forceUpdateSibling)
{
    if (isEmptyChildList() || forceUpdateSibling) {
        m_previousSibling = child->previousSibling();
        m_nextSibling = child->nextSibling();
    }
}

} // namespace Starfish
