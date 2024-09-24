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

#include "core/dom/MutationRecord.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/NodeList.h"

namespace Starfish {
MutationRecord::MutationRecord(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
}

MutationRecord::MutationRecord(ExecutionContext* executionContext, String* type,
                               Node* target, String* attributeName,
                               String* attributeNamespace,
                               Nullable<String*> oldValue)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_type(type)
    , m_target(target)
    , m_attributeName(attributeName)
    , m_attributeNamespace(attributeNamespace)
    , m_oldValue(oldValue)
{
}

MutationRecord::MutationRecord(ExecutionContext* executionContext, String* type,
                               Node* target, GCVector<Node*>& addedNodes,
                               GCVector<Node*>& removedNodes,
                               Node* previousSibling, Node* nextSibling)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_type(type)
    , m_target(target)
    , m_addedNodes(new NodeList(target, true))
    , m_removedNodes(new NodeList(target, true))
    , m_previousSibling(previousSibling)
    , m_nextSibling(nextSibling)
{
    m_addedNodes->getNodeListImpl().setItems(addedNodes);
    m_removedNodes->getNodeListImpl().setItems(removedNodes);
}

ScriptBindingInstance* MutationRecord::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

NodeList* MutationRecord::addedNodes()
{
    if (!m_addedNodes) {
        m_addedNodes = new NodeList(m_target, true);
        GCVector<Node*> emptyVector;

        // Fill the cache with empty vectors.
        m_addedNodes->getNodeListImpl().setItems(emptyVector);
    }
    return m_addedNodes;
}

NodeList* MutationRecord::removedNodes()
{
    if (!m_removedNodes) {
        m_removedNodes = new NodeList(m_target, true);
        GCVector<Node*> emptyVector;

        // Fill the cache with empty vectors.
        m_removedNodes->getNodeListImpl().setItems(emptyVector);
    }
    return m_removedNodes;
}

} // namespace Starfish
