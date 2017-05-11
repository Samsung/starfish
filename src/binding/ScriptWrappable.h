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
typedef uint64_t DOMTimeStamp;

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
ESString* toJSString(String* v);

ESValue defaultFunction(ESVMInstance* instance);
ESValue errorOnConstructorFunction(ESVMInstance* instance);

ESString* createScriptString(String* str);
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

#define THROW_DOM_EXCEPTION(INSTANCE, ERR_CODE, MSG) \
    throw new DOMException(INSTANCE, ERR_CODE, MSG); \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

#define THROW_EXCEPTION(MSG)                                \
    ESVMInstance::currentInstance()->throwError(            \
        ESValue(TypeError::create(ESString::create(MSG)))); \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

#define _CHECK_TYPEOF(v, type)                                              \
    (v.isObject() && (v.asESPointer()->asESObject()->extraData() ==         \
                      kEscargotObjectCheckMagic) &&                         \
     (((ScriptWrappable*)v.asESPointer()->asESObject()->extraPointerData()) \
          ->is##type()))

#define CHECK_TYPEOF(v, type)            \
    if (!_CHECK_TYPEOF(v, type)) {       \
        THROW_EXCEPTION(ILLEGAL_INVOKE); \
    }

#define GENERATE_THIS_AND_CHECK_TYPE(type)                         \
    ESValue thisValue =                                            \
        instance->currentExecutionContext()->resolveThisBinding(); \
    CHECK_TYPEOF(thisValue, type);                                 \
    type* originalObj =                                            \
        (type*)(thisValue.asESPointer()->asESObject()->extraPointerData());

#define GENERATE_WINDOW()                                          \
    ESValue thisValue =                                            \
        instance->currentExecutionContext()->resolveThisBinding(); \
    if (!(thisValue.isUndefinedOrNull() ||                         \
          thisValue.asESPointer()->asESObject() ==                 \
              instance->globalObject())) {                         \
        THROW_EXCEPTION(ILLEGAL_INVOKE);                           \
    }                                                              \
    Window* window = (Window*)instance->globalObject()->extraPointerData();

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
            return scriptObjectSlowCase();
        }
        return m_object;
    }

    void giveUpScriptValue()
    {
        m_object = (ESObject*)1;
    }

    bool isGivenUpScriptValue()
    {
        return ((size_t)m_object & (size_t)1);
    }

    ScriptObject scriptObjectSlowCase();
    ScriptValue scriptValue()
    {
        return scriptObject();
    }

    virtual void init(ScriptBindingInstance* instance) = 0;
    virtual void postInit(ScriptBindingInstance* instance)
    {
    }

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
