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
#include "core/page/Window.h"
#if defined(STARFISH_WEBWORKER_HOST)
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#endif

#if defined(STARFISH_ENABLE_WASM)
#include "core/fetch/Response.h"
#include "core/fetch/ResponseData.h"
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

#ifdef STARFISH_ENABLE_WASM
static ValueRef* compilePotentialWASMResponse(ExecutionStateRef* state,
                                              ValueRef* thisValue, size_t argc,
                                              ValueRef** argv,
                                              bool isNewExpression)
{
    // Upon fulfillment of source with value unwrappedSource:
    // Let response be unwrappedSource's response.
    CHECK_TYPEOF(argv[0], Response);
    Response* response = (Response*)(argv[0]->asObject()->extraData());

    // Let mimeType be the result of extracting a MIME type from response's
    // header list. If mimeType is not `application/wasm`, reject returnValue
    // with a TypeError and abort these substeps.
    // TODO update application/wasm category
    /*
    String* mimeType = response->headers()->extractMIMEType();
    if (!mimeType->equals("application/wasm")) {
        THROW_EXCEPTION(ILLEGAL_INVOKE);
    }
    */

    // If response is not CORS-same-origin, reject returnValue with a TypeError
    // and abort these substeps.
    ResponseType responseType = response->typeValue();
    if (responseType != ResponseType::Basic &&
        responseType != ResponseType::Cors &&
        responseType != ResponseType::Default) {
        THROW_EXCEPTION(ILLEGAL_INVOKE);
    }

    // If response's status is not an ok status, reject returnValue with a
    // TypeError and abort these substeps.
    if (!response->ok()) {
        THROW_EXCEPTION(ILLEGAL_INVOKE);
    }

    // consume response's body as an ArrayBuffer, and let bodyPromise be the
    // result.
    PromiseObjectRef* bodyPromise =
        response->arrayBuffer()->scriptValue()->asPromiseObject();

    // Note) bodyPromise is already resolved or rejected
    STARFISH_ASSERT(bodyPromise->state() == PromiseObjectRef::FulFilled ||
                    bodyPromise->state() == PromiseObjectRef::Rejected);

    // Upon fulfillment of bodyPromise with value bodyArrayBuffer:
    if (bodyPromise->state() == PromiseObjectRef::FulFilled) {
        // Let stableBytes be a copy of the bytes held by the buffer
        // bodyArrayBuffer.
        ValueRef* bodyArrayBuffer = bodyPromise->promiseResult();
        ValueRef* stableBytes =
            WASMOperationsRef::copyStableBufferBytes(state, bodyArrayBuffer);

        // Asynchronously compile the WebAssembly module stableBytes using the
        // networking task source and resolve returnValue with the result.
        return WASMOperationsRef::asyncCompileModule(state, stableBytes);
    }

    // Upon rejection of bodyPromise with reason reason:
    // Reject returnValue with reason.
    STARFISH_ASSERT(bodyPromise->state() == PromiseObjectRef::Rejected);
    state->throwException(bodyPromise->promiseResult());

    return nullptr;
}

// https://webassembly.github.io/spec/web-api/#dom-webassembly-compilestreaming
static ValueRef* compileStreamingWASMFunction(ExecutionStateRef* state,
                                              ValueRef* thisValue, size_t argc,
                                              ValueRef** argv,
                                              bool isNewExpression)
{
    ValueRef* source = argv[0];
    if (!source->isPromiseObject()) {
        // check `source` argument type
        PromiseObjectRef* returnValue = PromiseObjectRef::create(state);

        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH, "0", "source", "Promise");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "compileStreaming",
                        "WebAssembly", reason);
        ValueRef* error = ErrorObjectRef::create(
            state, ErrorObjectRef::TypeError,
            StringRef::createFromASCII(msg, strlen(msg)));

        returnValue->reject(state, error);
        return returnValue;
    }

    // returns the result of compiling a potential WebAssembly response with
    // source
    auto compiler = FunctionObjectRef::create(
        state, FunctionObjectRef::NativeFunctionInfo(
                   AtomicStringRef::emptyAtomicString(),
                   compilePotentialWASMResponse, 1, true, false));
    return source->asPromiseObject()->then(state, compiler);
}

// https://webassembly.github.io/spec/web-api/index.html#dom-webassembly-instantiatestreaming
static ValueRef* instantiateStreamingWASMFunction(ExecutionStateRef* state,
                                                  ValueRef* thisValue,
                                                  size_t argc, ValueRef** argv,
                                                  bool isNewExpression)
{
    ValueRef* source = argv[0];
    ValueRef* importObject = argv[1];

    if (!source->isObject() ||
        (!source->isPromiseObject() &&
         (!source->asObject()->extraData() ||
          !((ScriptWrappable*)source->asObject()->extraData())
               ->isResponse()))) {
        // check `source` argument type
        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH, "0", "source",
                        "Response or Promise");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "instantiateStreaming",
                        "WebAssembly", reason);
        ValueRef* error = ErrorObjectRef::create(
            state, ErrorObjectRef::TypeError,
            StringRef::createFromASCII(msg, strlen(msg)));

        PromiseObjectRef* sourceReturn = PromiseObjectRef::create(state);
        sourceReturn->reject(state, error);
        return sourceReturn;
    }

    PromiseObjectRef* promiseOfModule = nullptr;

    // Let promiseOfModule be the result of compiling a potential WebAssembly
    // response with source.
    auto compiler = FunctionObjectRef::create(
        state, FunctionObjectRef::NativeFunctionInfo(
                   AtomicStringRef::emptyAtomicString(),
                   compilePotentialWASMResponse, 1, true, false));

    if (source->isPromiseObject()) {
        promiseOfModule =
            source->asPromiseObject()->then(state, compiler)->asPromiseObject();
    } else {
        STARFISH_ASSERT(
            ((ScriptWrappable*)source->asObject()->extraData())->isResponse());
        PromiseObjectRef* sourceReturn = PromiseObjectRef::create(state);
        sourceReturn->fulfill(state, source);

        promiseOfModule =
            sourceReturn->then(state, compiler)->asPromiseObject();
    }

    // Return the result of instantiating the promise of a module
    // promiseOfModule with imports importObject.
    return WASMOperationsRef::instantiatePromiseOfModuleWithImportObject(
        state, promiseOfModule, importObject);
}
#endif

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
        globalBindingNameData->m_valueGetter(globalBindingNameData->m_instance);
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
    ExecutionStateRef* state, ObjectRef* object, StringRef* name,
    GlobalBindingNameAccessorGetter getter,
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
static void printToDebuggerInConsole(ExecutionStateRef* state, String* message,
                                     const char* head)
{
    StringBuilder sb;
    sb.appendString(head, strlen(head));
    sb.appendString(message);
    state->context()->printDebugger(toJSString(sb.finalize()));
}
#endif

static String* _createConcatenatedStringForConsole(ExecutionStateRef* state,
                                                   size_t argc, ValueRef** argv)
{
    StringBuilder sb;
    sb.appendString(toBrowserStringForConsole(
        state, argc > 0 ? argv[0] : ValueRef::createUndefined()));
    for (size_t i = 1; i < argc; i++) {
        sb.appendString(" ");
        sb.appendString(toBrowserStringForConsole(state, argv[i]));
    }
    return sb.finalize();
}

static ValueRef* _logConsoleFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    String* str = _createConcatenatedStringForConsole(state, argc, argv);
    fetchWebBase(state->context())->console()->log(str);
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, str, "console.log : ");
#endif
    return ValueRef::createUndefined();
}

static ValueRef* _infoConsoleFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    String* str = _createConcatenatedStringForConsole(state, argc, argv);
    fetchWebBase(state->context())->console()->info(str);
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, str, "console.info : ");
#endif
    return ValueRef::createUndefined();
}

static ValueRef* _errorConsoleFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    String* str = _createConcatenatedStringForConsole(state, argc, argv);
    fetchWebBase(state->context())->console()->error(str);
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, str, "console.error : ");
#endif
    return ValueRef::createUndefined();
}

static ValueRef* _warnConsoleFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    String* str = _createConcatenatedStringForConsole(state, argc, argv);
    fetchWebBase(state->context())->console()->warn(str);
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, str, "console.warn : ");
#endif
    return ValueRef::createUndefined();
}

static ValueRef* _debugConsoleFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    String* str = _createConcatenatedStringForConsole(state, argc, argv);
    fetchWebBase(state->context())->console()->debug(str);
#if defined(STARFISH_ENABLE_DEBUGGER)
    printToDebuggerInConsole(state, str, "console.debug : ");
#endif
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
                STARFISH_UNSUPPORTED("module \"%s\"",          \
                                       #exportName);                           \
                return ValueRef::createUndefined();                            \
            },                                                                 \
            [](ExecutionStateRef* state, ObjectRef* self,                      \
               ObjectRef::NativeDataAccessorPropertyData* data,                \
               ValueRef* setterInputData) -> bool {                            \
                STARFISH_UNSUPPORTED("module \"%s\"",          \
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

#ifdef STARFISH_ENABLE_WASM
    ValueRef* wasm = globalObject->getOwnProperty(
        state, StringRef::createFromASCII("WebAssembly"));
    STARFISH_ASSERT(wasm->isObject());

    wasm->asObject()->defineDataProperty(
        state, StringRef::createFromASCII("compileStreaming"),
        FunctionObjectRef::createBuiltinFunction(
            state, FunctionObjectRef::NativeFunctionInfo(
                       AtomicStringRef::create(context, "compileStreaming"),
                       compileStreamingWASMFunction, 1, true, false)),
        true, true, true);

    wasm->asObject()->defineDataProperty(
        state, StringRef::createFromASCII("instantiateStreaming"),
        FunctionObjectRef::createBuiltinFunction(
            state, FunctionObjectRef::NativeFunctionInfo(
                       AtomicStringRef::create(context, "instantiateStreaming"),
                       instantiateStreamingWASMFunction, 2, true, false)),
        true, true, true);
#endif
}
} // namespace Starfish
