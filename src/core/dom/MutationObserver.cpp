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

#include "core/dom/MutationObserver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"

namespace Starfish {

MutationCallback* MutationCallback::toMutationCallback(ScriptValue callback)
{
    if (!isCallableScriptValue(callback)) {
        return nullptr;
    }

    return new MutationCallback(callback);
}

MutationCallback::MutationCallback(ScriptValue callback)
    : m_mutationCallback(callback)
{
}

MutationObserverRegistration::MutationObserverRegistration(
    MutationObserver* observer, Node* target,
    MutationObserverOptionType options,
    const GCUnorderedSet<String*>& attributeFilter)
    : m_observer(observer)
    , m_target(target)
    , m_options(options)
    , m_attributeFilter(attributeFilter)
{
}

MutationObserverOptionType MutationObserverRegistration::mutationTypes()
{
    return m_options & MutationObserverOptionType::kAllMutationType;
}

void MutationObserverRegistration::update(
    MutationObserverOptionType options,
    const GCUnorderedSet<String*>& attributeFilter)
{
    m_options = options;
    m_attributeFilter = attributeFilter;
}

MutationObserver::MutationObserver(ExecutionContext* executionContext,
                                   MutationCallback* callBack)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_callback(callBack)
{
}

ScriptBindingInstance* MutationObserver::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

void MutationObserver::observe(Node* node)
{
    observe(node, {});
}

void MutationObserver::observe(Node* node, MutationObserverInit options)
{
    // https://dom.spec.whatwg.org/#dom-mutationobserver-observe
    MutationObserverOptionType optionType;

    if ((options.hasAttributeOldValue() || options.hasAttributeFilter()) &&
        !options.hasAttributes()) {
        optionType |= MutationObserverOptionType::kAttributes;
    }
    if (options.hasAttributes() && options.attributes()) {
        optionType |= MutationObserverOptionType::kAttributes;
    }

    if (options.hasCharacterDataOldValue() && !options.hasCharacterData()) {
        optionType |= MutationObserverOptionType::kCharacterData;
    }
    if (options.hasCharacterData() && options.characterData()) {
        optionType |= MutationObserverOptionType::kCharacterData;
    }
    if (options.hasChildList() && options.childList()) {
        optionType |= MutationObserverOptionType::kChildList;
    }
    if (options.hasSubtree() && options.subtree()) {
        optionType |= MutationObserverOptionType::kSubtree;
    }

    if (!(optionType & MutationObserverOptionType::kChildList) &&
        !(optionType & MutationObserverOptionType::kAttributes) &&
        !(optionType & MutationObserverOptionType::kCharacterData)) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "Invalid MutationObserverInit");
    }

    if (options.hasAttributeOldValue() && options.attributeOldValue()) {
        optionType |= MutationObserverOptionType::kAttributeOldValue;
    }
    if ((optionType & MutationObserverOptionType::kAttributeOldValue) &&
        !(optionType & MutationObserverOptionType::kAttributes)) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "Invalid MutationObserverInit");
    }

    if (options.hasAttributeFilter() &&
        !(optionType & MutationObserverOptionType::kAttributes)) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "Invalid MutationObserverInit");
    }
    GCUnorderedSet<String*> attributeFilter;
    if (options.hasAttributeFilter()) {
        for (auto& filter : options.attributeFilter()) {
            attributeFilter.insert(filter);
        }
        optionType |= MutationObserverOptionType::kAttributeFilter;
    }
    STARFISH_LOG_INFO("attributeFilter.size(): %ld", attributeFilter.size());
    if (options.hasCharacterDataOldValue() && options.characterDataOldValue()) {
        optionType |= MutationObserverOptionType::kCharacterDataOldValue;
    }
    if ((optionType & MutationObserverOptionType::kCharacterDataOldValue) &&
        !(optionType & MutationObserverOptionType::kCharacterData)) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "Invalid MutationObserverInit");
    }

    std::pair<bool, MutationObserverRegistration*> resultPair =
        node->registerOrUpdateMutationObserver(this, optionType,
                                               attributeFilter);
    if (resultPair.first) {
        STARFISH_ASSERT(!m_registrations.contains(resultPair.second));
        m_registrations.insert(resultPair.second);
    }
    node->document()->addMutationObserverTypes(
        resultPair.second->mutationTypes());
}

void MutationObserver::disconnect()
{
    for (auto* registration : m_registrations) {
        registration->target()->unregisterMutationObserver(registration);
    }
    GCUnorderedSet<MutationObserverRegistration*>().swap(m_registrations);
    GCVector<MutationRecord*>().swap(m_queuedRecords);
}

GCVector<MutationRecord*> MutationObserver::takeRecords()
{
    GCVector<MutationRecord*> records;
    records = m_queuedRecords;
    m_queuedRecords.clear();
    return records;
}

} // namespace Starfish
