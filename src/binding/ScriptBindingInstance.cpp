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
#include "core/dom/DOMImplementation.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLCollection.h"
#include "core/extra/Console.h"
#include "core/page/History.h"
#include "core/page/Location.h"
#include "core/page/Navigator.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Window.h"

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "core/extra/Avplay.h"
#endif

#include <EscargotPublic.h>

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"
#endif

namespace StarFish {

using namespace Escargot;

ScriptBindingInstance::ScriptBindingInstance(
    ScriptEngineInstance* engineInstance, Window* ownerWindow)
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
    m_ownerWindow = ownerWindow;
    m_ownerDocument = nullptr;
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
void ScriptBindingInstance::close()
{
    if (m_ownerWindow->browsingContext()->isTopLevelBrowsingContext()) {
        m_scriptContext->vmInstance()->clearCachesRelatedWithContext();
    }
#ifdef TIZEN_DEVICE_API
    m_deviceAPI = nullptr;
    DeviceAPI::close(m_scriptContext);
#endif
    m_scriptContext->destroy();
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
    fetchStarFish(state->context())
        ->console()
        ->log(toBrowserStringForConsole(state, val));

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchStarFish(state->context())
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
    fetchStarFish(state->context())
        ->console()
        ->info(toBrowserStringForConsole(state, val));

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchStarFish(state->context())
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
    fetchStarFish(state->context())
        ->console()
        ->error(toBrowserStringForConsole(state, val));

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchStarFish(state->context())
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
    fetchStarFish(state->context())
        ->console()
        ->warn(toBrowserStringForConsole(state, val));

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchStarFish(state->context())
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
    fetchStarFish(state->context())
        ->console()
        ->debug(toBrowserStringForConsole(state, val));

    for (size_t i = 1; i < argc; i++) {
        ValueRef* val = argv[i];
        fetchStarFish(state->context())
            ->console()
            ->debug(toBrowserStringForConsole(state, val));
    }
    return ValueRef::createUndefined();
}

static ValueRef* virtualIdentifierCallback(ExecutionStateRef* state,
                                           ValueRef* key)
{
    Window* self = fetchWindow(state->context());

    auto callee = state->resolveCallee();
    if (callee) {
        void* data = callee->asObject()->extraData();
        if (data) {
            ScriptWrappable* w = (ScriptWrappable*)data;
            if (w->isAttributeEventFunction()) {
                auto elementDOMObject =
                    ((AttributeEventFunction*)w)->element()->scriptValue();
                if (elementDOMObject->isObject()) {
                    bool exist = elementDOMObject->asObject()->hasOwnProperty(
                        state, key);
                    if (exist) {
                        return elementDOMObject->asObject()->getOwnProperty(
                            state, key);
                    }
                    return ValueRef::createEmpty();
                }
            }
        }
    }

    uint32_t idx = key->toArrayIndex(state);
    if (idx != ValueRef::InvalidArrayIndexValue) {
        Window* result = self->defaultIndexedGetter(idx);
        if (result != nullptr) {
            return result->scriptValue();
        }
    }
    String* name = toBrowserString(state, key);
    Nullable<ScriptObject> coll = self->defaultNamedGetter(name);
    if (coll.hasValue()) {
        return ValueRef::create(coll.getValue());
    }

    if (name->equals("self")) {
        return self->scriptValue();
    }

    return ValueRef::createEmpty();
}

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
static ValueRef* _openAvplayFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())
        ->avplay()
        ->open(toBrowserString(state, argv[0]));
    return ValueRef::createUndefined();
}

static ValueRef* _prepareAvplayFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->prepare();
    return ValueRef::createUndefined();
}

static ValueRef* _setDisplayRectAvplayFunction(ExecutionStateRef* state,
                                               ValueRef* thisValue, size_t argc,
                                               ValueRef** argv,
                                               bool isNewExpression)
{
    double arg1 = argv[0]->toNumber(state);
    double arg2 = argv[1]->toNumber(state);
    double arg3 = argv[2]->toNumber(state);
    double arg4 = argv[3]->toNumber(state);
    fetchStarFish(state->context())
        ->avplay()
        ->setDisplayRect(arg1, arg2, arg3, arg4);
    return ValueRef::createUndefined();
}

static ValueRef* _playAvplayFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->play();
    return ValueRef::createUndefined();
}

static ValueRef* _closeAvplayFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->close();
    return ValueRef::createUndefined();
}

static ValueRef* _pauseAvplayFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->pause();
    return ValueRef::createUndefined();
}

static ValueRef* _stopAvplayFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->stop();
    return ValueRef::createUndefined();
}

static ValueRef* _suspendAvplayFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->suspend();
    return ValueRef::createUndefined();
}

static ValueRef* _restoreAvplayFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->restore();
    return ValueRef::createUndefined();
}

static ValueRef* _getStateAvplayFunction(ExecutionStateRef* state,
                                         ValueRef* thisValue, size_t argc,
                                         ValueRef** argv, bool isNewExpression)
{
    String* ret = fetchStarFish(state->context())->avplay()->getState();
    return ValueRef::create(toJSString(ret));
}

static ValueRef* _getCurrentTimeAvplayFunction(ExecutionStateRef* state,
                                               ValueRef* thisValue, size_t argc,
                                               ValueRef** argv,
                                               bool isNewExpression)
{
    return ValueRef::create(
        fetchStarFish(state->context())->avplay()->getCurrentTime());
}

static ValueRef* _getDurationAvplayFunction(ExecutionStateRef* state,
                                            ValueRef* thisValue, size_t argc,
                                            ValueRef** argv,
                                            bool isNewExpression)
{
    return ValueRef::create(
        fetchStarFish(state->context())->avplay()->getDuration());
}

static ValueRef* _setStreamingPropertyAvplayFunction(ExecutionStateRef* state,
                                                     ValueRef* thisValue,
                                                     size_t argc,
                                                     ValueRef** argv,
                                                     bool isNewExpression)
{
    String* arg1 = toBrowserString(state, argv[0]);
    String* arg2 = toBrowserString(state, argv[1]);
    fetchStarFish(state->context())->avplay()->setStreamingProperty(arg1, arg2);
    return ValueRef::createUndefined();
}

static ValueRef* _prepareAsyncAvplayFunction(ExecutionStateRef* state,
                                             ValueRef* thisValue, size_t argc,
                                             ValueRef** argv,
                                             bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->prepareAsync(argv[0]);
    return ValueRef::createUndefined();
}

static ValueRef* _setListenerAvplayFunction(ExecutionStateRef* state,
                                            ValueRef* thisValue, size_t argc,
                                            ValueRef** argv,
                                            bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->setListener(argv[0]);
    return ValueRef::createUndefined();
}

static ValueRef* _seekToAvplayFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    fetchStarFish(state->context())->avplay()->seekTo(argv[0]->toNumber(state));
    return ValueRef::createUndefined();
}
#endif

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
    fnWindow();

    ownerWindow()->init(this, ownerWindow());

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

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    ObjectRef* avplay = ObjectRef::create(state);

#define DECLARE_AVPLAY_APIS(name)                                           \
    avplay->defineDataProperty(                                             \
        state, ValueRef::create(StringRef::fromASCII(#name)),               \
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(          \
            state, FunctionObjectRef::NativeFunctionInfo(                   \
                       AtomicStringRef::create(context, #name),             \
                       _##name##AvplayFunction, 1, nullptr, true, false))), \
        true, true, true);
    AVPLAY_APIS(DECLARE_AVPLAY_APIS)
#undef DECLARE_AVPLAY_APIS

    ObjectRef* webapis = ObjectRef::create(state);
    globalObject->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("webapis")),
        ValueRef::create(webapis), true, true, true);

    webapis->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("avplay")),
        ValueRef::create(avplay), true, true, true);
#endif

#ifdef TIZEN_DEVICE_API
    m_deviceAPI = DeviceAPI::initialize(m_scriptContext);
#endif

    context->setVirtualIdentifierCallback(virtualIdentifierCallback);

    state->destroy();
}
}
