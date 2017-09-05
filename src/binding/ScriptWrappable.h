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

#ifndef __StarFishScriptWrappable__
#define __StarFishScriptWrappable__

#include "binding/ScriptBindingInstance.h"

namespace StarFish {

class Document;
class Element;
class Serializable;
class Transferable;

// https://heycam.github.io/webidl/#common-DOMTimeStamp
typedef uint64_t DOMTimeStamp;

typedef Escargot::ValueRef* ScriptValue;
typedef Escargot::ObjectRef* ScriptObject;
typedef Escargot::StringRef* ScriptString;
typedef Escargot::FunctionObjectRef* ScriptFunction;
typedef Escargot::ArrayBufferObjectRef* ScriptArrayBuffer;
typedef Escargot::ArrayBufferViewRef* ScriptArrayBufferView;

ScriptValue scriptNull();
ScriptValue scriptUndefined();
ScriptValue scriptStringToScriptValue(ScriptString s);
bool scriptValueIsBoolean(ScriptValue v);
bool scriptValueAsBoolean(ScriptValue v);

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    Escargot::ExecutionStateRef* state, Escargot::ObjectRef* obj,
    Escargot::StringRef* propertyName,
    Escargot::ScriptNativeFunctionPointer getter,
    Escargot::ScriptNativeFunctionPointer setter, bool isEnumerable = true,
    bool isConfigurable = true);

StarFish* fetchStarFish(Escargot::ContextRef* context);
Window* fetchWindow(Escargot::ContextRef* context);
Document* fetchDocument(Escargot::ContextRef* context);

String* toBrowserString(ScriptBindingInstance* instance, Escargot::ValueRef* v,
                        bool* result = nullptr);
String* toBrowserString(Escargot::ExecutionStateRef* state,
                        Escargot::ValueRef* v);
String* toBrowserString(Escargot::ExecutionStateRef* state,
                        Escargot::StringRef* v);
ScriptString toJSString(String* v);

ScriptValue errorOnConstructorFunction(Escargot::ExecutionStateRef* state,
                                       Escargot::ValueRef* thisValue,
                                       size_t argc, Escargot::ValueRef** argv,
                                       bool isNewExpression);
void throwJSTypeErrorException(Escargot::ExecutionStateRef* state,
                               String* message);

ScriptString createScriptString(String* str);
ScriptValue createScriptValue(ScriptString s);
ScriptValue createScriptValue(ScriptArrayBuffer buffer);
ScriptValue createScriptValue(ScriptArrayBufferView buffer);
ScriptValue createScriptFunction(ScriptBindingInstance* instance,
                                 String** argNames, size_t argc,
                                 String* functionBody, bool& error);
ScriptValue createAttributeStringEventFunction(Element* target,
                                               String* functionBody,
                                               bool& result);
ScriptValue callScriptFunction(ScriptBindingInstance* instance, ScriptValue fn,
                               ScriptValue* argv, size_t argc,
                               ScriptValue thisValue);
ScriptValue evaluateString(ScriptBindingInstance* instance, String* string,
                           String* fileName = String::emptyString,
                           bool* result = nullptr);
ScriptValue createArrayBuffer(ScriptBindingInstance* instance, void* bufferSrc,
                              size_t len);
ScriptValue parseJSON(ScriptBindingInstance* instance, String* jsonData);
double parseDate(ScriptBindingInstance* instance, String* date);

void throwScriptTypeError(String* message);

bool isCallableScriptValue(ScriptValue v);
bool isObjectScriptValue(ScriptValue v);

uint8_t* arrayBufferRawData(ScriptArrayBuffer buffer);
uint8_t* arrayBufferViewRawData(ScriptArrayBufferView buffer);
unsigned arrayBufferSize(ScriptArrayBuffer buffer);
unsigned arrayBufferViewSize(ScriptArrayBufferView buffer);

#ifdef STARFISH_ENABLE_TEST
void invokeTestStartFunction(ScriptBindingInstance* instance);
#endif

#define FOR_EACH_FORWARD_DECLARATION(exportName) class exportName;
STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_FORWARD_DECLARATION)
#undef FOR_EACH_FORWARD_DECLARATION

#define THROW_DOM_EXCEPTION(INSTANCE, ERR_CODE, MSG) \
    throw new DOMException(INSTANCE, ERR_CODE, MSG); \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

#define THROW_EXCEPTION(MSG)                                         \
    state->throwException(                                           \
        Escargot::ValueRef::create(Escargot::ErrorObjectRef::create( \
            state, Escargot::ErrorObjectRef::TypeError,              \
            Escargot::StringRef::fromASCII(MSG))));                  \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

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
    if (!(thisValue->isUndefinedOrNull() ||                                  \
          thisValue->toObject(state) == state->context()->globalObject())) { \
        THROW_EXCEPTION(ILLEGAL_INVOKE);                                     \
    }                                                                        \
    Window* window = (Window*)state->context()->globalObject()->extraData();

class ScriptWrappable : public gc {
public:
#define FOR_EACH_REFLECT_FN(exportName) \
    virtual bool is##exportName() const \
    {                                   \
        return false;                   \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_REFLECT_FN);
#undef FOR_EACH_REFLECT_FN

#define FOR_EACH_CAST_FN(exportName)           \
    virtual exportName* as##exportName() const \
    {                                          \
        STARFISH_ASSERT(is##exportName());     \
        return (exportName*)this;              \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_CAST_FN);
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

protected:
    Escargot::ObjectRef* m_object;
};

class AttributeEventFunction : public ScriptWrappable {
public:
    AttributeEventFunction(Element* element)
        : ScriptWrappable(element)
    {
        m_element = element;
    }

    virtual void init(ScriptBindingInstance* instance, void* domObjectPointer)
    {
    }
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return nullptr;
    }

    virtual bool isAttributeEventFunction() const override
    {
        return true;
    }

    Element* element()
    {
        return m_element;
    }

    Element* m_element;
};

class Promise : public gc {
public:
    Promise(ScriptBindingInstance* instance);
    void fulfill(ScriptValue v);
    void reject(ScriptValue v);
    ScriptValue scriptValue()
    {
        return m_scriptValue;
    }

protected:
    ScriptValue m_scriptValue;
    ScriptBindingInstance* m_instance;
};
}

#endif
