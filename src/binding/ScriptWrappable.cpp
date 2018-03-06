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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/ErrorEvent.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Window.h"
#include "core/style/CSSStyleLookupTrie.h"

#include <EscargotPublic.h>

using namespace Escargot;

namespace StarFish {

ScriptValue scriptNull()
{
    return ValueRef::createNull();
}

ScriptValue scriptUndefined()
{
    return ValueRef::createUndefined();
}

ScriptValue scriptStringToScriptValue(ScriptString s)
{
    return ValueRef::create(s);
}

bool scriptValueIsBoolean(ScriptValue v)
{
    return v->isBoolean();
}

bool scriptValueAsBoolean(ScriptValue v)
{
    return v->asBoolean();
}

ScriptObject scriptError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    auto ret =
        ErrorObjectRef::create(state, ErrorObjectRef::None, toJSString(msg));
    state->destroy();
    return ret;
}

ScriptObject scriptEvalError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    auto ret = ErrorObjectRef::create(state, ErrorObjectRef::EvalError,
                                      toJSString(msg));
    state->destroy();
    return ret;
}

ScriptObject scriptRangeError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    auto ret = ErrorObjectRef::create(state, ErrorObjectRef::RangeError,
                                      toJSString(msg));
    state->destroy();
    return ret;
}

ScriptObject scriptReferenceError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    auto ret = ErrorObjectRef::create(state, ErrorObjectRef::ReferenceError,
                                      toJSString(msg));
    state->destroy();
    return ret;
}

ScriptObject scriptTypeError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    auto ret = ErrorObjectRef::create(state, ErrorObjectRef::TypeError,
                                      toJSString(msg));
    state->destroy();
    return ret;
}

ScriptObject scriptURIError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    auto ret = ErrorObjectRef::create(state, ErrorObjectRef::URIError,
                                      toJSString(msg));
    state->destroy();
    return ret;
}

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    ExecutionStateRef* state, ObjectRef* obj, StringRef* propertyName,
    ScriptNativeFunctionPointer getter, ScriptNativeFunctionPointer setter,
    bool isEnumerable, bool isConfigurable)
{
    FunctionObjectRef::NativeFunctionInfo nativeFunctionInfo(
        AtomicStringRef::emptyAtomicString(),
        (FunctionObjectRef::NativeFunctionPointer)getter, 0, nullptr, true,
        false);
    ValueRef* getterValue =
        ValueRef::create(FunctionObjectRef::create(state, nativeFunctionInfo));
    ValueRef* setterValue = ValueRef::createEmpty();
    if (setter) {
        FunctionObjectRef::NativeFunctionInfo nativeFunctionInfo(
            AtomicStringRef::emptyAtomicString(),
            (FunctionObjectRef::NativeFunctionPointer)setter, 1, nullptr, true,
            false);
        setterValue = ValueRef::create(
            FunctionObjectRef::create(state, nativeFunctionInfo));
    }
    ObjectRef::PresentAttribute attr = (ObjectRef::PresentAttribute)0;
    if (isEnumerable) {
        attr = (ObjectRef::PresentAttribute)(
            attr | ObjectRef::PresentAttribute::EnumerablePresent);
    }
    if (isConfigurable) {
        attr = (ObjectRef::PresentAttribute)(
            attr | ObjectRef::PresentAttribute::ConfigurablePresent);
    }
    obj->defineAccessorProperty(
        state, ValueRef::create(propertyName),
        ObjectRef::AccessorPropertyDescriptor(getterValue, setterValue, attr));
}

Window* fetchWindow(ContextRef* ctx)
{
    Window* window = (Window*)ctx->globalObject()->extraData();
    return window;
}

Document* fetchDocument(ContextRef* ctx)
{
    Window* window = (Window*)ctx->globalObject()->extraData();
    return window->document();
}

StarFish* fetchStarFish(ContextRef* ctx)
{
    Window* window = (Window*)ctx->globalObject()->extraData();
    return window->starFish();
}

StaticStrings* fetchStaticStrings(ContextRef* ctx)
{
    return fetchStarFish(ctx)->staticStrings();
}

String* toBrowserString(ExecutionStateRef* state, ValueRef* v)
{
    return toBrowserString(state, v->toString(state));
}

String* toBrowserString(ScriptBindingInstance* instance, Escargot::ValueRef* v,
                        bool* result)
{
    ContextRef* ctx = instance->scriptContext();
    SandBoxRef* sb = SandBoxRef::create(ctx);

    auto sbresult = sb->run([&](ExecutionStateRef* state) -> ValueRef* {
        return ValueRef::create(v->toString(state));
    });
    sb->destroy();
    if (!sbresult.error->isEmpty()) {
        if (result) {
            *result = false;
        }
        return String::emptyString;
    } else {
        if (result) {
            *result = true;
        }
        std::string s = sbresult.result->asString()->toStdUTF8String();
        return String::fromUTF8(s.data(), s.length());
    }
}

String* toBrowserString(ExecutionStateRef* state, StringRef* v)
{
    std::string s = v->toStdUTF8String();
    String* newStr = String::fromUTF8(s.data(), s.length());
    return newStr;
}

StringRef* toJSString(String* v)
{
    return createScriptString(v);
}

ScriptObject toCalleeObject(Escargot::ExecutionStateRef* state)
{
    auto callee = state->resolveCallee();
    if (callee) {
        return callee->asObject();
    }
    return nullptr;
}

ScriptValue errorOnConstructorFunction(Escargot::ExecutionStateRef* state,
                                       Escargot::ValueRef* thisValue,
                                       size_t argc, Escargot::ValueRef** argv,
                                       bool isNewExpression)
{
    StringRef* msg = StringRef::fromASCII("Illegal constructor");
    ObjectRef* err =
        ErrorObjectRef::create(state, ErrorObjectRef::Code::TypeError, msg);
    state->throwException(ValueRef::create(err));
    return Escargot::ValueRef::createUndefined();
}

ScriptWrappable::ScriptWrappable(void* extraPointerData)
{
    STARFISH_ASSERT(!((size_t)extraPointerData & (size_t)1));
    m_object = (ObjectRef*)((size_t)extraPointerData | (size_t)1);
}

ScriptObject ScriptWrappable::generateScriptObject()
{
    void* domObjectPointer;
    if (isWindow()) {
        domObjectPointer = this;
    } else {
        domObjectPointer = (void*)((size_t)m_object - 1);
    }

    init(scriptBindingInstance(), domObjectPointer);
    STARFISH_ASSERT(!isGivenUpScriptValue());

    return m_object;
}

ScriptValue ScriptWrappable::scriptValue()
{
    return ValueRef::create(scriptObject());
}

static int utf32ToUtf16(char32_t i, char16_t* u)
{
    if (i < 0xffff) {
        *u = (char16_t)(i & 0xffff);
        return 1;
    } else if (i < 0x10ffff) {
        i -= 0x10000;
        *u++ = 0xd800 | (i >> 10);
        *u = 0xdc00 | (i & 0x3ff);
        return 2;
    } else {
        // produce error char
        *u = 0xFFFD;
        return 1;
    }
}

ScriptWrappable* toScriptWrappable(ScriptValue v)
{
    if (v->isObject()) {
        return toScriptWrappable(v->asObject());
    }
    return nullptr;
}

ScriptWrappable* toScriptWrappable(ScriptObject v)
{
    if (v->extraData()) {
        return (ScriptWrappable*)v->extraData();
    }
    return nullptr;
}

StringRef* createScriptString(String* str)
{
    auto data = str->bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        return StringRef::fromASCII(data.asciiData(), data.length);
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        return StringRef::fromUTF16(data.utf16Data(), data.length);
    } else {
        UTF16StringDataNonGCStd out;
        for (size_t i = 0; i < str->length(); i++) {
            char32_t src = str->charAt(i);
            char16_t dst[2];
            int ret = utf32ToUtf16(src, dst);

            if (ret == 1) {
                out.push_back(src);
            } else if (ret == 2) {
                out.push_back(dst[0]);
                out.push_back(dst[1]);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }

        return StringRef::fromUTF16(out.data(), out.length());
    }
}

ScriptValue createScriptValue(ScriptObject object)
{
    return ValueRef::create(object);
}

ScriptValue createScriptValue(ScriptString s)
{
    StringRef* str = s;
    return ValueRef::create(str);
}

ScriptValue createScriptValue(ScriptArrayBuffer buffer)
{
    return ValueRef::create(buffer);
}

ScriptValue createScriptValue(ScriptArrayBufferView buffer)
{
    return ValueRef::create(buffer);
}

ScriptValue createScriptValue(ScriptUint8ClampedArray array)
{
    return ValueRef::create(array);
}

ScriptValue createScriptValue(uint32_t value)
{
    return ValueRef::create((unsigned long)value);
}

ScriptValue createScriptValue(String* value)
{
    return ValueRef::create(createScriptString(value));
}

ScriptValue createScriptFunction(ScriptBindingInstance* instance,
                                 String** argNames, size_t argc,
                                 String* functionBody, bool& error)
{
    error = false;

    ContextRef* ctx = instance->scriptContext();

    SandBoxRef* sb = SandBoxRef::create(ctx);
    auto result = sb->run([&](ExecutionStateRef* state) -> ValueRef* {
        ValueRef** argv = (ValueRef**)alloca(sizeof(ValueRef*) * (1 + argc));
        for (size_t i = 0; i < argc; i++) {
            argv[i] = ValueRef::create(createScriptString(argNames[i]));
        }
        argv[argc] = ValueRef::create(createScriptString(functionBody));
        return state->context()->globalObject()->function()->call(
            state, ValueRef::createUndefined(), argc + 1, argv);
    });
    sb->destroy();
    if (!result.error->isEmpty()) {
        error = true;
        // Dispatch error event to window
        ErrorEventInit errorInfo;
        errorInfo.setMessage(toBrowserString(instance, result.error));
        if (result.stackTraceData.size() > 0) {
            size_t lastIndex = result.stackTraceData.size() - 1;
            errorInfo.setFilename(toBrowserString(
                instance,
                ValueRef::create(result.stackTraceData[lastIndex].fileName)));
            errorInfo.setLineno(result.stackTraceData[lastIndex].loc.line);
            errorInfo.setColno(result.stackTraceData[lastIndex].loc.column);
        }
        errorInfo.setError(result.error);
        instance->ownerWindow()->dispatchErrorEvent(errorInfo);

        STARFISH_LOG_ERROR("Uncaught %s\n",
                           errorInfo.message()->toUTF8NonGCString().data());
        for (size_t i = 0; i < result.stackTraceData.size(); i++) {
            STARFISH_LOG_ERROR(
                "at %s(%d:%d)\n",
                toBrowserString(
                    instance,
                    ValueRef::create(result.stackTraceData[i].fileName))
                    ->toUTF8NonGCString()
                    .data(),
                (int)result.stackTraceData[i].loc.line,
                (int)result.stackTraceData[i].loc.column);
        }
        return result.error;
    } else {
        return result.result;
    }
}

ScriptValue createAttributeStringEventFunction(Element* target,
                                               String* functionBody,
                                               bool& result)
{
    String* name[] = { String::createASCIIString("event") };
    ScriptValue fn = createScriptFunction(target->scriptBindingInstance(), name,
                                          1, functionBody, result);

    if (fn->isFunction()) {
        fn->asFunction()->setExtraData(new AttributeEventFunction(target));
        fn->asFunction()->markFunctionNeedsSlowVirtualIdentifierOperation();
    }

    return fn;
}

ScriptValue callScriptFunction(ScriptBindingInstance* instance, ScriptValue fn,
                               ScriptValue* argv, size_t argc,
                               ScriptValue thisValue)
{
    ScriptValue result = ValueRef::createUndefined();
    if (fn->isFunction()) {
        ContextRef* ctx = instance->scriptContext();
        SandBoxRef* sb = SandBoxRef::create(ctx);
        auto sbresult = sb->run([&](ExecutionStateRef* state) -> ValueRef* {
            return fn->asFunction()->call(state, thisValue, argc, argv);
        });
        sb->destroy();
        if (!sbresult.error->isEmpty()) {
            // Dispatch error event to window
            ErrorEventInit errorInfo;
            errorInfo.setMessage(toBrowserString(instance, sbresult.error));
            if (sbresult.stackTraceData.size() > 0) {
                size_t lastIndex = sbresult.stackTraceData.size() - 1;
                errorInfo.setFilename(toBrowserString(
                    instance,
                    ValueRef::create(
                        sbresult.stackTraceData[lastIndex].fileName)));
                errorInfo.setLineno(
                    sbresult.stackTraceData[lastIndex].loc.line);
                errorInfo.setColno(
                    sbresult.stackTraceData[lastIndex].loc.column);
            }
            errorInfo.setError(sbresult.error);
            instance->ownerWindow()->dispatchErrorEvent(errorInfo);

            STARFISH_LOG_ERROR("Uncaught %s\n",
                               errorInfo.message()->toUTF8NonGCString().data());
            for (size_t i = 0; i < sbresult.stackTraceData.size(); i++) {
                STARFISH_LOG_ERROR(
                    "at %s(%d:%d)\n",
                    toBrowserString(
                        instance,
                        ValueRef::create(sbresult.stackTraceData[i].fileName))
                        ->toUTF8NonGCString()
                        .data(),
                    (int)sbresult.stackTraceData[i].loc.line,
                    (int)sbresult.stackTraceData[i].loc.column);
            }
        } else {
            result = sbresult.result;
        }
    }

    clearStack<102400>();

    return result;
}

ScriptValue callHandleEventFunction(ScriptBindingInstance* instance,
                                    ScriptValue obj, ScriptValue* argv,
                                    size_t argc, ScriptValue thisValue)
{
    ScriptValue result = ValueRef::createUndefined();
    ContextRef* ctx = instance->scriptContext();
    SandBoxRef* sb = SandBoxRef::create(ctx);
    auto sbresult = sb->run([&](ExecutionStateRef* state) -> ValueRef* {
        ValueRef* v = obj->asObject()->get(
            state, ValueRef::create(StringRef::fromASCII("handleEvent")));
        return v;
    });
    sb->destroy();

    if (!sbresult.error->isEmpty()) {
        STARFISH_LOG_ERROR(
            "Uncaught %s\n",
            toBrowserString(instance, ValueRef::create(sbresult.error))
                ->toUTF8NonGCString()
                .data());
    } else {
        return callScriptFunction(instance, sbresult.result, argv, argc,
                                  thisValue);
    }

    return result;
}

ScriptValue evaluateString(ScriptBindingInstance* instance, String* string,
                           String* fileName, bool* result)
{
    ContextRef* ctx = instance->scriptContext();
    ScriptParserRef::ScriptParserResult scriptRef =
        ctx->scriptParser()->parse(toJSString(string), toJSString(fileName));

    if (scriptRef.m_error->length()) {
        STARFISH_LOG_ERROR(
            "Script parse error %s\n",
            toBrowserString(instance, ValueRef::create(scriptRef.m_error))
                ->toUTF8NonGCString()
                .data());
        if (result)
            *result = false;
        return scriptUndefined();
    }

    SandBoxRef* sb = SandBoxRef::create(ctx);
    auto sbresult = sb->run([&](ExecutionStateRef* state) -> ValueRef* {
        return scriptRef.m_script->execute(state);
    });

    clearStack<102400>();

    sb->destroy();
    if (!sbresult.error->isEmpty()) {
        // Dispatch error event to window
        ErrorEventInit errorInfo;
        errorInfo.setMessage(toBrowserString(instance, sbresult.error));
        if (sbresult.stackTraceData.size() > 0) {
            size_t lastIndex = sbresult.stackTraceData.size() - 1;
            errorInfo.setFilename(toBrowserString(
                instance,
                ValueRef::create(sbresult.stackTraceData[lastIndex].fileName)));
            errorInfo.setLineno(sbresult.stackTraceData[lastIndex].loc.line);
            errorInfo.setColno(sbresult.stackTraceData[lastIndex].loc.column);
        }
        errorInfo.setError(sbresult.error);
        instance->ownerWindow()->dispatchErrorEvent(errorInfo);

        STARFISH_LOG_ERROR("Uncaught %s\n",
                           errorInfo.message()->toUTF8NonGCString().data());
        for (size_t i = 0; i < sbresult.stackTraceData.size(); i++) {
            STARFISH_LOG_ERROR(
                "at %s(%d:%d)\n",
                toBrowserString(
                    instance,
                    ValueRef::create(sbresult.stackTraceData[i].fileName))
                    ->toUTF8NonGCString()
                    .data(),
                (int)sbresult.stackTraceData[i].loc.line,
                (int)sbresult.stackTraceData[i].loc.column);
        }
        if (result)
            *result = true;
        return sbresult.error;
    } else {
        if (result)
            *result = true;
        return sbresult.result;
    }
}

ScriptValue createArrayBuffer(ScriptBindingInstance* instance, void* bufferSrc,
                              size_t len)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    ArrayBufferObjectRef* obj = ArrayBufferObjectRef::create(state);
    obj->attachBuffer(bufferSrc, len);
    state->destroy();
    return ValueRef::create(obj);
}

ScriptUint8ClampedArray createEmptyUint8ClampedArray(
    ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    return Uint8ClampedArrayObjectRef::create(state);
}

void registerJavaScriptNativeInterface(
    ScriptBindingInstance* instance, String* exposedObjectName,
    String* jsFunctionName, void* scriptObject,
    Escargot::ScriptNativeFunctionPointer scriptNativeFunctionPointer)
{
    ContextRef* context = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);
    GlobalObjectRef* globalObject = context->globalObject();

    ObjectRef* targetObject = nullptr;
    auto key = ValueRef::create(toJSString(exposedObjectName));
    if (globalObject->hasOwnProperty(state, key)) {
        targetObject = globalObject->getOwnProperty(state, key)->asObject();
    } else {
        targetObject = ObjectRef::create(state);
        globalObject->defineDataProperty(
            state, ValueRef::create(toJSString(exposedObjectName)),
            ValueRef::create(targetObject), false, false, true);
    }

    StringRef* nativeCallbackString = toJSString(jsFunctionName);
    FunctionObjectRef* nativeCallbackESFn = FunctionObjectRef::create(
        state, FunctionObjectRef::NativeFunctionInfo(
                   AtomicStringRef::create(
                       context, jsFunctionName->toUTF8NonGCString().data()),
                   scriptNativeFunctionPointer, 1, nullptr, true, false));

    nativeCallbackESFn->setExtraData(scriptObject);
    targetObject->defineDataProperty(
        state, Escargot::ValueRef::create(nativeCallbackString),
        Escargot::ValueRef::create(nativeCallbackESFn), false, false, true);
    state->destroy();
}
void unregisterJavaScriptNativeInterface(ScriptBindingInstance* instance,
                                         String* exposedObjectName,
                                         String* jsFunctionName)
{
    ContextRef* context = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);
    GlobalObjectRef* globalObject = context->globalObject();

    auto key = ValueRef::create(toJSString(exposedObjectName));
    if (globalObject->hasOwnProperty(state, key)) {
        ObjectRef* targetObject =
            globalObject->getOwnProperty(state, key)->asObject();
        auto nativeCallbackName = ValueRef::create(toJSString(jsFunctionName));
        if (targetObject->hasOwnProperty(state, nativeCallbackName)) {
            targetObject->deleteOwnProperty(state, nativeCallbackName);
        }
    }
    // TODO : delete global object when there is no callback in that object

    state->destroy();
}

ScriptValue parseJSON(ScriptBindingInstance* instance, String* jsonData)
{
    ContextRef* ctx = instance->scriptContext();
    ScriptValue jsonArg[1] = { ValueRef::create(createScriptString(jsonData)) };
    FunctionObjectRef* jsonParseFn = ctx->globalObject()->jsonParse();
    return callScriptFunction(instance, ValueRef::create(jsonParseFn), jsonArg,
                              1, ValueRef::create(ctx->globalObject()->json()));
}

double parseDate(ScriptBindingInstance* instance, String* date)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    DateObjectRef* obj = DateObjectRef::create(state);
    obj->setTimeValue(state, ValueRef::create(createScriptString(date)));
    double ret = obj->primitiveValue();
    state->destroy();
    return ret;
}

String* timeToUTCString(ScriptBindingInstance* instance, int64_t value)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    DateObjectRef* obj = DateObjectRef::create(state);
    obj->setTimeValue(value);
    return toBrowserString(state, obj->toUTCString(state));
}

bool isCallableScriptValue(ScriptValue v)
{
    if (v->isFunction()) {
        return true;
    }
    return false;
}

bool isObjectScriptValue(ScriptValue v)
{
    if (v->isObject()) {
        return true;
    }
    return false;
}

#ifdef STARFISH_ENABLE_TEST
void invokeTestStartFunction(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    ScriptValue fn = ctx->globalObject()->get(
        state, ValueRef::create(StringRef::fromASCII("testStart")));
    state->destroy();
    callScriptFunction(instance, fn, nullptr, 0, scriptUndefined());
}
#endif

uint8_t* arrayBufferRawData(ScriptArrayBuffer buffer)
{
    return buffer->rawBuffer();
}

uint8_t* arrayBufferViewRawData(ScriptArrayBufferView buffer)
{
    return buffer->rawBuffer();
}

unsigned arrayBufferSize(ScriptArrayBuffer buffer)
{
    return buffer->bytelength();
}

unsigned arrayBufferViewSize(ScriptArrayBufferView buffer)
{
    return buffer->bytelength();
}

Promise::Promise(ScriptBindingInstance* instance)
{
    m_instance = instance;
    ContextRef* ctx = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    m_scriptValue = ValueRef::create(PromiseObjectRef::create(state));
    state->destroy();
}

void Promise::fulfill(ScriptValue v)
{
    ContextRef* ctx = m_instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    m_scriptValue->asObject()->asPromiseObject()->fulfill(state, v);
    state->destroy();
}

void Promise::reject(ScriptValue v)
{
    ContextRef* ctx = m_instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    m_scriptValue->asObject()->asPromiseObject()->reject(state, v);
    state->destroy();
}
}
