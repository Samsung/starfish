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

class MutationRecord final : public ScriptWrappable {
public:
    MutationRecord(ExecutionContext* executionContext)
        : ScriptWrappable(this)
        , m_type(String::emptyString)
        , m_target(nullptr)
    {
    }

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isMutationRecord() const override;

    String* type()
    {
        return m_type;
    }

    void setType(String* type)
    {
        m_type = type;
    }

    Node* target()
    {
        return m_target;
    }

    void setTarget(Node* target)
    {
        m_target = target;
    }

private:
    ExecutionContext* m_executionContext = nullptr;
    String* m_type;
    Node* m_target;
};

} // namespace Starfish

#endif
