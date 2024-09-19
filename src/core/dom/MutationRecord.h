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

#ifndef __StarfishMutationRecord__
#define __StarfishMutationRecord__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class Node;
class NodeList;

class MutationRecord final : public ScriptWrappable {
public:
    MutationRecord(ExecutionContext* executionContext);
    MutationRecord(ExecutionContext* executionContext, String* type,
                   Node* target, String* attributeName,
                   String* attributeNamespace, String* oldValue);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isMutationRecord() const override;

    String* type()
    {
        return m_type;
    }

    Node* target()
    {
        return m_target;
    }

    NodeList* addedNodes();
    NodeList* removedNodes();

    Node* previousSibling()
    {
        return m_previousSibling;
    }

    Node* nextSibling()
    {
        return m_nextSibling;
    }

    String* attributeName()
    {
        return m_attributeName;
    }

    String* attributeNamespace()
    {
        return m_attributeNamespace;
    }

    String* oldValue()
    {
        return m_oldValue;
    }

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

private:
    ExecutionContext* m_executionContext = nullptr;
    String* m_type = nullptr;
    Node* m_target = nullptr;
    NodeList* m_addedNodes = nullptr;
    NodeList* m_removedNodes = nullptr;
    Node* m_previousSibling = nullptr;
    Node* m_nextSibling = nullptr;
    String* m_attributeName = nullptr;
    String* m_attributeNamespace = nullptr;
    String* m_oldValue = nullptr;
};

} // namespace Starfish

#endif
