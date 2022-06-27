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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/extra/Console.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"

#if !defined(STARFISH_WEBWORKER_HOST)
#include "core/page/Window.h"
#else
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#endif

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "core/extra/Avplay.h"
#endif

#include <EscargotPublic.h>

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"
#endif

namespace Starfish {

using namespace Escargot;

ScriptBindingInstance::ScriptBindingInstance(
    ScriptEngineInstance* engineInstance)
{
    /*
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO(
                    "ScriptBindingInstance::~ScriptBindingInstance");
            },
            NULL, NULL, NULL);
    */
    m_scriptContext =
        ContextRef::create(engineInstance->engineInstance()).release();
}

class GlobalBindingNameAccessorPropertyData
    : public ObjectRef::NativeDataAccessorPropertyData {
public:
    GlobalBindingNameAccessorPropertyData(
        ScriptBindingInstance* instance,
        GlobalBindingNameAccessorGetter valueGetter,
        GlobalBindingNameAccessorSetter valueSetter)
        : NativeDataAccessorPropertyData(true, false, true, accessorGetter,
                                         accessorSetter)
        , m_instance(instance)
        , m_valueGetter(valueGetter)
        , m_valueSetter(valueSetter)
    {
    }

    void* operator new(size_t size)
    {
        return GC_MALLOC(size);
    }

    static ValueRef* accessorGetter(
        ExecutionStateRef* state, ObjectRef* self, ValueRef* receiver,
        ObjectRef::NativeDataAccessorPropertyData* data)
    {
        auto globalBindingNameData =
            reinterpret_cast<GlobalBindingNameAccessorPropertyData*>(data);
        return globalBindingNameData->m_valueGetter(
            globalBindingNameData->m_instance);
    }

    static bool accessorSetter(ExecutionStateRef* state, ObjectRef* self,
                               ValueRef* receiver,
                               ObjectRef::NativeDataAccessorPropertyData* data,
                               ValueRef* setterInputData)
    {
        auto globalBindingNameData =
            reinterpret_cast<GlobalBindingNameAccessorPropertyData*>(data);
        globalBindingNameData->m_valueGetter(
                                    globalBindingNameData->m_instance);
        globalBindingNameData->m_valueSetter(globalBindingNameData->m_instance,
                                              setterInputData);
        return true;
    }

    ScriptBindingInstance* m_instance = nullptr;
    GlobalBindingNameAccessorGetter m_valueGetter = nullptr;
    GlobalBindingNameAccessorSetter m_valueSetter = nullptr;
};

void ScriptBindingInstance::initBinding()
{
    ContextRef* context = scriptContext();
    Evaluator::execute(
        context,
        [](ExecutionStateRef* state, ScriptBindingInstance* self) -> ValueRef* {
            self->initJavaScriptBinding(state->context(), state);
            return ValueRef::createUndefined();
        },
        this);

#ifdef TIZEN_DEVICE_API
    m_deviceAPI = DeviceAPI::initialize(m_scriptContext);
#endif
}

void ScriptBindingInstance::defineGlobalBindingNameAccessor(
    ExecutionStateRef* state, ObjectRef* object,
    StringRef* name, GlobalBindingNameAccessorGetter getter,
    GlobalBindingNameAccessorSetter setter)
{
    object->defineNativeDataAccessorProperty(
        state, name,
        new GlobalBindingNameAccessorPropertyData(this, getter, setter));
}

void ScriptBindingInstance::destroy()
{
#ifdef TIZEN_DEVICE_API
    DeviceAPI::close(m_scriptContext);
#endif
    m_scriptContext->clearRelatedQueuedJobs();
    m_scriptContext->setVirtualIdentifierCallback(nullptr);
}

static String* toBrowserStringForConsole(ExecutionStateRef* state,
                                         ValueRef* value)
{
    if (value->isObject() && value->asObject()->isErrorObject()) {
        ObjectRef* o = value->asObject();
        ValueRef* stack =
            o->getOwnProperty(state, StringRef::createFromASCII("stack"));
        if (stack->isString()) {
            return toBrowserString(state, stack);
        }
    }
    return toBrowserString(state, value);
}

#if defined(STARFISH_ENABLE_DEBUGGER)
static void printToDebuggerInConsole(ExecutionStateRef* state, ValueRef* val,
                                     const char* head)
{
    StringBuilder sb;
    sb.appendString(head, strlen(head));
    sb.appendString(toBrowserStringForConsole(state, val));
    state->context()->printDebugger(toJSString(sb.finalize()));
}
#endif

static ValueRef* _logConsoleFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchWebBase(state->context())
        ->console()
        ->log(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, val, "console.log : ");
#endif
    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->log(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
        printToDebuggerInConsole(state, val, "console.log : ");
#endif
    }

    return ValueRef::createUndefined();
}

static ValueRef* _infoConsoleFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchWebBase(state->context())
        ->console()
        ->info(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, val, "console.info : ");
#endif
    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->info(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
        printToDebuggerInConsole(state, val, "console.info : ");
#endif
    }
    return ValueRef::createUndefined();
}

static ValueRef* _errorConsoleFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchWebBase(state->context())
        ->console()
        ->error(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, val, "console.error : ");
#endif

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->error(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
        printToDebuggerInConsole(state, val, "console.error : ");
#endif
    }
    return ValueRef::createUndefined();
}

static ValueRef* _warnConsoleFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchWebBase(state->context())
        ->console()
        ->warn(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, val, "console.warn : ");
#endif

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->warn(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
        printToDebuggerInConsole(state, val, "console.warn : ");
#endif
    }
    return ValueRef::createUndefined();
}

static ValueRef* _debugConsoleFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchWebBase(state->context())
        ->console()
        ->debug(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, val, "console.debug : ");
#endif

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->debug(toBrowserStringForConsole(state, val));
#if defined(STARFISH_ENABLE_DEBUGGER)
        printToDebuggerInConsole(state, val, "console.debug : ");
#endif
    }
    return ValueRef::createUndefined();
}

void ScriptBindingInstance::initJavaScriptBinding(ContextRef* context,
                                                  ExecutionStateRef* state)
{
    STARFISH_ASSERT(context != nullptr);
    STARFISH_ASSERT(state != nullptr);
    // binding names first
    GlobalObjectRef* globalObject = context->globalObject();

#define DECLARE_NAME_FOR_UNIMPL_BINDING(exportName)                            \
    ObjectRef::NativeDataAccessorPropertyData* newData##exportName =           \
        new ObjectRef::NativeDataAccessorPropertyData(                         \
            true, false, true,                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data) -> ValueRef* { \
                STARFISH_UNIMPLEMENTED("Unimplemented module \"%s\"",          \
                                       #exportName);                           \
                return ValueRef::createUndefined();                            \
            },                                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data,                \
               ValueRef* setterInputData) -> bool {                            \
                STARFISH_UNIMPLEMENTED("Unimplemented module \"%s\"",          \
                                       #exportName);                           \
                return false;                                                  \
            });                                                                \
    globalObject->defineNativeDataAccessorProperty(                            \
        state, StringRef::createFromASCII(#exportName), newData##exportName);
    STARFISH_ENUM_BINDING_UNIMPL_NAMES(DECLARE_NAME_FOR_UNIMPL_BINDING)
#undef DECLARE_NAME_FOR_UNIMPL_BINDING

    ObjectRef* console = ObjectRef::create(state);

#define DECLARE_CONSOLE_APIS(name)                                 \
    console->defineDataProperty(                                   \
        state, StringRef::createFromASCII(#name),                  \
        FunctionObjectRef::createBuiltinFunction(                  \
            state, FunctionObjectRef::NativeFunctionInfo(          \
                       AtomicStringRef::create(context, #name),    \
                       _##name##ConsoleFunction, 1, true, false)), \
        true, true, true);
    CONSOLE_APIS(DECLARE_CONSOLE_APIS)
#undef DECLARE_CONSOLE_APIS

    globalObject->defineDataProperty(state,
                                     StringRef::createFromASCII("console"),
                                     console, true, true, true);
}
} // namespace Starfish
