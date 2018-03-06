/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishScriptBindingInstance__
#define __StarFishScriptBindingInstance__

namespace Escargot {
class VMInstanceRef;
class ContextRef;
class StringRef;
class ValueRef;
class PointerValueRef;
class ObjectRef;
class GlobalObjectRef;
class FunctionObjectRef;
class ScriptRef;
class ScriptParserRef;
class ExecutionStateRef;
class ArrayBufferObjectRef;
class ArrayBufferViewRef;
class Uint8ClampedArrayObjectRef;
typedef ValueRef* (*ScriptNativeFunctionPointer)(ExecutionStateRef* state,
                                                 ValueRef* thisValue,
                                                 size_t argc, ValueRef** argv,
                                                 bool isNewExpression);
}

#include "binding/ScriptEngineInstance.h"
#include "binding/Interfaces.h"

#ifdef TIZEN_DEVICE_API
namespace DeviceAPI {
class ExtensionManagerInstance;
}
#endif

namespace StarFish {

class StarFish;
class Window;
class Document;
class ScriptEngineInstance;
class ScriptBindingInstance;
class String;

#define FOR_EACH_DECLARE_FN(exportName)               \
    Escargot::FunctionObjectRef* binding##exportName( \
        ScriptBindingInstance* scriptBindingInstance);

STARFISH_ENUM_BINDING_NAMES(FOR_EACH_DECLARE_FN);
#undef FOR_EACH_DECLARE_FN

class ScriptBindingInstance : public gc {
public:
    ScriptBindingInstance(ScriptEngineInstance* engineInstance,
                          Window* ownerWindow);
    void initBinding(Document* ownerDocument);
    void close();

    Document* ownerDocument()
    {
        return m_ownerDocument;
    }

    Window* ownerWindow()
    {
        return m_ownerWindow;
    }

#define FOR_EACH_GETTER_FN(exportName)                                   \
    Escargot::FunctionObjectRef* fn##exportName()                        \
    {                                                                    \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                     \
            m_fn##exportName = binding##exportName(this);                \
            m_value##exportName =                                        \
                reinterpret_cast<Escargot::ValueRef*>(m_fn##exportName); \
        }                                                                \
        return m_fn##exportName;                                         \
    }

    STARFISH_ENUM_BINDING_NAMES(FOR_EACH_GETTER_FN)
#undef FOR_EACH_GETTER_FN

#define FOR_EACH_GETTER_VALUE_FN(exportName)                             \
    Escargot::ValueRef* value##exportName()                              \
    {                                                                    \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                     \
            m_fn##exportName = binding##exportName(this);                \
            m_value##exportName =                                        \
                reinterpret_cast<Escargot::ValueRef*>(m_fn##exportName); \
        }                                                                \
        return m_value##exportName;                                      \
    }

    STARFISH_ENUM_BINDING_NAMES(FOR_EACH_GETTER_VALUE_FN)
#undef FOR_EACH_GETTER_VALUE_FN

#define FOR_EACH_SCRIPT_FN(exportName) \
    Escargot::FunctionObjectRef* m_fn##exportName;
    STARFISH_ENUM_BINDING_NAMES(FOR_EACH_SCRIPT_FN)
#undef FOR_EACH_SCRIPT_FN

public:
#define FOR_EACH_SCRIPTVALUE_FN(exportName) \
    Escargot::ValueRef* m_value##exportName;
    STARFISH_ENUM_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
#undef FOR_EACH_SCRIPTVALUE_FN

    Escargot::ContextRef* scriptContext()
    {
        return m_scriptContext;
    }

#ifdef TIZEN_DEVICE_API
    ::DeviceAPI::ExtensionManagerInstance* deviceAPI()
    {
        return m_deviceAPI;
    }
#endif

protected:
    Escargot::ContextRef* m_scriptContext;
    Window* m_ownerWindow;
    Document* m_ownerDocument;
#ifdef TIZEN_DEVICE_API
    ::DeviceAPI::ExtensionManagerInstance* m_deviceAPI;
#endif
};
}

#endif
