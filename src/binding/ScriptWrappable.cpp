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
#include "binding/ScriptWrappable.h"
#include "binding/ScriptBindingInstance.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/ErrorEvent.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/GlobalScope.h"

#if defined(STARFISH_ENABLE_DEBUGGER)
#include "core/page/BrowsingContext.h"
#endif

#if !defined(STARFISH_WEBWORKER_HOST)
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"
#else
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#endif /* !defined(STARFISH_WEBWORKER_HOST) */

#include <EscargotPublic.h>

using namespace Escargot;

namespace Starfish {

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

unsigned scriptValueAsNumber(ScriptValue v)
{
    return v->asNumber();
}

ScriptObject scriptError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, String* msg) -> ValueRef* {
                   return ErrorObjectRef::create(state, ErrorObjectRef::None,
                                                 toJSString(msg));
               },
               msg)
        .result->asObject();
}

ScriptObject scriptEvalError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, String* msg) -> ValueRef* {
                   return ErrorObjectRef::create(
                       state, ErrorObjectRef::EvalError, toJSString(msg));
               },
               msg)
        .result->asObject();
}

ScriptObject scriptRangeError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, String* msg) -> ValueRef* {
                   return ErrorObjectRef::create(
                       state, ErrorObjectRef::RangeError, toJSString(msg));
               },
               msg)
        .result->asObject();
}

ScriptObject scriptReferenceError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, String* msg) -> ValueRef* {
                   return ErrorObjectRef::create(
                       state, ErrorObjectRef::ReferenceError, toJSString(msg));
               },
               msg)
        .result->asObject();
}

ScriptObject scriptTypeError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, String* msg) -> ValueRef* {
                   return ErrorObjectRef::create(
                       state, ErrorObjectRef::TypeError, toJSString(msg));
               },
               msg)
        .result->asObject();
}

ScriptObject scriptURIError(ScriptBindingInstance* instance, String* msg)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, String* msg) -> ValueRef* {
                   return ErrorObjectRef::create(
                       state, ErrorObjectRef::URIError, toJSString(msg));
               },
               msg)
        .result->asObject();
}

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    ExecutionStateRef* state, ObjectRef* obj, StringRef* propertyName,
    ScriptNativeFunctionPointer getter, ScriptNativeFunctionPointer setter,
    bool isEnumerable, bool isConfigurable)
{
    FunctionObjectRef::NativeFunctionInfo nativeFunctionInfo(
        AtomicStringRef::emptyAtomicString(),
        (FunctionObjectRef::NativeFunctionPointer)getter, 0, true, false);
    ValueRef* getterValue =
        ValueRef::create(FunctionObjectRef::create(state, nativeFunctionInfo));
    OptionalRef<ValueRef> setterValue;
    if (setter) {
        FunctionObjectRef::NativeFunctionInfo nativeFunctionInfo(
            AtomicStringRef::emptyAtomicString(),
            (FunctionObjectRef::NativeFunctionPointer)setter, 1, true, false);
        setterValue = FunctionObjectRef::create(state, nativeFunctionInfo);
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
        state, propertyName,
        ObjectRef::AccessorPropertyDescriptor(getterValue, setterValue, attr));
}

static void loggingJSErrorInfo(
    ScriptBindingInstance* instance,
    const ::Escargot::Evaluator::EvaluatorResult& sbResult)
{
    STARFISH_LOG_ERROR(
        "Uncaught %s",
        sbResult.resultOrErrorToString(instance->scriptContext())
            ->toStdUTF8String()
            .data());
    for (size_t i = 0; i < sbResult.stackTrace.size(); i++) {
        STARFISH_LOG_ERROR(
            "at %s(%d:%d)",
            toBrowserString(instance, sbResult.stackTrace[i].srcName)
                ->toUTF8NonGCString()
                .data(),
            (int)sbResult.stackTrace[i].loc.line,
            (int)sbResult.stackTrace[i].loc.column);

        Escargot::StringRef* src = sbResult.stackTrace[i].sourceCode;
        if (src->length()) {
            const size_t preLineMax = 40;
            const size_t afterLineMax = 40;

            size_t preLineSoFar = 0;
            size_t afterLineSoFar = 0;

            size_t start = sbResult.stackTrace[i].loc.index;
            int64_t idx = (int64_t)start;
            while (start - idx < preLineMax) {
                if (idx == 0) {
                    break;
                }
                if (src->charAt((size_t)idx) == '\r' ||
                    src->charAt((size_t)idx) == '\n') {
                    idx++;
                    break;
                }
                idx--;
            }
            preLineSoFar = idx;

            idx = start;
            while (idx - start < afterLineMax) {
                if ((size_t)idx == src->length() - 1) {
                    break;
                }
                if (src->charAt((size_t)idx) == '\r' ||
                    src->charAt((size_t)idx) == '\n') {
                    break;
                }
                idx++;
            }
            afterLineSoFar = idx;

            if (preLineSoFar <= afterLineSoFar &&
                preLineSoFar <= src->length() &&
                afterLineSoFar <= src->length()) {
                auto subSrc = src->substring(preLineSoFar, afterLineSoFar);
                STARFISH_LOG_INFO("%s", subSrc->toStdUTF8String().data());
                std::string sourceCodePosition;
                for (size_t i = preLineSoFar; i < start; i++) {
                    sourceCodePosition += " ";
                }
                sourceCodePosition += "^\n";
                STARFISH_LOG_INFO("%s", sourceCodePosition.data());
            }
        }
    }
}

template <typename T>
T* fetchGlobalObject(ContextRef* ctx)
{
    return static_cast<T*>(ctx->globalObject()->extraData());
}

ExecutionContext* fetchExecutionContext(ContextRef* ctx)
{
    return fetchGlobalObject(ctx)->executionContext();
}

WebBase* fetchWebBase(ContextRef* ctx)
{
    return fetchGlobalObject(ctx)->webBase();
}

ScriptBindingInstance* fetchScriptBindingInstance(ContextRef* ctx)
{
    return fetchGlobalObject(ctx)->scriptBindingInstance();
}

#if !defined(STARFISH_WEBWORKER_HOST)
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

Document* fetchResponsibleDocument(ExecutionStateRef* state)
{
    Window* window =
        (Window*)state->resolveCallerLexicalGlobalObject()->extraData();
    return window->document();
}

WebView* fetchWebView(ContextRef* ctx)
{
    Window* window = (Window*)ctx->globalObject()->extraData();
    return window->webView();
}

StaticStrings* fetchStaticStrings(ContextRef* ctx)
{
    return fetchWebView(ctx)->starfish()->staticStrings();
}
#endif /* !defined(STARFISH_WEBWORKER_HOST) */

class EscargotStringView : public String {
public:
    EscargotStringView(StringRef* str)
        : String()
        , m_data(str)
    {
    }

    virtual size_t length() const override
    {
        return m_data->length();
    }

    virtual char32_t charAt(const size_t& idx) const override
    {
        return m_data->charAt(idx);
    }

    virtual StringBufferAccessData bufferAccessData() const override
    {
        auto jsBufData = m_data->stringBufferAccessData();
        StringBufferAccessData ret;
        ret.bufferDataKind = jsBufData.has8BitContent
                                 ? StringBufferAccessData::ASCIIData
                                 : StringBufferAccessData::BMPData;
        ret.isNullTerminated = false;
        ret.buffer = jsBufData.buffer;
        ret.length = jsBufData.length;
        return ret;
    }

    bool isStringView() override
    {
        return true;
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(EscargotStringView));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(EscargotStringView)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(EscargotStringView, m_data));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(EscargotStringView));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    StringRef* m_data;
};

static String* toBrowserString(StringRef* v)
{
    auto bufData = v->stringBufferAccessData();
    if (bufData.has8BitContent) {
        bool hasASCIIContent = true;
        for (size_t i = 0; i < bufData.length; i++) {
            StringRef::StringBufferAccessDataRef::LChar ch =
                bufData.uncheckedCharAtFor8Bit(i);
            if (UNLIKELY(ch >= 128)) {
                hasASCIIContent = false;
                break;
            }
        }

        if (hasASCIIContent) {
            return new EscargotStringView(v);
        } else {
            auto b = v->toStdUTF8String();
            return String::fromUTF8(b.data(), b.length());
        }
    } else {
        bool hasBMPContent = true;
        for (size_t i = 0; i < bufData.length; i++) {
            auto ch = bufData.uncheckedCharAtFor16Bit(i);
            if (UNLIKELY(U16_IS_LEAD(ch))) {
                hasBMPContent = false;
                break;
            }
        }

        if (hasBMPContent) {
            return new EscargotStringView(v);
        } else {
            return String::fromUTF16((char16_t*)bufData.buffer, bufData.length);
        }
    }
}

String* toBrowserString(ExecutionStateRef* state, ValueRef* v)
{
    return toBrowserString(state, v->toString(state));
}

String* toBrowserString(ScriptBindingInstance* instance, Escargot::ValueRef* v,
                        bool* result)
{
    ContextRef* ctx = instance->scriptContext();
    auto sbresult = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, Escargot::ValueRef* v) -> ValueRef* {
            return ValueRef::create(v->toString(state));
        },
        v);
    if (sbresult.error.hasValue()) {
        if (result) {
            *result = false;
        }
        return String::emptyString;
    } else {
        if (result) {
            *result = true;
        }
        return toBrowserString(sbresult.result->asString());
    }
}

String* toBrowserString(ExecutionStateRef* state, StringRef* v)
{
    return toBrowserString(v);
}

StringRef* toJSString(String* v)
{
    return createScriptString(v);
}

ScriptObject toCalleeObject(Escargot::ExecutionStateRef* state)
{
    auto callee = state->resolveCallee();
    if (callee) {
        return callee.value()->asObject();
    }
    return nullptr;
}

ScriptValue errorOnConstructorFunction(Escargot::ExecutionStateRef* state,
                                       Escargot::ValueRef* thisValue,
                                       size_t argc, Escargot::ValueRef** argv,
                                       bool isNewExpression)
{
    StringRef* msg = StringRef::createFromASCII("Illegal constructor");
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
    if (isGlobalScope()) {
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
        return StringRef::createFromASCII(data.asciiData(), data.length);
    } else if (data.bufferDataKind == StringBufferAccessData::BMPData) {
        return StringRef::createFromUTF16(data.utf16Data(), data.length);
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
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
        }

        return StringRef::createFromUTF16(out.data(), out.length());
    }
}

ScriptString createScriptString(const char* utf8Buffer, size_t len)
{
    return StringRef::createFromUTF8(utf8Buffer, len);
}

ScriptString createScriptASCIIString(const char* asciiBuffer, size_t len)
{
    return StringRef::createFromASCII(asciiBuffer, len);
}

ScriptValue createScriptValue(ScriptObject object)
{
    return object;
}

ScriptValue createScriptValue(ScriptString s)
{
    StringRef* str = s;
    return str;
}

ScriptValue createScriptValue(ScriptArrayBuffer buffer)
{
    return buffer;
}

ScriptValue createScriptValue(ScriptArrayBufferView buffer)
{
    return buffer;
}

ScriptValue createScriptValue(ScriptUint8ClampedArray array)
{
    return array;
}

ScriptValue createScriptValue(uint32_t value)
{
    return ValueRef::create((unsigned long)value);
}

ScriptValue createScriptValue(String* value)
{
    return createScriptString(value);
}

ScriptValue createScriptValue(double value)
{
    return ValueRef::create(value);
}

ScriptValue createScriptFunction(ScriptBindingInstance* instance,
                                 String** argNames, size_t argc,
                                 String* functionBody, bool& error)
{
    error = false;

    ContextRef* ctx = instance->scriptContext();

    auto result = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, String** argNames, size_t argc,
           String* functionBody) -> ValueRef* {
            ValueRef** argv = ALLOCA(sizeof(ValueRef*) * (1 + argc), ValueRef*);
            for (size_t i = 0; i < argc; i++) {
                argv[i] = ValueRef::create(createScriptString(argNames[i]));
            }
            argv[argc] = ValueRef::create(createScriptString(functionBody));
            return state->context()->globalObject()->function()->call(
                state, ValueRef::createUndefined(), argc + 1, argv);
        },
        argNames, argc, functionBody);
    if (result.error.hasValue()) {
        ScriptValue errorValue = result.error.value();
        error = true;
        // Dispatch error event to window
        ErrorEventInit errorInfo;
        errorInfo.setMessage(toBrowserString(instance, errorValue));
        if (result.stackTrace.size() > 0) {
            size_t lastIndex = result.stackTrace.size() - 1;
            errorInfo.setFilename(toBrowserString(
                instance, result.stackTrace[lastIndex].srcName));
            errorInfo.setLineno(result.stackTrace[lastIndex].loc.line);
            errorInfo.setColno(result.stackTrace[lastIndex].loc.column);
        }
        errorInfo.setError(errorValue);
        instance->dispatchErrorEventToGlobalScope(errorInfo);

        loggingJSErrorInfo(instance, result);
        return errorValue;
    } else {
        return result.result;
    }
}

ScriptValue createAttributeStringEventFunction(EventTarget* target,
                                               String* functionBody,
                                               bool& result)
{
    String* name[] = { String::createASCIIString("event") };
    ScriptValue fn = createScriptFunction(target->scriptBindingInstance(), name,
                                          1, functionBody, result);

    if (fn->isFunctionObject()) {
        fn->asFunctionObject()->setExtraData(
            new AttributeEventFunction(target));
        fn->asFunctionObject()
            ->markFunctionNeedsSlowVirtualIdentifierOperation();
    }

    return fn;
}

ScriptValue callScriptFunction(ScriptBindingInstance* instance, ScriptValue fn,
                               ScriptValue* argv, size_t argc,
                               ScriptValue thisValue)
{
    ScriptValue result = ValueRef::createUndefined();
    if (fn->isCallable()) {
        ContextRef* ctx = instance->scriptContext();
        auto sbresult = Evaluator::execute(
            ctx,
            [](ExecutionStateRef* state, ScriptValue fn, ScriptValue* argv,
               size_t argc, ScriptValue thisValue) -> ValueRef* {
                return fn->asObject()->call(state, thisValue, argc, argv);
            },
            fn, argv, argc, thisValue);
        if (sbresult.error.hasValue()) {
            // Dispatch error event to window
            ScriptValue errorValue = sbresult.error.value();
            ErrorEventInit errorInfo;
            errorInfo.setMessage(toBrowserString(instance, errorValue));
            if (sbresult.stackTrace.size() > 0) {
                size_t lastIndex = sbresult.stackTrace.size() - 1;
                errorInfo.setFilename(toBrowserString(
                    instance,
                    ValueRef::create(sbresult.stackTrace[lastIndex].srcName)));
                errorInfo.setLineno(sbresult.stackTrace[lastIndex].loc.line);
                errorInfo.setColno(sbresult.stackTrace[lastIndex].loc.column);
            }
            errorInfo.setError(errorValue);
            instance->dispatchErrorEventToGlobalScope(errorInfo);
            loggingJSErrorInfo(instance, sbresult);
        } else {
            result = sbresult.result;
        }
    }

    clearStack<DEFAULT_CLEAR_STACK_SIZE>();

    return result;
}

ScriptValue callScriptFunctionWithError(ScriptBindingInstance* instance,
                                        ScriptValue fn, ScriptValue* argv,
                                        size_t argc, ScriptValue thisValue,
                                        bool& error)
{
    ScriptValue result = ValueRef::createUndefined();
    if (fn->isCallable()) {
        ContextRef* ctx = instance->scriptContext();
        auto sbresult = Evaluator::execute(
            ctx,
            [](ExecutionStateRef* state, ScriptValue fn, ScriptValue* argv,
               size_t argc, ScriptValue thisValue) -> ValueRef* {
                return fn->asObject()->call(state, thisValue, argc, argv);
            },
            fn, argv, argc, thisValue);
        if (sbresult.error.hasValue()) {
            // Dispatch error event to window
            ScriptValue errorValue = sbresult.error.value();
            ErrorEventInit errorInfo;
            errorInfo.setMessage(toBrowserString(instance, errorValue));
            if (sbresult.stackTrace.size() > 0) {
                size_t lastIndex = sbresult.stackTrace.size() - 1;
                errorInfo.setFilename(toBrowserString(
                    instance, sbresult.stackTrace[lastIndex].srcName));
                errorInfo.setLineno(sbresult.stackTrace[lastIndex].loc.line);
                errorInfo.setColno(sbresult.stackTrace[lastIndex].loc.column);
            }
            errorInfo.setError(errorValue);
            instance->dispatchErrorEventToGlobalScope(errorInfo);
            loggingJSErrorInfo(instance, sbresult);
            error = true;
            ctx->throwException(errorValue);
        } else {
            result = sbresult.result;
        }
    }

    clearStack<DEFAULT_CLEAR_STACK_SIZE>();

    return result;
}

ScriptValue callHandleEventFunction(ScriptBindingInstance* instance,
                                    ScriptValue obj, ScriptValue* argv,
                                    size_t argc, ScriptValue thisValue)
{
    ScriptValue result = ValueRef::createUndefined();
    ContextRef* ctx = instance->scriptContext();
    auto sbresult = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptValue obj) -> ValueRef* {
            ValueRef* v = obj->asObject()->get(
                state, StringRef::createFromASCII("handleEvent"));
            return v;
        },
        obj);

    if (sbresult.error.hasValue()) {
        loggingJSErrorInfo(instance, sbresult);
    } else {
        return callScriptFunction(instance, sbresult.result, argv, argc,
                                  thisValue);
    }

    return result;
}

ScriptValue callHandleNodeFilterFunction(ScriptBindingInstance* instance,
                                         ScriptValue obj, ScriptValue* argv,
                                         size_t argc, ScriptValue thisValue,
                                         bool& error)
{
    ScriptValue result = ValueRef::createUndefined();
    ContextRef* ctx = instance->scriptContext();
    auto sbresult = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptValue obj) -> ValueRef* {
            ValueRef* v = obj->asObject()->get(
                state, StringRef::createFromASCII("acceptNode"));
            return v;
        },
        obj);

    if (sbresult.error.hasValue()) {
        loggingJSErrorInfo(instance, sbresult);
        error = true;
    } else {
        return callScriptFunctionWithError(instance, sbresult.result, argv,
                                           argc, thisValue, error);
    }

    return result;
}

void jsGlobalObjectDefinePropertyIfNotExists(ScriptBindingInstance* instance,
                                             String* attrName,
                                             ScriptValue targetObject)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(
        context,
        [](ExecutionStateRef* state, String* attrName,
           ScriptValue targetObject) -> ValueRef* {
            ContextRef* context = state->context();
            GlobalObjectRef* globalObject = context->globalObject();

            ValueRef* name = toJSString(attrName);
            if (!globalObject->hasOwnProperty(state, name)) {
                globalObject->defineDataProperty(state, name, targetObject,
                                                 false, false, true);
            }

            return ValueRef::createUndefined();
        },
        attrName, targetObject);
}

ScriptValue evaluateString(ScriptBindingInstance* instance, String* string,
                           String* fileName, bool* result)
{
    if (UNLIKELY(!instance->isScriptingEnabled())) {
        if (result) {
            *result = false;
        }
        return scriptUndefined();
    }

#if defined(STARFISH_ENABLE_DEBUGGER)
    // currently, debugger only supports ScriptBindingWindowInstance
    if (instance->hasWindow() && instance->isScriptingEnabled() &&
        !instance->isDebuggerEnabled()) {
        static unsigned port;
        Window* window = instance->ownerWindow();
        if (window->browsingContext()->isTopLevelBrowsingContext()) {
            port = 6501;
        }
        struct DebuggerCallbackParam {
            std::string url;
            int port;
            bool* ret;
        };

        DebuggerCallbackParam* param = new DebuggerCallbackParam();
        bool shouldInit = true;
        param->port = port;
        param->url = window->document()->urlString()->toUTF8NonGCString();
        param->ret = &shouldInit;
        window->webView()->callPublicWebViewHandler(DebuggerShouldInit, param,
                                                    true);
        delete param;

        if (shouldInit) {
            while (true) {
                window->scriptBindingInstance()->startDebugger(port, 1000);
                if (window->scriptBindingInstance()->isDebuggerEnabled()) {
                    port++;
                    window->setInterval(
                        [](void* data) {
                            ScriptBindingInstance* w =
                                (ScriptBindingInstance*)data;
                            w->pumpDebuggerEvents();
                        },
                        100, instance);
                    break;
                }

                DebuggerCallbackParam* param = new DebuggerCallbackParam();
                bool shouldWait = true;
                param->port = port;
                param->url =
                    window->document()->urlString()->toUTF8NonGCString();
                param->ret = &shouldWait;
                window->webView()->callPublicWebViewHandler(
                    DebuggerShouldContinueWaiting, param, true);
                delete param;

                if (!shouldWait) {
                    break;
                }
            }
        }
    }

#endif

    ContextRef* ctx = instance->scriptContext();

    StringRef* source = toJSString(string);
#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
    size_t parseStart = longTickCount();
#endif

#if defined(STARFISH_ENABLE_DEBUGGER)
    static size_t s_evalUniqueID;
    std::string s = fileName->toUTF8NonGCString();
    s += "_";
    s += std::to_string(++s_evalUniqueID);
    s += ".js";
    auto scriptRef = ctx->scriptParser()->initializeScript(
        source, toJSString(String::fromUTF8(s.data(), s.length())));
#else
    auto scriptRef =
        ctx->scriptParser()->initializeScript(source, toJSString(fileName));
#endif

    if (!scriptRef.isSuccessful()) {
        STARFISH_LOG_ERROR(
            "Script parse error: %s %s", fileName->toUTF8NonGCString().data(),
            toBrowserString(instance, scriptRef.parseErrorMessage)
                ->toUTF8NonGCString()
                .data());
        if (result)
            *result = false;
        return scriptUndefined();
    }

#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
    size_t parseEnd = longTickCount();
    float time = (float)((parseEnd - parseStart) / 1000.f);
    STARFISH_LOG_INFO("js parse %f ms", time);
#endif

#if defined(STARFISH_ENABLE_DEBUGGER)
    ctx->setAsAlwaysStopState();
#endif

    auto sbresult = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptRef* script) -> ValueRef* {
            return script->execute(state);
        },
        scriptRef.script.value());

#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
    size_t executeEnd = longTickCount();
    time = (float)((executeEnd - parseEnd) / 1000.f);
    STARFISH_LOG_INFO("js execute %f ms", time);
#endif

    clearStack<DEFAULT_CLEAR_STACK_SIZE>();

    if (sbresult.error.hasValue()) {
        // Dispatch error event to window
        ScriptValue errorValue = sbresult.error.value();
        ErrorEventInit errorInfo;
        errorInfo.setMessage(toBrowserString(instance, errorValue));
        if (sbresult.stackTrace.size() > 0) {
            size_t lastIndex = sbresult.stackTrace.size() - 1;
            errorInfo.setFilename(toBrowserString(
                instance, sbresult.stackTrace[lastIndex].srcName));
            errorInfo.setLineno(sbresult.stackTrace[lastIndex].loc.line);
            errorInfo.setColno(sbresult.stackTrace[lastIndex].loc.column);
        }
        errorInfo.setError(errorValue);
        instance->dispatchErrorEventToGlobalScope(errorInfo);
        loggingJSErrorInfo(instance, sbresult);
        if (result)
            *result = true;
        return errorValue;
    } else {
        if (result)
            *result = true;
        return sbresult.result;
    }
}

ScriptArrayBuffer createScriptArrayBuffer(ScriptBindingInstance* instance,
                                          void* bufferSrc, size_t len)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, void* bufferSrc,
                  size_t len) -> ValueRef* {
                   ArrayBufferObjectRef* obj =
                       ArrayBufferObjectRef::create(state);
                   BackingStoreRef* backingStore =
                       BackingStoreRef::createNonSharedBackingStore(
                           bufferSrc, len,
                           [](void* data, size_t length, void* deleterData) {
                               // bufferSrc is not a shared buffer case
                               // free it when BackingStore is released
                               free(data);
                           },
                           nullptr);
                   obj->attachBuffer(backingStore);
                   return obj;
               },
               bufferSrc, len)
        .result->asArrayBufferObject();
}
ScriptArrayBuffer createScriptArrayBuffer(ScriptBindingInstance* instance,
                                          size_t len)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, size_t len) -> ValueRef* {
                   ArrayBufferObjectRef* obj =
                       ArrayBufferObjectRef::create(state);
                   obj->allocateBuffer(state, len);
                   return obj;
               },
               len)
        .result->asArrayBufferObject();
}

ScriptInt8Array createEmptyInt8Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Int8ArrayObjectRef::create(state);
                              })
        .result->asInt8ArrayObject();
}

ScriptUint8Array createScriptUint8Array(ScriptBindingInstance* instance,
                                        void* scriptFreeableBuffer, size_t len)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, void* scriptFreeableBuffer,
                  size_t len) -> ValueRef* {
                   auto buf = ArrayBufferObjectRef::create(state);
                   BackingStoreRef* backingStore =
                       BackingStoreRef::createNonSharedBackingStore(
                           scriptFreeableBuffer, len,
                           [](void* data, size_t length, void* deleterData) {
                               // scriptFreeableBuffer is not a shared buffer
                               // case free it when BackingStore is released
                               free(data);
                           },
                           nullptr);
                   buf->attachBuffer(backingStore);
                   auto arr = Uint8ArrayObjectRef::create(state);
                   arr->setBuffer(buf, 0, len, len);
                   return arr;
               },
               scriptFreeableBuffer, len)
        .result->asUint8ArrayObject();
}

ScriptUint8Array createEmptyUint8Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Uint8ArrayObjectRef::create(state);
                              })
        .result->asUint8ArrayObject();
}

ScriptInt16Array createEmptyInt16Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Int16ArrayObjectRef::create(state);
                              })
        .result->asInt16ArrayObject();
}

ScriptUint16Array createEmptyUint16Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Uint16ArrayObjectRef::create(state);
                              })
        .result->asUint16ArrayObject();
}

ScriptInt32Array createEmptyInt32Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Int32ArrayObjectRef::create(state);
                              })
        .result->asInt32ArrayObject();
}

ScriptUint32Array createEmptyUint32Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Uint32ArrayObjectRef::create(state);
                              })
        .result->asUint32ArrayObject();
}

ScriptFloat32Array createEmptyFloat32Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Float32ArrayObjectRef::create(state);
                              })
        .result->asFloat32ArrayObject();
}

ScriptFloat64Array createEmptyFloat64Array(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Float64ArrayObjectRef::create(state);
                              })
        .result->asFloat64ArrayObject();
}

ScriptUint8ClampedArray createEmptyUint8ClampedArray(
    ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    return Evaluator::execute(ctx,
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return Uint8ClampedArrayObjectRef::create(
                                      state);
                              })
        .result->asUint8ClampedArrayObject();
}

void registerJavaScriptNativeInterface(
    ScriptBindingInstance* instance, String* exposedObjectName,
    String* jsFunctionName, void* scriptObject,
    Escargot::ScriptNativeFunctionPointer scriptNativeFunctionPointer)
{
    ContextRef* context = instance->scriptContext();

    Evaluator::execute(
        context,
        [](ExecutionStateRef* state, String* exposedObjectName,
           String* jsFunctionName, void* scriptObject,
           Escargot::ScriptNativeFunctionPointer scriptNativeFunctionPointer)
            -> ValueRef* {
            ContextRef* context = state->context();
            GlobalObjectRef* globalObject = context->globalObject();

            ObjectRef* targetObject = nullptr;
            ValueRef* key = toJSString(exposedObjectName);
            if (globalObject->hasOwnProperty(state, key)) {
                targetObject =
                    globalObject->getOwnProperty(state, key)->asObject();
            } else {
                targetObject = ObjectRef::create(state);
                globalObject->defineDataProperty(
                    state, toJSString(exposedObjectName), targetObject, false,
                    false, true);
            }

            StringRef* nativeCallbackString = toJSString(jsFunctionName);
            auto u8FunctionName = jsFunctionName->toUTF8NonGCString();
            FunctionObjectRef* nativeCallbackESFn = FunctionObjectRef::create(
                state,
                FunctionObjectRef::NativeFunctionInfo(
                    AtomicStringRef::create(context, u8FunctionName.data(),
                                            u8FunctionName.length()),
                    scriptNativeFunctionPointer, 1, true, false));

            nativeCallbackESFn->setExtraData(scriptObject);
            targetObject->defineDataProperty(
                state, Escargot::ValueRef::create(nativeCallbackString),
                Escargot::ValueRef::create(nativeCallbackESFn), false, false,
                true);

            return ValueRef::createUndefined();
        },
        exposedObjectName, jsFunctionName, scriptObject,
        scriptNativeFunctionPointer);
}
void unregisterJavaScriptNativeInterface(ScriptBindingInstance* instance,
                                         String* exposedObjectName,
                                         String* jsFunctionName)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(
        context,
        [](ExecutionStateRef* state, String* exposedObjectName,
           String* jsFunctionName) -> ValueRef* {
            ContextRef* context = state->context();

            GlobalObjectRef* globalObject = context->globalObject();

            ValueRef* key = toJSString(exposedObjectName);
            if (globalObject->hasOwnProperty(state, key)) {
                ObjectRef* targetObject =
                    globalObject->getOwnProperty(state, key)->asObject();
                ValueRef* nativeCallbackName = toJSString(jsFunctionName);
                if (targetObject->hasOwnProperty(state, nativeCallbackName)) {
                    targetObject->deleteOwnProperty(state, nativeCallbackName);
                }
            }
            return ValueRef::createUndefined();
        },
        exposedObjectName, jsFunctionName);
}

void unregisterJavaScriptNativeInterface(ScriptBindingInstance* instance,
                                         String* exposedObjectName)
{
    ContextRef* context = instance->scriptContext();

    Evaluator::execute(
        context,
        [](ExecutionStateRef* state, String* exposedObjectName) -> ValueRef* {
            ContextRef* context = state->context();
            GlobalObjectRef* globalObject = context->globalObject();

            ValueRef* key = toJSString(exposedObjectName);
            if (globalObject->hasOwnProperty(state, key)) {
                ObjectRef* targetObject =
                    globalObject->getOwnProperty(state, key)->asObject();
                globalObject->deleteOwnProperty(state, key);
            }
            return ValueRef::createUndefined();
        },
        exposedObjectName);
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

    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, String* date) -> ValueRef* {
                   DateObjectRef* obj = DateObjectRef::create(state);
                   obj->setTimeValue(
                       state, ValueRef::create(createScriptString(date)));
                   double ret = obj->primitiveValue();
                   return ValueRef::create(ret);
               },
               date)
        .result->asNumber();
}

String* timeToUTCString(ScriptBindingInstance* instance, int64_t value)
{
    ContextRef* ctx = instance->scriptContext();
    return toBrowserString(
        instance, Evaluator::execute(
                      ctx,
                      [](ExecutionStateRef* state, int64_t value) -> ValueRef* {
                          DateObjectRef* obj = DateObjectRef::create(state);
                          obj->setTimeValue(value);
                          return obj->toUTCString(state);
                      },
                      value)
                      .result);
}

bool isCallableScriptValue(ScriptValue v)
{
    return v->isCallable();
}

bool isObjectScriptValue(ScriptValue v)
{
    if (v->isObject()) {
        return true;
    }
    return false;
}

bool isNumberScriptValue(ScriptValue v)
{
    if (v->isNumber()) {
        return true;
    }
    return false;
}

bool isBooleanScriptValue(ScriptValue v)
{
    if (v->isBoolean()) {
        return true;
    }
    return false;
}

bool isNullOrUndefinedScriptValue(ScriptValue v)
{
    if (v->isNull() || v->isUndefined()) {
        return true;
    }

    return false;
}

#ifdef STARFISH_ENABLE_TEST
void invokeTestStartFunction(ScriptBindingInstance* instance)
{
    ContextRef* ctx = instance->scriptContext();
    ScriptValue fn =
        Evaluator::execute(ctx, [](ExecutionStateRef* state) -> ValueRef* {
            ScriptValue fn = state->context()->globalObject()->get(
                state, StringRef::createFromASCII("testStart"));
            return fn;
        }).result;
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
    return buffer->byteLength();
}

unsigned arrayBufferViewSize(ScriptArrayBufferView buffer)
{
    return buffer->byteLength();
}

void detachArrayBuffer(ScriptBindingInstance* instance,
                       ScriptArrayBuffer buffer)
{
    ContextRef* ctx = instance->scriptContext();
    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptArrayBuffer buffer) -> ValueRef* {
            buffer->detachArrayBuffer();
            return ValueRef::createUndefined();
        },
        buffer);
}

Promise::Promise(ScriptBindingInstance* instance)
{
    m_instance = instance;
    ContextRef* ctx = instance->scriptContext();
    m_scriptValue =
        Evaluator::execute(ctx, [](ExecutionStateRef* state) -> ValueRef* {
            return ValueRef::create(PromiseObjectRef::create(state));
        }).result;
}

void Promise::fulfill(ScriptValue v)
{
    ContextRef* ctx = m_instance->scriptContext();
    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, Promise* self,
           ScriptValue v) -> ValueRef* {
            self->m_scriptValue->asObject()->asPromiseObject()->fulfill(state,
                                                                        v);
            return ValueRef::createUndefined();
        },
        this, v);
}

void Promise::reject(ScriptValue v)
{
    ContextRef* ctx = m_instance->scriptContext();
    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, Promise* self,
           ScriptValue v) -> ValueRef* {
            self->m_scriptValue->asObject()->asPromiseObject()->reject(state,
                                                                       v);
            return ValueRef::createUndefined();
        },
        this, v);
}

AttributeEventFunction::AttributeEventFunction(EventTarget* target)
    : ScriptWrappable(target)
{
    m_target = target;
}
} // namespace Starfish
