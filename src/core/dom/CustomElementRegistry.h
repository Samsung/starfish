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

#ifndef __StarfishCustomElementRegistry__
#define __StarfishCustomElementRegistry__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

struct ElementDefinitionOptions {
    ElementDefinitionOptions()
        : m_extends(nullptr)
    {
    }

    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, extends, Extends);

    // Define memebers
    DEFINE_MEMBER_WITH_HASFLAG(String*, extends, Extends);
};

class CustomElementConstructor : public gc {
public:
    static CustomElementConstructor* toCustomElementConstructor(
        ScriptValue constructor);
    CustomElementConstructor(ScriptValue constructor);

    ScriptValue scriptValue()
    {
        return m_customElementConstructor;
    }

private:
    ScriptValue m_customElementConstructor = nullptr;
};

class CustomElementRegistry : public ScriptWrappable {
public:
    CustomElementRegistry(ExecutionContext* executionContext);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isCustomElementRegistry() const override;

    void define(String* name, CustomElementConstructor* constructor,
                ElementDefinitionOptions options = {});

    CustomElementConstructor* get(String* name);

    Nullable<String*> getName(CustomElementConstructor* constructor);

private:
    ScriptBindingInstance* m_scriptBindingInstance;
};
} // namespace Starfish

#endif
