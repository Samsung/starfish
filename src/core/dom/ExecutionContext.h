/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarfishExecutionContext__
#define __StarfishExecutionContext__

namespace Starfish {

class GlobalScope;
class ScriptBindingInstance;
class WebBase;
class ScriptContext;

class ExecutionContext {
public:
    ExecutionContext(GlobalScope* globalScope, ScriptBindingInstance* instance)
        : m_globalScope(globalScope)
        , m_scriptBindingInstance(instance)
    {
    }

    GlobalScope* globalScope() const
    {
        return m_globalScope;
    }

    ScriptBindingInstance* ownerScriptBindingInstance() const
    {
        return m_scriptBindingInstance;
    }

    WebBase* webBase() const;
    ScriptContext* scriptContext() const;

private:
    GlobalScope* const m_globalScope;

protected:
    ScriptBindingInstance* const m_scriptBindingInstance;
};
}

#endif
