/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishScriptBindingInstance__
#define __StarfishScriptBindingInstance__

namespace Escargot {
class AtomicStringRef;
class ContextRef;
class ValueRef;
class StringRef;
class ObjectRef;
class FunctionObjectRef;
class PromiseObjectRef;
class ExecutionStateRef;
class ScriptRef;
class PlatformRef;
} // namespace Escargot

#include "binding/generated/Interfaces.h"

#ifdef TIZEN_DEVICE_API
namespace DeviceAPI {
class ExtensionManagerInstance;
}
#endif

namespace Starfish {

class Starfish;
class Window;
class Document;
class ScriptEngineInstance;
class ScriptBindingInstance;
class String;
class ErrorEventInit;

#define FOR_EACH_DECLARE_FN(exportName)               \
    Escargot::FunctionObjectRef* binding##exportName( \
        ScriptBindingInstance* scriptBindingInstance);

STARFISH_ENUM_BINDING_NAMES(FOR_EACH_DECLARE_FN);
#undef FOR_EACH_DECLARE_FN

class ScriptBindingInstance;

using GlobalBindingNameAccessorGetter =
    std::function<Escargot::ValueRef*(ScriptBindingInstance*)>;
using GlobalBindingNameAccessorSetter =
    std::function<void(ScriptBindingInstance*, Escargot::ValueRef*)>;

class ScriptBindingInstance : public gc {
    friend class ScriptEngineInstance;

public:
    ScriptBindingInstance(ScriptEngineInstance* engineInstance);
    virtual ~ScriptBindingInstance()
    {
    }

    void initBinding();

    void defineGlobalBindingNameAccessor(
        Escargot::ExecutionStateRef* state, Escargot::ObjectRef* object,
        Escargot::StringRef* name, GlobalBindingNameAccessorGetter getter,
        GlobalBindingNameAccessorSetter setter);

    virtual void destroy();

    virtual void dispatchErrorEventToGlobalScope(ErrorEventInit& errorInfo) = 0;
#if defined(STARFISH_ENABLE_DEBUGGER)
    virtual bool isDebuggerEnabled()
    {
        return false;
    }
    virtual void startDebugger(unsigned port, int acceptTimeout)
    {
    }
    virtual void pumpDebuggerEvents()
    {
    }
#endif

    // TODO: Remove ownerDocument and ownerWindow
    virtual Window* ownerWindow() = 0;
    virtual Document* ownerDocument() = 0;
    virtual bool hasWindow()
    {
        return false;
    }
    virtual bool isScriptingEnabled() = 0;
    // Fundamental scripting availability, ignoring a CDP
    // Emulation.setScriptExecutionDisabled override. Used by the inspector so
    // Runtime.evaluate keeps working while page scripts are blocked.
    virtual bool isScriptingEnabledIgnoringCDP()
    {
        return isScriptingEnabled();
    }

#define FOR_EACH_BINDING_DECLARATION(exportName)                         \
    Escargot::FunctionObjectRef* fn##exportName()                        \
    {                                                                    \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                     \
            m_fn##exportName = binding##exportName(this);                \
            m_value##exportName =                                        \
                reinterpret_cast<Escargot::ValueRef*>(m_fn##exportName); \
        }                                                                \
        return m_fn##exportName;                                         \
    }                                                                    \
                                                                         \
    Escargot::ValueRef* value##exportName()                              \
    {                                                                    \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                     \
            m_fn##exportName = binding##exportName(this);                \
            m_value##exportName =                                        \
                reinterpret_cast<Escargot::ValueRef*>(m_fn##exportName); \
        }                                                                \
        return m_value##exportName;                                      \
    }                                                                    \
                                                                         \
    void setValue##exportName(Escargot::ValueRef* value)                 \
    {                                                                    \
        m_value##exportName = value;                                     \
    }                                                                    \
                                                                         \
    Escargot::FunctionObjectRef* m_fn##exportName = nullptr;             \
    Escargot::ValueRef* m_value##exportName = nullptr;

    STARFISH_ENUM_BINDING_NAMES(FOR_EACH_BINDING_DECLARATION)
#undef FOR_EACH_BINDING_DECLARATION

    Escargot::ContextRef* scriptContext()
    {
        return m_scriptContext;
    }

    ScriptEngineInstance* engineInstance()
    {
        return m_engineInstance;
    }

#define STARFISH_COMMONLY_USED_SCRIPT_STRINGS(F) \
    F(prototype, Prototype)                      \
    F(constructor, Constructor)                  \
    F(__proto__, __proto__)                      \
    F(length, Length)                            \
    F(done, Done)                                \
    F(value, Value)                              \
    F(next, Next)

#define FOR_EACH_STARFISH_COMMONLY_USED_SCRIPT_STRINGS(_, value) \
    Escargot::StringRef* string##value();
    STARFISH_COMMONLY_USED_SCRIPT_STRINGS(
        FOR_EACH_STARFISH_COMMONLY_USED_SCRIPT_STRINGS)
#undef FOR_EACH_STARFISH_COMMONLY_USED_SCRIPT_STRINGS

#ifdef TIZEN_DEVICE_API
    ::DeviceAPI::ExtensionManagerInstance* deviceAPI()
    {
        return m_deviceAPI;
    }
#endif

    GCVector<std::tuple<Escargot::StringRef*, Escargot::ScriptRef*,
                        Escargot::PromiseObjectRef*, Escargot::PlatformRef*>>&
    dynamicImportedModuleData()
    {
        return m_dynamicImportedModuleData;
    }

protected:
    Escargot::ContextRef* m_scriptContext;
    ScriptEngineInstance* m_engineInstance;

#define FOR_EACH_STARFISH_COMMONLY_USED_SCRIPT_STRINGS(_, value) \
    Escargot::AtomicStringRef* m_string##value;
    STARFISH_COMMONLY_USED_SCRIPT_STRINGS(
        FOR_EACH_STARFISH_COMMONLY_USED_SCRIPT_STRINGS)
#undef FOR_EACH_STARFISH_COMMONLY_USED_SCRIPT_STRINGS

#ifdef TIZEN_DEVICE_API
    ::DeviceAPI::ExtensionManagerInstance* m_deviceAPI = nullptr;
#endif

    GCVector<std::tuple<Escargot::StringRef*, Escargot::ScriptRef*,
                        Escargot::PromiseObjectRef*, Escargot::PlatformRef*>>
        m_dynamicImportedModuleData;

    virtual void initJavaScriptBinding(Escargot::ContextRef* context,
                                       Escargot::ExecutionStateRef* state);
};
} // namespace Starfish

#endif
