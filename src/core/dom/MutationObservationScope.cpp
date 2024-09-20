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

namespace Starfish {

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
    m_isStarted = true;
    m_target = target;
    m_name = name;
    m_optionTypes = MutationObserverOptionType::kAttributes |
                    MutationObserverOptionType::kAttributeOldValue;
    m_type =
        AtomicString::createAtomicString(m_target->starfish(), "attributes");

    if (m_target->isElement() && m_name &&
        m_name.getValue().toString()->equals("style")) {
        m_target->asElement()->inlineStyle()->setMutationObservation(false);
    }
    enqueueMutationRecordIfNeeds(oldValue);
}

void MutationObservationScope::startCharacterDataMutationScope(
    Node* target, Nullable<String*> oldValue)
{
    m_isStarted = true;
    m_target = target;
    m_optionTypes = MutationObserverOptionType::kCharacterData |
                    MutationObserverOptionType::kCharacterDataOldValue;
    m_type =
        AtomicString::createAtomicString(m_target->starfish(), "characterData");
    enqueueMutationRecordIfNeeds(oldValue);
}

void MutationObservationScope::endMutationScope()
{
    if (m_isStarted && m_target->isElement() &&
        m_optionTypes == MutationObserverOptionType::kAttributes && m_name &&
        m_name.getValue().toString()->equals("style")) {
        m_target->asElement()->inlineStyle()->setMutationObservation(true);
    }
    m_isStarted = false;
}

void MutationObservationScope::enqueueMutationRecordIfNeeds(
    Nullable<String*> oldValue)
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
            oldValue) {
            attributeOldValue = oldValue.getValue();
        }
        MutationRecord* record = new MutationRecord(
            m_target->executionContext(), m_type.string(), m_target, attrName,
            attrNamesapce, attributeOldValue);
        registration->observer()->enqueueMutationRecord(record);
    }
}

} // namespace Starfish
