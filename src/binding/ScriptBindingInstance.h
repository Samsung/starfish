/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
