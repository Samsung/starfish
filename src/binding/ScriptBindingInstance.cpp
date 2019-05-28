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
                    "ScriptBindingInstance::~ScriptBindingInstance\n");
            },
            NULL, NULL, NULL);
    */
    m_scriptContext = ContextRef::create(engineInstance->engineInstance());
#ifdef TIZEN_DEVICE_API
    m_deviceAPI = nullptr;
#endif

#define FOR_EACH_SCRIPTVALUE_FN(exportName) m_value##exportName = nullptr;
    STARFISH_ENUM_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
#undef FOR_EACH_SCRIPTVALUE_FN

#define FOR_EACH_SCRIPT_FN(exportName) m_fn##exportName = nullptr;
    STARFISH_ENUM_BINDING_NAMES(FOR_EACH_SCRIPT_FN)
#undef FOR_EACH_SCRIPT_FN
}

void ScriptBindingInstance::initBinding()
{
    ContextRef* context = scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);
    initJavaScriptBinding(context, state);
    state->destroy();

#ifdef TIZEN_DEVICE_API
    m_deviceAPI = DeviceAPI::initialize(m_scriptContext);
#endif
}

void ScriptBindingInstance::destroy()
{
#ifdef TIZEN_DEVICE_API
    DeviceAPI::close(m_scriptContext);
#endif
    m_scriptContext->clearRelatedQueuedPromiseJobs();
    m_scriptContext->setVirtualIdentifierCallback(nullptr);
}

static String* toBrowserStringForConsole(ExecutionStateRef* state,
                                         ValueRef* value)
{
    if (value->isObject() && value->asObject()->isErrorObject()) {
        ObjectRef* o = value->asObject();
        ValueRef* stack = o->getOwnProperty(
            state, ValueRef::create(StringRef::fromASCII("stack")));
        if (stack->isString()) {
            return toBrowserString(state, stack);
        }
    }
    return toBrowserString(state, value);
}

static ValueRef* _logConsoleFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchWebBase(state->context())
        ->console()
        ->log(toBrowserStringForConsole(state, val));

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->log(toBrowserStringForConsole(state, val));
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

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->info(toBrowserStringForConsole(state, val));
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

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->error(toBrowserStringForConsole(state, val));
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

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->warn(toBrowserStringForConsole(state, val));
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

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchWebBase(state->context())
            ->console()
            ->debug(toBrowserStringForConsole(state, val));
    }
    return ValueRef::createUndefined();
}

void ScriptBindingInstance::initJavaScriptBinding(ContextRef* context, ExecutionStateRef* state)
{
    STARFISH_ASSERT(context != nullptr && state != nullptr);
    // binding names first
    GlobalObjectRef* globalObject = context->globalObject();
#define DECLARE_NAME_FOR_BINDING(exportName)                                   \
    ObjectRef::NativeDataAccessorPropertyData* newData##exportName =           \
        new ObjectRef::NativeDataAccessorPropertyData(                         \
            true, false, true,                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data) -> ValueRef* { \
                ScriptBindingInstance* instance;                               \
                if (self->isGlobalObject() && self->extraData()) {             \
                    instance = ((STARFISH_GLOBAL_BINDING_CLASS*)self->extraData()) \
                        ->scriptBindingInstance();                             \
                } else {                                                       \
                    instance = fetchScriptBindingInstance(state->context());   \
                }                                                              \
                return instance->value##exportName();                          \
            },                                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data,                \
               ValueRef* setterInputData) -> bool {                            \
                ScriptBindingInstance* instance;                               \
                if (self->isGlobalObject() && self->extraData()) {             \
                    instance = ((STARFISH_GLOBAL_BINDING_CLASS*)self->extraData()) \
                        ->scriptBindingInstance();                             \
                } else {                                                       \
                    instance = fetchScriptBindingInstance(state->context());   \
                }                                                              \
                instance->value##exportName();                                 \
                instance->m_value##exportName = setterInputData;               \
                return true;                                                   \
            });                                                                \
    globalObject->defineNativeDataAccessorProperty(                            \
        state, ValueRef::create(StringRef::fromASCII(#exportName)),            \
        newData##exportName);
    STARFISH_ENUM_GLOBAL_BINDING_NAMES(DECLARE_NAME_FOR_BINDING)
#undef DECLARE_NAME_FOR_BINDING

#define DECLARE_NAME_FOR_UNIMPL_BINDING(exportName)                            \
    ObjectRef::NativeDataAccessorPropertyData* newData##exportName =           \
        new ObjectRef::NativeDataAccessorPropertyData(                         \
            true, false, true,                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data) -> ValueRef* { \
                STARFISH_BINDING_ASSERT_UNIMPLEMENTED(                         \
                    "Unimplemented module \"%s\"\n", #exportName);             \
                return ValueRef::createUndefined();                            \
            },                                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data,                \
               ValueRef* setterInputData) -> bool {                            \
                STARFISH_BINDING_ASSERT_UNIMPLEMENTED(                         \
                    "Unimplemented module \"%s\"\n", #exportName);             \
                return false;                                                  \
            });                                                                \
    globalObject->defineNativeDataAccessorProperty(                            \
        state, ValueRef::create(StringRef::fromASCII(#exportName)),            \
        newData##exportName);
    STARFISH_ENUM_BINDING_UNIMPL_NAMES(DECLARE_NAME_FOR_UNIMPL_BINDING)
#undef DECLARE_NAME_FOR_UNIMPL_BINDING

    fnEventTarget();

    ObjectRef* console = ObjectRef::create(state);

#define DECLARE_CONSOLE_APIS(name)                                           \
    console->defineDataProperty(                                             \
        state, ValueRef::create(StringRef::fromASCII(#name)),                \
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(           \
            state, FunctionObjectRef::NativeFunctionInfo(                    \
                       AtomicStringRef::create(context, #name),              \
                       _##name##ConsoleFunction, 1, nullptr, true, false))), \
        true, true, true);
    CONSOLE_APIS(DECLARE_CONSOLE_APIS)
#undef DECLARE_CONSOLE_APIS

    globalObject->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("console")),
        ValueRef::create(console), true, true, true);
}
}
