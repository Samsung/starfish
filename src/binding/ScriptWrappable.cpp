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
#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
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
    // NOTE: input string contains whitecharacters as is, i.e., "\n" is stored
    // as '\','n'
    // The right way is, input string should already have '\n', and white spaces
    // should be removed from here.
    // For time being, we simply remove "\n" and other whitespaces strings.
    // newStr = newStr->replaceAll(String::fromUTF8("\n"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\t"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\f"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\r"), String::spaceString);
    return newStr;
}

StringRef* toJSString(String* v)
{
    return createScriptString(v);
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

void throwJSTypeErrorException(ExecutionStateRef* state, String* message)
{
    StringRef* msg = toJSString(message);
    ObjectRef* err =
        ErrorObjectRef::create(state, ErrorObjectRef::Code::TypeError, msg);
    state->throwException(ValueRef::create(err));
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

StringRef* createScriptString(String* str)
{
    auto data = str->bufferAccessData();
    if (data.hasASCIIContent) {
        return StringRef::fromASCII(data.asciiData(), data.length);
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
        STARFISH_LOG_ERROR(
            "Uncaught %s\n",
            toBrowserString(instance, ValueRef::create(result.error))
                ->toUTF8NonGCString()
                .data());

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
            STARFISH_LOG_ERROR(
                "Uncaught %s\n",
                toBrowserString(instance, ValueRef::create(sbresult.error))
                    ->toUTF8NonGCString()
                    .data());
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
    sb->destroy();
    if (!sbresult.error->isEmpty()) {
        STARFISH_LOG_ERROR("Uncaught %s\n",
                           toBrowserString(instance, sbresult.error)
                               ->toUTF8NonGCString()
                               .data());
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
    return buffer->buffer()->rawBuffer();
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
