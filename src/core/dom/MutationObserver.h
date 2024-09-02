/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishMutationObserver__
#define __StarfishMutationObserver__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

struct MutationObserverInit {
    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, childList, ChildList);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, attributes, Attributes);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, characterData, CharacterData);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, subtree, Subtree);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, attributeOldValue,
                                      AttributeOldValue);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, characterDataOldValue,
                                      CharacterDataOldValue);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<String*>, attributeFilter,
                                      AttributeFilter);

    // Define getter/setters
    DEFINE_MEMBER_WITH_HASFLAG(bool, childList, ChildList);
    DEFINE_MEMBER_WITH_HASFLAG(bool, attributes, Attributes);
    DEFINE_MEMBER_WITH_HASFLAG(bool, characterData, CharacterData);
    DEFINE_MEMBER_WITH_HASFLAG(bool, subtree, Subtree);
    DEFINE_MEMBER_WITH_HASFLAG(bool, attributeOldValue, AttributeOldValue);
    DEFINE_MEMBER_WITH_HASFLAG(bool, characterDataOldValue,
                               CharacterDataOldValue);
    DEFINE_MEMBER_WITH_HASFLAG(GCVector<String*>, attributeFilter,
                               AttributeFilter);
};

class MutationCallback : public gc {
public:
    static MutationCallback* toMutationCallback(ScriptValue callback);

    MutationCallback(ScriptValue callback);

private:
    ScriptValue m_mutationCallback;
};

class MutationObserver final : public ScriptWrappable {
public:
    MutationObserver(ExecutionContext* executionContext,
                     MutationCallback* callBack);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isMutationObserver() const override;

    void observe(Node* node)
    {
        STARFISH_UNSUPPORTED_METHOD();
    }

    void observe(Node* node, MutationObserverInit options)
    {
        STARFISH_UNSUPPORTED_METHOD();
    }

    void disconnect()
    {
        STARFISH_UNSUPPORTED_METHOD();
    }

private:
    ExecutionContext* m_executionContext;
    MutationCallback* m_callback = nullptr;
};
} // namespace Starfish

#endif
