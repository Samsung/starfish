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

#ifndef __StarfishScriptWrappable__
#define __StarfishScriptWrappable__

#include "binding/generated/Interfaces.h"
#include "StarfishBase.h" // ASSERT, UNLIKELY, RELEASE_ASSERT_SHOULD_NOT_BE_HERE
#include <GCUtil.h>       // gc

namespace Escargot {
class VMInstanceRef;
class ContextRef;
class StringRef;
class ValueRef;
class PointerValueRef;
class ObjectRef;
class GlobalObjectRef;
class FunctionObjectRef;
class ArrayObjectRef;
class ScriptRef;
class ScriptParserRef;
class ExecutionStateRef;
class ArrayBufferObjectRef;
class ArrayBufferViewRef;
class Uint8ArrayObjectRef;
class Int8ArrayObjectRef;
class Int16ArrayObjectRef;
class Uint16ArrayObjectRef;
class Uint32ArrayObjectRef;
class Int32ArrayObjectRef;
class Uint8ClampedArrayObjectRef;
class Float32ArrayObjectRef;
class Float64ArrayObjectRef;
class SharedArrayBufferObjectRef;
class MapObjectRef;
class SetObjectRef;

template <typename T>
class OptionalRef;
typedef ValueRef* (*ScriptNativeFunctionPointer)(ExecutionStateRef* state,
                                                 ValueRef* thisValue,
                                                 size_t argc, ValueRef** argv,
                                                 bool isNewExpression);
} // namespace Escargot

namespace Starfish {

class ScriptWrapple;
class Document;
class Element;
class Serializable;
class StaticStrings;
class Transferable;
class ScriptContext;
class WebBase;
class ExecutionContext;
class EventTarget;
class WebView;
class ScriptBindingInstance;
class ScriptEngineInstance;
class Window;
class String;
class Promise;
class WorkerGlobalScope;

// https://heycam.github.io/webidl/#common-DOMTimeStamp
typedef uint64_t DOMTimeStamp;

typedef Escargot::ValueRef* ScriptValue;
typedef Escargot::ObjectRef* ScriptObject;
typedef Escargot::StringRef* ScriptString;
typedef Escargot::ArrayObjectRef* ScriptArrayObject;
typedef Escargot::ArrayBufferObjectRef* ScriptArrayBuffer;
typedef Escargot::ArrayBufferViewRef* ScriptArrayBufferView;
typedef Escargot::Int8ArrayObjectRef* ScriptInt8Array;
typedef Escargot::Uint8ArrayObjectRef* ScriptUint8Array;
typedef Escargot::Int16ArrayObjectRef* ScriptInt16Array;
typedef Escargot::Uint16ArrayObjectRef* ScriptUint16Array;
typedef Escargot::Uint32ArrayObjectRef* ScriptUint32Array;
typedef Escargot::Int32ArrayObjectRef* ScriptInt32Array;
typedef Escargot::Uint8ClampedArrayObjectRef* ScriptUint8ClampedArray;
typedef Escargot::Float32ArrayObjectRef* ScriptFloat32Array;
typedef Escargot::Float64ArrayObjectRef* ScriptFloat64Array;
typedef Escargot::SharedArrayBufferObjectRef* ScriptSharedArrayBuffer;
typedef Escargot::ExecutionStateRef* ScriptExecutionState;
typedef Escargot::OptionalRef<Escargot::ValueRef> ScriptOptionalValue;
typedef Escargot::ScriptRef* ScriptModule;
typedef Escargot::MapObjectRef* ScriptMap;
typedef Escargot::SetObjectRef* ScriptSet;

void staticallyInitScriptEngine();
void staticallyDestroyScriptEngine();

ScriptValue scriptNull();
ScriptValue scriptUndefined();
ScriptValue scriptStringToScriptValue(ScriptString s);

bool isCallableScriptValue(ScriptValue v);
bool isConstructibleScriptValue(ScriptValue v);
bool isObjectScriptValue(ScriptValue v);
bool isNumberScriptValue(ScriptValue v);
bool isBooleanScriptValue(ScriptValue v);
bool isNullOrUndefinedScriptValue(ScriptValue v);
bool isStringScriptValue(ScriptValue v);

bool scriptValueAsBoolean(ScriptValue v);
unsigned scriptValueAsNumber(ScriptValue v);
ScriptObject scriptValueAsObject(ScriptValue v);

Optional<bool> scriptValueToBoolean(ScriptBindingInstance* instance,
                                    ScriptValue v,
                                    bool throwsException = false);

ScriptObject scriptError(ScriptBindingInstance*, String* msg);
ScriptObject scriptEvalError(ScriptBindingInstance*, String* msg);
ScriptObject scriptRangeError(ScriptBindingInstance*, String* msg);
ScriptObject scriptReferenceError(ScriptBindingInstance*, String* msg);
ScriptObject scriptTypeError(ScriptBindingInstance*, String* msg);
ScriptObject scriptURIError(ScriptBindingInstance*, String* msg);

ScriptString scriptStringPrototype(ScriptBindingInstance*);
ScriptString scriptStringConstructor(ScriptBindingInstance*);
ScriptString scriptStringLength(ScriptBindingInstance*);
ScriptString scriptString__proto__(ScriptBindingInstance*);

Optional<GCVector<ScriptValue>> scriptReadIterableValue(
    ScriptBindingInstance*, ScriptValue iterable, bool throwsException = false);
inline GCVector<ScriptValue> scriptReadIterableValueThrowsException(
    ScriptBindingInstance* instance, ScriptValue iterable)
{
    return scriptReadIterableValue(instance, iterable, true).value();
}

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    Escargot::ExecutionStateRef* state, Escargot::ObjectRef* obj,
    Escargot::StringRef* propertyName,
    Escargot::ScriptNativeFunctionPointer getter,
    Escargot::ScriptNativeFunctionPointer setter, bool isEnumerable = true,
    bool isConfigurable = true);

ExecutionContext* fetchExecutionContext(Escargot::ContextRef* context);
WebBase* fetchWebBase(Escargot::ContextRef* context);
ScriptBindingInstance* fetchScriptBindingInstance(
    Escargot::ContextRef* context);

#if defined(STARFISH_WEBWORKER_NOT_HOST)
WebView* fetchWebView(Escargot::ContextRef* context);
Window* fetchWindow(Escargot::ContextRef* context);
Document* fetchDocument(Escargot::ContextRef* context);
Document* fetchResponsibleDocument(Escargot::ExecutionStateRef* state);
StaticStrings* fetchStaticStrings(Escargot::ContextRef* context);
#endif

String* toBrowserString(ScriptBindingInstance* instance, Escargot::ValueRef* v,
                        bool* result = nullptr);
String* toBrowserString(Escargot::ExecutionStateRef* state,
                        Escargot::ValueRef* v);
String* toBrowserString(Escargot::ExecutionStateRef* state,
                        Escargot::StringRef* v);
ScriptString toJSString(String* v);
ScriptObject toCalleeObject(Escargot::ExecutionStateRef* state);

ScriptValue errorOnConstructorFunction(Escargot::ExecutionStateRef* state,
                                       Escargot::ValueRef* thisValue,
                                       size_t argc, Escargot::ValueRef** argv,
                                       bool isNewExpression);

ScriptString createScriptString(String* str);
ScriptString createScriptString(const char* utf8Buffer, size_t len);
ScriptString createScriptASCIIString(const char* asciiBuffer, size_t len);
template <size_t N>
static ScriptString createScriptASCIIString(const char (&str)[N])
{
    return createScriptASCIIString(str, N - 1);
}
ScriptValue createScriptValue(ScriptObject object);
ScriptValue createScriptValue(ScriptString s);
ScriptValue createScriptValue(ScriptArrayBuffer buffer);
ScriptValue createScriptValue(ScriptArrayBufferView buffer);
ScriptValue createScriptValue(ScriptUint8Array array);
ScriptValue createScriptValue(ScriptUint8ClampedArray array);

ScriptValue createScriptValue(bool value);
ScriptValue createScriptValue(int32_t value);
ScriptValue createScriptValue(uint32_t value);
ScriptValue createScriptValue(int64_t value);
ScriptValue createScriptValue(uint64_t value);
ScriptValue createScriptValue(String* value);
ScriptValue createScriptValue(double value);

ScriptValue createScriptFunction(ScriptBindingInstance* instance,
                                 String** argNames, size_t argc,
                                 String* functionBody, bool& error);
ScriptValue createScriptFunction(
    ScriptBindingInstance* instance, const std::string& name,
    Escargot::ScriptNativeFunctionPointer nativeFunction, size_t argument,
    bool isStrict = true, bool isConstructor = true);
ScriptValue createAttributeStringEventFunction(EventTarget* instance,
                                               String* functionBody,
                                               bool& result);
ScriptValue callScriptFunction(ScriptBindingInstance* instance, ScriptValue fn,
                               ScriptValue* argv, size_t argc,
                               ScriptValue thisValue);
ScriptValue callScriptFunctionWithError(ScriptBindingInstance* instance,
                                        ScriptValue fn, ScriptValue* argv,
                                        size_t argc, ScriptValue thisValue,
                                        bool& error);
void callConstructor(ScriptBindingInstance* instance, ScriptValue fn,
                     ScriptValue* argv, size_t argc, ScriptObject thisValue);
ScriptValue callHandleEventFunction(ScriptBindingInstance* instance,
                                    ScriptValue obj, ScriptValue* argv,
                                    size_t argc, ScriptValue thisValue);
ScriptValue callHandleNodeFilterFunction(ScriptBindingInstance* instance,
                                         ScriptValue obj, ScriptValue* argv,
                                         size_t argc, ScriptValue thisValue,
                                         bool& error);
ScriptValue evaluateString(ScriptBindingInstance* instance, String* string,
                           String* fileName = String::emptyString,
                           bool* result = nullptr);
Optional<ScriptModule> initModule(ScriptBindingInstance* instance,
                                  String* string,
                                  String* fileName = String::emptyString);
GCVector<String*> moduleRequests(ScriptModule module);
bool executeModule(ScriptBindingInstance* instance, ScriptModule module);
bool isExecutableModule(ScriptModule module);
void notifyDynamicLoadedModuleResult(ScriptBindingInstance* instance,
                                     ScriptModule module, Promise* promise);
void notifyDynamicLoadedModuleError(ScriptBindingInstance* instance,
                                    Promise* promise);

Optional<bool> setScriptObjectProperty(ScriptBindingInstance* instance,
                                       ScriptObject object, ScriptValue key,
                                       ScriptValue value,
                                       bool throwsException = false);
Optional<bool> setScriptObjectProperty(ScriptBindingInstance* instance,
                                       ScriptObject object, ScriptString key,
                                       ScriptValue value,
                                       bool throwsException = false);

bool setScriptObjectPropertyThrowsException(ScriptBindingInstance* instance,
                                            ScriptObject object,
                                            ScriptValue key, ScriptValue value);
bool setScriptObjectPropertyThrowsException(ScriptBindingInstance* instance,
                                            ScriptObject object,
                                            ScriptString key,
                                            ScriptValue value);

Optional<ScriptValue> getScriptObjectProperty(ScriptBindingInstance* instance,
                                              ScriptObject object,
                                              ScriptValue key);
Optional<ScriptValue> getScriptObjectProperty(ScriptBindingInstance* instance,
                                              ScriptObject object,
                                              ScriptString key);

ScriptValue getScriptObjectPropertyThrowsException(
    ScriptBindingInstance* instance, ScriptObject object, ScriptValue key);
ScriptValue getScriptObjectPropertyThrowsException(
    ScriptBindingInstance* instance, ScriptObject object, ScriptString key);

Optional<ScriptValue> getScriptObjectOwnProperty(
    ScriptBindingInstance* instance, ScriptObject object, ScriptValue key);

void jsGlobalObjectDefinePropertyIfNotExists(ScriptBindingInstance* instance,
                                             String* attrName,
                                             ScriptValue targetObject);

ScriptArrayBuffer createScriptArrayBuffer(ScriptBindingInstance* instance,
                                          void* bufferSrc, size_t len);
ScriptArrayBuffer createScriptArrayBuffer(ScriptBindingInstance* instance,
                                          size_t len);

ScriptUint8Array createScriptUint8Array(ScriptBindingInstance* instance,
                                        void* scriptFreeableBuffer, size_t len);
ScriptInt8Array createEmptyInt8Array(ScriptBindingInstance* instance);
ScriptUint8Array createEmptyUint8Array(ScriptBindingInstance* instance);
ScriptInt16Array createEmptyInt16Array(ScriptBindingInstance* instance);
ScriptUint16Array createEmptyUint16Array(ScriptBindingInstance* instance);
ScriptUint8ClampedArray createEmptyUint8ClampedArray(
    ScriptBindingInstance* instance);
ScriptUint32Array createEmptyUint32Array(ScriptBindingInstance* instance);
ScriptInt32Array createEmptyInt32Array(ScriptBindingInstance* instance);
ScriptFloat32Array createEmptyFloat32Array(ScriptBindingInstance* instance);
ScriptFloat64Array createEmptyFloat64Array(ScriptBindingInstance* instance);
ScriptObject createEmptyScriptObject(ScriptBindingInstance* instance);
ScriptObject createScriptObject(ScriptBindingInstance* instance,
                                ScriptObject constructor, void* extraData);
ScriptObject createScriptObject(ScriptBindingInstance* instance,
                                Escargot::FunctionObjectRef* constructor,
                                const std::string& name, void* extraData);

void registerJavaScriptNativeInterface(
    ScriptBindingInstance* instance, String* exposedObjectName,
    String* jsFunctionName, void* scriptObject,
    Escargot::ScriptNativeFunctionPointer scriptNativeFunctionPointer);
void unregisterJavaScriptNativeInterface(ScriptBindingInstance* instance,
                                         String* exposedObjectName,
                                         String* jsFunctionName);
void unregisterJavaScriptNativeInterface(ScriptBindingInstance* instance,
                                         String* exposedObjectName);

ScriptValue parseJSON(ScriptBindingInstance* instance, String* jsonData);
ScriptValue parseJSONStringToScriptValueOrNull(ScriptBindingInstance* instance,
                                               String* jsonData);
double parseDate(ScriptBindingInstance* instance, String* date);
String* timeToUTCString(ScriptBindingInstance* instance, int64_t time);

void throwScriptTypeError(ScriptBindingInstance* instance, String* message);
void throwScriptException(ScriptBindingInstance* instance, ScriptValue e);

uint8_t* arrayBufferRawData(ScriptArrayBuffer buffer);
uint8_t* arrayBufferViewRawData(ScriptArrayBufferView buffer);
unsigned arrayBufferByteSize(ScriptArrayBuffer buffer);
unsigned arrayBufferViewSize(ScriptArrayBufferView buffer);
unsigned arrayBufferViewByteSize(ScriptArrayBufferView buffer);
template <typename T, typename U>
ScriptArrayBufferView createTypedArray(ScriptBindingInstance* instance,
                                       const std::vector<U>& vector);
template <typename U>
ScriptArrayObject createArray(ScriptBindingInstance* instance,
                              const std::vector<U>& vector);

void detachArrayBuffer(ScriptBindingInstance* instance,
                       ScriptArrayBuffer buffer);

void enqueueMicrotask(ScriptBindingInstance* instance, void (*fn)(void*),
                      void* data);

#ifdef STARFISH_ENABLE_TEST
void invokeTestStartFunction(ScriptBindingInstance* instance);
#endif

#define DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(className)                   \
    virtual void init(ScriptBindingInstance* instance, void* domObjectPointer) \
        override;                                                              \
    virtual bool is##className() const override;                               \
    virtual ScriptBindingInstance* scriptBindingInstance() override;

#define FOR_EACH_FORWARD_DECLARATION(exportName) class exportName;
STARFISH_ENUM_BINDING_CLASSES(FOR_EACH_FORWARD_DECLARATION)
#undef FOR_EACH_FORWARD_DECLARATION

#define THROW_EXCEPTION(MSG)                                            \
    state->throwException(                                              \
        Escargot::ValueRef::create(Escargot::ErrorObjectRef::create(    \
            state, Escargot::ErrorObjectRef::TypeError,                 \
            toJSString(String::createASCIIString(MSG, strlen(MSG)))))); \
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();

#define _CHECK_TYPEOF(v, type)                        \
    (v->isObject() && (v->asObject()->extraData()) && \
     (((ScriptWrappable*)v->asObject()->extraData())->is##type()))

#define CHECK_TYPEOF(v, type)            \
    if (!_CHECK_TYPEOF(v, type)) {       \
        THROW_EXCEPTION(ILLEGAL_INVOKE); \
    }

#define GENERATE_THIS_AND_CHECK_TYPE(type) \
    CHECK_TYPEOF(thisValue, type);         \
    type* originalObj = (type*)(thisValue->asObject()->extraData());

#define GENERATE_WINDOW()                                                    \
    Window* window = nullptr;                                                \
    if (thisValue->isUndefinedOrNull()) {                                    \
        window =                                                             \
            (Window*)state->resolveCallerLexicalGlobalObject()->extraData(); \
    } else {                                                                 \
        ObjectRef* mayBeWindowObject = nullptr;                              \
        if (!((ScriptWrappable*)(mayBeWindowObject =                         \
                                     thisValue->toObject(state))             \
                  ->extraData())                                             \
                 ->isWindow()) {                                             \
            THROW_EXCEPTION(ILLEGAL_INVOKE);                                 \
        }                                                                    \
        window = (Window*)mayBeWindowObject->extraData();                    \
    }

#define GENERATE_WORKER_GLOBALSCOPE(Type)                                    \
    if (!(thisValue->isUndefinedOrNull() == true ||                          \
          thisValue->toObject(state) == state->context()->globalObject())) { \
        THROW_EXCEPTION(ILLEGAL_INVOKE);                                     \
    }                                                                        \
    Type* originalObj =                                                      \
        static_cast<Type*>(state->context()->globalObject()->extraData());

class ScriptWrappable : public gc {
public:
#define FOR_EACH_REFLECT_FN(exportName) \
    virtual bool is##exportName() const \
    {                                   \
        return false;                   \
    }

    STARFISH_ENUM_BINDING_CLASSES(FOR_EACH_REFLECT_FN);
#undef FOR_EACH_REFLECT_FN

#define FOR_EACH_CAST_FN(exportName)       \
    exportName* as##exportName() const     \
    {                                      \
        STARFISH_ASSERT(is##exportName()); \
        return (exportName*)this;          \
    }

    STARFISH_ENUM_BINDING_CLASSES(FOR_EACH_CAST_FN);
#undef FOR_EACH_CAST_FN

    ScriptWrappable(void* extraPointerData);
    virtual ~ScriptWrappable()
    {
    }

    ScriptObject scriptObject()
    {
        if (UNLIKELY(isGivenUpScriptValue())) {
            return generateScriptObject();
        }
        return m_object;
    }

    void giveUpScriptValue()
    {
        m_object = (Escargot::ObjectRef*)1;
    }

    bool isGivenUpScriptValue()
    {
        return ((size_t)m_object & (size_t)1);
    }

    ScriptObject generateScriptObject();
    ScriptValue scriptValue();

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) = 0;
    virtual void postInit(ScriptBindingInstance* instance)
    {
    }
    virtual ScriptBindingInstance* scriptBindingInstance() = 0;

    virtual bool isGlobalScope() const
    {
        return false;
    }

    virtual bool isAttributeEventFunction() const
    {
        return false;
    }

    virtual bool isSerializable() const
    {
        return false;
    }

    virtual bool isTransferable() const
    {
        return false;
    }

    virtual Serializable* toSerializable() const
    {
        return nullptr;
    }

    virtual Transferable* toTransferable() const
    {
        return nullptr;
    }

    virtual bool isJavaScriptNativeHandler() const
    {
        return false;
    }

protected:
    void overrideScriptObject(ScriptObject obj)
    {
        m_object = obj;
    }
    Escargot::ObjectRef* m_object;
};

ScriptWrappable* toScriptWrappable(ScriptValue v);
ScriptWrappable* toScriptWrappable(ScriptObject v);

class AttributeEventFunction : public ScriptWrappable {
public:
    AttributeEventFunction(EventTarget* target);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override
    {
    }
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return nullptr;
    }

    virtual bool isAttributeEventFunction() const override
    {
        return true;
    }

    EventTarget* target()
    {
        return m_target;
    }

    EventTarget* m_target;
};

class Promise : public gc {
public:
    Promise(ScriptBindingInstance* instance);
    Promise(ScriptBindingInstance* instance, ScriptValue scriptValue);
    void fulfill(ScriptValue v);
    void reject(ScriptValue v);
    ScriptValue then(ScriptValue handler);
    ScriptValue then(ScriptValue onFulfilled, ScriptValue onRejected);
    ScriptValue promiseResult();
    ScriptValue scriptValue()
    {
        return m_scriptValue;
    }

    void onSettled()
    {
        m_isSettled = true;
        if (m_onSettled) {
            m_onSettled(m_onSettledData);
        }
    }

    void setOnSettled(void (*onSettled)(void*), void* data)
    {
        m_onSettled = onSettled;
        m_onSettledData = data;
    }

    bool isSettled()
    {
        return m_isSettled;
    }

protected:
    ScriptValue m_scriptValue = nullptr;
    ScriptBindingInstance* m_instance = nullptr;
    void (*m_onSettled)(void*) = nullptr;
    void* m_onSettledData = nullptr;
    bool m_isSettled = false;
};

Promise* toPromise(ScriptBindingInstance* instance, ScriptValue scriptValue);

// Freeze ArrayObjectRef per Web IDL FrozenArray<T> spec
void freezeArray(Escargot::ExecutionStateRef* state, Escargot::ArrayObjectRef* array);

} // namespace Starfish

#endif
