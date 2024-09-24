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

#ifndef __StarfishMutationObservationScope__
#define __StarfishMutationObservationScope__

#include <cstdint>

namespace Starfish {

class Node;
enum class MutationObserverOptionType : uint8_t;

class MutationObservationScope {
public:
    STARFISH_MAKE_STACK_ALLOCATED();

    MutationObservationScope();

    ~MutationObservationScope();

    void startAttributeMutationScope(Node* target,
                                     const Optional<QualifiedName>& name,
                                     Nullable<String*> oldValue);
    void startCharacterDataMutationScope(Node* target,
                                         Nullable<String*> oldValue);

    void startChildListMutationScope(Node* target);
    void childAdded(Node* child);
    void childRemoved(Node* child, bool forceUpdateSibling = false);

    // Ensures that |end| is implicitly called when an object is destroyed.
    void endMutationScope();

private:
    void enqueueMutationRecordIfNeeds();
    void enqueueChildListMutationRecordIfNeeds();
    bool isEmptyChildList();
    void updateSiblingIfNeeds(Node* child, bool forceUpdateSibling);

    Node* m_target = nullptr;
    bool m_isStarted = false;
    Optional<QualifiedName> m_name;
    MutationObserverOptionType m_optionTypes;
    AtomicString m_type;
    Nullable<String*> m_oldValue;
    GCVector<Node*> m_addedChilds;
    GCVector<Node*> m_removedChilds;
    Node* m_previousSibling = nullptr;
    Node* m_nextSibling = nullptr;

    static std::unordered_set<Node*> m_onScopeSet;
};

} // namespace Starfish

#endif
