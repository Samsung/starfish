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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include <Escargot.h>

namespace StarFish {

using namespace escargot;

class Document;
class Element;

const uint32_t kEscargotObjectCheckMagic = 0x0fff;
const uint32_t kEventStringAttributeCheckMagic = 0x0ffe;

// https://heycam.github.io/webidl/#common-DOMTimeStamp
typedef unsigned long long DOMTimeStamp;

typedef ESValue ScriptValue;
typedef ESObject* ScriptObject;
typedef ESFunctionObject* ScriptFunction;

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    ESObject* obj, ESString* propertyName, NativeFunctionType getter,
    NativeFunctionType setter, bool isEnumerable = true,
    bool isConfigurable = true);

ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance);

Document* fetchDocument(ESVMInstance* instance);
StarFish* fetchStarFish(ESVMInstance* instance);

String* toBrowserString(const ESValue& v);
String* toBrowserString(const ESString* v);
ESValue toJSString(String* v);

ESValue defaultFunction(ESVMInstance* instance);
ESValue errorOnConstructorFunction(ESVMInstance* instance);

ScriptValue createScriptString(String* str);
ScriptValue createScriptFunction(String** argNames, size_t argc,
                                 String* functionBody, bool& error);
ScriptValue createAttributeStringEventFunction(Element* target,
                                               String* functionBody,
                                               bool& result);
ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc,
                               ScriptValue thisValue);
ScriptValue createArrayBuffer(void* bufferSrc, size_t len);
ScriptValue parseJSON(String* jsonData);

bool isCallableScriptValue(ScriptValue v);

#define FOR_EACH_FORWARD_DECLARATION(exportName) class exportName;
STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_FORWARD_DECLARATION)
#undef FOR_EACH_FORWARD_DECLARATION

#define DEFINE_FUNCTION(functionName, parentName)                         \
    ESString* functionName##String = ESString::create(#functionName);     \
    ESFunctionObject* functionName##Function = ESFunctionObject::create(  \
        NULL, defaultFunction, functionName##String, 0, true, true);      \
    functionName##Function->defineAccessorProperty(                       \
        ESVMInstance::currentInstance()->strings().prototype.string(),    \
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), \
        false, false, false);                                             \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->forceNonVectorHiddenClass(false);                               \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->set__proto__(parentName);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentName)         \
    ESString* functionName##String = ESString::create(#functionName);     \
    ESFunctionObject* functionName##Function =                            \
        ESFunctionObject::create(NULL, errorOnConstructorFunction,        \
                                 functionName##String, 0, true, true);    \
    functionName##Function->defineAccessorProperty(                       \
        ESVMInstance::currentInstance()->strings().prototype.string(),    \
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), \
        false, false, false);                                             \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->forceNonVectorHiddenClass(false);                               \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->set__proto__(parentName);

#define DEFINE_FUNCTION_WITH_PARENTFUNC(functionName, parentFunction) \
    DEFINE_FUNCTION(functionName, parentFunction->protoType())        \
    functionName##Function->set__proto__(parentFunction);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(functionName,          \
                                                        parentFunction)        \
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentFunction->protoType()) \
    functionName##Function->set__proto__(parentFunction);

#define INVALID_INDEX (ESValue::ESInvalidIndexValue)
#define TO_INDEX_UINT32(argValue, idx)                                        \
    uint32_t idx;                                                             \
    idx = argValue.toIndex();                                                 \
    if (idx == INVALID_INDEX) {                                               \
        double __number = argValue.toNumber();                                \
        idx = __number < 0 ? INVALID_INDEX                                    \
                           : (std::isnan(__number) ? 0 : (uint32_t)__number); \
    }

#define _THROW_DOM_EXCEPTION(INSTANCE, ERR_CODE, MSG) \
    throw new DOMException(INSTANCE, ERR_CODE, MSG);  \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

#define _THROW_EXCEPTION(MSG)                               \
    ESVMInstance::currentInstance()->throwError(            \
        ESValue(TypeError::create(ESString::create(MSG)))); \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

// TypeError: Illegal invocation
#define THROW_ILLEGAL_INVOCATION() _THROW_EXCEPTION("Illegal invocation");

#define THROW_EXCEPTION(TEMPLATE_STR, ...)                \
    {                                                     \
        COMPOSE_ERROR_MESSAGE(TEMPLATE_STR, __VA_ARGS__); \
        _THROW_EXCEPTION(errorMsg);                       \
    }

#define THROW_DOM_EXCEPTION(INSTANCE, ERR_CODE, TEMPLATE_STR, ...) \
    {                                                              \
        COMPOSE_ERROR_MESSAGE(TEMPLATE_STR, __VA_ARGS__);          \
        _THROW_DOM_EXCEPTION(INSTANCE, ERR_CODE, errorMsg);        \
    }

#define _CHECK_TYPEOF(v, type)                                              \
    (v.isObject() && (v.asESPointer()->asESObject()->extraData() ==         \
                      kEscargotObjectCheckMagic) &&                         \
     (((ScriptWrappable*)v.asESPointer()->asESObject()->extraPointerData()) \
          ->is##type()))

#define CHECK_TYPEOF(v, type)      \
    if (!_CHECK_TYPEOF(v, type)) { \
        THROW_ILLEGAL_INVOCATION() \
    }

#define GENERATE_THIS_AND_CHECK_TYPE(type)                         \
    ESValue thisValue =                                            \
        instance->currentExecutionContext()->resolveThisBinding(); \
    CHECK_TYPEOF(thisValue, type);                                 \
    type* originalObj =                                            \
        (type*)(thisValue.asESPointer()->asESObject()->extraPointerData());

#define GENERATE_ARG_AND_CHECK_TYPE(i, type)                               \
    ESValue arg##i = instance->currentExecutionContext()->readArgument(i); \
    CHECK_TYPEOF(arg##i, type)                                             \
    type* val##i =                                                         \
        (type*)(arg##i.asESPointer()->asESObject()->extraPointerData());

#define GENERATE_NULLABLE_ARG_AND_CHECK_TYPE(i, type)                        \
    ESValue arg##i = instance->currentExecutionContext()->readArgument(i);   \
    type* val##i = nullptr;                                                  \
    if (!arg##i.isUndefinedOrNull()) {                                       \
        CHECK_TYPEOF(arg##i, type)                                           \
        val##i =                                                             \
            (type*)(arg##i.asESPointer()->asESObject()->extraPointerData()); \
    }

class ScriptWrappable : public gc {
    friend class Window;

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
        if (UNLIKELY((size_t)m_object & (size_t)1)) {
            return scriptObjectSlowCase();
        }
        return m_object;
    }

    void giveUpScriptValue()
    {
        m_object = (ESObject*)1;
    }

    ScriptObject scriptObjectSlowCase();
    ScriptValue scriptValue()
    {
        return scriptObject();
    }

    virtual void init(ScriptBindingInstance* instance) = 0;

    bool hasProperty(String* name);

private:
    ESObject* m_object;
};

#ifdef USE_ES6_FEATURE
class Promise : public gc {
public:
    Promise();
    void fulfill(ScriptValue v);
    void reject(ScriptValue v);
    ScriptValue scriptValue()
    {
        return m_scriptValue;
    }

protected:
    ScriptValue m_scriptValue;
};
#endif
}

#endif
