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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"
#ifdef STARFISH_ENABLE_EXP
#include "core/dom/DOMImplementation.h"
#endif
#include "core/extra/Console.h"
#include "core/page/History.h"
#include "core/page/Location.h"
#include "core/page/Navigator.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Window.h"

#include <EscargotPublic.h>

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"
#endif

namespace StarFish {

using namespace Escargot;

ScriptBindingInstance::ScriptBindingInstance(
    ScriptEngineInstance* engineInstance, Window* ownerWindow)
{
    m_scriptContext = ContextRef::create(engineInstance->engineInstance());
    m_ownerWindow = ownerWindow;
    m_ownerDocument = nullptr;

#define FOR_EACH_SCRIPTVALUE_FN(exportName) m_value##exportName = nullptr;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
    STARFISH_ENUM_LAZY_BINDING_NICKNAMES(FOR_EACH_SCRIPTVALUE_FN)
#undef FOR_EACH_SCRIPTVALUE_FN

#define FOR_EACH_SCRIPT_FN(exportName) m_fn##exportName = nullptr;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPT_FN)
    STARFISH_ENUM_LAZY_BINDING_NICKNAMES(FOR_EACH_SCRIPT_FN)
#undef FOR_EACH_SCRIPT_FN
}
void ScriptBindingInstance::close()
{
#ifdef TIZEN_DEVICE_API
    // TODO (escargot2)
    DeviceAPI::close(fetchData(this)->m_instance);
#endif
}

#if defined(STARFISH_ENABLE_TEST)
static ValueRef* wptTestEndFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    const char* hide = getenv("HIDE_WINDOW");
    if ((hide && strlen(hide))) {
        ::exit(0);
    }
    return ValueRef::createUndefined();
}
#endif

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

static ValueRef* logFunction(ExecutionStateRef* state, ValueRef* thisValue,
                             size_t argc, ValueRef** argv, bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchStarFish(state->context())
        ->console()
        ->log(toBrowserStringForConsole(state, val));
    return ValueRef::createUndefined();
}

static ValueRef* infoFunction(ExecutionStateRef* state, ValueRef* thisValue,
                              size_t argc, ValueRef** argv,
                              bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchStarFish(state->context())
        ->console()
        ->info(toBrowserStringForConsole(state, val));
    return ValueRef::createUndefined();
}

static ValueRef* errorFunction(ExecutionStateRef* state, ValueRef* thisValue,
                               size_t argc, ValueRef** argv,
                               bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchStarFish(state->context())
        ->console()
        ->error(toBrowserStringForConsole(state, val));
    return ValueRef::createUndefined();
}

static ValueRef* warnFunction(ExecutionStateRef* state, ValueRef* thisValue,
                              size_t argc, ValueRef** argv,
                              bool isNewExpression)
{
    ValueRef* val = argc >= 1 ? argv[0] : ValueRef::createUndefined();
    fetchStarFish(state->context())
        ->console()
        ->warn(toBrowserStringForConsole(state, val));
    return ValueRef::createUndefined();
}

void ScriptBindingInstance::initBinding(Document* ownerDocument)
{
    m_ownerDocument = ownerDocument;

    ContextRef* context = scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);

    // binding names first
    GlobalObjectRef* globalObject = context->globalObject();
#define DECLARE_NAME_FOR_BINDING(exportName)                                   \
    ObjectRef::NativeDataAccessorPropertyData* newData##exportName =           \
        new ObjectRef::NativeDataAccessorPropertyData(                         \
            true, false, true,                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data) -> ValueRef* { \
                return fetchDocument(state->context())                         \
                    ->scriptBindingInstance()                                  \
                    ->value##exportName();                                     \
            },                                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data,                \
               ValueRef* setterInputData) -> bool {                            \
                fetchDocument(state->context())                                \
                    ->scriptBindingInstance()                                  \
                    ->value##exportName();                                     \
                fetchDocument(state->context())                                \
                    ->scriptBindingInstance()                                  \
                    ->m_value##exportName = setterInputData;                   \
                return true;                                                   \
            });                                                                \
    globalObject->defineNativeDataAccessorProperty(                            \
        state, ValueRef::create(StringRef::fromASCII(#exportName)),            \
        newData##exportName);
    STARFISH_ENUM_LAZY_BINDING_NAMES(DECLARE_NAME_FOR_BINDING)
    STARFISH_ENUM_LAZY_BINDING_NICKNAMES(DECLARE_NAME_FOR_BINDING)
#undef DECLARE_NAME_FOR_BINDING

    fnEventTarget();
    fnWindow();

    ownerWindow()->init(this, ownerWindow());

#if defined(STARFISH_ENABLE_TEST)
    globalObject->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("wptTestEnd")),
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(
            state, FunctionObjectRef::NativeFunctionInfo(
                       AtomicStringRef::create(context, "wptTestEnd"),
                       wptTestEndFunction, 0, nullptr, true, false))),
        false, false, true);
#endif

    ObjectRef* console = ObjectRef::create(state);

    console->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("log")),
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(
            state, FunctionObjectRef::NativeFunctionInfo(
                       AtomicStringRef::create(context, "log"), logFunction, 1,
                       nullptr, true, false))),
        false, false, false);

    console->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("info")),
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(
            state, FunctionObjectRef::NativeFunctionInfo(
                       AtomicStringRef::create(context, "info"), infoFunction,
                       1, nullptr, true, false))),
        false, false, false);

    console->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("error")),
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(
            state, FunctionObjectRef::NativeFunctionInfo(
                       AtomicStringRef::create(context, "error"), errorFunction,
                       1, nullptr, true, false))),
        false, false, false);

    console->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("warn")),
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(
            state, FunctionObjectRef::NativeFunctionInfo(
                       AtomicStringRef::create(context, "warn"), warnFunction,
                       1, nullptr, true, false))),
        false, false, false);

    globalObject->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("console")),
        ValueRef::create(console), false, false, false);

#ifdef TIZEN_DEVICE_API
    DeviceAPI::initialize(fetchData(this)->m_instance);
#endif

    state->destroy();
}
}
