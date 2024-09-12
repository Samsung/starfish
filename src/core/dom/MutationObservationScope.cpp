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
    end();
}

void MutationObservationScope::startAttributeMutationScope(
    Node* target, const Optional<QualifiedName>& name, String* oldValue)
{
    m_isStarted = true;
    m_target = target;
    m_name = name;
    m_optionTypes = MutationObserverOptionType::kAttributes;

    if (m_target->isElement() && m_name &&
        m_name.getValue().toString()->equals("style")) {
        m_target->asElement()->inlineStyle()->setMutationObservation(false);
    }
    enqueueMutationRecordIfNeeds(oldValue);
}

void MutationObservationScope::end()
{
    if (m_isStarted && m_target->isElement() &&
        m_optionTypes == MutationObserverOptionType::kAttributes && m_name &&
        m_name.getValue().toString()->equals("style")) {
        m_target->asElement()->inlineStyle()->setMutationObservation(true);
    }
    m_isStarted = false;
}

void MutationObservationScope::enqueueMutationRecordIfNeeds(String* oldValue)
{
    if (!m_target->document()->hasMutationObserversOfType(m_optionTypes)) {
        return;
    }

    GCVector<MutationObserverRegistration*> interestedObserversRegistry =
        m_target->interestedObservers(m_optionTypes, m_name);

    String* attrName = nullptr;
    String* attrNamesapce = nullptr;
    if (m_name.hasValue()) {
        QualifiedName qname = m_name.getValue();
        attrName = qname.toString();
        if (qname.hasNamespaceURI()) {
            attrNamesapce = qname.namespaceURI().getValue().string();
        }
    }
    MutationRecord* record = new MutationRecord(
        m_target->executionContext(), String::createASCIIString("attributes"),
        m_target, attrName, attrNamesapce, oldValue);
    for (auto* registration : interestedObserversRegistry) {
        registration->observer()->enqueueMutationRecord(record);
    }
}

} // namespace Starfish
