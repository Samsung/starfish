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

#include "StarfishConfig.h"
#include "core/dom/CustomElementRegistry.h"

namespace Starfish {

CustomElementConstructor* CustomElementConstructor::toCustomElementConstructor(
    ScriptValue constructor)
{
    if (!isCallableScriptValue(constructor)) {
        return nullptr;
    }

    return new CustomElementConstructor(constructor);
}

CustomElementConstructor::CustomElementConstructor(ScriptValue constructor)
    : m_customElementConstructor(constructor)
{
}

CustomElementRegistry::CustomElementRegistry(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(executionContext->scriptBindingInstance())
{
    STARFISH_UNIMPLEMENTED();
}

ScriptBindingInstance* CustomElementRegistry::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

void CustomElementRegistry::define(String* name,
                                   CustomElementConstructor* constructor,
                                   ElementDefinitionOptions options)
{
    STARFISH_UNIMPLEMENTED();
}

CustomElementConstructor* CustomElementRegistry::get(String* name)
{
    STARFISH_UNIMPLEMENTED();
    return new CustomElementConstructor(scriptUndefined());
}

Nullable<String*> CustomElementRegistry::getName(
    CustomElementConstructor* constructor)
{
    STARFISH_UNIMPLEMENTED();
    return nullptr;
}

} // namespace Starfish
